#include "scheduledevent.h"

#include <bit>
#include <concepts>
#include <cstring>
#include <new>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <mmsystem.h>
#endif

#include "ear/earplayback.h"
#include "ear/playbackimplementations.h"
#include "ear/registry.h"
#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include "ear/timer.h"
#endif
#include "environment.h"
#include "error.h"

#if TGR_PLATFORM != TGR_PLATFORM_WIN
// An event cuts its path to the bytes of a path on Windows.
#define MAX_PATH 260
#endif

namespace {
static inline void ReleasePlaybackStorage(EarPlayback *playback) {
    if (playback) {
        // Native cleanup invokes only the base destructor before deallocation.
        playback->EarPlayback::~EarPlayback();
        ::operator delete(playback);
    }
}

template <typename Implementation>
    requires std::derived_from<Implementation, EarPlayback>
static inline EarPlayback *AllocatePlaybackImplementation() {
    void *storage = MfcAllocateWithNewHandler(sizeof(Implementation));
    return storage ? new (storage) Implementation : nullptr;
}

// @ghidraAddress EAR: 0x1002e7a8
const double g_earInitialEventAttenuationScale = 0.0001;
// @ghidraAddress EAR: 0x1002e7b0
const double g_earEventTimeUnitScale = 10000;
// @ghidraAddress EAR: 0x1002e7b8
const double g_earDopplerUnity = 1;
// @ghidraAddress EAR: 0x1002e710
const double g_earPlaybackAttenuationScale = 0.0001;
// @ghidraAddress EAR: 0x1002e7c8
const double g_earStopControlPercentScale = 0.02;

static inline std::uint32_t AbsoluteDistance(std::int32_t distance) {
    std::uint32_t bits = static_cast<std::uint32_t>(distance);
    return distance < 0 ? 0u - bits : bits;
}

static inline std::uint16_t TruncateGain(long double gain) {
    if (gain >= -0x1p63L && gain < 0x1p63L) {
        return static_cast<std::uint16_t>(static_cast<std::int64_t>(gain));
    }
    return 0;
}

static inline std::int32_t SubtractTimer(std::int32_t timer, std::uint32_t elapsed) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(timer) - elapsed);
}

static inline std::int32_t AddTimer(std::int32_t timer, std::uint32_t elapsed) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(timer) + elapsed);
}
} // namespace

ScheduledEarEvent::ScheduledEarEvent(std::uint16_t flags, const void *incoming) {
    playbackInitialized = 0;
    playbackStarted = 0;
    looping = 0;
    streaming = 0;
    frequencyScaleOverrideEnabled = 0;
    dopplerEnabled = 0;
    fadingOut = 0;
    lifetimeCountdownSuppressed = 0;
    activeDurationLimitEnabled = 0;
    attenuationRequired = 0;
    distanceLimitEnabled = 0;
    trackElapsedTime = 0;
    suppressionEnabled = 0;
    suppressionMarker = 0;
    twinEventHandle = 0;
    attenuation = 10000;
    directionIndex = 0;
    matrixId = 16;
    startDelayMs = 0;
    remainingLifetimeMs = 0;
    remainingActiveTimeMs = 0;
    elapsedTimeMs = 0;
    attenuationDecrement = 0;
    distance = 0;
    maximumDistance = 0;
    previousDistance = 0;
    previousRadialSpeed = 0;
    previousDistanceTime = 0;
    accumulatedDistance = 0;
    accumulatedTime = 0;
    distanceSampleCount = 0;
    crossingActive = 0;
    crossingInitialSpeed = 0;
    crossingSpeedStep = 0;
    crossingStep = 0;
    radialDirection = 1;
    environmentId = 0;
    std::memset(unknownA6, 0, sizeof(unknownA6));
    frequencyScale = 1;
    baseFrequencyScale = 1;
    startPosition = 0;
    backendParameter = 0;
    notificationFlags = 0;
    std::memset(unknownCc, 0, sizeof(unknownCc));
    priority = 0;
    notificationEvent = 0;
    // Native construction does not initialise reservedEventControl.
    allocatedSource = nullptr;
    playback = nullptr;
    sourceType = flags & 0xff;
    sourceFlags = flags & 0xff00;
    source.address = incoming;
    if ((sourceType == 0 || sourceType == 31) && !(sourceFlags & 0x6000)) {
        allocatedSource = static_cast<char *>(MfcAllocateWithNewHandler(MAX_PATH));
        if (!allocatedSource) {
            SetEarLastError(0x10);
            this->~ScheduledEarEvent();
            return;
        }
        std::strncpy(allocatedSource, static_cast<const char *>(incoming), MAX_PATH);
        allocatedSource[MAX_PATH - 1] = '\0';
        source.address = allocatedSource;
    } else if (sourceType == 0 && (sourceFlags & 0x6000) == 0x2000) {
        sourceType = 2;
    }
    if (!CreateEarPlaybackImplementation(sourceFlags)) {
        SetEarLastError(0x17);
        this->~ScheduledEarEvent();
    }
    if (!playback) {
        SetEarLastError(0x10);
        this->~ScheduledEarEvent();
    }
}

// The twin keeps the path of the event without owning it, and the original leaves the path, the
// playback flags, the timers it does not copy, and the reserved control as the allocation held.
ScheduledEarEvent::ScheduledEarEvent(const ScheduledEarEvent &other) {
    sourceType = other.sourceType;
    sourceFlags = other.sourceFlags;
    // The result is discarded; a failed playback leaves the twin without one.
    CreateEarPlaybackImplementation(sourceFlags);
    source = other.source;
    playbackInitialized = other.playbackInitialized;
    looping = other.looping;
    streaming = other.streaming;
    frequencyScaleOverrideEnabled = other.frequencyScaleOverrideEnabled;
    dopplerEnabled = other.dopplerEnabled;
    lifetimeCountdownSuppressed = other.lifetimeCountdownSuppressed;
    activeDurationLimitEnabled = other.activeDurationLimitEnabled;
    attenuationRequired = other.attenuationRequired;
    trackElapsedTime = other.trackElapsedTime;
    suppressionEnabled = other.suppressionEnabled;
    environmentId = other.environmentId;
    std::memcpy(unknownA6, other.unknownA6, sizeof(unknownA6));
    suppressionMarker = 1;
    twinEventHandle = 0;
    attenuation = other.attenuation;
    directionIndex = other.directionIndex;
    matrixId = other.matrixId;
    startDelayMs = other.startDelayMs;
    remainingLifetimeMs = other.remainingLifetimeMs;
    remainingActiveTimeMs = other.remainingActiveTimeMs;
    distance = other.distance;
    previousDistance = other.previousDistance;
    previousRadialSpeed = other.previousRadialSpeed;
    previousDistanceTime = other.previousDistanceTime;
    accumulatedDistance = other.accumulatedDistance;
    accumulatedTime = other.accumulatedTime;
    distanceSampleCount = other.distanceSampleCount;
    frequencyScale = other.frequencyScale;
    baseFrequencyScale = other.baseFrequencyScale;
    backendParameter = other.backendParameter;
    // The original copies the time of the previous distance a second time.
    previousDistanceTime = other.previousDistanceTime;
    notificationFlags = other.notificationFlags;
    notificationEvent = 0;
}

ScheduledEarEvent::~ScheduledEarEvent() {
    if (playback) {
        playback->ReleaseEarPlaybackResources();
        ReleasePlaybackStorage(playback);
        playback = nullptr;
    }
    if (allocatedSource && suppressionMarker != 1) {
        ::operator delete(allocatedSource);
        // Native failure cleanup can invoke destruction repeatedly on the same storage, so the
        // barrier keeps the store from being dropped as dead.
        allocatedSource = nullptr;
        __asm__ __volatile__("" : : : "memory");
    }
}

int ScheduledEarEvent::CreateEarPlaybackImplementation(std::uint16_t flags) {
    switch (sourceType) {
    case 0:
        playback = AllocatePlaybackImplementation<PcmWaveStream>();
        break;
    case 2:
        playback = AllocatePlaybackImplementation<MemoryWaveStream>();
        break;
    case 3:
        playback = AllocatePlaybackImplementation<SampleWavePlayback>();
        break;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    case 6:
        playback = AllocatePlaybackImplementation<MidiPlayback>();
        break;
    case 30:
        playback = AllocatePlaybackImplementation<CdAudioPlayback>();
        break;
#endif
    case 31:
        playback = AllocatePlaybackImplementation<ImaAdpcmWaveStream>();
        break;
    default:
        playback = nullptr;
        return 0;
    }
#ifdef ENABLE_PATCHES
    if (playback == nullptr) {
        return 0;
    }
#else
    // Native allocation failure still proceeds to the flags write.
#endif
    playback->SetEarPlaybackFlags(flags);
    return 1;
}

void ScheduledEarEvent::ReleaseScheduledEventPlayback() {
    if (!attenuationRequired) {
        RequestScheduledEventStop();
        return;
    }
    if (attenuation || !playbackInitialized) {
        return;
    }
    playback->ReleaseEarPlaybackResources();
    ReleasePlaybackStorage(playback);
    playback = nullptr;
    playbackInitialized = 0;
    CreateEarPlaybackImplementation(sourceFlags);
    if (!playback) {
        SetEarLastError(0x10);
        this->~ScheduledEarEvent();
    }
}

void ScheduledEarEvent::DeactivateScheduledEarEvent() {
    if (!g_ActiveEventRegistry.IsEmpty()) {
        g_ActiveEventRegistry.RemoveKey(notificationEvent);
    }
    if (!playbackInitialized) {
        playback->MarkComplete();
    } else if (streaming) {
        playback->CompleteEarPlayback();
    } else {
        playback->MarkComplete();
    }
}

int ScheduledEarEvent::InitializeScheduledEarEventPlayback() {
    playbackInitialized = 1;
    // Native code clears a local that it never reads.
    [[maybe_unused]] int unusedLocal = 0;
    playback->SetEarPlaybackNotification(notificationFlags, notificationEvent);
    if (frequencyScaleOverrideEnabled || dopplerEnabled) {
        playback->SetBackendPlaybackOverrideEnabled(1);
    }
    if (suppressionEnabled) {
#ifdef ENABLE_PATCHES
        EarEnvironment *environment = nullptr;
        if (!g_EnvironmentRegistry.LookupWordPointerMapValue(environmentId, environment) ||
            environment == nullptr) {
            playbackInitialized = 0;
            DeactivateScheduledEarEvent();
            return 0;
        }
#else
        EarEnvironment *environment;
        // Native suppression assumes a registered environment.
        g_EnvironmentRegistry.LookupWordPointerMapValue(environmentId, environment);
#endif
        attenuation =
            TruncateGain(static_cast<long double>(environment->GetEnvironmentInitialGain()) *
                         g_earInitialEventAttenuationScale * g_earEventTimeUnitScale);
        matrixId = 1;
        streaming = 1;
        looping = 1;
        if (!suppressionMarker) {
            directionIndex = 4;
        } else {
            directionIndex = 8;
            startPosition = 0.5;
            baseFrequencyScale = 0.96;
        }
        playback->SetPlaybackSuppressionParameters(suppressionEnabled, suppressionMarker);
    }
    if (sourceType == 3) {
        EarWaveSample *preset;
        if (!g_PresetRegistry.LookupWordPointerMapValue(source.presetHandle, preset)) {
            SetEarLastError(0x29);
            playbackInitialized = 0;
            DeactivateScheduledEarEvent();
            return 0;
        }
        playback->ApplyPreset(preset);
        streaming = 0;
    }
    playback->SetReservedPlaybackEventControl(reservedEventControl);
    playback->SetEarPlaybackBaseFrequencyScale(baseFrequencyScale);
    playback->SetCachedBackendPlaybackParameter(backendParameter);
    playback->SetEarPlaybackStartPosition(startPosition);
    playback->SetEarPlaybackStartMilliseconds(static_cast<std::uint16_t>(elapsedTimeMs));
    playback->SetEarPlaybackDirectionalMatrix(directionIndex, matrixId);
    playbackInitialized = playback->InitializeEarPlayback(source.address, streaming, looping);
    if (!playbackInitialized) {
        DeactivateScheduledEarEvent();
        return 0;
    }
    if (dopplerEnabled) {
        startDelayMs = AddTimer(startDelayMs, 50);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        previousDistanceTime = timeGetTime();
#else
        previousDistanceTime = GetEarTimeMilliseconds();
#endif
    }
    streaming = playback->IsEarPlaybackStreaming();
    return 1;
}

int ScheduledEarEvent::GetScheduledEventPlaybackStatus() {
    return playback->GetStatus();
}

int ScheduledEarEvent::IsScheduledEventPlaybackComplete() {
    return playback->IsBufferedEarPlaybackComplete();
}

std::uint32_t ScheduledEarEvent::GetScheduledEventMediaLength() {
    return playback->GetEarPlaybackMediaLength();
}

void ScheduledEarEvent::MuteScheduledEventPlayback() {
    playback->SetEarPlaybackMute(1);
}

void ScheduledEarEvent::UnmuteScheduledEventPlayback() {
    playback->SetEarPlaybackMute(0);
}

void ScheduledEarEvent::PauseScheduledEventPlayback() {
    playback->PauseEarPlayback();
}

int ScheduledEarEvent::ResumeScheduledEventPlayback() {
    return playback->ResumeEarPlayback();
}

int ScheduledEarEvent::IsScheduledEventPlaybackActive() {
    return playback->IsActive();
}

int ScheduledEarEvent::IsScheduledEventPlaybackMuted() {
    return playback->GetEarPlaybackMute();
}

int ScheduledEarEvent::HasScheduledEventFrequencyOverride() {
    return frequencyScaleOverrideEnabled;
}

int ScheduledEarEvent::IsScheduledEventPendingExpiryDisabled() {
    return lifetimeCountdownSuppressed;
}

int ScheduledEarEvent::IsScheduledEventPlaybackStreaming() {
    return playback->IsEarPlaybackStreaming();
}

int ScheduledEarEvent::IsScheduledEventStopping() {
    return fadingOut;
}

void ScheduledEarEvent::RequestScheduledEventStop() {
    playback->SetEarPlaybackLooping(0);
    playback->MarkComplete();
}

void ScheduledEarEvent::ApplyScheduledEventStopControl(std::uint16_t control) {
    if (control & 1) {
        looping = 0;
        playback->SetEarPlaybackLooping(looping);
        StartScheduledEarEvent();
        return;
    }
    fadingOut = 1;
    switch (control & 15) {
    case 2:
        attenuationDecrement = 500;
        break;
    case 4:
        attenuationDecrement = 250;
        break;
    case 8:
        attenuationDecrement = 125;
        break;
    case 14:
        attenuationDecrement =
            TruncateGain(static_cast<long double>(g_earEventTimeUnitScale) /
                         (static_cast<long double>(control >> 4) * g_earStopControlPercentScale));
        break;
    default:
        attenuationDecrement = 0;
        break;
    }
}

void ScheduledEarEvent::UpdateScheduledEventPlayback(double gain) {
    if (fadingOut) {
        if (!attenuation) {
            RequestScheduledEventStop();
        } else {
            std::uint16_t decrement =
                attenuation >= attenuationDecrement ? attenuationDecrement : attenuation;
            attenuation -= decrement;
        }
    }
    double scaledGain = static_cast<double>(static_cast<long double>(attenuation) *
                                            g_earPlaybackAttenuationScale * gain);
    playback->SetEarPlaybackDirectionalGain(directionIndex, scaledGain, matrixId);
    if (frequencyScaleOverrideEnabled || dopplerEnabled) {
        UpdateScheduledEventDopplerScale();
    }
    playback->ProcessEarPlaybackSmoothing();
    playback->SetCachedBackendPlaybackParameter(backendParameter);
    if (!attenuation && attenuationRequired && !fadingOut) {
        ReleaseScheduledEventPlayback();
    }
}

void ScheduledEarEvent::UpdateScheduledEventDopplerScale() {
    if (frequencyScaleOverrideEnabled || dopplerEnabled) {
        playback->SetEarPlaybackFrequencyScale(
            static_cast<double>(CalculateScheduledEventDopplerScale()));
    }
}

long double ScheduledEarEvent::CalculateScheduledEventDopplerScale() {
    if (!dopplerEnabled) {
        return frequencyScale;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    DWORD now = timeGetTime();
#else
    DWORD now = GetEarTimeMilliseconds();
#endif
    if (!previousDistance || !previousDistanceTime) {
        previousDistance = distance;
        previousDistanceTime = now;
        return frequencyScale;
    }
    EarEnvironment *environment;
    if (!g_EnvironmentRegistry.LookupWordPointerMapValue(environmentId, environment)) {
        SetEarLastError(0x1f);
        return frequencyScale;
    }
    ++distanceSampleCount;
    accumulatedDistance += AbsoluteDistance(SubtractTimer(previousDistance, distance));
    accumulatedTime += now - previousDistanceTime;
    // Native unsigned quotients feed signed x87 integer loads.
    std::uint32_t averageDistance = accumulatedDistance / distanceSampleCount;
    std::uint32_t averageTime = accumulatedTime / distanceSampleCount;
    std::uint16_t crossover = environment->GetEnvironmentDopplerCrossoverDistance();
    if (std::bit_cast<std::int32_t>(AbsoluteDistance(distance)) <= crossover || crossingActive) {
        if (!crossingActive) {
            crossingInitialSpeed = previousRadialSpeed;
            crossingSpeedStep = static_cast<double>(static_cast<long double>(crossingInitialSpeed) /
                                                    (static_cast<long double>(crossover) /
                                                     std::bit_cast<std::int32_t>(averageDistance)));
            crossingActive = 1;
            crossingStep = 1;
        }
        previousRadialSpeed =
            static_cast<double>(static_cast<long double>(crossingInitialSpeed) -
                                static_cast<long double>(crossingSpeedStep) * crossingStep);
        crossingStep =
            static_cast<double>(static_cast<long double>(crossingStep) + g_earDopplerUnity);
        if (-crossingInitialSpeed >= previousRadialSpeed) {
            crossingActive = 0;
        }
    } else {
        crossingActive = 0;
    }
    double radialSpeed = 0;
    if (previousDistance == distance) {
        radialSpeed = previousRadialSpeed;
    } else {
        if (!averageTime) {
            return frequencyScale;
        }
        if (!crossingActive) {
            radialSpeed = static_cast<double>(
                static_cast<long double>(std::bit_cast<std::int32_t>(averageDistance)) /
                std::bit_cast<std::int32_t>(averageTime));
        } else {
            radialSpeed = previousRadialSpeed;
        }
        radialDirection = previousDistance < distance ? -1 : 1;
        previousDistance = distance;
        previousDistanceTime = now;
        previousRadialSpeed = radialSpeed;
        // Native distance updates increment the sample count twice.
        ++distanceSampleCount;
    }
    std::int32_t direction = crossingActive ? 1 : radialDirection;
    double velocity = static_cast<double>(static_cast<long double>(direction) * radialSpeed);
    double denominator =
        static_cast<double>(g_earDopplerUnity - static_cast<long double>(velocity) /
                                                    environment->GetEnvironmentSoundSpeed());
    // Unordered comparisons also select the cached frequency scale.
    if (!(denominator < 0 || denominator > 0)) {
        return frequencyScale;
    }
    return (static_cast<long double>(g_earDopplerUnity) / denominator) * frequencyScale;
}

int ScheduledEarEvent::StartScheduledEarEvent() {
    if (!playbackInitialized) {
        playbackInitialized = InitializeScheduledEarEventPlayback();
    }
    if (!playbackInitialized) {
        DeactivateScheduledEarEvent();
        SetEarLastError(0x1a);
        return 0;
    }
    if (playback->GetStatus()) {
        return 0;
    }
    if (frequencyScaleOverrideEnabled || dopplerEnabled) {
        playback->SetEarPlaybackFrequencyScale(
            static_cast<double>(CalculateScheduledEventDopplerScale()));
    }
    playback->ProcessEarPlaybackSmoothing();
    if (!playback->StartEarPlayback()) {
        DeactivateScheduledEarEvent();
        return 0;
    }
    playbackStarted = 1;
    return 1;
}

int ScheduledEarEvent::CanStartScheduledEarEvent() {
    return !playbackInitialized && startDelayMs <= 0 && (!attenuationRequired || attenuation > 0) &&
           (!distanceLimitEnabled || distance < maximumDistance);
}

void ScheduledEarEvent::AdvanceScheduledEventTimers(std::uint32_t elapsedMilliseconds) {
    if (playbackInitialized) {
        if (activeDurationLimitEnabled && playback->IsActive()) {
            remainingActiveTimeMs = SubtractTimer(remainingActiveTimeMs, elapsedMilliseconds);
            if (remainingActiveTimeMs <= 0) {
                playback->MarkComplete();
            }
        }
    } else {
        startDelayMs = SubtractTimer(startDelayMs, elapsedMilliseconds);
        if (trackElapsedTime) {
            elapsedTimeMs = AddTimer(elapsedTimeMs, elapsedMilliseconds);
        }
        if (remainingLifetimeMs == -1000) {
            return;
        }
        if (!lifetimeCountdownSuppressed) {
            remainingLifetimeMs = SubtractTimer(remainingLifetimeMs, elapsedMilliseconds);
        }
        if (remainingLifetimeMs < -500) {
            playback->MarkComplete();
        }
    }
}

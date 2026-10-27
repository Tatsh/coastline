#include "scheduledevent.h"

#include <bit>

#include <mmsystem.h>

#include "earplayback.h"
#include "environment.h"
#include "error.h"
#include "registry.h"

namespace {
// @ghidraAddress EAR: 0x1002e7a8
const double g_earInitialEventAttenuationScale = 0.0001;
// @ghidraAddress EAR: 0x1002e7b0
const double g_earEventTimeUnitScale = 10000;
// @ghidraAddress EAR: 0x1002e7b8
const double g_earDopplerUnity = 1;

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
    playback->SetEarPlaybackNotification(notificationFlags, notificationEvent);
    if (frequencyScaleOverrideEnabled || dopplerEnabled) {
        playback->SetBackendPlaybackOverrideEnabled(1);
    }
    if (suppressionEnabled) {
        EarEnvironment *environment;
        // Native suppression assumes a registered environment.
        g_EnvironmentRegistry.LookupWordPointerMapValue(environmentId, environment);
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
        EarPreset *preset;
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
        previousDistanceTime = timeGetTime();
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

void ScheduledEarEvent::RequestScheduledEventStop() {
    playback->SetEarPlaybackLooping(0);
    playback->MarkComplete();
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
    DWORD now = timeGetTime();
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
    double radialSpeed;
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

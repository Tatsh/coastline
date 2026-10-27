#include "scheduledevent.h"

#include <bit>

#include <mmsystem.h>

#include "earplayback.h"
#include "environment.h"
#include "error.h"
#include "registry.h"

namespace {
// @ghidraAddress EAR: 0x1002e7a8
const double g_earInitialGainUnitScale = 0.0001;
// @ghidraAddress EAR: 0x1002e7b0
const double g_earScheduledGainScale = 10000;

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
                         g_earInitialGainUnitScale * g_earScheduledGainScale);
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
           (!repeatLimitEnabled || repeatCount < repeatLimit);
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

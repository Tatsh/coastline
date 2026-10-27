#include "scheduledevent.h"

#include <bit>

#include "earplayback.h"
#include "error.h"

namespace {
static inline std::int32_t SubtractTimer(std::int32_t timer, std::uint32_t elapsed) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(timer) - elapsed);
}

static inline std::int32_t AddTimer(std::int32_t timer, std::uint32_t elapsed) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(timer) + elapsed);
}
} // namespace

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

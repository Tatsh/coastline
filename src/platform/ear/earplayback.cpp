#include "earplayback.h"

const double g_earPlaybackScaleZero = 0;
const double g_earPlaybackMaximumFrequencyScale = 6.5535;

int EarPlayback::GetStatus() {
    return status;
}

int EarPlayback::IsActive() {
    return active;
}

void EarPlayback::MarkComplete() {
    completionRequested = 1;
}

void EarPlayback::SetEarPlaybackDirectionalMatrix(std::uint16_t direction, std::uint16_t matrix) {
    directionIndex = direction;
    matrixId = matrix;
}

void EarPlayback::SetPlaybackSuppressionParameters(std::uint32_t enabled, std::uint16_t marker) {
    suppressionEnabled = enabled;
    suppressionMarker = marker;
}

void EarPlayback::SetEarPlaybackStartMilliseconds(std::uint16_t milliseconds) {
    startMilliseconds = milliseconds;
}

void EarPlayback::SetReservedPlaybackEventControl(std::uint32_t control) {
    reservedEventControl = control;
}

void EarPlayback::SetEarPlaybackMute(int value) {
    muted = value;
    SetEarPlaybackDirectionalGain(directionIndex, 2.0, matrixId);
}

int EarPlayback::GetEarPlaybackMute() {
    return muted;
}

int EarPlayback::IsEarPlaybackStreaming() {
    return streaming;
}

void EarPlayback::SetBackendPlaybackOverrideEnabled(int enabled) {
    backendOverrideEnabled = enabled;
}

int EarPlayback::GetBackendPlaybackOverrideEnabled() {
    return backendOverrideEnabled;
}

void EarPlayback::SetEarPlaybackBaseFrequencyScale(double scale) {
    baseFrequencyScale = scale;
}

void EarPlayback::SetCachedBackendPlaybackParameter(double value) {
    // Unordered comparisons preserve the cached value.
    if (cachedBackendParameter < value || cachedBackendParameter > value) {
        cachedBackendParameter = value;
        OnBackendPlaybackParameterChanged();
    }
}

void EarPlayback::SetEarPlaybackFrequencyScale(double scale) {
    if (frequencyScale < scale || frequencyScale > scale) {
        double clamped = scale < g_earPlaybackScaleZero ? g_earPlaybackScaleZero : scale;
        if (clamped > g_earPlaybackMaximumFrequencyScale) {
            clamped = g_earPlaybackMaximumFrequencyScale;
        }
        frequencyScale = clamped;
        if (active) {
            previousFrequency = frequency;
        }
        frequency = static_cast<std::uint32_t>(
            static_cast<std::int64_t>(static_cast<long double>(baseFrequency) * frequencyScale));
    }
}

int EarPlayback::GetEarPlaybackLooping() {
    return looping;
}

void EarPlayback::SetEarPlaybackLooping(int enabled) {
    looping = enabled;
}

void EarPlayback::SetEarPlaybackStartPosition(double position) {
    if (position < g_earPlaybackScaleZero || position > g_earPlaybackScaleZero) {
        startPositionSpecified = 1;
        startPosition = position;
    }
}

void EarPlayback::SetEarPlaybackNotification(std::uint32_t flags, std::uint16_t event) {
    notificationFlags = flags;
    notificationEvent = event;
}

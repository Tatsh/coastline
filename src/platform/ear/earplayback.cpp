#include "earplayback.h"

const double g_earPlaybackScaleZero = 0;
const double g_earPlaybackMaximumFrequencyScale = 6.5535;
const double g_earMaximumDirectionalGain = 1;
std::uint16_t g_wPlaybackSmoothingPassCount = 2;
int g_earPlaybackSmoothingEnabled;

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

void EarPlayback::SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                                double gain,
                                                std::uint16_t matrix) {
    if (!sample) {
        return;
    }
    if (matrix == matrixId && direction == directionIndex &&
        !(gain < targetGain || gain > targetGain)) {
        return;
    }
    if (active) {
        ResetEarPlaybackSmoothingPass();
        previousGain = targetGain;
        previousMatrixEntry = targetMatrixEntry;
    }
    // Values above unity request the cached gain.
    double effectiveGain = gain > g_earMaximumDirectionalGain ? targetGain : gain;
    CopyDirectionalMatrixEntry(matrix, direction, &targetMatrixEntry);
    directionIndex = direction;
    if (!muted) {
        targetGain = effectiveGain;
    }
    matrixId = matrix;
}

void EarPlayback::ProcessEarPlaybackSmoothing() {
    if (!active) {
        smoothingPass = 0;
    }
    if (smoothingPass == 0 || smoothingPass == g_wPlaybackSmoothingPassCount) {
        previousGain = targetGain;
        previousMatrixEntry = targetMatrixEntry;
        previousFrequency = frequency;
    }
    UpdateDirectSoundPlaybackFrequency();
    if (smoothingPass > g_wPlaybackSmoothingPassCount) {
        return;
    }
    UpdateDirectSoundPlaybackGainAndPan();
    if (g_earPlaybackSmoothingEnabled) {
        smoothingPass = static_cast<std::uint16_t>(g_wPlaybackSmoothingPassCount + 1);
    } else {
        ++smoothingPass;
    }
}

void EarPlayback::ResetEarPlaybackSmoothingPass() {
    smoothingPass = 1;
}

void EarPlayback::ReleaseEarPlaybackResources() {
    CloseRegisteredEarPlayback();
}

int EarPlayback::InitializeEarPlayback(const void *source, int streaming, int looping) {
    return 1;
}

std::uint32_t EarPlayback::GetEarPlaybackMediaLength() {
    return 0;
}

void EarPlayback::FinalizeDirectSoundPlaybackState() {
}

void EarPlayback::RefillEarPlaybackStream() {
}

void EarPlayback::CopyEarPlaybackSamplesToOutput() {
}

void EarPlayback::CompleteEarPlayback() {
}

void EarPlayback::OnBackendPlaybackParameterChanged() {
}

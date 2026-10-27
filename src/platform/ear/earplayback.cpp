#include "earplayback.h"

#include <bit>
#include <cstring>

#include <dsound.h>

#include "error.h"
#include "volume.h"
#include "window.h"

std::uint16_t g_wPlaybackSmoothingPassCount = 2;
int g_earPlaybackSmoothingEnabled;

namespace {
// @ghidraAddress EAR: 0x1002ea68
const EarDirectionalMatrixEntry g_earPlaybackDefaultMatrixEntry = {0, 0};
// @ghidraAddress EAR: 0x1002ea78
const double g_earPlaybackScaleZero = 0;
// @ghidraAddress EAR: 0x1002ea80
const double g_earPlaybackMaximumFrequencyScale = 6.5535;
// @ghidraAddress EAR: 0x1002ea70
const double g_earMaximumDirectionalGain = 1;
// @ghidraAddress EAR: 0x100360c0
const double g_adPlaybackSmoothingWeights[3] = {1.0, 0.5, 1.0};
// @ghidraAddress EAR: 0x1002ee40
const double g_earDirectSoundGainPercentScale = 100;
// @ghidraAddress EAR: 0x1003b878
std::uint32_t g_earBufferedPlaybackCurrentHalfStart;
// @ghidraAddress EAR: 0x1003b87c
std::uint32_t g_earBufferedPlaybackCursor;
} // namespace

EarPlayback::EarPlayback() {
    sample = nullptr;
    notificationFlags = 0;
    notificationEvent = 0;
    looping = 0;
    streaming = 0;
    completionRequested = 0;
    streamHalfIndex = 1;
    active = 0;
    muted = 0;
    status = 0;
    backendOverrideEnabled = 0;
    startPositionSpecified = 0;
    suppressionEnabled = 0;
    suppressionMarker = 0;
    startMilliseconds = 0;
    std::memset(unknown70, 0, sizeof(unknown70));
    streamHalfBytes = 0;
    targetMatrixEntry = g_earPlaybackDefaultMatrixEntry;
    previousMatrixEntry = g_earPlaybackDefaultMatrixEntry;
    directionIndex = 0;
    targetGain = 0;
    previousGain = 0;
    frequency = 0;
    previousFrequency = 0;
    matrixId = 16;
    reservedEventControl = 0;
    smoothingPass = 0;
    startPosition = 0;
    frequencyScale = 1;
    baseFrequencyScale = 1;
    cachedBackendParameter = -1;
    loopDrainCount = 0;
    // Native construction does not initialise flags or baseFrequency.
    InitializeDirectSoundPlaybackState();
}

void EarPlayback::SetEarPlaybackFlags(std::uint16_t value) {
    flags = value;
}

int EarPlayback::GetStatus() {
    return status;
}

int EarPlayback::StartEarPlayback() {
    if (notificationFlags & 1) {
        PostEarWindowMessage(notificationEvent, 1);
    }
    if (!PlayDirectSoundEventBuffer()) {
        return 0;
    }
    active = 1;
    status = 0;
    return 1;
}

void EarPlayback::PauseEarPlayback() {
    status = 1;
    StopDirectSoundEventBuffer();
}

int EarPlayback::ResumeEarPlayback() {
    if (status && active) {
        StartEarPlayback();
    }
    status = 0;
    return 1;
}

int EarPlayback::CheckEarPlaybackLoopCompletion() {
    return IsDirectSoundBufferStopped();
}

int EarPlayback::IsBufferedEarPlaybackComplete() {
    if (completionRequested) {
        return 1;
    }
    if ((looping && !streaming) || !active) {
        return 0;
    }
    g_earBufferedPlaybackCursor = GetDirectSoundPlaybackCursor();
    if (streaming) {
        g_earBufferedPlaybackCurrentHalfStart = streamHalfIndex * streamHalfBytes;
        if (g_earBufferedPlaybackCursor >= g_earBufferedPlaybackCurrentHalfStart &&
            g_earBufferedPlaybackCursor <=
                g_earBufferedPlaybackCurrentHalfStart + streamHalfBytes) {
            RefillEarPlaybackStream();
        }
    } else {
        completionRequested = IsDirectSoundBufferStopped();
    }
    return completionRequested;
}

void EarPlayback::OnPlaybackWaveSampleAttached(EarWaveSample *sample) {
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

void EarDirectSoundPlaybackState::Initialize() {
    buffer = nullptr;
    writeCursor = 0;
    playCursor = 0;
}

void EarPlayback::InitializeDirectSoundPlaybackState() {
    directSound.Initialize();
    directSound.playCursor = 1;
}

DWORD EarPlayback::GetDirectSoundPlaybackCursor() {
    if (sample && !completionRequested) {
        // Native heap allocation and cursor offsets provide DWORD alignment.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-packed-member"
        directSound.buffer->GetCurrentPosition(&directSound.playCursor, &directSound.writeCursor);
#pragma clang diagnostic pop
    }
    return directSound.playCursor;
}

void EarPlayback::UpdateDirectSoundPlaybackFrequency() {
    if (!directSound.buffer) {
        return;
    }
    std::uint32_t smoothed;
    if (frequency != previousFrequency) {
        std::int32_t delta = std::bit_cast<std::int32_t>(frequency - previousFrequency);
        std::uint32_t step = static_cast<std::uint32_t>(static_cast<std::int64_t>(
            static_cast<long double>(delta) * g_adPlaybackSmoothingWeights[smoothingPass]));
        smoothed = previousFrequency + step;
    } else {
        smoothed = previousFrequency;
    }
    if (smoothed < DSBFREQUENCY_MIN) {
        smoothed = DSBFREQUENCY_MIN;
    }
    if (smoothed > 100000) {
        smoothed = 100000;
    }
    directSound.buffer->SetFrequency(smoothed);
}

int EarPlayback::PlayDirectSoundEventBuffer() {
    if (!directSound.buffer) {
        SetEarLastError(19);
        return FALSE;
    }
    if (suppressionEnabled && suppressionMarker == 1) {
        return TRUE;
    }
    DWORD flags = looping || streaming ? DSBPLAY_LOOPING : 0;
    if (directSound.buffer->Play(0, 0, flags) != DS_OK) {
        SetEarLastError(20);
        return FALSE;
    }
    return TRUE;
}

int EarPlayback::StopDirectSoundEventBuffer() {
    if (!directSound.buffer) {
        return FALSE;
    }
    if (active && directSound.buffer->Stop() != DS_OK) {
        SetEarLastError(37);
        return FALSE;
    }
    return TRUE;
}

void EarPlayback::ReleaseDirectSoundEventBuffer() {
    if (directSound.buffer) {
        directSound.buffer->Stop();
        directSound.buffer->Release();
        directSound.buffer = nullptr;
    }
}

int EarPlayback::IsDirectSoundBufferStopped() {
    DWORD bufferStatus;
    directSound.buffer->GetStatus(&bufferStatus);
    return (bufferStatus & DSBSTATUS_PLAYING) == 0;
}

void EarPlayback::UpdateDirectSoundPlaybackGainAndPan() {
    double target = muted ? 0.0 :
                            static_cast<double>(static_cast<long double>(targetGain) *
                                                g_dMasterAttenuationLevel * targetMatrixEntry.gain *
                                                g_earDirectSoundGainPercentScale);
    double previous =
        muted ?
            0.0 :
            static_cast<double>(static_cast<long double>(previousGain) * g_dMasterAttenuationLevel *
                                previousMatrixEntry.gain * g_earDirectSoundGainPercentScale);
    double interpolated = static_cast<double>((static_cast<long double>(target) - previous) *
                                                  g_adPlaybackSmoothingWeights[smoothingPass] +
                                              previous);
    std::int32_t delta =
        std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(targetMatrixEntry.pan) -
                                    static_cast<std::uint32_t>(previousMatrixEntry.pan));
    std::uint32_t step = static_cast<std::uint32_t>(static_cast<std::int64_t>(
        static_cast<long double>(delta) * g_adPlaybackSmoothingWeights[smoothingPass]));
    LONG pan = std::bit_cast<LONG>(static_cast<std::uint32_t>(previousMatrixEntry.pan) + step);
    if (sample) {
        directSound.buffer->SetVolume(ConvertPercentGainToDirectSoundVolume(interpolated));
        directSound.buffer->SetPan(pan);
    }
}

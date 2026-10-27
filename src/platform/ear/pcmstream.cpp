#include <bit>
#include <cstdlib>
#include <cstring>

#include <dsound.h>

#include "directsoundoutput.h"
#include "error.h"
#include "playbackimplementations.h"
#include "registry.h"
#include "wavechunks.h"

namespace {
// @ghidraAddress EAR: 0x1003b9a4
DWORD g_dwDirectSoundLockRegion2Bytes;
// @ghidraAddress EAR: 0x1003b9a8
DWORD g_dwDirectSoundLockOffset;
// @ghidraAddress EAR: 0x1003b9ac
void *g_pDirectSoundLockRegion1;
// @ghidraAddress EAR: 0x1003b9b0
void *g_pDirectSoundLockRegion2;
// @ghidraAddress EAR: 0x1003b9b4
DWORD g_dwDirectSoundLockRegion1Bytes;
// @ghidraAddress EAR: 0x1002ebc8
const double g_earPcmHalfBufferScale = 0.5;
// @ghidraAddress EAR: 0x1002ebd0
const double g_earPcmMillisecondScale = 0.001;

static inline std::uint32_t TruncatePlaybackInteger(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    return value >= -0x1p63L && value < 0x1p63L ?
               static_cast<std::uint32_t>(static_cast<std::int64_t>(value)) :
               0;
}
} // namespace
PcmWaveStream::PcmWaveStream() {
    sampleBuffer = nullptr;
    file = nullptr;
    dataBytes = 0;
    readOffset = 0;
    writeOffset = 0;
}

int PcmWaveStream::OpenWaveFileStream(const char *path) {
    file = mmioOpenA(const_cast<char *>(path), nullptr, 0);
    if (!file) {
        SetEarLastError(0x0d);
        return 0;
    }
    return 1;
}

int PcmWaveStream::CheckWavePlaybackDeviceAccess() {
    return 1;
}

void PcmWaveStream::CloseWaveFileStream() {
    if (file) {
        if (!(GetPlaybackSourceFlags() & 0x6000)) {
            mmioClose(file, 0);
        } else {
            mmioSeek(file, originalFileOffset, SEEK_SET);
        }
        file = nullptr;
    }
}

int PcmWaveStream::FailPcmWavePreparation(std::uint32_t error) {
    SetEarLastError(error);
    if (sampleBuffer) {
        std::free(sampleBuffer);
    }
    sampleBuffer = nullptr;
    CloseWaveFileStream();
    return 0;
}

int PcmWaveStream::PreparePcmWaveStream(const void *source) {
    DWORD offset = 0;
    if (!file) {
        return FailPcmWavePreparation(0x0d);
    }
    originalFileOffset = mmioSeek(file, 0, SEEK_CUR);
    if (mmioDescend(file, &g_RiffWaveChunk, nullptr, 0)) {
        return FailPcmWavePreparation(0x0e);
    }
    if (g_RiffWaveChunk.ckid != FOURCC_RIFF ||
        g_RiffWaveChunk.fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
        return FailPcmWavePreparation(0x0f);
    }
    if (mmioDescend(file, &g_WaveFormatChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
        return FailPcmWavePreparation(0x0e);
    }
    // Native reading includes cbSize even for a 16-byte PCM format chunk.
    if (mmioRead(file, static_cast<char *>(static_cast<void *>(&format)), sizeof(format)) !=
        sizeof(format)) {
        return FailPcmWavePreparation(0x0e);
    }
    if (g_WaveFormatChunk.cksize != sizeof(format)) {
        mmioSeek(file, std::bit_cast<LONG>(g_WaveFormatChunk.cksize - sizeof(format)), SEEK_CUR);
    }
    if (format.wFormatTag != WAVE_FORMAT_PCM) {
        return FailPcmWavePreparation(0x0f);
    }
    if (mmioAscend(file, &g_WaveFormatChunk, 0)) {
        return FailPcmWavePreparation(0x0e);
    }
    if (mmioDescend(file, &g_WaveDataChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
        return FailPcmWavePreparation(0x0e);
    }
    dataBytes = g_WaveDataChunk.cksize;
    if (!CheckWavePlaybackDeviceAccess()) {
        return FailPcmWavePreparation(6);
    }
    if (IsEarPlaybackStreaming() && dataBytes > g_earMaximumStreamingBufferBytes) {
        SetPlaybackBufferBytes(g_earMaximumStreamingBufferBytes);
    } else {
        SetPlaybackBufferBytes(dataBytes);
        DisablePlaybackStreaming();
    }
    SetStreamHalfBytes(TruncatePlaybackInteger(static_cast<long double>(GetPlaybackBufferBytes()) *
                                               g_earPcmHalfBufferScale));
    sampleBuffer = static_cast<char *>(std::malloc(GetPlaybackBufferBytes()));
    if (!sampleBuffer) {
        return FailPcmWavePreparation(0x10);
    }
    std::memset(sampleBuffer, format.wBitsPerSample == 16 ? 0 : 0x80, GetPlaybackBufferBytes());
    dataStartOffset = mmioSeek(file, 0, SEEK_CUR);
    if (dataStartOffset == -1) {
        return FailPcmWavePreparation(0x0e);
    }
    if (GetPlaybackStartMilliseconds()) {
        long double position = static_cast<long double>(format.nAvgBytesPerSec) *
                               GetPlaybackStartMilliseconds() * g_earPcmMillisecondScale;
        offset = TruncatePlaybackInteger(position) % dataBytes;
        SpecifyPlaybackStartPosition();
    }
    if (HasPlaybackStartPosition() && IsEarPlaybackStreaming()) {
        offset += TruncatePlaybackInteger(static_cast<long double>(dataBytes) *
                                          GetPlaybackStartPosition());
        if (offset % 2) {
            --offset;
        }
        readOffset = offset;
        if (mmioSeek(file,
                     std::bit_cast<LONG>(static_cast<DWORD>(dataStartOffset) + readOffset),
                     SEEK_SET) == -1) {
            return FailPcmWavePreparation(0x0e);
        }
        ClearPlaybackStartPosition();
    }
    readOffset += static_cast<DWORD>(
        mmioRead(file, sampleBuffer, std::bit_cast<LONG>(GetPlaybackBufferBytes())));
    if (readOffset != GetPlaybackBufferBytes() + offset) {
        if (readOffset == offset - 1) {
            return FailPcmWavePreparation(0x0e);
        }
        if (IsPlaybackLooping()) {
            if (mmioSeek(file, dataStartOffset, SEEK_SET) == -1) {
                return FailPcmWavePreparation(0x0e);
            }
            offset = readOffset - offset;
            readOffset = static_cast<DWORD>(
                mmioRead(file,
                         sampleBuffer + offset,
                         std::bit_cast<LONG>(GetPlaybackBufferBytes() - offset)));
            if (readOffset != GetPlaybackBufferBytes() - offset) {
                return FailPcmWavePreparation(0x0e);
            }
        } else {
            DisablePlaybackStreaming();
        }
    }
    if (!IsEarPlaybackStreaming()) {
        CloseWaveFileStream();
    }
    return 1;
}

int PcmWaveStream::InitializeEarPlayback(const void *source, int streaming, int looping) {
    SetPlaybackStreamingAndLooping(streaming, looping);
    if (!source) {
        SetEarLastError(0x0d);
        return 0;
    }
    std::uint16_t mode = GetPlaybackSourceFlags() & 0x6000;
    if (!mode) {
        if (!OpenWaveFileStream(static_cast<const char *>(source))) {
            return 0;
        }
    } else if (mode == 0x4000) {
        file = static_cast<HMMIO>(const_cast<void *>(source));
    }
    if (!PreparePcmWaveStream(source)) {
        return 0;
    }
    SetPlaybackBaseFrequency(TruncatePlaybackInteger(
        static_cast<long double>(format.nSamplesPerSec) * GetPlaybackBaseFrequencyScale()));
    SetPlaybackHandle(AllocateEarEventId());
    int count = g_PlaybackRegistry.GetCount();
    if (static_cast<UINT>(count) == g_PlaybackRegistry.GetHashTableSize()) {
        SetEarLastError(0x1a);
        SetPlaybackHandle(0);
    }
    if (!HasPlaybackHandle()) {
        if (sampleBuffer) {
            std::free(sampleBuffer);
            sampleBuffer = nullptr;
        }
        return 0;
    }
    g_PlaybackRegistry.SetDwordPointerMapValue(GetPlaybackHandle(), this);
    return 1;
}

std::uint16_t PcmWaveStream::AllocateEarEventId() {
    DSBUFFERDESC description = {};
    // The native DirectSound descriptor predates the later GUID extension.
    description.dwSize = 20;
    description.dwFlags = 0x101e0;
    description.dwBufferBytes = GetPlaybackBufferBytes();
    description.lpwfxFormat = &format;
    if (CreateDirectSoundPlaybackBuffer(g_pEarDirectSoundOutput->GetDirectSoundInterface(),
                                        &description) != DS_OK) {
        SetEarLastError(0x11);
        return 0;
    }
    ResetPlaybackBufferFrequency();
    ProcessEarPlaybackSmoothing();
    void *region1;
    void *region2;
    DWORD region1Bytes;
    DWORD region2Bytes;
    HRESULT result = GetDirectSoundBuffer()->Lock(
        0, GetPlaybackBufferBytes(), &region1, &region1Bytes, &region2, &region2Bytes, 0);
    if (result == DSERR_BUFFERLOST) {
        GetDirectSoundBuffer()->Restore();
        result = GetDirectSoundBuffer()->Lock(
            0, GetPlaybackBufferBytes(), &region1, &region1Bytes, &region2, &region2Bytes, 0);
        if (result != DS_OK) {
            SetEarLastError(0x13);
            return 0;
        }
    }
    // Other initial lock failures still proceed through the native copy path.
    std::memcpy(region1, sampleBuffer, region1Bytes);
    if (region2) {
        std::memcpy(region2, sampleBuffer + region1Bytes, region2Bytes);
    }
    if (GetDirectSoundBuffer()->Unlock(region1, region1Bytes, region2, region2Bytes) != DS_OK) {
        return 0;
    }
    if (HasNonStreamingStartPosition()) {
        long double position =
            static_cast<long double>(GetPlaybackBufferBytes()) * GetPlaybackStartPosition();
        DWORD offset = TruncatePlaybackInteger(position);
        if (GetDirectSoundBuffer()->SetCurrentPosition(offset) != DS_OK) {
            return 0;
        }
        ClearPlaybackStartPosition();
    }
    return g_wEarObjectHandleCounter++;
}

void PcmWaveStream::ReleaseEarPlaybackResources() {
    CloseRegisteredEarPlayback();
}

void PcmWaveStream::CloseRegisteredEarPlayback() {
    EarPlayback::CloseRegisteredEarPlayback();
    CloseWaveFileStream();
    if (sampleBuffer) {
        std::free(sampleBuffer);
    }
    sampleBuffer = nullptr;
}

void PcmWaveStream::CompleteEarPlayback() {
    if (!sampleBuffer || !HasPlaybackHandle()) {
        return;
    }
    if (CheckEarPlaybackLoopCompletion()) {
        DisablePlaybackLooping();
        MarkComplete();
    }
}

std::uint32_t PcmWaveStream::GetEarPlaybackMediaLength() {
    return dataBytes;
}

void PcmWaveStream::CopyEarPlaybackSamplesToOutput() {
    g_dwDirectSoundLockOffset = GetStreamHalfBytes() * !GetStreamHalfIndex();
    HRESULT result = GetDirectSoundBuffer()->Lock(g_dwDirectSoundLockOffset,
                                                  GetStreamHalfBytes(),
                                                  &g_pDirectSoundLockRegion1,
                                                  &g_dwDirectSoundLockRegion1Bytes,
                                                  &g_pDirectSoundLockRegion2,
                                                  &g_dwDirectSoundLockRegion2Bytes,
                                                  0);
    if (result == DSERR_BUFFERLOST) {
        GetDirectSoundBuffer()->Restore();
        result = GetDirectSoundBuffer()->Lock(g_dwDirectSoundLockOffset,
                                              GetStreamHalfBytes(),
                                              &g_pDirectSoundLockRegion1,
                                              &g_dwDirectSoundLockRegion1Bytes,
                                              &g_pDirectSoundLockRegion2,
                                              &g_dwDirectSoundLockRegion2Bytes,
                                              0);
        if (result != DS_OK) {
            SetEarLastError(0x13);
            MarkComplete();
            return;
        }
    }
    // Other initial lock failures still proceed through the native copy path.
    std::memcpy(g_pDirectSoundLockRegion1,
                sampleBuffer + g_dwDirectSoundLockOffset,
                g_dwDirectSoundLockRegion1Bytes);
    if (g_pDirectSoundLockRegion2) {
        std::memcpy(g_pDirectSoundLockRegion2,
                    sampleBuffer + g_dwDirectSoundLockOffset + g_dwDirectSoundLockRegion1Bytes,
                    g_dwDirectSoundLockRegion2Bytes);
    }
    if (GetDirectSoundBuffer()->Unlock(g_pDirectSoundLockRegion1,
                                       g_dwDirectSoundLockRegion1Bytes,
                                       g_pDirectSoundLockRegion2,
                                       g_dwDirectSoundLockRegion2Bytes) != DS_OK) {
        MarkComplete();
    }
}

int PcmWaveStream::CheckEarPlaybackLoopCompletion() {
    if (IncrementLoopDrainCount() < 2) {
        CopyEarPlaybackSamplesToOutput();
    } else {
        GetDirectSoundBuffer()->Stop();
        EndStreamingPlaybackLoop();
    }
    return IsDirectSoundBufferStopped();
}

void PcmWaveStream::RefillEarPlaybackStream() {
    writeOffset = GetStreamHalfBytes() * !GetStreamHalfIndex();
    std::memset(
        sampleBuffer + writeOffset, format.wBitsPerSample == 16 ? 0 : 0x80, GetStreamHalfBytes());
    if (!file) {
        CompleteEarPlayback();
        FlipStreamHalfIndex();
        return;
    }
    DWORD remaining = dataBytes - readOffset;
    DWORD requested = remaining < GetStreamHalfBytes() ? remaining : GetStreamHalfBytes();
    DWORD read = static_cast<DWORD>(
        mmioRead(file, sampleBuffer + writeOffset, std::bit_cast<LONG>(requested)));
    if (read != GetStreamHalfBytes()) {
        if (read == 0xffffffff) {
            SetEarLastError(0x0e);
            CompleteEarPlayback();
            return;
        }
        if (read == requested) {
            if (IsPlaybackLooping()) {
                if (mmioSeek(file, dataStartOffset, SEEK_SET) == -1) {
                    SetEarLastError(0x0e);
                    CompleteEarPlayback();
                    return;
                }
                readOffset = 0;
                DWORD extra =
                    static_cast<DWORD>(mmioRead(file,
                                                sampleBuffer + writeOffset + read,
                                                std::bit_cast<LONG>(GetStreamHalfBytes() - read)));
                readOffset += extra;
                if (readOffset != GetStreamHalfBytes() - read) {
                    SetEarLastError(0x0e);
                    CompleteEarPlayback();
                    return;
                }
                read = 0;
            } else {
                if (!(GetPlaybackSourceFlags() & 0x6000)) {
                    mmioClose(file, 0);
                }
                file = nullptr;
            }
        }
    }
    readOffset += read;
    CopyEarPlaybackSamplesToOutput();
    FlipStreamHalfIndex();
}

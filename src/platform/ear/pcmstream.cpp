#include <bit>
#include <cstdlib>
#include <cstring>

#include <dsound.h>

#include "directsoundoutput.h"
#include "error.h"
#include "playbackimplementations.h"

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
} // namespace
PcmWaveStream::PcmWaveStream() {
    sampleBuffer = nullptr;
    file = nullptr;
    dataBytes = 0;
    readOffset = 0;
    writeOffset = 0;
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
        DWORD offset = position >= -0x1p63L && position < 0x1p63L ?
                           static_cast<DWORD>(static_cast<std::int64_t>(position)) :
                           0;
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
    if (file) {
        if (!(GetPlaybackSourceFlags() & 0x6000)) {
            mmioClose(file, 0);
        } else {
            mmioSeek(file, originalFileOffset, SEEK_SET);
        }
        file = nullptr;
    }
    if (sampleBuffer) {
        std::free(sampleBuffer);
    }
    sampleBuffer = nullptr;
}

void PcmWaveStream::CompleteEarPlayback() {
    if (!sampleBuffer || !HasPlaybackWaveSample()) {
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

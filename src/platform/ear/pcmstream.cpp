#include <bit>
#include <cstdlib>
#include <cstring>

#include <dsound.h>

#include "error.h"

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
#include "playbackimplementations.h"

PcmWaveStream::PcmWaveStream() {
    sampleBuffer = nullptr;
    file = nullptr;
    dataBytes = 0;
    readOffset = 0;
    writeOffset = 0;
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

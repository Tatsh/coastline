#include "platform/win/sound.h"

#include <windows.h>

#include "platform/win/earsound.h"
#include "platform/win/settings.h"

IDirectSound *g_pDirectSound;
int g_dwDirectSoundRefCount;
IDirectSoundBuffer *g_pDirectSoundPrimaryBuffer;
WAVEFORMATEX *g_pDirectSoundWaveFormat;
void *g_pSoundBufferList;
int g_dwEarMusicChannel = 100;
int g_nMusicDeviceRefCount;
bool g_bEarMusicActive;

// A block of locked global memory is unlocked and freed.
static inline void FreeLockedGlobalMemory(void *pMemory) {
    GlobalUnlock(GlobalHandle(pMemory));
    GlobalFree(GlobalHandle(pMemory));
}

bool ShutdownDirectSound(void) {
    if (--g_dwDirectSoundRefCount != 0) {
        return true;
    }
    ResetAudioSubsystem();
    if (g_pDirectSoundPrimaryBuffer != nullptr) {
        g_pDirectSoundPrimaryBuffer->Stop();
        g_pDirectSoundPrimaryBuffer->Release();
        g_pDirectSoundPrimaryBuffer = nullptr;
    }
    if (g_pDirectSound != nullptr) {
        g_pDirectSound->Release();
        g_pDirectSound = nullptr;
    }
    if (g_pDirectSoundWaveFormat != nullptr) {
        FreeLockedGlobalMemory(g_pDirectSoundWaveFormat);
        g_pDirectSoundWaveFormat = nullptr;
    }
    if (g_pSoundBufferList != nullptr) {
        FreeLockedGlobalMemory(g_pSoundBufferList);
        g_pSoundBufferList = nullptr;
    }
    return true;
}

// The music modes test for no music again, although the caller already chose the mode.
static inline bool ReleaseCdAudioDevice(void) {
    bool bStopped;

    if (g_dwPlayMusicMode == 0) {
        return true;
    }
    g_nMusicDeviceRefCount--;
    bStopped = mciSendCommandA(g_dwMciCdAudioDeviceId, MCI_STOP, 0, 0) == 0;
    if (g_nMusicDeviceRefCount != 0) {
        return bStopped;
    }
    return (mciSendCommandA(g_dwMciCdAudioDeviceId, MCI_CLOSE, 0, 0) == 0) && bStopped;
}

static inline bool ReleaseEarMusicChannel(void) {
    bool bCleared;

    if (g_dwPlayMusicMode == 0) {
        return true;
    }
    g_nMusicDeviceRefCount--;
    bCleared = !g_bEarMusicActive || g_pfnEarClearChannel(g_dwEarMusicChannel, 0) == 0;
    if (g_nMusicDeviceRefCount == 0) {
        ShutdownEarSound();
    }
    return bCleared;
}

bool ReleaseMusicDeviceReference(void) {
    if (g_dwPlayMusicMode == 1) {
        return ReleaseCdAudioDevice();
    }
    return ReleaseEarMusicChannel();
}

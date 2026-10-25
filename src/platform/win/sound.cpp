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
int g_nCurrentMusicTrack;
int g_nMusicTrackMin;
int g_nMusicTrackMax;
HWND g_hMusicNotifyWnd;
bool g_bCdAudioPlaying;
bool g_bMusicPaused;

int GetEarMusicCurrentTrack(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return 0;
    }
    return g_bEarMusicActive ? g_nCurrentMusicTrack : 0;
}

int GetCdAudioCurrentTrack(void) {
    MCI_STATUS_PARMS status;

    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0 || !g_bEarMusicActive) {
        return 0;
    }
    // The callback is not set and the result of the command is not checked.
    status.dwItem = MCI_STATUS_CURRENT_TRACK;
    mciSendCommandA(
        g_dwMciCdAudioDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&status));
    return static_cast<int>(status.dwReturn);
}

bool PostCdAudioPlayNotify(int nTrack) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return true;
    }
    g_bCdAudioPlaying = true;
    g_nCurrentMusicTrack = nTrack;
    if (g_bEarMusicActive && g_bMusicPaused) {
        PostMessageA(
            g_hMusicNotifyWnd, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, g_dwMciCdAudioDeviceId);
    }
    return true;
}

MCIERROR MciPlayCdTracks(HWND hWnd, unsigned char nTrack) {
    MCI_PLAY_PARMS play;
    MCIERROR nError;

    play.dwCallback = reinterpret_cast<DWORD_PTR>(hWnd);
    play.dwFrom = nTrack;
    play.dwTo = g_nMusicTrackMax & 0xff;
    nError = mciSendCommandA(g_dwMciCdAudioDeviceId,
                             MCI_PLAY,
                             MCI_NOTIFY | MCI_FROM | MCI_TO,
                             reinterpret_cast<DWORD_PTR>(&play));
    if (nError != 0) {
        mciSendCommandA(g_dwMciCdAudioDeviceId, MCI_CLOSE, 0, 0);
        return nError;
    }
    return 0;
}

// The commands of the music do nothing without music, a user of its device, and a started
// channel.
static inline bool IsMusicPlaying(void) {
    return g_dwPlayMusicMode != 0 && g_nMusicDeviceRefCount != 0 && g_bEarMusicActive;
}

int ShutDownEarMusicChannel(void) {
    if (!IsMusicPlaying()) {
        return 1;
    }
    g_bEarMusicActive = false;
    return g_pfnEarShutDownChannel(g_dwEarMusicChannel);
}

bool PlayCdAudioFromCurrentTrack(void) {
    if (!IsMusicPlaying()) {
        return true;
    }
    return MciPlayCdTracks(g_hMusicNotifyWnd, static_cast<unsigned char>(g_nCurrentMusicTrack)) ==
           0;
}

bool SetCurrentMusicTrack(int nModeIndex) {
    if (g_dwPlayMusicMode == 1) {
        return PostCdAudioPlayNotify(nModeIndex);
    }
    return PlayEarMusicTrack(nModeIndex);
}

int GetCurrentMusicTrack(void) {
    if (g_dwPlayMusicMode == 1) {
        return GetCdAudioCurrentTrack();
    }
    return GetEarMusicCurrentTrack();
}

bool AdvanceMusicTrackWrapping(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return true;
    }
    g_nCurrentMusicTrack = GetCurrentMusicTrack() + 1;
    if (g_nCurrentMusicTrack > g_nMusicTrackMax) {
        g_nCurrentMusicTrack = g_nMusicTrackMin;
    }
    SetCurrentMusicTrack(g_nCurrentMusicTrack);
    return true;
}

// A command of the CD audio that fails closes the device.
static inline bool SendCdAudioCommand(UINT nCommand) {
    if (mciSendCommandA(g_dwMciCdAudioDeviceId, nCommand, 0, 0) == 0) {
        return true;
    }
    mciSendCommandA(g_dwMciCdAudioDeviceId, MCI_CLOSE, 0, 0);
    return false;
}

bool PauseCdAudio(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return true;
    }
    g_bCdAudioPlaying = true;
    if (!g_bEarMusicActive) {
        return true;
    }
    return SendCdAudioCommand(MCI_PAUSE);
}

bool PauseMusicPlayback(void) {
    if (g_dwPlayMusicMode == 1) {
        return PauseCdAudio();
    }
    if (!IsMusicPlaying()) {
        return true;
    }
    // The channel receives control 4, and any result but 0 succeeds.
    return g_pfnEarChangeChannelControl(g_dwEarMusicChannel, 4) != 0;
}

bool StopMusicTrack(void) {
    if (!IsMusicPlaying()) {
        return true;
    }
    if (g_dwPlayMusicMode == 1) {
        return SendCdAudioCommand(MCI_STOP);
    }
    return g_pfnEarClearChannel(g_dwEarMusicChannel, 0) == 0;
}

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

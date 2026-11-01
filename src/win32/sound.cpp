#include "win32/sound.h"

#include <cstdlib>
#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <mmreg.h>
#include <msacm.h>
#include <objbase.h>

#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/soundchannel.h"
#include "win32/app.h"
#include "win32/earsound.h"
#include "win32/timer.h"

MCIDEVICEID g_dwMciCdAudioDeviceId;
int g_dwEarMusicChannel = 100;
int g_nMusicDeviceRefCount;
int g_bMusicActive;
int g_nCurrentMusicTrack;
int g_nMusicTrackMin;
int g_nMusicTrackMax;
HWND g_hMusicNotifyWnd;
int g_bCdAudioPlaying;
unsigned char g_bCdMusicVolume;
int g_bEarOutputInitialized;
EarPlayParams g_EarPlayParams;

// The track the music starts at, the first after the track of the data of the CD.
#define FIRST_MUSIC_CD_TRACK 2

// Prepare the state of the music for the first user of its device.
static inline void ResetMusicTracks(void) {
    srand(GetHighResolutionTime());
    g_nCurrentMusicTrack = FIRST_MUSIC_CD_TRACK;
    g_nMusicTrackMin = 0;
    g_nMusicTrackMax = 0;
}

bool OpenCdaudioMci(HWND hWnd) {
    MCI_OPEN_PARMSA open;
    MCI_SET_PARMS set;
    MCI_STATUS_PARMS status;
    MCIERROR nError;
    char szMessage[0x400];

    if (!g_dwPlayMusicMode) {
        return true;
    }
    if (++g_nMusicDeviceRefCount != 1) {
        return true;
    }
    g_hMusicNotifyWnd = hWnd;
    ResetMusicTracks();
    g_bCdAudioPlaying = false;
    g_bMusicActive = false;
    open.lpstrDeviceType = "cdaudio";
    if (mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE, reinterpret_cast<DWORD_PTR>(&open)) != 0) {
        return false;
    }
    g_dwMciCdAudioDeviceId = open.wDeviceID;
    set.dwTimeFormat = MCI_FORMAT_TMSF;
    if (mciSendCommandA(g_dwMciCdAudioDeviceId,
                        MCI_SET,
                        MCI_SET_TIME_FORMAT,
                        reinterpret_cast<DWORD_PTR>(&set)) != 0) {
        mciSendCommandA(g_dwMciCdAudioDeviceId, MCI_CLOSE, 0, 0);
        return false;
    }
    status.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    nError = mciSendCommandA(g_dwMciCdAudioDeviceId,
                             MCI_STATUS,
                             MCI_STATUS_ITEM | MCI_WAIT,
                             reinterpret_cast<DWORD_PTR>(&status));
    if (nError != 0) {
        wsprintfA(szMessage, "MCI_STATUS returned %d\n", nError);
        OutputDebugStringA(szMessage);
        mciSendCommandA(g_dwMciCdAudioDeviceId, MCI_CLOSE, 0, 0);
        return false;
    }
    g_nMusicTrackMin = FIRST_MUSIC_CD_TRACK;
    g_nMusicTrackMax = static_cast<int>(status.dwReturn);
    g_bMusicActive = true;
    return true;
}

bool InitializeMusicPlayback(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0 || g_bMusicActive != 0) {
        return true;
    }
    if (g_pfnEarRegisterChannel(g_dwEarMusicChannel, 6, 10000, 5) == 0) {
        g_bMusicActive = 0;
        return true;
    }
    if (g_bEarOutputInitialized == 0) {
        g_pfnEarRegisterMatrix(1);
    }
    int nChannel = g_dwEarMusicChannel;
    g_bEarOutputInitialized = 1;
    g_EarPlayParams.wSize = 128;
    g_EarPlayParams.wType = 30;
    g_EarPlayParams.nTrack = 0;
    g_EarPlayParams.hEvent = 0;
    g_EarPlayParams.wChannel = nChannel;
    g_EarPlayParams.awUnknown0e[0] = 1;
    g_EarPlayParams.awUnknown0e[1] = 0;
    g_EarPlayParams.awUnknown0e[2] = 10000;
    g_EarPlayParams.adwUnknown14[0] = 10000;
    g_EarPlayParams.adwUnknown14[1] = 0;
    g_EarPlayParams.dwPlayFlags = 0;
    g_EarPlayParams.adwUnknown20[0] = 0;
    g_EarPlayParams.adwUnknown20[1] = 0;
    g_EarPlayParams.awUnknown28[0] = 4;
    g_EarPlayParams.awUnknown28[1] = 0;
    g_EarPlayParams.adwUnknown2c[0] = 0;
    g_EarPlayParams.adwUnknown2c[1] = 0;
    g_EarPlayParams.adwUnknown2c[2] = 0;
    g_EarPlayParams.adwUnknown2c[3] = 1;
    g_EarPlayParams.awUnknown3c[0] = 0;
    g_EarPlayParams.awUnknown3c[1] = 10000;
    g_EarPlayParams.awUnknown3c[2] = 10000;
    g_EarPlayParams.wPlaybackControl = 0;
    g_EarPlayParams.awUnknown44[0] = 0;
    g_EarPlayParams.awUnknown44[1] = 0;
    g_EarPlayParams.dwUnknown48 = 0;
    g_EarPlayParams.awUnknown4c[0] = 0;
    g_EarPlayParams.awUnknown4c[1] = 0;
    for (unsigned i = 0; i < 8; ++i) {
        g_EarPlayParams.adwUnknown50[i] = i == 4 ? 4 : 0;
    }
    g_EarPlayParams.awUnknown70[0] = 0;
    g_EarPlayParams.awUnknown70[1] = 1000;
    g_EarPlayParams.awUnknown70[2] = 2500;
    g_EarPlayParams.awUnknown70[3] = 5000;
    g_EarPlayParams.awUnknown70[4] = 7500;
    g_EarPlayParams.awUnknown70[5] = 9000;
    g_EarPlayParams.awUnknown70[6] = 10000;
    /* The final two bytes retain callback changes. */
    g_nMusicTrackMin = g_pfnEarChangeChannelControl(nChannel, 0x10000020);
    g_nMusicTrackMax = g_pfnEarChangeChannelControl(g_dwEarMusicChannel, 0x10000040);
    g_bMusicActive = 1;
    return true;
}

WPARAM GetEarMusicEventHandle(void) {
    return g_EarPlayParams.hEvent;
}

bool PlayEarMusicTrack(int nTrack) {
    if (g_dwPlayMusicMode != 0 && g_nMusicDeviceRefCount != 0) {
        if (nTrack < g_nMusicTrackMin) {
            nTrack = g_nMusicTrackMin;
        }
        if (nTrack > g_nMusicTrackMax) {
            nTrack = g_nMusicTrackMax;
        }
        g_nCurrentMusicTrack = nTrack;
        if (g_bMusicActive != 0) {
            g_pfnEarClearChannel(g_dwEarMusicChannel, 0);
            g_EarPlayParams.dwPlayFlags = 0x100;
            g_EarPlayParams.nTrack = nTrack;
            g_EarPlayParams.wPlaybackControl = 0;
            g_EarPlayParams.hEvent = g_pfnEarMixEvent(&g_EarPlayParams);
        }
    }
    return true;
}

bool InitSoftwareMusicStack(HWND hWnd) {
    if (!g_dwPlayMusicMode) {
        return true;
    }
    if (++g_nMusicDeviceRefCount != 1) {
        return true;
    }
    ResetMusicTracks();
    InitializeEarSound(hWnd);
    g_bMusicActive = false;
    g_bEarOutputInitialized = false;
    return InitializeMusicPlayback();
}

// The value of g_dwPlayMusicMode that plays the CD.
#define PLAY_MUSIC_MODE_CD 1

bool RouteMusicInitByFlag(HWND hWnd) {
    if (g_dwPlayMusicMode == PLAY_MUSIC_MODE_CD) {
        return OpenCdaudioMci(hWnd);
    }
    return InitSoftwareMusicStack(hWnd);
}

// The first track of music RandomizeMusicTrack() chooses, and the tracks it leaves out of the range
// of its random number.
#define RANDOM_MUSIC_FIRST_TRACK 3
#define RANDOM_MUSIC_TRACKS_LEFT_OUT 5
// The most tracks with which RandomizeMusicTrack() may choose the track that plays.
#define RANDOM_MUSIC_REPEAT_TRACKS 6
// The range of rand().
#define RAND_RANGE 0x8000

int RandomizeMusicTrack(void) {
    int nTrack;

    do {
        nTrack = rand() * (g_nMusicTrackMax - RANDOM_MUSIC_TRACKS_LEFT_OUT) / RAND_RANGE +
                 RANDOM_MUSIC_FIRST_TRACK;
        if (nTrack < RANDOM_MUSIC_FIRST_TRACK) {
            nTrack = RANDOM_MUSIC_FIRST_TRACK;
        }
        if (nTrack > g_nMusicTrackMax) {
            nTrack = g_nMusicTrackMax;
        }
    } while (g_nMusicTrackMax > RANDOM_MUSIC_REPEAT_TRACKS && nTrack == g_nCurrentMusicTrack);
    return nTrack;
}

int GetEarMusicCurrentTrack(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return 0;
    }
    return g_bMusicActive ? g_nCurrentMusicTrack : 0;
}

int GetCdAudioCurrentTrack(void) {
    MCI_STATUS_PARMS status;

    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0 || !g_bMusicActive) {
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
    if (g_bMusicActive && g_bCdMusicVolume) {
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
    return g_dwPlayMusicMode != 0 && g_nMusicDeviceRefCount != 0 && g_bMusicActive;
}

int ShutDownEarMusicChannel(void) {
    if (!IsMusicPlaying()) {
        return 1;
    }
    g_bMusicActive = false;
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

bool SelectPreviousMusicTrack(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return true;
    }
    g_nCurrentMusicTrack = GetCurrentMusicTrack() - 1;
    if (g_nCurrentMusicTrack < g_nMusicTrackMin) {
        g_nCurrentMusicTrack = g_nMusicTrackMin;
    }
    SetCurrentMusicTrack(g_nCurrentMusicTrack);
    return true;
}

bool SelectNextMusicTrack(void) {
    if (g_dwPlayMusicMode == 0 || g_nMusicDeviceRefCount == 0) {
        return true;
    }
    g_nCurrentMusicTrack = GetCurrentMusicTrack() + 1;
    if (g_nCurrentMusicTrack > g_nMusicTrackMax) {
        g_nCurrentMusicTrack = g_nMusicTrackMax;
    }
    SetCurrentMusicTrack(g_nCurrentMusicTrack);
    return true;
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
    if (!g_bMusicActive) {
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

bool UnpauseMusicPlayback(void) {
    if (g_dwPlayMusicMode == 1) {
        return ReplayCurrentMusicTrack();
    }
    if (!IsMusicPlaying()) {
        return true;
    }
    // The channel receives control 12, and any result but 0 succeeds.
    return g_pfnEarChangeChannelControl(g_dwEarMusicChannel, 12) != 0;
}

bool ReplayCurrentMusicTrack(void) {
    if (!IsMusicPlaying()) {
        return true;
    }
    return SetCurrentMusicTrack(g_nCurrentMusicTrack);
}

bool HandleMusicPauseResume(unsigned char bVolume) {
    if (bVolume && !g_bCdMusicVolume) {
        if (g_bCdAudioPlaying) {
            // The volume is recorded before and after the track plays again, as in the original.
            g_bCdMusicVolume = bVolume;
            ReplayCurrentMusicTrack();
            g_bCdMusicVolume = bVolume;
            return true;
        }
    } else if (!bVolume && g_bCdMusicVolume && g_bCdAudioPlaying) {
        PauseCdAudio();
    }
    g_bCdMusicVolume = bVolume;
    return true;
}

// The attenuation of the EAR library at the largest volume.
#define EAR_MAX_ATTENUATION 10000
// The largest volume.
#define MAX_VOLUME 255

bool SetScaledAudioVolume(unsigned char bVolume) {
    if (IsMusicPlaying()) {
        g_pfnEarSetAttenuationLevel(g_dwEarMusicChannel,
                                    bVolume * EAR_MAX_ATTENUATION / MAX_VOLUME);
    }
    return true;
}

bool DispatchMusicVolumeChange(unsigned char bVolume) {
    if (g_dwPlayMusicMode == PLAY_MUSIC_MODE_CD) {
        return HandleMusicPauseResume(bVolume);
    }
    return SetScaledAudioVolume(bVolume);
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
    bCleared = !g_bMusicActive || g_pfnEarClearChannel(g_dwEarMusicChannel, 0) == 0;
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

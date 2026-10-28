#include "platform/win/sound.h"

#include <cstdlib>
#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <mmreg.h>
#include <msacm.h>
#include <objbase.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/earsound.h"
#include "platform/win/settings.h"
#include "platform/win/timer.h"
#include "platform/win/wavread.h"
#include "soundchannel.h"

MCIDEVICEID g_dwMciCdAudioDeviceId;
IDirectSound *g_pDirectSound;
int g_dwDirectSoundRefCount;
IDirectSoundBuffer *g_pDirectSoundPrimaryBuffer;
WAVEFORMATEX *g_pDirectSoundWaveFormat;
SoundBufferNode g_SoundBufferListHead;
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
const int g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x1c, 0x38, 0x55, 0x71, 0x8d, 0xaa, 0xc6, 0xe2, 0xff};
const int g_adwSfxVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x55, 0x78, 0x93, 0xaa, 0xbe, 0xd0, 0xe1, 0xf0, 0xff};
unsigned char g_bBgmVolume = 0xbf;
unsigned char g_bSfxVolume = 0xff;

// The track the music starts at, the first after the track of the data of the CD.
#define FIRST_MUSIC_CD_TRACK 2

// Prepare the state of the music for the first user of its device.
static inline void ResetMusicTracks(HWND hWnd) {
    g_hMusicNotifyWnd = hWnd;
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
    ResetMusicTracks(hWnd);
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
    ResetMusicTracks(hWnd);
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

void ApplyAudioVolumeSettings(void) {
    g_bBgmVolume =
        static_cast<unsigned char>(g_adwBgmVolumeLevels[g_GlobalRallyConfig.dwMusicVolume]);
    DispatchMusicVolumeChange(g_bBgmVolume);
    g_bSfxVolume =
        static_cast<unsigned char>(g_adwSfxVolumeLevels[g_GlobalRallyConfig.dwSfxVolume]);
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

// The buffers of the samples are static with control of the frequency, the pan, the volume, and
// the position.
#define SAMPLE_BUFFER_FLAGS                                                                        \
    (DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |               \
     DSBCAPS_GETCURRENTPOSITION2)

HRESULT CreateDirectSoundBufferFromSample(SoundBufferNode *pNode) {
    DSBUFFERDESC desc;
    DSBCAPS caps;
    void *pAudio = nullptr;
    DWORD dwAudioSize;
    void *pAudio2;
    DWORD dwAudioSize2;
    HRESULT hResult;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = SAMPLE_BUFFER_FLAGS;
    if (pNode->bStickyFocus) {
        desc.dwFlags |= DSBCAPS_STICKYFOCUS;
    }
    desc.dwBufferBytes = pNode->dwSampleSize;
    desc.lpwfxFormat = pNode->pWaveFormat;
    hResult = g_pDirectSound->CreateSoundBuffer(&desc, &pNode->pBuffer, nullptr);
    if (hResult == DS_OK) {
        hResult = pNode->pBuffer->Lock(
            0, pNode->dwSampleSize, &pAudio, &dwAudioSize, &pAudio2, &dwAudioSize2, 0);
    }
    if (hResult == DS_OK) {
        // The whole sample is copied into the first part, whatever its size.
        memcpy(pAudio, pNode->pSampleData, pNode->dwSampleSize);
        hResult = pNode->pBuffer->Unlock(pAudio, pNode->dwSampleSize, nullptr, 0);
        if (hResult == DS_OK) {
            pAudio = nullptr;
            hResult = pNode->pBuffer->SetVolume(DSBVOLUME_MAX);
        }
        if (hResult == DS_OK) {
            hResult = pNode->pBuffer->SetPan(DSBPAN_CENTER);
        }
        if (hResult == DS_OK) {
            caps.dwSize = sizeof(caps);
            hResult = pNode->pBuffer->GetCaps(&caps);
        }
        if (hResult == DS_OK) {
            pNode->bHardware = (caps.dwFlags & DSBCAPS_LOCHARDWARE) != 0;
            return hResult;
        }
    }
    if (pAudio != nullptr) {
        hResult = pNode->pBuffer->Unlock(pAudio, pNode->dwSampleSize, nullptr, 0);
    }
    if (pNode->pBuffer != nullptr) {
        pNode->pBuffer->Release();
        pNode->pBuffer = nullptr;
    }
    return hResult;
}

HRESULT ReleaseSoundBuffer(SoundBufferNode *pNode) {
    if (pNode->pBuffer != nullptr) {
        pNode->pBuffer->Release();
        pNode->pBuffer = nullptr;
    }
    return DS_OK;
}

HRESULT StopSoundBuffer(SoundBufferNode *pNode) {
    HRESULT hResult;

    if (!pNode->bPlaying) {
        return DS_OK;
    }
    hResult = pNode->pBuffer->Stop();
    if (hResult == DS_OK) {
        pNode->bPlaying = false;
    }
    return hResult;
}

HRESULT StopAllSoundBuffersInList(SoundBufferNode *pHead) {
    for (SoundBufferNode *pNode = pHead->pNext; pNode != nullptr; pNode = pNode->pNext) {
        StopSoundBuffer(pNode);
    }
    return DS_OK;
}

HRESULT FreeSoundBufferList(SoundBufferNode *pHead) {
    SoundBufferNode *pNode = pHead->pNext;
    SoundBufferNode *pNext;

    pHead->pNext = nullptr;
    while (pNode != nullptr) {
        ReleaseSoundBuffer(pNode);
        FreeLockedGlobalMemory(pNode->pWaveFormat);
        FreeLockedGlobalMemory(pNode->pSampleData);
        pNext = pNode->pNext;
        FreeLockedGlobalMemory(pNode);
        pNode = pNext;
    }
    return DS_OK;
}

SoundBufferNode *g_apSoundChannelBuffer[SOUND_CHANNEL_COUNT];

int AppendSoundBufferToList(SoundBufferNode *pHead, SoundBufferNode *pNode) {
    SoundBufferNode *pTail = pHead;

    pNode->pNext = nullptr;
    pNode->bPlaying = false;
    while (pTail->pNext != nullptr) {
        pTail = pTail->pNext;
    }
    pTail->pNext = pNode;
    return 0;
}

// The pan and the volume of a node at the centre, and the scale of their steps in hundredths of a
// decibel.
#define SOUND_LEVEL_CENTER 400
#define SOUND_LEVEL_STEP 10

HRESULT ApplySoundBufferPan(SoundBufferNode *pNode) {
    return pNode->pBuffer->SetPan((pNode->nPan - SOUND_LEVEL_CENTER) * SOUND_LEVEL_STEP);
}

HRESULT ApplySoundBufferFrequency(SoundBufferNode *pNode) {
    return pNode->pBuffer->SetFrequency(pNode->dwFrequency);
}

HRESULT ApplySoundBufferVolume(SoundBufferNode *pNode) {
    unsigned nScaled;

    if (!g_bSfxVolume) {
        return pNode->pBuffer->SetVolume(DSBVOLUME_MIN);
    }
    nScaled = static_cast<unsigned>(pNode->nVolume * g_bSfxVolume) / MAX_VOLUME;
    return pNode->pBuffer->SetVolume(
        static_cast<LONG>((nScaled - SOUND_LEVEL_CENTER) * SOUND_LEVEL_STEP));
}

HRESULT RestartOrPlaySoundBuffer(SoundBufferNode *pNode) {
    DWORD dwFlags = pNode->bLooping ? DSBPLAY_LOOPING : 0;
    DWORD dwStatus = 0;
    HRESULT hr;

    if (pNode->pBuffer->GetStatus(&dwStatus) == DS_OK && (dwStatus & DSBSTATUS_PLAYING)) {
        return pNode->pBuffer->SetCurrentPosition(0);
    }
    hr = pNode->pBuffer->Play(0, 0, dwFlags);
    if (hr == DS_OK) {
        pNode->bPlaying = true;
    }
    return hr;
}

HRESULT PlaySoundBuffer(SoundBufferNode *pNode, int bLooping) {
    pNode->bLooping = bLooping;
    return RestartOrPlaySoundBuffer(pNode);
}

// Whether sound effects play through DirectSound.
static inline bool IsDirectSoundActive(void) {
    return g_bSoundEnabledIni && g_pDirectSound != nullptr && g_dwDirectSoundRefCount != 0;
}

bool IsSoundChannelPlaying(int nChannel) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    SoundBufferNode *pNode = g_apSoundChannelBuffer[nChannel];
    if (pNode == nullptr) {
        return false;
    }
    return IsSoundBufferPlaying(pNode);
}

bool StopAudioChannel(int nChannel) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    SoundBufferNode *pNode = g_apSoundChannelBuffer[nChannel];
    if (pNode == nullptr) {
        return true;
    }
    return StopSoundBuffer(pNode) == DS_OK;
}

bool SetAudioChannelLooping(int nChannel, int bLooping) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    SoundBufferNode *pNode = g_apSoundChannelBuffer[nChannel];
    if (pNode == nullptr) {
        return false;
    }
    pNode->bLooping = bLooping;
    return true;
}

bool SetSoundBufferFrequency(SoundBufferNode *pNode, DWORD dwFrequency) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    if (pNode == nullptr) {
        return false;
    }
    pNode->dwFrequency = dwFrequency;
    // The result of the frequency is discarded.
    ApplySoundBufferFrequency(pNode);
    return true;
}

bool IsSoundBufferPlaying(SoundBufferNode *pNode) {
    DWORD dwStatus = 0;
    if (pNode->pBuffer->GetStatus(&dwStatus) != DS_OK) {
        return false;
    }
    return (dwStatus & DSBSTATUS_PLAYING) != 0;
}

bool SetSoundVoiceFrequencyFloat(int nSample, int nVoice, float fFrequency) {
    return SetSoundChannelFrequency(nSample, nVoice * 2, static_cast<DWORD>(fFrequency));
}

bool StopSoundVoice(int nSample, int nVoice) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    SoundBufferNode *pNode = g_aSoundChannelBuffers[nSample].channelBuffers[nVoice * 2].buffer;
    if (pNode == nullptr) {
        return true;
    }
    return StopSoundBuffer(pNode) == DS_OK;
}

bool SetSoundChannelFrequency(int nSample, int nChannel, DWORD dwFrequency) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    return SetSoundBufferFrequency(g_aSoundChannelBuffers[nSample].channelBuffers[nChannel].buffer,
                                   dwFrequency);
}

// The largest level of a side of SetSoundBufferStereoLevels().
#define MAX_STEREO_LEVEL 32

bool SetSoundBufferStereoLevels(SoundBufferNode *pNode, DWORD dwLevels) {
    int nLeft;
    int nRight;

    if (!IsDirectSoundActive()) {
        return true;
    }
    if (pNode == nullptr) {
        return false;
    }
    nLeft = static_cast<int>(dwLevels >> 16);
    nRight = static_cast<int>(dwLevels & 0xffff);
    if (nLeft > MAX_STEREO_LEVEL) {
        nLeft = MAX_STEREO_LEVEL;
    }
    if (nRight > MAX_STEREO_LEVEL) {
        nRight = MAX_STEREO_LEVEL;
    }
    // The louder side gives the volume, and the pan moves from the centre toward it.
    if (nLeft > nRight) {
        pNode->nPan = SOUND_LEVEL_CENTER;
        pNode->nVolume = nLeft * SOUND_LEVEL_CENTER / MAX_STEREO_LEVEL;
        if (nLeft != 0) {
            pNode->nPan = (nRight - nLeft) * SOUND_LEVEL_CENTER / nLeft + SOUND_LEVEL_CENTER;
        }
    } else {
        pNode->nPan = SOUND_LEVEL_CENTER;
        pNode->nVolume = nRight * SOUND_LEVEL_CENTER / MAX_STEREO_LEVEL;
        if (nRight != 0) {
            pNode->nPan = (nRight - nLeft) * SOUND_LEVEL_CENTER / nRight + SOUND_LEVEL_CENTER;
        }
    }
    return ApplySoundBufferVolume(pNode) == DS_OK && ApplySoundBufferPan(pNode) == DS_OK;
}

SoundChannelState g_aSoundChannelState[SOUND_CHANNEL_COUNT];

bool SetAudioChannelStereoLevels(int nChannel, DWORD dwLevels) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    if (!SetSoundBufferStereoLevels(g_apSoundChannelBuffer[nChannel], dwLevels)) {
        return false;
    }
    g_aSoundChannelState[nChannel].dwStereoLevels = dwLevels;
    return true;
}

double g_adChannelFrequencyScale[SOUND_CHANNEL_COUNT];

bool AssignAudioChannel(int nSample, int nChannel) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    g_adChannelFrequencyScale[nChannel] = g_aSoundChannelBuffers[nSample].dFrequencyScale;
    if (g_apSoundChannelBuffer[nChannel] != nullptr) {
        // The result of the stop is discarded.
        StopAudioChannel(nChannel);
    }
    g_apSoundChannelBuffer[nChannel] =
        g_aSoundChannelBuffers[nSample].channelBuffers[nChannel].buffer;
    return g_apSoundChannelBuffer[nChannel] != nullptr;
}

// 2 to the power of -32, the unit of the frequency of SetAudioChannelFrequency().
#define FREQUENCY_FRACTION_SCALE (1.0 / 4294967296.0)
// The units of the frequency of SetAudioChannelFrequency() in a sample a second.
#define FREQUENCY_FRACTION_UNITS 4294967296.0

bool SetAudioChannelFrequency(int nChannel, long long llFrequency) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    if (!SetSoundBufferFrequency(g_apSoundChannelBuffer[nChannel],
                                 static_cast<DWORD>(static_cast<double>(llFrequency) *
                                                    g_adChannelFrequencyScale[nChannel] *
                                                    FREQUENCY_FRACTION_SCALE))) {
        return false;
    }
    g_aSoundChannelState[nChannel].llFrequency = llFrequency;
    return true;
}

SoundChannelState g_aSoundChannelRequested[SOUND_CHANNEL_COUNT];

bool StartSoundChannelPlayback(int nSample, int nChannel, int bLooping) {
    SoundBufferNode *pNode;

    if (!IsDirectSoundActive()) {
        return true;
    }
    pNode = g_aSoundChannelBuffers[nSample].channelBuffers[nChannel].buffer;
    if (pNode == nullptr || PlaySoundBuffer(pNode, bLooping) != DS_OK) {
        return false;
    }
    // The inverse of SetAudioChannelFrequency(), from the frequency of the buffer.
    g_aSoundChannelRequested[nChannel].llFrequency =
        static_cast<long long>(static_cast<double>(static_cast<long long>(pNode->dwFrequency)) *
                               FREQUENCY_FRACTION_UNITS / g_adChannelFrequencyScale[nChannel]);
    g_apSoundChannelBuffer[nChannel] = pNode;
    g_aSoundChannelState[nChannel].llFrequency = g_aSoundChannelRequested[nChannel].llFrequency;
    return true;
}

int g_bAltLoopingSoundSet;

// The sample of the alternative set of the looping sounds.
#define ALT_LOOPING_SAMPLE 25

static inline void MoveLoopingSoundChannel(int nChannel, int nFromSample, int nToSample) {
    SoundBufferNode *pNode = g_aSoundChannelBuffers[nFromSample].channelBuffers[nChannel].buffer;

    if (pNode == nullptr || pNode != g_apSoundChannelBuffer[nChannel]) {
        return;
    }
    // The results of the calls are discarded.
    AssignAudioChannel(nToSample, nChannel);
    SetAudioChannelLooping(nChannel, true);
    SetAudioChannelFrequency(nChannel, g_aSoundChannelRequested[nChannel].llFrequency);
    SetAudioChannelStereoLevels(nChannel, g_aSoundChannelRequested[nChannel].dwStereoLevels);
    StartSoundChannelPlayback(nToSample, nChannel, true);
}

bool ResumeLoopingSoundChannels(void) {
    int nChannel;

    for (nChannel = 0; nChannel < SOUND_CHANNEL_COUNT; nChannel++) {
        if (g_bAltLoopingSoundSet) {
            MoveLoopingSoundChannel(nChannel, 0, ALT_LOOPING_SAMPLE);
        } else {
            MoveLoopingSoundChannel(nChannel, ALT_LOOPING_SAMPLE, 0);
        }
        if (g_aSoundChannelState[nChannel].nSample != g_aSoundChannelRequested[nChannel].nSample) {
            g_aSoundChannelState[nChannel].nSample = g_aSoundChannelRequested[nChannel].nSample;
        }
        if (g_aSoundChannelState[nChannel].llFrequency !=
            g_aSoundChannelRequested[nChannel].llFrequency) {
            SetAudioChannelFrequency(nChannel, g_aSoundChannelRequested[nChannel].llFrequency);
        }
        if (g_aSoundChannelState[nChannel].dwStereoLevels !=
            g_aSoundChannelRequested[nChannel].dwStereoLevels) {
            SetAudioChannelStereoLevels(nChannel,
                                        g_aSoundChannelRequested[nChannel].dwStereoLevels);
        }
        if (g_aSoundChannelRequested[nChannel].llFrequency != 0 &&
            !IsSoundChannelPlaying(nChannel)) {
            g_aSoundChannelRequested[nChannel].llFrequency = 0;
            g_aSoundChannelState[nChannel].llFrequency = 0;
        }
    }
    return true;
}

bool InitSoundChannelWithVolume(
    short nChannel, int nSample, int nLength, int nLoopLength, DWORD dwLevels) {
    // The length of the sample is not read.
    (void)nLength;
    g_aSoundChannelRequested[nChannel].nSample = nSample;
    g_aSoundChannelRequested[nChannel].dwReserved = 0;
    g_aSoundChannelRequested[nChannel].dwStereoLevels = dwLevels;
    if (!AssignAudioChannel(nSample, nChannel)) {
        return false;
    }
    // The results of the calls are discarded.
    SetAudioChannelLooping(nChannel, nLoopLength);
    SetAudioChannelStereoLevels(nChannel, dwLevels);
    return StartSoundChannelPlayback(nSample, nChannel, nLoopLength);
}

bool InitSoundChannel(s16 channel, s32 sample, u32 length, u32 loopLength) {
    return InitSoundChannelWithVolume(channel,
                                      sample,
                                      static_cast<int>(length),
                                      static_cast<int>(loopLength),
                                      MAX_STEREO_LEVEL << 16 | MAX_STEREO_LEVEL);
}

bool StartSoundChannelMuted(s16 channel, s32 sample, u32 length, u32 loopLength) {
    return InitSoundChannelWithVolume(
        channel, sample, static_cast<int>(length), static_cast<int>(loopLength), 0);
}

bool SetStereoAndPlaySoundChannel(int nSample, int nChannel, DWORD dwLevels, int bLooping) {
    SoundBufferNode *pNode;

    if (!IsDirectSoundActive()) {
        return true;
    }
    pNode = g_aSoundChannelBuffers[nSample].channelBuffers[nChannel].buffer;
    if (!SetSoundBufferStereoLevels(pNode, dwLevels)) {
        return false;
    }
    return PlaySoundBuffer(pNode, bLooping) == DS_OK;
}

bool PlaySoundChannelOneWithStereo(int nSample, DWORD dwLevels, int bLooping) {
    return SetStereoAndPlaySoundChannel(nSample, 1, dwLevels, bLooping);
}

bool PlaySoundChannelOneOnce(int nSample, DWORD dwLevels) {
    return PlaySoundChannelOneWithStereo(nSample, dwLevels, false);
}

bool SetSoundChannelStereo(int nSample, int nChannel, DWORD dwLevels) {
    if (!IsDirectSoundActive()) {
        return true;
    }
    return SetSoundBufferStereoLevels(
        g_aSoundChannelBuffers[nSample].channelBuffers[nChannel].buffer, dwLevels);
}

bool SetSoundVoiceStereo(int nSample, int nVoice, DWORD dwLevels) {
    return SetSoundChannelStereo(nSample, nVoice * 2, dwLevels);
}

bool PlayLoopedSoundVoiceWithStereo(int nSample, int nVoice, DWORD dwLevels) {
    return SetStereoAndPlaySoundChannel(nSample, nVoice * 2, dwLevels, true);
}

SoundBufferNode *LoadAudioFile(const char *pszPath) {
    SoundBufferNode *pNode = static_cast<SoundBufferNode *>(
        GlobalLock(GlobalAlloc(GMEM_ZEROINIT, sizeof(SoundBufferNode))));
    DWORD dwSamples;
    const char *pszName;

    if (pNode == nullptr) {
        return nullptr;
    }
    pNode->pSampleData = nullptr;
    pNode->pWaveFormat = nullptr;
    pNode->pBuffer = nullptr;
    pNode->bStickyFocus = false;
    strcpy(pNode->szPath, pszPath);
    if (WaveLoadFile(
            pszPath, &pNode->dwSampleSize, &dwSamples, &pNode->pWaveFormat, &pNode->pSampleData) !=
            0 ||
        CreateDirectSoundBufferFromSample(pNode) != DS_OK ||
        AppendSoundBufferToList(&g_SoundBufferListHead, pNode) != 0) {
        ReleaseSoundBuffer(pNode);
        if (pNode->pWaveFormat != nullptr) {
            FreeLockedGlobalMemory(pNode->pWaveFormat);
        }
        if (pNode->pSampleData != nullptr) {
            FreeLockedGlobalMemory(pNode->pSampleData);
        }
        FreeLockedGlobalMemory(pNode);
        return nullptr;
    }
    pszName = strrchr(pNode->szPath, '\\');
    pNode->nNameOffset = pszName != nullptr ? static_cast<int>(pszName + 1 - pNode->szPath) : 0;
    pNode->dwFrequency = pNode->pWaveFormat->nSamplesPerSec;
    pNode->nPan = SOUND_LEVEL_CENTER;
    pNode->nVolume = SOUND_LEVEL_CENTER;
    ApplySoundBufferVolume(pNode);
    ApplySoundBufferFrequency(pNode);
    ApplySoundBufferPan(pNode);
    return pNode;
}

bool MuteAllActiveSoundChannels(void) {
    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0) {
        return true;
    }
    for (SoundBufferNode *pNode : g_apSoundChannelBuffer) {
        if (pNode != nullptr) {
            pNode->pBuffer->SetVolume(DSBVOLUME_MIN);
            pNode->pBuffer->SetPan(DSBPAN_CENTER);
        }
    }
    return true;
}
int g_dwLoadedSoundSampleCount;

void ResetSoundBankTables(void) {
    for (int i = 0; i < SOUND_CHANNEL_COUNT; i++) {
        g_SoundBankIndexTableA[0].anChannels[i] = 0;
        g_SoundBankIndexTableB.anChannels[i] = 0;
        g_SoundBankIndexTableC.anChannels[i] = 0;
    }
}

void SetSoundBankEntryIndex(int nVoice, int nIndex) {
    int nEntry = nIndex + 1;
    g_SoundBankIndexTableA[0].anChannels[nVoice * 2] = nEntry;
    g_SoundBankIndexTableB.anChannels[nVoice * 2] = nEntry;
    g_SoundBankIndexTableC.anChannels[nVoice * 2] = nEntry;
}

// The size of the path of a sample.
#define SAMPLE_PATH_SIZE 0x400

// Load the file of a sound effect under the directory of the sound effects.
static inline SoundBufferNode *
LoadSfxFile(char *pszPath, const char *pszPrefix, const char *pszSuffix) {
    strcpy(pszPath, g_szSfxDirPath);
    strcat(pszPath, pszPrefix);
    strcat(pszPath, pszSuffix);
    return LoadAudioFile(pszPath);
}

bool LoadSfxWaveVariants(int nEffect) {
    int nChannel = nEffect * 2;
    char szPath[SAMPLE_PATH_SIZE];
    bool bLoaded = true;

    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0 ||
        !g_SoundBankIndexTableA[0].anChannels[nChannel]) {
        g_aSoundChannelBuffers[0].channelBuffers[nChannel].buffer = nullptr;
        g_SfxWaveVariantTable[nChannel].buffer = nullptr;
        g_LoopingSoundTable[nChannel].buffer = nullptr;
        return bLoaded;
    }
    g_aSoundChannelBuffers[0].channelBuffers[nChannel].buffer = LoadSfxFile(
        szPath, g_apszSfxNamePrefixes[g_SoundBankIndexTableA[0].anChannels[nChannel]], ".wav");
    if (g_aSoundChannelBuffers[0].channelBuffers[nChannel].buffer == nullptr) {
        bLoaded = false;
    }
    g_SfxWaveVariantTable[nChannel].buffer = LoadSfxFile(
        szPath, g_apszSfxNamePrefixes[g_SoundBankIndexTableB.anChannels[nChannel]], "h.wav");
    if (g_SfxWaveVariantTable[nChannel].buffer == nullptr) {
        bLoaded = false;
    }
    g_LoopingSoundTable[nChannel].buffer = LoadSfxFile(
        szPath, g_apszSfxNamePrefixes[g_SoundBankIndexTableC.anChannels[nChannel]], "r.wav");
    if (g_LoopingSoundTable[nChannel].buffer == nullptr) {
        return false;
    }
    return bLoaded;
}

bool LoadSoundSet(int nSet) {
    const char *const *ppszFiles;
    char szPath[SAMPLE_PATH_SIZE];
    bool bLoaded = true;

    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0) {
        return true;
    }
    if (nSet == SOUND_SET_FRONT_END) {
        ppszFiles = g_SoundSetTableB;
        g_dwLoadedSoundSampleCount = SOUND_SET_FRONT_END_COUNT;
    } else if (nSet == SOUND_SET_RACE) {
        ppszFiles = g_SoundSetTableA;
        g_dwLoadedSoundSampleCount = SOUND_SET_RACE_COUNT;
        for (int i = 0; i < SOUND_CHANNEL_COUNT; i++) {
            if (g_SoundBankIndexTableA[0].anChannels[i]) {
                // An odd channel loads the effect of the channel before it, as in the original.
                if (!LoadSfxWaveVariants(i / 2)) {
                    bLoaded = false;
                }
            } else {
                g_aSoundChannelBuffers[0].channelBuffers[i].buffer = nullptr;
                g_SfxWaveVariantTable[i].buffer = nullptr;
                g_LoopingSoundTable[i].buffer = nullptr;
            }
        }
    } else {
        // The original reads the files of another set through a pointer it does not set, and
        // the pointer is null here.
        ppszFiles = nullptr;
    }
    for (int nSample = 1; nSample < g_dwLoadedSoundSampleCount - 1; nSample++) {
        for (int i = 0; i < SOUND_CHANNEL_COUNT; i++) {
            SoundBufferNode *&pBuffer = g_aSoundChannelBuffers[nSample].channelBuffers[i].buffer;

            if (!g_SoundBankIndexTableA[nSample].anChannels[i]) {
                pBuffer = nullptr;
                continue;
            }
            // Each channel of the sample loads the file of the sample again.
            strcpy(szPath, g_szSfxDirPath);
            strcat(szPath, ppszFiles[nSample]);
            pBuffer = LoadAudioFile(szPath);
            if (pBuffer == nullptr) {
                bLoaded = false;
            }
        }
    }
    return bLoaded;
}

bool ResetAudioSubsystem(void) {
    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0) {
        return true;
    }
    StopAllSoundBuffersInList(&g_SoundBufferListHead);
    FreeSoundBufferList(&g_SoundBufferListHead);
    for (int i = 0; i < g_dwLoadedSoundSampleCount; i++) {
        memset(g_aSoundChannelBuffers[i].channelBuffers,
               0,
               sizeof(g_aSoundChannelBuffers[i].channelBuffers));
    }
    memset(g_apSoundChannelBuffer, 0, sizeof(g_apSoundChannelBuffer));
    return true;
}

// The wave format of the primary buffer.
#define PRIMARY_SAMPLE_RATE 22050
#define PRIMARY_CHANNEL_COUNT 2
#define PRIMARY_BITS_PER_SAMPLE 16

bool InitializeDirectSound(void) {
    DWORD dwFormatSize;
    DSBUFFERDESC desc;
    HRESULT hResult;

    if (!g_bSoundEnabledIni || ++g_dwDirectSoundRefCount != 1) {
        return true;
    }
    memset(g_apSoundChannelBuffer, 0, sizeof(g_apSoundChannelBuffer));
    ResetSoundBankTables();
    if (acmMetrics(nullptr, ACM_METRIC_MAX_SIZE_FORMAT, &dwFormatSize) != 0) {
        return false;
    }
    g_pDirectSoundWaveFormat =
        static_cast<WAVEFORMATEX *>(GlobalLock(GlobalAlloc(GMEM_ZEROINIT, dwFormatSize)));
    if (g_pDirectSoundWaveFormat == nullptr) {
        return false;
    }
    g_pDirectSoundWaveFormat->wFormatTag = WAVE_FORMAT_PCM;
    g_pDirectSoundWaveFormat->nChannels = PRIMARY_CHANNEL_COUNT;
    g_pDirectSoundWaveFormat->nSamplesPerSec = PRIMARY_SAMPLE_RATE;
    g_pDirectSoundWaveFormat->nAvgBytesPerSec =
        PRIMARY_SAMPLE_RATE * PRIMARY_CHANNEL_COUNT * PRIMARY_BITS_PER_SAMPLE / 8;
    g_pDirectSoundWaveFormat->nBlockAlign = PRIMARY_CHANNEL_COUNT * PRIMARY_BITS_PER_SAMPLE / 8;
    g_pDirectSoundWaveFormat->wBitsPerSample = PRIMARY_BITS_PER_SAMPLE;
    g_pDirectSoundWaveFormat->cbSize = 0;

    hResult = CoCreateInstance(CLSID_DirectSound,
                               nullptr,
                               CLSCTX_INPROC_SERVER,
                               IID_IDirectSound,
                               reinterpret_cast<void **>(&g_pDirectSound));
    // A DirectSound that is created as null counts as a success.
    if (FAILED(hResult) || g_pDirectSound == nullptr) {
        return SUCCEEDED(hResult);
    }
    hResult = g_pDirectSound->Initialize(nullptr);
    if (SUCCEEDED(hResult)) {
        hResult = g_pDirectSound->SetCooperativeLevel(g_hMainWindow, DSSCL_PRIORITY);
    }
    if (SUCCEEDED(hResult)) {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
        hResult = g_pDirectSound->CreateSoundBuffer(&desc, &g_pDirectSoundPrimaryBuffer, nullptr);
    }
    if (SUCCEEDED(hResult)) {
        hResult = g_pDirectSoundPrimaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
        if (SUCCEEDED(hResult)) {
            return true;
        }
        g_pDirectSoundPrimaryBuffer->Release();
        g_pDirectSoundPrimaryBuffer = nullptr;
    }
    g_pDirectSound->Release();
    g_pDirectSound = nullptr;
    return false;
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
    if (g_SoundBufferListHead.pSampleData != nullptr) {
        FreeLockedGlobalMemory(g_SoundBufferListHead.pSampleData);
        g_SoundBufferListHead.pSampleData = nullptr;
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

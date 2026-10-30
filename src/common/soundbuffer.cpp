#include "common/soundbuffer.h"

#include <cstdlib>
#include <cstring>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#include "win32/wavread.h"
#else
#include <SDL3/SDL.h>

#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/soundchannel.h"
#include "ear/configuration.h"
#include "ear/error.h"
#include "ear/initialization.h"
#include "ear/shutdown.h"
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
IDirectSound *g_pDirectSound;
#endif
int g_dwDirectSoundRefCount;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
IDirectSoundBuffer *g_pDirectSoundPrimaryBuffer;
WAVEFORMATEX *g_pDirectSoundWaveFormat;
#endif
SoundBufferNode g_SoundBufferListHead;
const int g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x1c, 0x38, 0x55, 0x71, 0x8d, 0xaa, 0xc6, 0xe2, 0xff};
const int g_adwSfxVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x55, 0x78, 0x93, 0xaa, 0xbe, 0xd0, 0xe1, 0xf0, 0xff};
#if TGR_PLATFORM == TGR_PLATFORM_WIN
// The music player of the SDL game defines the volumes, with the values of the console.
unsigned char g_bBgmVolume = 0xbf;
unsigned char g_bSfxVolume = 0xff;
#endif

#if TGR_PLATFORM != TGR_PLATFORM_WIN
namespace {
// Whether EAR started for the sound effects, where the Windows libraries test DirectSound.
bool g_bEarSoundStarted;
} // namespace

// The results and the levels of DirectSound the sound buffers report and set, in hundredths of a
// decibel for the levels.
#define DS_OK 0
#define DSERR_GENERIC static_cast<HRESULT>(0x80004005)
#define DSBVOLUME_MIN -10000
#define DSBVOLUME_MAX 0
#define DSBPAN_CENTER 0
// The key EAR is validated with, as the PC release passes it.
#define EAR_VALIDATION_KEY 0x9be9c9
#endif

// The largest volume.
#define MAX_VOLUME 255

void ApplyAudioVolumeSettings(void) {
    // The music of the SDL game plays at the level of the menus of the console from the splash
    // screens to the race, and the music volume of the PC configuration does not replace it.
#if TGR_PLATFORM != TGR_PLATFORM_SDL
    g_bBgmVolume =
        static_cast<unsigned char>(g_adwBgmVolumeLevels[g_GlobalRallyConfig.dwMusicVolume]);
#endif
    DispatchMusicVolumeChange(g_bBgmVolume);
    g_bSfxVolume =
        static_cast<unsigned char>(g_adwSfxVolumeLevels[g_GlobalRallyConfig.dwSfxVolume]);
}

// A block of locked global memory is unlocked and freed.
static inline void FreeLockedGlobalMemory(void *pMemory) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    GlobalUnlock(GlobalHandle(pMemory));
    GlobalFree(GlobalHandle(pMemory));
#else
    SDL_free(pMemory);
#endif
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
// The buffers of the samples are static with control of the frequency, the pan, the volume, and
// the position.
#define SAMPLE_BUFFER_FLAGS                                                                        \
    (DSBCAPS_STATIC | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME |               \
     DSBCAPS_GETCURRENTPOSITION2)
#endif

HRESULT CreateDirectSoundBufferFromSample(SoundBufferNode *pNode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#else
    // The mixer rejects a format DirectSound does not play, as DirectSound does.
    pNode->pBuffer = EarSdlVoice::Create(*pNode->pWaveFormat, pNode->dwSampleSize);
    if (pNode->pBuffer == nullptr) {
        return DSERR_GENERIC;
    }
    pNode->pBuffer->Write(0, pNode->pSampleData, pNode->dwSampleSize);
    pNode->pBuffer->SetVolume(DSBVOLUME_MAX);
    pNode->pBuffer->SetPan(DSBPAN_CENTER);
    pNode->bHardware = false;
    return DS_OK;
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    hResult = pNode->pBuffer->Stop();
#else
    pNode->pBuffer->Stop();
    hResult = DS_OK;
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return pNode->pBuffer->SetPan((pNode->nPan - SOUND_LEVEL_CENTER) * SOUND_LEVEL_STEP);
#else
    pNode->pBuffer->SetPan((pNode->nPan - SOUND_LEVEL_CENTER) * SOUND_LEVEL_STEP);
    return DS_OK;
#endif
}

HRESULT ApplySoundBufferFrequency(SoundBufferNode *pNode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return pNode->pBuffer->SetFrequency(pNode->dwFrequency);
#else
    pNode->pBuffer->SetFrequency(pNode->dwFrequency);
    return DS_OK;
#endif
}

HRESULT ApplySoundBufferVolume(SoundBufferNode *pNode) {
    unsigned nScaled;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bSfxVolume) {
        return pNode->pBuffer->SetVolume(DSBVOLUME_MIN);
    }
    nScaled = static_cast<unsigned>(pNode->nVolume * g_bSfxVolume) / MAX_VOLUME;
    return pNode->pBuffer->SetVolume(
        static_cast<LONG>((nScaled - SOUND_LEVEL_CENTER) * SOUND_LEVEL_STEP));
#else
    if (!g_bSfxVolume) {
        pNode->pBuffer->SetVolume(DSBVOLUME_MIN);
        return DS_OK;
    }
    nScaled = static_cast<unsigned>(pNode->nVolume * g_bSfxVolume) / MAX_VOLUME;
    pNode->pBuffer->SetVolume(static_cast<LONG>((nScaled - SOUND_LEVEL_CENTER) * SOUND_LEVEL_STEP));
    return DS_OK;
#endif
}

HRESULT RestartOrPlaySoundBuffer(SoundBufferNode *pNode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#else
    // A buffer that plays goes back to its start and retains the loop it started with.
    if (pNode->pBuffer->IsPlaying()) {
        return pNode->pBuffer->SetPlayCursor(0) ? DS_OK : DSERR_GENERIC;
    }
    pNode->pBuffer->Play(pNode->bLooping != 0);
    pNode->bPlaying = true;
    return DS_OK;
#endif
}

HRESULT PlaySoundBuffer(SoundBufferNode *pNode, int bLooping) {
    pNode->bLooping = bLooping;
    return RestartOrPlaySoundBuffer(pNode);
}

// Whether sound effects play through DirectSound.
static inline bool IsDirectSoundActive(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return g_bSoundEnabledIni && g_pDirectSound != nullptr && g_dwDirectSoundRefCount != 0;
#else
    return g_bSoundEnabledIni && g_bEarSoundStarted && g_dwDirectSoundRefCount != 0;
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    DWORD dwStatus = 0;
    if (pNode->pBuffer->GetStatus(&dwStatus) != DS_OK) {
        return false;
    }
    return (dwStatus & DSBSTATUS_PLAYING) != 0;
#else
    return pNode->pBuffer->IsPlaying();
#endif
}

bool SetSoundVoiceFrequencyFloat(int nSample, int nVoice, float fFrequency) {
    // The original truncates through 64 bits and keeps the low word, which turns a NaN into 0.
    return SetSoundChannelFrequency(
        nSample, nVoice * 2, static_cast<DWORD>(static_cast<long long>(fFrequency)));
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
    // The original truncates through 64 bits and keeps the low word, which turns a NaN into 0.
    if (!SetSoundBufferFrequency(
            g_apSoundChannelBuffer[nChannel],
            static_cast<DWORD>(static_cast<long long>(static_cast<double>(llFrequency) *
                                                      g_adChannelFrequencyScale[nChannel] *
                                                      FREQUENCY_FRACTION_SCALE)))) {
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

#if TGR_PLATFORM != TGR_PLATFORM_WIN
// Read a wave file into the sample and the format of a node, as WaveLoadFile() of Windows does,
// with the samples in a format the mixer plays.
static inline int LoadWaveSamples(const char *pszPath, SoundBufferNode *pNode) {
    SDL_AudioSpec spec;
    Uint8 *pData = nullptr;
    Uint32 dwBytes = 0;
    if (!SDL_LoadWAV(pszPath, &spec, &pData, &dwBytes)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "%s cannot be read: %s", pszPath, SDL_GetError());
        return 1;
    }
    if (spec.format != SDL_AUDIO_U8 && spec.format != SDL_AUDIO_S16LE) {
        SDL_AudioSpec target = {SDL_AUDIO_S16LE, spec.channels, spec.freq};
        Uint8 *pConverted = nullptr;
        int nConverted = 0;
        bool bConverted = SDL_ConvertAudioSamples(
            &spec, pData, static_cast<int>(dwBytes), &target, &pConverted, &nConverted);
        SDL_free(pData);
        if (!bConverted) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "%s cannot be played: %s", pszPath, SDL_GetError());
            return 1;
        }
        spec = target;
        pData = pConverted;
        dwBytes = static_cast<Uint32>(nConverted);
    }
    auto *pFormat = static_cast<EarWaveFormat *>(SDL_calloc(1, sizeof(EarWaveFormat)));
    if (pFormat == nullptr) {
        SDL_free(pData);
        return 1;
    }
    pFormat->wFormatTag = 1;
    pFormat->nChannels = static_cast<WORD>(spec.channels);
    pFormat->nSamplesPerSec = static_cast<DWORD>(spec.freq);
    pFormat->wBitsPerSample = static_cast<WORD>(SDL_AUDIO_BITSIZE(spec.format));
    pFormat->nBlockAlign = static_cast<WORD>(spec.channels * pFormat->wBitsPerSample / 8);
    pFormat->nAvgBytesPerSec = pFormat->nSamplesPerSec * pFormat->nBlockAlign;
    pNode->pWaveFormat = pFormat;
    pNode->pSampleData = pData;
    pNode->dwSampleSize = dwBytes;
    return 0;
}
#endif

SoundBufferNode *LoadAudioFile(const char *pszPath) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    SoundBufferNode *pNode = static_cast<SoundBufferNode *>(
        GlobalLock(GlobalAlloc(GMEM_ZEROINIT, sizeof(SoundBufferNode))));
    DWORD dwSamples;
#else
    SoundBufferNode *pNode = static_cast<SoundBufferNode *>(SDL_calloc(1, sizeof(SoundBufferNode)));
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    const char *pszName;

    if (pNode == nullptr) {
        return nullptr;
    }
    pNode->pSampleData = nullptr;
    pNode->pWaveFormat = nullptr;
    pNode->pBuffer = nullptr;
    pNode->bStickyFocus = false;
    strcpy(pNode->szPath, pszPath);
    if (
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        WaveLoadFile(
            pszPath, &pNode->dwSampleSize, &dwSamples, &pNode->pWaveFormat, &pNode->pSampleData) !=
            0 ||
#else
        LoadWaveSamples(pszPath, pNode) != 0 ||
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0) {
#else
    if (!g_bSoundEnabledIni || !g_bEarSoundStarted || g_dwDirectSoundRefCount == 0) {
#endif
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0 ||
#else
    if (!g_bSoundEnabledIni || !g_bEarSoundStarted || g_dwDirectSoundRefCount == 0 ||
#endif
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0) {
#else
    if (!g_bSoundEnabledIni || !g_bEarSoundStarted || g_dwDirectSoundRefCount == 0) {
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bSoundEnabledIni || g_pDirectSound == nullptr || g_dwDirectSoundRefCount == 0) {
#else
    if (!g_bSoundEnabledIni || !g_bEarSoundStarted || g_dwDirectSoundRefCount == 0) {
#endif
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
// The wave format of the primary buffer.
#define PRIMARY_SAMPLE_RATE 22050
#define PRIMARY_CHANNEL_COUNT 2
#define PRIMARY_BITS_PER_SAMPLE 16
#endif

bool InitializeDirectSound(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    DWORD dwFormatSize;
    DSBUFFERDESC desc;
    HRESULT hResult;
#endif

    if (!g_bSoundEnabledIni || ++g_dwDirectSoundRefCount != 1) {
        return true;
    }
    memset(g_apSoundChannelBuffer, 0, sizeof(g_apSoundChannelBuffer));
    ResetSoundBankTables();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#else
    // The mixer of EAR plays the buffers, and EAR mixes at the format of the primary buffer.
    EAR_DLL_AAA_Validate(EAR_VALIDATION_KEY);
    g_bEarSoundStarted = EAR_DLL_InitializeEar(0) != 0;
    if (!g_bEarSoundStarted) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "The sound effects are silent. EAR did not start, error %u.",
                    EAR_DLL_GetLastError());
    }
    return g_bEarSoundStarted;
#endif
}

bool ShutdownDirectSound(void) {
    if (--g_dwDirectSoundRefCount != 0) {
        return true;
    }
    ResetAudioSubsystem();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#else
    if (g_bEarSoundStarted) {
        EAR_DLL_ShutDownEar();
        g_bEarSoundStarted = false;
    }
#endif
    if (g_SoundBufferListHead.pSampleData != nullptr) {
        FreeLockedGlobalMemory(g_SoundBufferListHead.pSampleData);
        g_SoundBufferListHead.pSampleData = nullptr;
    }
    return true;
}

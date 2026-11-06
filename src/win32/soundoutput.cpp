#include "common/soundoutput.h"

#include <cstring>

#include <windows.h>

// These headers need the types of windows.h.
#include <mmreg.h>
#include <msacm.h>
#include <objbase.h>

#include "common/soundbuffer.h"
#include "win32/app.h"

#define PRIMARY_SAMPLE_RATE 22050
#define PRIMARY_CHANNEL_COUNT 2
#define PRIMARY_BITS_PER_SAMPLE 16

bool InitializePlatformSoundOutput(void) {
    DWORD dwFormatSize;
    DSBUFFERDESC desc;
    HRESULT hResult;

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

void ShutdownPlatformSoundOutput(void) {
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
        GlobalUnlock(GlobalHandle(g_pDirectSoundWaveFormat));
        GlobalFree(GlobalHandle(g_pDirectSoundWaveFormat));
        g_pDirectSoundWaveFormat = nullptr;
    }
}

bool IsPlatformSoundOutputActive(void) {
    return g_pDirectSound != nullptr;
}

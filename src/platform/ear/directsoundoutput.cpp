#include "directsoundoutput.h"

#include <new>

#include <objbase.h>

#include "error.h"
#include "mfc.h"
#include "window.h"

EarDirectSoundOutput *g_pEarDirectSoundOutput;

void EarDirectSoundOutput::FailInitialization(std::uint32_t error) {
    SetEarLastError(error);
    if (device) {
        device->Release();
        device = nullptr;
    }
    // Native failure cleanup also runs before COM initialisation and defers primary-buffer release.
    CoUninitialize();
}

EarDirectSoundOutput::EarDirectSoundOutput() : device(nullptr), primaryBuffer(nullptr) {
    if (!g_hEarNotifyWindow) {
        FailInitialization(0x21);
        return;
    }
    CoInitialize(nullptr);
    if (CoCreateInstance(CLSID_DirectSound,
                         nullptr,
                         CLSCTX_INPROC_SERVER,
                         IID_IDirectSound,
                         IID_PPV_ARGS_Helper(&device))) {
        FailInitialization(6);
        return;
    }
    if (device->Initialize(nullptr) ||
        device->SetCooperativeLevel(g_hEarNotifyWindow, DSSCL_EXCLUSIVE)) {
        FailInitialization(0x22);
        return;
    }
    DSCAPS capabilities;
    capabilities.dwSize = sizeof(capabilities);
    if (device->GetCaps(&capabilities)) {
        FailInitialization(0x22);
        return;
    }
    if (!(capabilities.dwFlags & 0x90a)) {
        FailInitialization(8);
        return;
    }
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = 22050;
    format.wBitsPerSample = 16;
    format.nBlockAlign = format.wBitsPerSample * format.nChannels / 8;
    format.nAvgBytesPerSec = format.nBlockAlign * format.nSamplesPerSec;
    DSBUFFERDESC description = {};
    description.dwSize = 20;
    description.dwFlags = DSBCAPS_PRIMARYBUFFER;
    if (device->CreateSoundBuffer(&description, &primaryBuffer, nullptr)) {
        FailInitialization(0x22);
        return;
    }
    if (primaryBuffer->SetFormat(&format)) {
        FailInitialization(8);
        return;
    }
    if (primaryBuffer->GetFormat(&format, sizeof(format), nullptr)) {
        FailInitialization(7);
        return;
    }
    if (format.nChannels != 2 || format.nSamplesPerSec != 22050 || format.wBitsPerSample != 16) {
        FailInitialization(8);
        return;
    }
    if (primaryBuffer->Play(0, 0, DSBPLAY_LOOPING)) {
        FailInitialization(0x14);
    }
}

EarDirectSoundOutput::~EarDirectSoundOutput() {
    if (primaryBuffer) {
        primaryBuffer->Stop();
        primaryBuffer->Release();
        primaryBuffer = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
    CoUninitialize();
}

IDirectSound *EarDirectSoundOutput::GetDirectSoundInterface() {
    return device;
}

void ShutdownDirectSoundOutput() {
    if (g_pEarDirectSoundOutput) {
        delete g_pEarDirectSoundOutput;
        g_pEarDirectSoundOutput = nullptr;
    }
}

int InitializeDirectSoundOutput() {
    void *storage = MfcAllocateWithNewHandler(sizeof(EarDirectSoundOutput));
    // Native allocation publishes null before the unchecked interface read.
    static_cast<EarDirectSoundOutput *volatile &>(g_pEarDirectSoundOutput) =
        storage ? new (storage) EarDirectSoundOutput : nullptr;
    if (!g_pEarDirectSoundOutput->GetDirectSoundInterface()) {
        delete g_pEarDirectSoundOutput;
        g_pEarDirectSoundOutput = nullptr;
        SetEarLastError(3);
        return 0;
    }
    return 1;
}

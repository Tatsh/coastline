#include "wavesample.h"

#include <cstdlib>
#include <cstring>

#include <mmsystem.h>

#include "error.h"
#include "memorybudget.h"
#include "preset.h"
#include "registry.h"
#include "runtime.h"
#include "wavechunks.h"

EarWaveSample::EarWaveSample() {
    buffer = nullptr;
    byteCount = 0;
    sourceKind = EAR_WAVE_SAMPLE_SOURCE_FILE;
    unknown1c = 0;
}

EarWaveSample::~EarWaveSample() {
    if (buffer) {
        std::free(buffer);
        buffer = nullptr;
    }
    ReleaseSampleMemory(byteCount);
}

int EarWaveSample::LoadWaveSample(const void *source, EarWaveSampleSource kind) {
    sourceKind = kind;
    if (!ReadWaveSampleData(source)) {
        return 0;
    }
    return 1;
}

int EarWaveSample::ReadWaveSampleData(const void *source) {
    char header[16];
    MMIOINFO info;
    // A header that is not of a wave file closes this handle before it is assigned.
    HMMIO file;
    if (sourceKind == EAR_WAVE_SAMPLE_SOURCE_MEMORY) {
        std::memmove(header, source, sizeof(header));
        if (std::memcmp(header, "RIFF", 4) || std::memcmp(header + 8, "WAVEfmt ", 8)) {
            SetEarLastError(0x0f);
            goto fail;
        }
        // Only these members of the information are set.
        info.pIOProc = nullptr;
        info.fccIOProc = FOURCC_MEM;
        info.pchBuffer = static_cast<HPSTR>(const_cast<void *>(source));
        std::memcpy(&info.cchBuffer, header + 4, sizeof(info.cchBuffer));
        info.adwInfo[0] = 0;
        file = mmioOpenA(nullptr, &info, MMIO_READWRITE);
    } else {
        file = mmioOpenA(static_cast<LPSTR>(const_cast<void *>(source)), nullptr, 0);
    }
    if (!file) {
        SetEarLastError(0x0d);
        goto fail;
    }
    if (mmioDescend(file, &g_RiffWaveChunk, nullptr, 0)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (g_RiffWaveChunk.ckid != FOURCC_RIFF ||
        g_RiffWaveChunk.fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
        SetEarLastError(0x0f);
        goto fail;
    }
    if (mmioDescend(file, &g_WaveFormatChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    // The whole chunk is read into the format, however large it is.
    if (mmioRead(file, reinterpret_cast<HPSTR>(&format), g_WaveFormatChunk.cksize) !=
        static_cast<LONG>(g_WaveFormatChunk.cksize)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (format.wFormatTag != WAVE_FORMAT_PCM) {
        SetEarLastError(0x0f);
        goto fail;
    }
    if (mmioAscend(file, &g_WaveFormatChunk, 0)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (mmioDescend(file, &g_WaveDataChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    byteCount = g_WaveDataChunk.cksize;
    if (!AcceptWaveSampleData()) {
        goto fail;
    }
    if (!ReserveSampleMemory(byteCount)) {
        SetEarLastError(0x10);
        byteCount = 0;
        goto fail;
    }
    buffer = static_cast<char *>(std::malloc(byteCount));
    if (!buffer) {
        SetEarLastError(0x10);
        goto fail;
    }
    std::memset(buffer, format.wBitsPerSample == 16 ? 0 : 0x80, byteCount);
    if (mmioRead(file, buffer, static_cast<LONG>(byteCount)) != static_cast<LONG>(byteCount)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    mmioClose(file, 0);
    file = nullptr;
    return 1;

fail:
    // The memory reserved for the samples is not given back.
    if (buffer) {
        std::free(buffer);
        buffer = nullptr;
    }
    if (file) {
        mmioClose(file, 0);
        file = nullptr;
    }
    return 0;
}

char *EarWaveSample::GetWaveSampleBuffer() const {
    return buffer;
}

DWORD EarWaveSample::GetWaveSampleByteCount() const {
    return byteCount;
}

WAVEFORMATEX EarWaveSample::CopyWaveSampleFormat() const {
    return format;
}

int EarWaveSample::AcceptWaveSampleData() {
    return 1;
}

int __stdcall EarShutDownPresetExportWrapper(uint32_t preset) {
    return EAR_DLL_ShutDownPreset(preset);
}

int __stdcall EAR_DLL_ShutDownPreset(uint32_t preset) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (g_PresetRegistry.IsEmpty()) {
        if (preset != 0) {
            SetEarLastError(0x29);
        }
        return 0;
    }
    int locked = 0;
    int shutDown = 1;
    while (g_bPlaybackUpdateLocked) {
    }
    if (!g_bRegistryMutationLocked) {
        locked = 1;
        g_bRegistryMutationLocked = locked;
    }
    if (preset != 0) {
        EarWaveSample *found;
        if (g_PresetRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(preset), found)) {
            delete found;
            g_PresetRegistry.RemoveKey(static_cast<uint16_t>(preset));
        } else {
            SetEarLastError(0x29);
            shutDown = 0;
        }
    } else {
        POSITION position = g_PresetRegistry.GetStartPosition();
        do {
            WORD key;
            void *value;
            g_PresetRegistry.GetNextAssoc(position, key, value);
            delete static_cast<EarWaveSample *>(value);
        } while (position);
        g_PresetRegistry.RemoveAll();
    }
    if (locked) {
        g_bRegistryMutationLocked = 0;
    }
    return shutDown;
}

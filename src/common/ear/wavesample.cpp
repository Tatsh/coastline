#include "wavesample.h"

#include <cstdlib>
#include <cstring>
#include <new>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <mmsystem.h>
#endif

#include "ear/registry.h"
#include "error.h"
#include "event.h"
#include "memorybudget.h"
#include "preset.h"
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    MMIOINFO info;
#else
    LONG memoryBytes;
#endif
#if defined(ENABLE_PATCHES) || TGR_PLATFORM != TGR_PLATFORM_WIN
    // The SDL game frees the handle it closes, and a handle never assigned must not be closed.
    EarWaveFile file = nullptr;
#else
    // A header that is not of a wave file closes this handle before it is assigned.
    EarWaveFile file;
#endif /* ENABLE_PATCHES */
    if (sourceKind == EAR_WAVE_SAMPLE_SOURCE_MEMORY) {
        std::memmove(header, source, sizeof(header));
        if (std::memcmp(header, "RIFF", 4) || std::memcmp(header + 8, "WAVEfmt ", 8)) {
            SetEarLastError(0x0f);
            goto fail;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        // Only these members of the information are set.
        info.pIOProc = nullptr;
        info.fccIOProc = FOURCC_MEM;
        info.pchBuffer = static_cast<HPSTR>(const_cast<void *>(source));
        std::memcpy(&info.cchBuffer, header + 4, sizeof(info.cchBuffer));
        info.adwInfo[0] = 0;
        file = mmioOpenA(nullptr, &info, MMIO_READWRITE);
#else
        // The size of the memory is the size of the RIFF chunk, without its 8-byte header.
        std::memcpy(&memoryBytes, header + 4, sizeof(memoryBytes));
        file = EarRiffOpenMemory(source, memoryBytes);
#endif
    } else {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        file = mmioOpenA(static_cast<LPSTR>(const_cast<void *>(source)), nullptr, 0);
#else
        file = EarRiffOpenPath(static_cast<const char *>(source));
#endif
    }
    if (!file) {
        SetEarLastError(0x0d);
        goto fail;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (mmioDescend(file, &g_RiffWaveChunk, nullptr, 0)) {
#else
    if (EarRiffDescend(file, &g_RiffWaveChunk, nullptr, 0)) {
#endif
        SetEarLastError(0x0e);
        goto fail;
    }
    if (g_RiffWaveChunk.ckid != EAR_RIFF_ID || g_RiffWaveChunk.fccType != EAR_WAVE_ID) {
        SetEarLastError(0x0f);
        goto fail;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (mmioDescend(file, &g_WaveFormatChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
#else
    if (EarRiffDescend(file, &g_WaveFormatChunk, &g_RiffWaveChunk, EAR_RIFF_FIND_CHUNK)) {
#endif
        SetEarLastError(0x0e);
        goto fail;
    }
    // The whole chunk is read into the format, however large it is.
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (mmioRead(file, reinterpret_cast<HPSTR>(&format), g_WaveFormatChunk.cksize) !=
        static_cast<LONG>(g_WaveFormatChunk.cksize)) {
#else
    if (EarRiffRead(file, &format, static_cast<LONG>(g_WaveFormatChunk.cksize)) !=
        static_cast<LONG>(g_WaveFormatChunk.cksize)) {
#endif
        SetEarLastError(0x0e);
        goto fail;
    }
    if (format.wFormatTag != EAR_WAVE_FORMAT_PCM) {
        SetEarLastError(0x0f);
        goto fail;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (mmioAscend(file, &g_WaveFormatChunk, 0)) {
#else
    if (EarRiffAscend(file, &g_WaveFormatChunk)) {
#endif
        SetEarLastError(0x0e);
        goto fail;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (mmioDescend(file, &g_WaveDataChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
#else
    if (EarRiffDescend(file, &g_WaveDataChunk, &g_RiffWaveChunk, EAR_RIFF_FIND_CHUNK)) {
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (mmioRead(file, buffer, static_cast<LONG>(byteCount)) != static_cast<LONG>(byteCount)) {
#else
    if (EarRiffRead(file, buffer, static_cast<LONG>(byteCount)) != static_cast<LONG>(byteCount)) {
#endif
        SetEarLastError(0x0e);
        goto fail;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    mmioClose(file, 0);
#else
    EarRiffClose(file);
#endif
    file = nullptr;
    return 1;

fail:
    // The memory reserved for the samples is not given back.
    if (buffer) {
        std::free(buffer);
        buffer = nullptr;
    }
    if (file) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        mmioClose(file, 0);
#else
        EarRiffClose(file);
#endif
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

EarWaveFormat EarWaveSample::CopyWaveSampleFormat() const {
    return format;
}

int EarWaveSample::AcceptWaveSampleData() {
    return 1;
}

uint32_t __stdcall EarRegisterPresetExportWrapper(const void *source, uint32_t control) {
    return EAR_DLL_RegisterPreset(source, control);
}

uint32_t __stdcall EAR_DLL_RegisterPreset(const void *source, uint32_t control) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    g_PresetRegistry.GrowWhenFull();
    EarWaveSampleSource kind = static_cast<EarWaveSampleSource>(control >> 16);
    uint16_t handle = static_cast<uint16_t>(control);
    EarWaveSample *sample;
    if (handle) {
        if (g_PresetRegistry.LookupWordPointerMapValue(handle, sample)) {
            SetEarLastError(0x28);
            return 0;
        }
    } else {
        do {
            handle = GenerateEarObjectHandle();
        } while (g_PresetRegistry.LookupWordPointerMapValue(handle, sample));
    }
    void *storage = MfcAllocateWithNewHandler(sizeof(EarWaveSample));
    sample = storage ? new (storage) EarWaveSample : nullptr;
    if (sample == nullptr) {
        SetEarLastError(0x10);
        return 0;
    }
    if (!sample->LoadWaveSample(source, kind)) {
        delete sample;
        return 0;
    }
    g_PresetRegistry.SetWordPointerMapValue(handle, sample);
    return handle;
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

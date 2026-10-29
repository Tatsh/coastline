#include "initialization.h"

#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include <SDL3/SDL_events.h>
#endif

#include "channel.h"
#include "configuration.h"
#include "directionalmatrix.h"
#include "ear/backend.h"
#include "ear/directsoundoutput.h"
#include "ear/registry.h"
#include "ear/timer.h"
#include "ear/window.h"
#include "environment.h"
#include "error.h"
#include "memorybudget.h"
#include "playbacksmoothing.h"
#include "runtime.h"
#include "wavechunks.h"

namespace {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
// @ghidraAddress EAR: 0x100371c4
const char g_earNotificationMessageName[] = "EAR Interactive Around-Sound";
#endif

static inline int FailEarInitialization() {
    std::uint32_t error = EarGetLastErrorExportWrapper();
    EarShutDownEarExportWrapper();
    SetEarLastError(error);
    return 0;
}
} // namespace

int __stdcall EarInitializeEarExportWrapper(std::uint32_t flags) {
    return EAR_DLL_InitializeEar(flags);
}

int __stdcall EAR_DLL_InitializeEar(std::uint32_t flags) {
    SetEarLastError(0);
    if ((flags >> 16) == 0xdeaf) {
        if (flags & 1) {
            g_bMixerAccessDisabled = 1;
        }
        if (flags & 2) {
            g_bConfigurationValidationDisabled = 1;
        }
        return 1;
    }
    g_bShowEarErrors = (flags & 0x40) != 0;
    ConfigurePlaybackSmoothingPasses(flags & 0x200);
    if (g_bEarComponentsInitialized) {
        return 1;
    }
    if (!g_bEarCodeValidated) {
        SetEarLastError(1);
        return 0;
    }
    InitializeSampleMemoryBudget();
    g_bEarComponentsInitialized = InitializeDirectSoundOutput();
    if (!g_bEarComponentsInitialized) {
        return FailEarInitialization();
    }
    g_bEarComponentsInitialized = SetReservedEarInitializationOption(flags & 1);
    if (!g_bEarComponentsInitialized) {
        return FailEarInitialization();
    }
    g_bEarComponentsInitialized = ValidateEarTitleConfiguration(flags & 8);
    if (!g_bEarComponentsInitialized) {
        return FailEarInitialization();
    }
    g_bEarComponentsInitialized = ValidateEarStereoSolution(flags & 2);
    if (!g_bEarComponentsInitialized) {
        return FailEarInitialization();
    }
    if ((flags & 0x4000) && !RejectUnsupportedInitializationFeature4000()) {
        return FailEarInitialization();
    }
    if ((flags & 0x2000) && !RejectUnsupportedInitializationFeature2000()) {
        return FailEarInitialization();
    }
    if (flags & 0x100) {
        g_bEarComponentsInitialized = ValidateEarAudioComponents();
        if (!g_bEarComponentsInitialized) {
            return FailEarInitialization();
        }
    }
    g_dwPreviousPlaybackUpdateTimeMs = 0;
    g_WaveFormatChunk.ckid = EAR_FORMAT_CHUNK_ID;
    g_WaveDataChunk.ckid = EAR_DATA_CHUNK_ID;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (timeGetDevCaps(&g_earTimerCapabilities, sizeof(g_earTimerCapabilities))) {
        SetEarLastError(0x18);
        return FailEarInitialization();
    }
#endif
    g_uPlaybackTimerId = 0;
    g_ChannelRegistry.InitHashTable(0x1d, TRUE);
    g_MatrixRegistry.InitHashTable(0x11, TRUE);
    g_EnvironmentRegistry.InitHashTable(0x11, TRUE);
    g_PresetRegistry.InitHashTable(0x1d, TRUE);
    g_PlaybackRegistry.InitHashTable(0x61, TRUE);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_uEarNotifyMessage = RegisterWindowMessageA(g_earNotificationMessageName);
#else
    // An SDL event type has no name, and the type of the window message stays registered.
    if (!g_uEarNotifyMessage) {
        g_uEarNotifyMessage = SDL_RegisterEvents(1);
    }
#endif
    g_bEarInitializationComplete = 1;
    if (flags & 4) {
        EarRegisterChannelExportWrapper(1, 0, 100, 30);
        EarRegisterMatrixExportWrapper(0x10);
        EarStartTimerExportWrapper();
    } else if (!(flags & 0x80)) {
        EarStartTimerExportWrapper();
    }
    RegisterDefaultEnvironment();
    return 1;
}

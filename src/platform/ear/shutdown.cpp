#include "shutdown.h"

#include "channel.h"
#include "directionalmatrix.h"
#include "directsoundoutput.h"
#include "environment.h"
#include "error.h"
#include "preset.h"
#include "runtime.h"
#include "timer.h"

void __stdcall EarShutDownEarExportWrapper(void) {
    EAR_DLL_ShutDownEar();
}

void __stdcall EAR_DLL_ShutDownEar(void) {
    SetEarLastError(0);
    // The timer thread sets the lock while it updates the playback.
    while (g_bPlaybackUpdateLocked) {
    }
    g_bRegistryMutationLocked = 1;
    if (g_uPlaybackTimerId) {
        EarShutDownTimerExportWrapper();
    }
    if (IsEarReady()) {
        EarShutDownChannelExportWrapper(0x10000);
        EarShutDownMatrixExportWrapper(0);
        EarShutDownEnvironmentExportWrapper(0);
        EarShutDownPresetExportWrapper(0);
        ShutdownMusicDevicesAndRestoreMixer();
    }
    ShutdownDirectSoundOutput();
    // The original clears the flags in a chain, each from the one cleared before it.
    g_bEarProcessAttached = 0;
    g_bPlaybackUpdateLocked = g_bEarProcessAttached;
    g_bRegistryMutationLocked = g_bPlaybackUpdateLocked;
    g_bEarInitializationComplete = g_bRegistryMutationLocked;
    g_bEarComponentsInitialized = g_bEarInitializationComplete;
}

void __stdcall EarResetEarExportWrapper(void) {
    EAR_DLL_ResetEar();
}

void __stdcall EAR_DLL_ResetEar(void) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return;
    }
    while (g_bPlaybackUpdateLocked) {
    }
    g_bRegistryMutationLocked = 1;
    EarShutDownChannelExportWrapper(0);
    EarShutDownMatrixExportWrapper(0);
    EarShutDownEnvironmentExportWrapper(0);
    RegisterDefaultEnvironment();
    g_dwPreviousPlaybackUpdateTimeMs = 0;
    g_bRegistryMutationLocked = 0;
}

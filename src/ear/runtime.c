#include "runtime.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <float.h>
#endif

#include "error.h"
#include "playback.h"

volatile int32_t g_bEarProcessAttached;
volatile int32_t g_bPlaybackUpdateLocked;
volatile int32_t g_bRegistryMutationLocked;

int32_t g_bEarCodeValidated;
int32_t g_bEarComponentsInitialized;
int32_t g_bEarInitializationComplete;
int32_t g_bMixerAccessDisabled;
int32_t g_bConfigurationValidationDisabled;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
BOOL WINAPI EarDllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
#ifndef _MSC_VER
    // The start-up of the MSVC runtime sets 53-bit x87 precision before this; mingw-w64 does not.
    if (reason == DLL_PROCESS_ATTACH) {
        _controlfp(_PC_53, _MCW_PC);
    }
#endif
    if (reason == DLL_PROCESS_ATTACH) {
        g_bEarProcessAttached = 1;
        DisableThreadLibraryCalls(module);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_bEarProcessAttached) {
            EarShutDownEarExportWrapper();
            ExitProcess(0);
        }
        g_bEarCodeValidated = 0;
    }
    return TRUE;
}
#endif

int IsEarReady(void) {
    return g_bEarCodeValidated && g_bEarComponentsInitialized && g_bEarInitializationComplete;
}

int __stdcall EarEarInactiveExportWrapper(void) {
    return EAR_DLL_EarInactive();
}

int __stdcall EAR_DLL_EarInactive(void) {
    SetEarLastError(0);
    return IsEarReady() == 0;
}

void __stdcall EarUpdateEarExportWrapper(void) {
    EAR_DLL_UpdateEar();
}

void __stdcall EAR_DLL_UpdateEar(void) {
    SetEarLastError(0);
    if (IsEarReady()) {
        UpdateEarPlaybackEngine();
    }
}

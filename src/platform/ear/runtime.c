#include "runtime.h"

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

BOOL WINAPI EarDllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
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

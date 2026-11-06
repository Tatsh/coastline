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

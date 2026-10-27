#include "runtime.h"

#include "error.h"

int32_t g_bEarCodeValidated;
int32_t g_bEarComponentsInitialized;
int32_t g_bEarInitializationComplete;

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

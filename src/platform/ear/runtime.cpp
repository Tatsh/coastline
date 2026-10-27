#include "runtime.h"

#include "error.h"

std::int32_t g_bEarCodeValidated;
std::int32_t g_bEarComponentsInitialized;
std::int32_t g_bEarInitializationComplete;

int IsEarReady() {
    return g_bEarCodeValidated && g_bEarComponentsInitialized && g_bEarInitializationComplete;
}

int __stdcall EarEarInactiveExportWrapper() {
    return EAR_DLL_EarInactive();
}

int __stdcall EAR_DLL_EarInactive() {
    SetEarLastError(0);
    return IsEarReady() == 0;
}

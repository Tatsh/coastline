#ifndef COASTLINE_EAR_RUNTIME_H
#define COASTLINE_EAR_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x10003c86 */
int IsEarReady(void);

/** @ghidraAddress EAR: 0x1000c680 */
int __stdcall EarEarInactiveExportWrapper(void);

/** @ghidraAddress EAR: 0x1000c68a */
int __stdcall EAR_DLL_EarInactive(void);

/**
 * @ghidraAddress EAR: 0x1000f270
 * @stub
 */
void __stdcall EarShutDownEarExportWrapper(void);

/** @ghidraAddress EAR: 0x1003b784 */
extern int32_t g_bMixerAccessDisabled;
/** @ghidraAddress EAR: 0x1003b788 */
extern int32_t g_bConfigurationValidationDisabled;

/** @ghidraAddress EAR: 0x1003b7a0 */
extern int32_t g_bEarCodeValidated;

/** @ghidraAddress EAR: 0x1003b7a4 */
extern int32_t g_bEarComponentsInitialized;

/** @ghidraAddress EAR: 0x1003b7a8 */
extern int32_t g_bEarInitializationComplete;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_RUNTIME_H */

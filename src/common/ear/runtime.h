#ifndef COASTLINE_EAR_RUNTIME_H
#define COASTLINE_EAR_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @return Nonzero when code validation, component initialisation, and final initialisation markers
 * are all set; otherwise zero. This query does not change the last EAR error.
 * @ghidraAddress EAR: 0x10003c86
 */
int IsEarReady(void);

/**
 * @return Nonzero when EAR is not ready, after clearing the last EAR error through the export.
 * @ghidraAddress EAR: 0x1000c680
 */
int __stdcall EarEarInactiveExportWrapper(void);

/**
 * Clear the last EAR error and query readiness.
 * @return Nonzero when EAR is not ready; otherwise zero.
 * @ghidraAddress EAR: 0x1000c68a
 */
int __stdcall EAR_DLL_EarInactive(void);

/**
 * Forward a complete library shutdown to EAR_DLL_ShutDownEar().
 * @ghidraAddress EAR: 0x1000f270
 */
void __stdcall EarShutDownEarExportWrapper(void);

/**
 * Forward a manual playback update to EAR_DLL_UpdateEar().
 * @ghidraAddress EAR: 0x1000fbd0
 */
void __stdcall EarUpdateEarExportWrapper(void);

/**
 * Update the playback of every event once the library is ready.
 *
 * @ghidraAddress EAR: 0x1000fbda
 */
void __stdcall EAR_DLL_UpdateEar(void);

/**
 * Whether the process attached the library, cleared at its shutdown.
 *
 * @ghidraAddress EAR: 0x1003b79c
 */
/** @cond */
extern volatile int32_t g_bEarProcessAttached;

/**
 * Whether the timer thread is updating the playback, which the shutdown waits on.
 *
 * @ghidraAddress EAR: 0x1003b7ac
 */
extern volatile int32_t g_bPlaybackUpdateLocked;

/**
 * Whether a shutdown or a reset is changing the registries, which the timer thread waits on.
 *
 * @ghidraAddress EAR: 0x1003b7b0
 */
extern volatile int32_t g_bRegistryMutationLocked;

/**
 * Nonzero when music mixer discovery, configuration, and restoration are bypassed.
 * @ghidraAddress EAR: 0x1003b784
 */
extern int32_t g_bMixerAccessDisabled;
/**
 * Nonzero when title configuration validation is bypassed.
 * @ghidraAddress EAR: 0x1003b788
 */
extern int32_t g_bConfigurationValidationDisabled;

/**
 * Nonzero after successful numeric-code validation; readiness requires this marker.
 * @ghidraAddress EAR: 0x1003b7a0
 */
extern int32_t g_bEarCodeValidated;

/**
 * Latest component-initialisation stage result; readiness requires a nonzero marker.
 * @ghidraAddress EAR: 0x1003b7a4
 */
extern int32_t g_bEarComponentsInitialized;

/**
 * Final library initialisation marker required for readiness.
 * @ghidraAddress EAR: 0x1003b7a8
 */
extern int32_t g_bEarInitializationComplete;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_RUNTIME_H */

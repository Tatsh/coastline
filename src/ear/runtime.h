#ifndef COASTLINE_EAR_RUNTIME_H
#define COASTLINE_EAR_RUNTIME_H

#include <stdint.h>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x10003c86 */
int IsEarReady(void);

/** @ghidraAddress EAR: 0x1000c680 */
int __stdcall EarEarInactiveExportWrapper(void);

/** @ghidraAddress EAR: 0x1000c68a */
int __stdcall EAR_DLL_EarInactive(void);

/** @ghidraAddress EAR: 0x1000f270 */
void __stdcall EarShutDownEarExportWrapper(void);

/** @ghidraAddress EAR: 0x1000fbd0 */
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Record the attachment of the process, or shut the library down and exit the process when the
 * process detaches after attaching it.
 *
 * @param module The module of the library.
 * @param reason The reason of the call.
 * @param reserved Unused.
 * @return TRUE.
 * @ghidraAddress EAR: 0x100065aa
 */
BOOL WINAPI EarDllMain(HINSTANCE module, DWORD reason, LPVOID reserved);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_RUNTIME_H */

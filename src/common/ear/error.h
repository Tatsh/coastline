#ifndef COASTLINE_EAR_ERROR_H
#define COASTLINE_EAR_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Store an EAR error and display a nonzero error when automatic messages are enabled.
 * @param error The error-table index, 0..46; zero clears the error without showing a message.
 * The index is stored without a range check and must be valid before displaying an error.
 * @ghidraAddress EAR: 0x100077c0
 */
void __stdcall SetEarLastError(uint32_t error);

/**
 * @return The last EAR error through the export without clearing it.
 * @ghidraAddress EAR: 0x1000c960
 */
uint32_t __stdcall EarGetLastErrorExportWrapper(void);

/**
 * Forward display of the last nonzero EAR error to EAR_DLL_ShowLastError().
 * @ghidraAddress EAR: 0x1000f8d0
 */
void __stdcall EarShowLastErrorExportWrapper(void);

/**
 * @return The last stored EAR error without clearing it; zero indicates no error.
 * @ghidraAddress EAR: 0x1000c96a
 */
uint32_t __stdcall EAR_DLL_GetLastError(void);

/**
 * Show the last nonzero error in a modal message over the notification window without clearing it.
 * Stored nonzero errors must index g_earErrorMessages; the index is not checked here.
 * @ghidraAddress EAR: 0x1000f8da
 */
void __stdcall EAR_DLL_ShowLastError(void);

/**
 * Nonzero when SetEarLastError() automatically displays stored nonzero errors.
 * @ghidraAddress EAR: 0x1003b7e8
 */
/** @cond */
extern uint32_t g_bShowEarErrors;

/**
 * Static terminated error descriptions indexed 0..46; callers borrow strings without releasing
 * the strings.
 * @ghidraAddress EAR: 0x10036840
 */
extern const char *const g_earErrorMessages[47];
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_ERROR_H */

#ifndef COASTLINE_EAR_ERROR_H
#define COASTLINE_EAR_ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress EAR: 0x100077c0
 */
void __stdcall SetEarLastError(uint32_t error);

/** @ghidraAddress EAR: 0x1000c960 */
uint32_t __stdcall EarGetLastErrorExportWrapper(void);

/** @ghidraAddress EAR: 0x1000f8d0 */
void __stdcall EarShowLastErrorExportWrapper(void);

/** @ghidraAddress EAR: 0x1000c96a */
uint32_t __stdcall EAR_DLL_GetLastError(void);

/** @ghidraAddress EAR: 0x1000f8da */
void __stdcall EAR_DLL_ShowLastError(void);

/** @ghidraAddress EAR: 0x1003b7e8 */
extern uint32_t g_bShowEarErrors;

/** @ghidraAddress EAR: 0x10036840 */
extern const char *const g_earErrorMessages[47];

#ifdef __cplusplus
}
#endif

#endif

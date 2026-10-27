#ifndef COASTLINE_EAR_ERROR_H
#define COASTLINE_EAR_ERROR_H

#include <cstdint>

/**
 * @ghidraAddress EAR: 0x100077c0
 */
void __stdcall SetEarLastError(std::uint32_t error);

/** @ghidraAddress EAR: 0x1000c960 */
std::uint32_t __stdcall EarGetLastErrorExportWrapper();

/** @ghidraAddress EAR: 0x1000f8d0 */
void __stdcall EarShowLastErrorExportWrapper();

/** @ghidraAddress EAR: 0x1000c96a */
extern "C" std::uint32_t __stdcall EAR_DLL_GetLastError();

/** @ghidraAddress EAR: 0x1000f8da */
extern "C" void __stdcall EAR_DLL_ShowLastError();

/** @ghidraAddress EAR: 0x1003b7e8 */
extern std::uint32_t g_bShowEarErrors;

/** @ghidraAddress EAR: 0x10036840 */
extern const char *const g_earErrorMessages[47];

#endif

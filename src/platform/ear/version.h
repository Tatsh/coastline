#ifndef COASTLINE_EAR_VERSION_H
#define COASTLINE_EAR_VERSION_H

#include <cstdint>

/** @ghidraAddress EAR: 0x1000c8c0 */
std::uint32_t __stdcall EarGetVersionExportWrapper();

/** @ghidraAddress EAR: 0x1000c8ca */
extern "C" std::uint32_t __stdcall EAR_DLL_GetVersion();

#endif

#ifndef COASTLINE_EAR_DISTANCE_H
#define COASTLINE_EAR_DISTANCE_H

#include <cstdint>

/** @ghidraAddress EAR: 0x1000fb70 */
void __stdcall EarSetUserDistanceUnitExportWrapper(std::uint32_t unitIndex, std::uint32_t value);

/** @ghidraAddress EAR: 0x1000fb84 */
extern "C" void __stdcall EAR_DLL_SetUserDistanceUnit(std::uint32_t unitIndex, std::uint32_t value);

/** @ghidraAddress EAR: 0x100360e8 */
extern double g_adDistanceUnitToCentimeters[4];

#endif

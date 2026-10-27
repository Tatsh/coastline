#ifndef COASTLINE_EAR_DISTANCE_H
#define COASTLINE_EAR_DISTANCE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1000fb70 */
void __stdcall EarSetUserDistanceUnitExportWrapper(uint32_t unitIndex, uint32_t value);

/** @ghidraAddress EAR: 0x1000fb84 */
void __stdcall EAR_DLL_SetUserDistanceUnit(uint32_t unitIndex, uint32_t value);

/** @ghidraAddress EAR: 0x100360e8 */
extern double g_adDistanceUnitToCentimeters[4];

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_DISTANCE_H */

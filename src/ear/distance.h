#ifndef COASTLINE_EAR_DISTANCE_H
#define COASTLINE_EAR_DISTANCE_H

#include <stdint.h>

#include "eventparameters.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1000fb70 */
void __stdcall EarSetUserDistanceUnitExportWrapper(uint32_t unitIndex, uint32_t value);

/** @ghidraAddress EAR: 0x1000fb84 */
void __stdcall EAR_DLL_SetUserDistanceUnit(uint32_t unitIndex, uint32_t value);

/** @ghidraAddress EAR: 0x100360e8 */
extern double g_adDistanceUnitToCentimeters[4];

/**
 * @param params The parameters of the event.
 * @return The distance of the event across the ground, rounded up.
 * @ghidraAddress EAR: 0x10004445
 */
int32_t __stdcall CalculateRoundedPlanarDistance(const EarEventParams *params);

/**
 * Find the gain of an event at a distance, falling linearly between its two distances. The
 * maximum gain is first limited to 10000.
 *
 * @param distance The distance of the event.
 * @param params The parameters of the event.
 * @return The gain.
 * @ghidraAddress EAR: 0x1000447c
 */
uint16_t __stdcall CalculateDistanceAttenuation(uint32_t distance, EarEventParams *params);

/**
 * Convert the position and the distances of an event from its unit to centimetres.
 *
 * @param params The parameters of the event.
 * @ghidraAddress EAR: 0x10004601
 */
void __stdcall ConvertPlayDistancesToCentimeters(EarEventParams *params);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_DISTANCE_H */

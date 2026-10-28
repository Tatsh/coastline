#ifndef COASTLINE_EAR_DIRECTION_H
#define COASTLINE_EAR_DIRECTION_H

#include <stdint.h>

#include "eventparameters.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Find the direction of an event among the directions of a directional matrix.
 *
 * @param distance The distance of the event.
 * @param directionCount The number of directions of the matrix.
 * @param params The parameters of the event.
 * @return The direction, or the direction of the parameters when the event is not directional.
 * @ghidraAddress EAR: 0x10004230
 */
uint16_t __stdcall CalculateDirectionalPresetIndex(uint32_t distance,
                                                   uint16_t directionCount,
                                                   const EarEventParams *params);

/**
 * Find the sector of a range of directions a position lies in, the directions turning from the
 * front.
 *
 * @param first The first direction of the range.
 * @param last The last direction of the range.
 * @param x The position across.
 * @param z The position ahead.
 * @return The direction.
 * @ghidraAddress EAR: 0x1000483c
 */
uint16_t __stdcall CalculateDirectionSector(uint16_t first, uint16_t last, int32_t x, int32_t z);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_DIRECTION_H */

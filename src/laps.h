#ifndef COASTLINE_LAPS_H
#define COASTLINE_LAPS_H

/**
 * @file
 * The laps of the race.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The laps of the race.
 *
 * @ghidraAddress USA: 0x8028b304
 * @ghidraAddress EUR: 0x8028d5b4
 * @ghidraAddress JPN: 0x8028cfe4
 * @ghidraAddress BRD3D: 0x100bd3e0
 * @ghidraAddress BRGlide: 0x100bcbe8
 */
extern s32 g_dwTotalLaps;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_LAPS_H */

#ifndef COASTLINE_VEHICLECOUNT_H
#define COASTLINE_VEHICLECOUNT_H

/**
 * @file
 * The numbers of the racers and the vehicles of a race.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The racers in the race.
 *
 * @ghidraAddress USA: 0x8028b7f0
 * @ghidraAddress EUR: 0x8028daa0
 * @ghidraAddress JPN: 0x8028d4d0
 * @ghidraAddress BRD3D: 0x100b36f8
 * @ghidraAddress BRGlide: 0x100b2f00
 */
extern s32 g_dwNumVehicles;

/**
 * The vehicles of g_aVehicles the race uses.
 *
 * @ghidraAddress USA: 0x8028b7f4
 * @ghidraAddress EUR: 0x8028daa4
 * @ghidraAddress JPN: 0x8028d4d4
 * @ghidraAddress BRD3D: 0x100b36fc
 * @ghidraAddress BRGlide: 0x100b2f04
 */
extern s32 g_dwNumActiveVehicles;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLECOUNT_H */

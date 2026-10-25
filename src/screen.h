#ifndef COASTLINE_SCREEN_H
#define COASTLINE_SCREEN_H

/**
 * @file
 * The size of the frame.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The size of the frame in pixels.
 *
 * @ghidraAddress USA: 0x8028aab0
 * @ghidraAddress EUR: 0x8028ccb8
 * @ghidraAddress JPN: 0x8028c788
 * @ghidraAddress BRD3D: 0x106c0684
 * @ghidraAddress BRGlide: 0x106e7714
 */
extern s32 g_dwScreenWidth;

/**
 * @ghidraAddress USA: 0x8028aab4
 * @ghidraAddress EUR: 0x8028ccbc
 * @ghidraAddress JPN: 0x8028c78c
 * @ghidraAddress BRD3D: 0x106c299c
 * @ghidraAddress BRGlide: 0x106e9a2c
 */
extern s32 g_dwScreenHeight;

#ifdef __cplusplus
}
#endif

#endif

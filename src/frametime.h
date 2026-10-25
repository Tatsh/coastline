#ifndef COASTLINE_FRAMETIME_H
#define COASTLINE_FRAMETIME_H

/**
 * @file
 * The time of the last frame.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The time the last frame took, in seconds.
 *
 * @ghidraAddress USA: 0x8028aad8
 * @ghidraAddress EUR: 0x8028cce0
 * @ghidraAddress JPN: 0x8028c7b0
 * @ghidraAddress BRD3D: 0x106c2cfc
 * @ghidraAddress BRGlide: 0x106e9d8c
 */
extern f32 g_fFrameDeltaTime;

#ifdef __cplusplus
}
#endif

#endif

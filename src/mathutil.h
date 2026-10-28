#ifndef COASTLINE_MATHUTIL_H
#define COASTLINE_MATHUTIL_H

/**
 * @file
 * Small numeric helpers.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert a float to an integer, truncating towards zero.
 *
 * @param value The value to convert.
 * @return The truncated value.
 * @ghidraAddress USA: 0x802562e0
 * @ghidraAddress EUR: 0x802578b0
 * @ghidraAddress JPN: 0x80257240
 * @ghidraAddress BRD3D: 0x1002b920
 * @ghidraAddress BRGlide: 0x10018990
 */
s32 FloatToInt(float value);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_MATHUTIL_H */

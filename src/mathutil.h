#ifndef COASTLINE_MATHUTIL_H
#define COASTLINE_MATHUTIL_H

/**
 * @file
 * Small numeric helpers.
 */

#include <PR/ultratypes.h>

/**
 * Convert a float to an integer, truncating towards zero.
 *
 * @param value The value to convert.
 * @return The truncated value.
 * @ghidraAddress USA: 0x802562e0
 */
s32 FloatToInt(float value);

#endif

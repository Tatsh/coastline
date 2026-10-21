#ifndef COASTLINE_TIMING_H
#define COASTLINE_TIMING_H

/**
 * @file
 * The markers that time the parts of a frame.
 */

#include <PR/ultratypes.h>

/**
 * Mark the time since the start of the frame on a bar, in a colour of 8-bit components.
 *
 * @ghidraAddress USA: 0x8022d7e0
 */
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha);

#endif

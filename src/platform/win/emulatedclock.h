#ifndef COASTLINE_WIN_EMULATEDCLOCK_H
#define COASTLINE_WIN_EMULATEDCLOCK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The time of the console the game emulates, in ticks of its CPU counter.
 *
 * @ghidraAddress BRD3D: 0x118abde0
 * @ghidraAddress BRGlide: 0x118eeef8
 */
extern u64 g_qwEmulatedOsTime;

/**
 * Advance g_qwEmulatedOsTime by a frame of 1/30 of a second.
 *
 * @ghidraAddress BRD3D: 0x10078c10
 * @ghidraAddress BRGlide: 0x10071f00
 */
u64 AdvanceEmulatedOsTime(void);

#ifdef __cplusplus
}
#endif

#endif

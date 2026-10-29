#ifndef COASTLINE_WIN_EMULATEDCLOCK_H
#define COASTLINE_WIN_EMULATEDCLOCK_H

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#endif /* COASTLINE_WIN_EMULATEDCLOCK_H */

#ifndef COASTLINE_INSTANTREPLAY_H
#define COASTLINE_INSTANTREPLAY_H

/**
 * @file
 * Whether an instant replay is playing.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether an instant replay is playing. The PC libraries also compare it with 2.
 *
 * @ghidraAddress USA: 0x80270788
 * @ghidraAddress EUR: 0x80272758
 * @ghidraAddress JPN: 0x80272208
 * @ghidraAddress BRD3D: 0x106909e0
 * @ghidraAddress BRGlide: 0x105ccb88
 */
extern s32 gInstantReplayActive;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_INSTANTREPLAY_H */

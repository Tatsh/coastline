#ifndef COASTLINE_PLATFORM_WIN_SNAPSHOT_H
#define COASTLINE_PLATFORM_WIN_SNAPSHOT_H

/**
 * @file
 * The snapshots of frames of the PC port.
 */

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether the snapshots of frames are set up.
 *
 * @ghidraAddress BRD3D: 0x104afd1c
 * @ghidraAddress BRGlide: 0x104ab504
 */
extern int g_bFrameSnapshotsInitialized;

/**
 * Mark the snapshots of frames as not set up.
 *
 * @ghidraAddress BRD3D: 0x10016990
 * @ghidraAddress BRGlide: 0x10013f00
 */
void InvalidateFrameSnapshotSlots(void);

/**
 * The number of frames rendered from interpolated snapshots since the slots were set up.
 *
 * @ghidraAddress BRD3D: 0x104afd08
 * @ghidraAddress BRGlide: 0x104ab4f0
 */
extern int g_dwInterpolatedFrameCount;

/**
 * Get the number of frames rendered from interpolated snapshots since the slots were set up.
 *
 * @return The number of frames.
 * @ghidraAddress BRD3D: 0x10016a50
 * @ghidraAddress BRGlide: 0x10013fc0
 */
int GetInterpolatedFrameCount(void);

#ifdef __cplusplus
}
#endif

#endif // COASTLINE_PLATFORM_WIN_SNAPSHOT_H

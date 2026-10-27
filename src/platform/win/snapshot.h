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

enum { FRAME_SNAPSHOT_COUNT = 5 };

typedef struct {
    unsigned sequence;
    unsigned char unreconstructed4[0x2e0f0 - 4]; /* +0x04 */
} FrameSnapshot;

/** @ghidraAddress BRD3D: 0x1039b760 */
extern FrameSnapshot g_aFrameSnapshots[FRAME_SNAPSHOT_COUNT];
/** @ghidraAddress BRD3D: 0x1039b728 */
extern int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
/** @ghidraAddress BRD3D: 0x104afd14 */
extern int g_dwFrameSnapshotBlendFromSlot;
/** @ghidraAddress BRD3D: 0x1039b75c */
extern int g_nLastInterpolatedSnapshot;
/** @ghidraAddress BRD3D: 0x104afd00 */
extern int g_dwFrameSnapshotWriteSlot;
/** @ghidraAddress BRD3D: 0x104afd04 */
extern int g_dwFrameSnapshotCurrentSlot;
/** @ghidraAddress BRD3D: 0x104afd18 */
extern int g_dwFrameSnapshotPreviousSlot;
/**
 * @ghidraAddress BRD3D: 0x100a66e8
 * @ghidraAddress BRGlide: 0x100a5ea8
 */
extern int g_bFrameTimerReset;
/** @ghidraAddress BRD3D: 0x1039b73c */
extern float g_fFrameSnapshotBlendCarry;
/** @ghidraAddress BRD3D: 0x104afd0c */
extern unsigned g_dwFrameSnapshotBlendStartTime;

/** @ghidraAddress BRD3D: 0x10016910 */
void InitFrameSnapshotSlots(void);

/** @ghidraAddress BRD3D: 0x100169b0 */
void AcquireNextFrameSnapshotSlot(void);

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

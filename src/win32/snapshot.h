#ifndef COASTLINE_PLATFORM_WIN_SNAPSHOT_H
#define COASTLINE_PLATFORM_WIN_SNAPSHOT_H

/**
 * @file
 * The snapshots of frames of the PC port.
 */

#include "common/racerslot.h"
#include "vehicle.h"
#include "win32/particlesnapshot.h"
#include "win32/snapshotops.h"

enum { FRAME_SNAPSHOT_COUNT = 5 };

class FrameSnapshot {
private:
    unsigned sequence;
    RacerSlot racers[RACER_SLOTS];
    [[maybe_unused]] unsigned char unreconstructedA04[4]; /* +0xa04 */
    Vehicle vehicles[MAX_VEHICLES];
    ViewportRect viewport;
    unsigned unreconstructed2c0e0; /* +0x2c0e0 */
    ParticleSnapshot particles;
    [[maybe_unused]] unsigned char unreconstructed2e0ec[4]; /* +0x2e0ec */

public:
    unsigned GetSequence() const {
        return sequence;
    }
    void SetSequence(unsigned value) {
        sequence = value;
    }
    const CameraFrame &GetPrimaryCamera() const {
        return *vehicles[0].GetActiveCamera();
    }
    void Capture();
    void RenderRacingScreen();
    void InterpolateFrom(const FrameSnapshot &from, const FrameSnapshot &to, long double share);
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x1039b760
 * @ghidraAddress BRGlide: 0x10396f48
 */
extern FrameSnapshot g_aFrameSnapshots[FRAME_SNAPSHOT_COUNT];
/**
 * @ghidraAddress BRD3D: 0x10481c10
 * @ghidraAddress BRGlide: 0x1047d3f8
 */
extern FrameSnapshot g_InterpolatedFrameSnapshot;

/** @ghidraAddress BRD3D: 0x1039b728 */
extern int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
/** @ghidraAddress BRD3D: 0x104afd14 */
extern int g_dwFrameSnapshotBlendFromSlot;
/** @ghidraAddress BRD3D: 0x1039b75c */
extern int g_nLastInterpolatedSnapshot;

/** @ghidraAddress BRD3D: 0x104afd04 */
extern int g_dwFrameSnapshotCurrentSlot;
/** @ghidraAddress BRD3D: 0x104afd18 */
extern int g_dwFrameSnapshotPreviousSlot;
/** @ghidraAddress BRD3D: 0x1039b73c */
extern float g_fFrameSnapshotBlendCarry;
/** @ghidraAddress BRD3D: 0x104afd0c */
extern unsigned g_dwFrameSnapshotBlendStartTime;

/**
 * @ghidraAddress BRD3D: 0x10016910
 * @ghidraAddress BRGlide: 0x10013e80
 */
void InitFrameSnapshotSlots(void);

/**
 * @ghidraAddress BRD3D: 0x100169b0
 * @ghidraAddress BRGlide: 0x10013f20
 */
int AcquireNextFrameSnapshotSlot(void);

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

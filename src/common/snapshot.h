#ifndef COASTLINE_PLATFORM_WIN_SNAPSHOT_H
#define COASTLINE_PLATFORM_WIN_SNAPSHOT_H

/**
 * @file
 * The snapshots of frames of the PC port.
 */

#include "common/particlesnapshot.h"
#include "common/racerslot.h"
#include "common/snapshotops.h"
#include "common/vehicle.h"

/** The capacity of the rotating race snapshot pool. */
enum { FRAME_SNAPSHOT_COUNT = 5 /*!< Five slots for captured and currently rendered frames. */ };

/** Captured race geometry, cameras, and particles for interpolation between simulation steps. */
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
    /** @return The monotonically assigned capture sequence for this pool slot. */
    unsigned GetSequence() const {
        return sequence;
    }
    /**
     * Assign a capture sequence without modifying the stored race frame.
     *
     * @param value The capture sequence, or zero when the slot pool is initialised.
     */
    void SetSequence(unsigned value) {
        sequence = value;
    }
    /** @return The captured first vehicle's active camera; its camera pointer must be valid. */
    const CameraFrame &GetPrimaryCamera() const {
        return *vehicles[0].GetActiveCamera();
    }
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    /**
     * @param index The vehicle index, from 0 through MAX_VEHICLES - 1.
     * @return The captured vehicle, valid for this snapshot's lifetime and replaced by capture.
     */
    const Vehicle &GetVehicle(int index) const {
        return vehicles[index];
    }
    /** @return Mutable storage for MAX_VEHICLES captured vehicles, valid for this snapshot's
     * lifetime.
     */
    Vehicle *GetVehicles() {
        return vehicles;
    }
    /** @return Captured particles, valid for this snapshot's lifetime and replaced by capture. */
    const ParticleSnapshot &GetParticles() const {
        return particles;
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */
    /** Capture active vehicles, racers, the primary viewport, and particles from the race. */
    void Capture();
    /** Render the racing screen from this snapshot using the active renderer. */
    inline void RenderRacingScreen();
    /**
     * Interpolate vehicle poses and cameras while copying racers, viewport, and particles.
     *
     * @param from The earlier captured frame.
     * @param to The later captured frame.
     * @param share The interpolation fraction, from 0 for @p from through 1 for @p to.
     */
    inline void
    InterpolateFrom(const FrameSnapshot &from, const FrameSnapshot &to, long double share);
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The rotating snapshot pool, indexed by the current, previous, and blend slot selectors.
 *
 * @ghidraAddress BRD3D: 0x1039b760
 * @ghidraAddress BRGlide: 0x10396f48
 */
extern FrameSnapshot g_aFrameSnapshots[FRAME_SNAPSHOT_COUNT];
/**
 * Temporary output of interpolation between two captured race frames.
 *
 * @ghidraAddress BRD3D: 0x10481c10
 * @ghidraAddress BRGlide: 0x1047d3f8
 */
extern FrameSnapshot g_InterpolatedFrameSnapshot;

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** @return The active engine's slot-use flags, one per snapshot slot.
 * @ghidraAddress BRD3D: 0x1039b728
 */
int (*GetSdlSnapshotUsage(void))[FRAME_SNAPSHOT_COUNT];
/** Slot-use flags for the active SDL engine's snapshot pool. */
#define g_abFrameSnapshotInUse (*GetSdlSnapshotUsage())
#else
/** Nonzero for each snapshot slot currently referenced by a renderer.
 * @ghidraAddress BRD3D: 0x1039b728
 */
extern int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
#endif
/** The earlier snapshot selected for the current blend, or -1 before selection.
 * @ghidraAddress BRD3D: 0x104afd14
 */
extern int g_dwFrameSnapshotBlendFromSlot;
/** The later snapshot slot selected for interpolation, or -1 before selection.
 * @ghidraAddress BRD3D: 0x1039b75c
 */
extern int g_nLastInterpolatedSnapshot;

/** The most recent completed snapshot slot, or -1 before the first capture.
 * @ghidraAddress BRD3D: 0x104afd04
 */
extern int g_dwFrameSnapshotCurrentSlot;
/** The completed snapshot slot preceding the current slot, or -1 before two captures.
 * @ghidraAddress BRD3D: 0x104afd18
 */
extern int g_dwFrameSnapshotPreviousSlot;
/** Fractional interpolation progress retained from the previous rendered frame.
 * @ghidraAddress BRD3D: 0x1039b73c
 */
extern float g_fFrameSnapshotBlendCarry;
/** The millisecond timer value of the previous interpolation update, or zero before the first.
 * @ghidraAddress BRD3D: 0x104afd0c
 */
extern unsigned g_dwFrameSnapshotBlendStartTime;

/**
 * Initialise slot flags, sequence counters, and interpolation timers once per valid pool.
 *
 * @ghidraAddress BRD3D: 0x10016910
 * @ghidraAddress BRGlide: 0x10013e80
 */
void InitFrameSnapshotSlots(void);

/**
 * Select the oldest unused slot, advancing the current and previous completed slot selectors.
 * At least one unused slot other than the current write slot must be available.
 *
 * @return The selected write slot, from 0 through FRAME_SNAPSHOT_COUNT - 1.
 * @ghidraAddress BRD3D: 0x100169b0
 * @ghidraAddress BRGlide: 0x10013f20
 */
int AcquireNextFrameSnapshotSlot(void);

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/**
 * Blend the last two snapshots of the race into g_InterpolatedFrameSnapshot as
 * InterpolateFrameSnapshotsAndRender() blends them, without drawing, for the SDL game to draw.
 *
 * @param share How far the frame is from the previous snapshot to the current one, from 0 to 1.
 * @return The blended snapshot, or null before the race has two snapshots.
 */
FrameSnapshot *InterpolateFrameSnapshots(float share);
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

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

#ifndef COASTLINE_DRIVING_H
#define COASTLINE_DRIVING_H

/**
 * @file
 * Driving: the controls of a player's vehicle and the views a replay chooses, where a vehicle is
 * on the route, and the computer's drivers, which look ahead along the route for the farthest
 * point they can drive straight to.
 */

#include <PR/ultratypes.h>

#include "frameclear.h"
#include "track.h"
#include "vecmath.h"
#include "vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The points of the route a search for a straight line looks ahead. */
#define TRACE_DEPTH 8

/**
 * The axis that points up.
 *
 * @ghidraAddress USA: 0x8028bab0
 * @ghidraAddress EUR: 0x8028dd64
 * @ghidraAddress JPN: 0x8028d794
 */
extern Vec3f g_vUpAxis;

/**
 * How much of its velocity a computer's vehicle keeps each frame in the championship, by season,
 * race and the class of the racer.
 *
 * @ghidraAddress USA: 0x8028b9f0
 * @ghidraAddress EUR: 0x8028dca4
 * @ghidraAddress JPN: 0x8028d6d4
 */
extern f32 g_aafAiVelocityRetention[SEASONS][SEASON_RACES][2];

/**
 * Whether a replay switches from the trackside camera to the cockpit next.
 *
 * @ghidraAddress USA: 0x8028b9ec
 * @ghidraAddress EUR: 0x8028dca0
 * @ghidraAddress JPN: 0x8028d6d0
 * @ghidraAddress BRD3D: 0x10af9bb8
 */
extern s32 g_bReplayCockpitNext;

/**
 * The jolts each vehicle took, from 0 to 5.
 *
 * @ghidraAddress USA: 0x8031b1d8
 * @ghidraAddress EUR: 0x8031d6d8
 * @ghidraAddress JPN: 0x8031c308
 * @ghidraAddress BRD3D: 0x106c5468
 */
extern f32 g_afVehicleJolts[];

/**
 * The deepest a search reached along a straight line.
 *
 * @ghidraAddress USA: 0x8028b804
 * @ghidraAddress EUR: 0x8028dab4
 * @ghidraAddress JPN: 0x8028d4e4
 */
extern s32 g_dwBestTraceDepth;

/**
 * The section and point where that search started.
 *
 * @ghidraAddress USA: 0x8028b814
 * @ghidraAddress EUR: 0x8028dac4
 * @ghidraAddress JPN: 0x8028d4f4
 */
extern TrackSection *g_pBestTraceSection;

/**
 * @ghidraAddress USA: 0x8028b810
 * @ghidraAddress EUR: 0x8028dac0
 * @ghidraAddress JPN: 0x8028d4f0
 */
extern s32 g_dwBestTracePoint;

/**
 * The points of that line.
 *
 * @ghidraAddress USA: 0x8028b800
 * @ghidraAddress EUR: 0x8028dab0
 * @ghidraAddress JPN: 0x8028d4e0
 */
extern s32 g_dwBestTraceLength;

/**
 * Cleared at the start of a search and never read.
 *
 * @ghidraAddress USA: 0x8028b81c
 * @ghidraAddress EUR: 0x8028dacc
 * @ghidraAddress JPN: 0x8028d4fc
 */
extern s32 g_dwTraceUnusedA;

/**
 * Cleared at the start of a search and never read.
 *
 * @ghidraAddress USA: 0x8028b818
 * @ghidraAddress EUR: 0x8028dac8
 * @ghidraAddress JPN: 0x8028d4f8
 */
extern s32 g_dwTraceUnusedB;

/**
 * Set when a line of a search left the route; never read.
 *
 * @ghidraAddress USA: 0x8028b820
 * @ghidraAddress EUR: 0x8028dad0
 * @ghidraAddress JPN: 0x8028d500
 */
extern s32 g_bTraceBlocked;

/**
 * Whether a computer's vehicle should ease to the left or to the right of where it steers.
 *
 * @ghidraAddress USA: 0x8028b80c
 * @ghidraAddress EUR: 0x8028dabc
 * @ghidraAddress JPN: 0x8028d4ec
 */
extern s32 g_bAiNudgeLeft;

/**
 * @ghidraAddress USA: 0x8028b808
 * @ghidraAddress EUR: 0x8028dab8
 * @ghidraAddress JPN: 0x8028d4e8
 */
extern s32 g_bAiNudgeRight;

/**
 * The lanes and the middle of the deepest straight line.
 *
 * @ghidraAddress USA: 0x8031b430
 * @ghidraAddress EUR: 0x8031d930
 * @ghidraAddress JPN: 0x8031c560
 */
extern Vec3f g_avBestLeftLane[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b490
 * @ghidraAddress EUR: 0x8031d990
 * @ghidraAddress JPN: 0x8031c5c0
 */
extern Vec3f g_avBestCenterLine[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b4f0
 * @ghidraAddress EUR: 0x8031d9f0
 * @ghidraAddress JPN: 0x8031c620
 */
extern Vec3f g_avBestRightLane[TRACE_DEPTH];

/**
 * The points of the line a search follows: a fifth of the way in from each side, halfway from there
 * to the middle, and the middle.
 *
 * @ghidraAddress USA: 0x8031b550
 * @ghidraAddress EUR: 0x8031da50
 * @ghidraAddress JPN: 0x8031c680
 */
extern Vec3f g_avTraceLeftLane[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b5b0
 * @ghidraAddress EUR: 0x8031dab0
 * @ghidraAddress JPN: 0x8031c6e0
 */
extern Vec3f g_avTraceLeftHalf[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b610
 * @ghidraAddress EUR: 0x8031db10
 * @ghidraAddress JPN: 0x8031c740
 */
extern Vec3f g_avTraceCenterLine[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b670
 * @ghidraAddress EUR: 0x8031db70
 * @ghidraAddress JPN: 0x8031c7a0
 */
extern Vec3f g_avTraceRightHalf[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b6d0
 * @ghidraAddress EUR: 0x8031dbd0
 * @ghidraAddress JPN: 0x8031c800
 */
extern Vec3f g_avTraceRightLane[TRACE_DEPTH];

/**
 * The last point of the deepest line, and its point halfway.
 *
 * @ghidraAddress USA: 0x8031b730
 * @ghidraAddress EUR: 0x8031dc30
 * @ghidraAddress JPN: 0x8031c860
 */
extern Vec3f g_vTraceFarPoint;

/**
 * @ghidraAddress USA: 0x8031b740
 * @ghidraAddress EUR: 0x8031dc40
 * @ghidraAddress JPN: 0x8031c870
 */
extern Vec3f g_vTraceHalfwayPoint;

/**
 * Add a jolt to a vehicle, at most 2.5 at once and 5 in all.
 *
 * @param index The index of the vehicle.
 * @param amount The jolt.
 * @ghidraAddress USA: 0x8021be28
 * @ghidraAddress EUR: 0x8021d008
 * @ghidraAddress JPN: 0x8021c8d8
 * @ghidraAddress BRD3D: 0x100347ba
 */
void AddVehicleJolt(s32 index, f32 amount);

#if TGR_PLATFORM == TGR_PLATFORM_N64 && (TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU)
/**
 * Stand the trackside camera of a vehicle at the nearest point of the track no more than 10 below
 * the vehicle and within 150 of it, or switch from the trackside camera to the cockpit or the
 * chase camera when there is none.
 *
 * @param vehicle The vehicle the camera watches.
 * @ghidraAddress EUR: 0x802277d0
 * @ghidraAddress JPN: 0x80226fc0
 */
void ChooseReplayCamera(Vehicle *vehicle);
#endif

/**
 * Read the controls of a vehicle, choose the view of a replay, and step the vehicle.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226488
 * @ghidraAddress EUR: 0x80227980
 * @ghidraAddress JPN: 0x80227170
 */
void ProcessVehicleInputAndPhysics(Vehicle *vehicle);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** Apply effect bytes requested by the current input mask. */
void ApplyPlayerVehicleEffects(Vehicle *vehicle);
/** Consume the input poll's respawn request. */
void ApplyPlayerVehicleRespawnRequest(Vehicle *vehicle);
#endif

/**
 * Search the route ahead of a vehicle for straight lines it could drive, keeping the deepest.
 *
 * @param vehicle The vehicle.
 * @param depth The points searched so far; 0 starts a search.
 * @param pointIndex The point of the section to search from.
 * @param section The section to search, or NULL for the start of the route.
 * @return Whether this search or one it made found a deeper line.
 * @ghidraAddress USA: 0x80226d9c
 * @ghidraAddress EUR: 0x80228100
 * @ghidraAddress JPN: 0x802278f0
 */
s32 TraceTrackSplineRecursive(Vehicle *vehicle, s32 depth, s32 pointIndex, TrackSection *section);

/**
 * Find the point of the route nearest a vehicle along a curve through the points, with the axes
 * of the route there and the offset of the vehicle from it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80227214
 * @ghidraAddress EUR: 0x80228578
 * @ghidraAddress JPN: 0x80227d68
 */
void ComputeTrackPosition(Vehicle *vehicle);

/**
 * Drive a computer's vehicle for a frame: steer for a point ahead, keep to the route, pass the
 * others, back out when stuck, and step it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022762c
 * @ghidraAddress EUR: 0x80228990
 * @ghidraAddress JPN: 0x80228180
 */
void UpdateAiVehicleBehavior(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif

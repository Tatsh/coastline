#ifndef COASTLINE_DRIVING_H
#define COASTLINE_DRIVING_H

/**
 * @file
 * Driving: the controls of a player's vehicle and the views a replay chooses, where a vehicle is
 * on the route, and the computer's drivers, which look ahead along the route for the farthest
 * point they can drive straight to.
 */

#include <PR/ultratypes.h>

#include "track.h"
#include "vecmath.h"
#include "vehicle.h"

/** The points of the route a search for a straight line looks ahead. */
#define TRACE_DEPTH 8

/** A block of variables cleared at the start of each frame. */
typedef struct {
    u8 *start; /*!< The first byte of the block, or NULL to end the list. */
    u32 size;  /*!< The size of the block, in bytes. */
} PerFrameVariable;

/**
 * The variables of the searches for straight lines, cleared each frame, ending in a NULL start.
 *
 * @ghidraAddress USA: 0x8028b834
 * @ghidraAddress JPN: 0x8028d514
 * @ghidraAddress EUR: 0x8028dae4
 */
extern PerFrameVariable g_aPerFrameVariables[];

/**
 * The axis that points up.
 *
 * @ghidraAddress USA: 0x8028bab0
 * @ghidraAddress JPN: 0x8028d794
 * @ghidraAddress EUR: 0x8028dd64
 */
extern Vec3f g_vUpAxis;

/**
 * How much of its velocity a computer's vehicle keeps each frame in the championship, by season,
 * race and the class of the racer.
 *
 * @ghidraAddress USA: 0x8028b9f0
 */
extern f32 g_aafAiVelocityRetention[SEASONS][SEASON_RACES][2];

/**
 * Whether a replay that leaves the trackside camera takes the cockpit next, rather than the chase
 * camera.
 *
 * @ghidraAddress USA: 0x8028b9ec
 */
extern s32 g_bReplayCockpitNext;

/**
 * The jolts each vehicle took, from 0 to 5.
 *
 * @ghidraAddress USA: 0x8031b1d8
 * @ghidraAddress JPN: 0x8031c308
 * @ghidraAddress EUR: 0x8031d6d8
 */
extern f32 g_afVehicleJolts[];

/**
 * The deepest a search reached along a straight line.
 *
 * @ghidraAddress USA: 0x8028b804
 * @ghidraAddress JPN: 0x8028d4e4
 * @ghidraAddress EUR: 0x8028dab4
 */
extern s32 g_dwBestTraceDepth;

/**
 * The section and point where that search started.
 *
 * @ghidraAddress USA: 0x8028b814
 * @ghidraAddress JPN: 0x8028d4f4
 * @ghidraAddress EUR: 0x8028dac4
 */
extern TrackSection *g_pBestTraceSection;

/**
 * @ghidraAddress USA: 0x8028b810
 * @ghidraAddress JPN: 0x8028d4f0
 * @ghidraAddress EUR: 0x8028dac0
 */
extern s32 g_dwBestTracePoint;

/**
 * The points of that line.
 *
 * @ghidraAddress USA: 0x8028b800
 * @ghidraAddress JPN: 0x8028d4e0
 * @ghidraAddress EUR: 0x8028dab0
 */
extern s32 g_dwBestTraceLength;

/**
 * Cleared at the start of a search and never read.
 *
 * @ghidraAddress USA: 0x8028b81c
 * @ghidraAddress JPN: 0x8028d4fc
 * @ghidraAddress EUR: 0x8028dacc
 */
extern s32 g_dwTraceUnusedA;

/**
 * Cleared at the start of a search and never read.
 *
 * @ghidraAddress USA: 0x8028b818
 * @ghidraAddress JPN: 0x8028d4f8
 * @ghidraAddress EUR: 0x8028dac8
 */
extern s32 g_dwTraceUnusedB;

/**
 * Set when a line of a search left the route; never read.
 *
 * @ghidraAddress USA: 0x8028b820
 * @ghidraAddress JPN: 0x8028d500
 * @ghidraAddress EUR: 0x8028dad0
 */
extern s32 g_bTraceBlocked;

/**
 * Whether a computer's vehicle should ease to the left or to the right of where it steers.
 *
 * @ghidraAddress USA: 0x8028b80c
 * @ghidraAddress JPN: 0x8028d4ec
 * @ghidraAddress EUR: 0x8028dabc
 */
extern s32 g_bAiNudgeLeft;

/**
 * @ghidraAddress USA: 0x8028b808
 * @ghidraAddress JPN: 0x8028d4e8
 * @ghidraAddress EUR: 0x8028dab8
 */
extern s32 g_bAiNudgeRight;

/**
 * The lanes and the middle of the deepest straight line.
 *
 * @ghidraAddress USA: 0x8031b430
 * @ghidraAddress JPN: 0x8031c560
 * @ghidraAddress EUR: 0x8031d930
 */
extern Vec3f g_avBestLeftLane[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b490
 * @ghidraAddress JPN: 0x8031c5c0
 * @ghidraAddress EUR: 0x8031d990
 */
extern Vec3f g_avBestCenterLine[TRACE_DEPTH];

/**
 * @ghidraAddress USA: 0x8031b4f0
 * @ghidraAddress JPN: 0x8031c620
 * @ghidraAddress EUR: 0x8031d9f0
 */
extern Vec3f g_avBestRightLane[TRACE_DEPTH];

/**
 * The points of the line a search follows: a fifth of the way in from each side, halfway from there
 * to the middle, and the middle.
 *
 * @ghidraAddress USA: 0x8031b550
 */
extern Vec3f g_avTraceLeftLane[TRACE_DEPTH];

/** @ghidraAddress USA: 0x8031b5b0 */
extern Vec3f g_avTraceLeftHalf[TRACE_DEPTH];

/** @ghidraAddress USA: 0x8031b610 */
extern Vec3f g_avTraceCenterLine[TRACE_DEPTH];

/** @ghidraAddress USA: 0x8031b670 */
extern Vec3f g_avTraceRightHalf[TRACE_DEPTH];

/** @ghidraAddress USA: 0x8031b6d0 */
extern Vec3f g_avTraceRightLane[TRACE_DEPTH];

/**
 * The last point of the deepest line, and its point halfway.
 *
 * @ghidraAddress USA: 0x8031b730
 * @ghidraAddress JPN: 0x8031c860
 * @ghidraAddress EUR: 0x8031dc30
 */
extern Vec3f g_vTraceFarPoint;

/**
 * @ghidraAddress USA: 0x8031b740
 * @ghidraAddress JPN: 0x8031c870
 * @ghidraAddress EUR: 0x8031dc40
 */
extern Vec3f g_vTraceHalfwayPoint;

/**
 * Clear the variables g_aPerFrameVariables lists.
 *
 * @ghidraAddress USA: 0x80225f30
 * @ghidraAddress JPN: 0x80226a60
 * @ghidraAddress EUR: 0x80227270
 */
void ClearPerFrameVariables(void);

/**
 * Add a jolt to a vehicle, at most 2.5 at once and 5 in all.
 *
 * @param index The index of the vehicle.
 * @param amount The jolt.
 * @ghidraAddress USA: 0x8021be28
 * @ghidraAddress JPN: 0x8021c8d8
 * @ghidraAddress EUR: 0x8021d008
 */
void AddVehicleJolt(s32 index, f32 amount);

/**
 * Read the controls of a vehicle, choose the view of a replay, and step the vehicle.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80226488
 * @ghidraAddress JPN: 0x80227170
 * @ghidraAddress EUR: 0x80227980
 */
void ProcessVehicleInputAndPhysics(Vehicle *vehicle);

/**
 * Search the route ahead of a vehicle for straight lines it could drive, keeping the deepest.
 *
 * @param vehicle The vehicle.
 * @param depth The points searched so far; 0 starts a search.
 * @param pointIndex The point of the section to search from.
 * @param section The section to search, or NULL for the start of the route.
 * @return Whether this search or one it made found a deeper line.
 * @ghidraAddress USA: 0x80226d9c
 * @ghidraAddress JPN: 0x802278f0
 * @ghidraAddress EUR: 0x80228100
 */
s32 TraceTrackSplineRecursive(Vehicle *vehicle, s32 depth, s32 pointIndex, TrackSection *section);

/**
 * Find the point of the route nearest a vehicle along a curve through the points, with the axes
 * of the route there and the offset of the vehicle from it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80227214
 * @ghidraAddress JPN: 0x80227d68
 * @ghidraAddress EUR: 0x80228578
 */
void ComputeTrackPosition(Vehicle *vehicle);

/**
 * Drive a computer's vehicle for a frame: steer for a point ahead, keep to the route, pass the
 * others, back out when stuck, and step it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022762c
 * @ghidraAddress JPN: 0x80228180
 * @ghidraAddress EUR: 0x80228990
 */
void UpdateAiVehicleBehavior(Vehicle *vehicle);

#endif

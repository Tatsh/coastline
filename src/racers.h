#ifndef COASTLINE_RACERS_H
#define COASTLINE_RACERS_H

/**
 * @file
 * The racers of a race: their progress through the gates and laps, their places, and the sharing
 * of the computer's vehicles among them. A racer far from the players has no vehicle and moves
 * along the route by itself.
 */

#include <PR/ultratypes.h>

#include "track.h"
#include "vecmath.h"
#include "vehicle.h"

/** The point the route walks last found. @ghidraAddress USA: 0x8031b750 */
extern Vec3f g_vTrackRoutePoint;

/** The section and point of that point. @ghidraAddress USA: 0x8028b824 */
extern TrackSection *g_pTrackRouteSection;

/** @ghidraAddress USA: 0x8028b828 */
extern s32 g_dwTrackRoutePointIndex;

/** The gates and laps FindTrackRoutePoint() passed. @ghidraAddress USA: 0x8028b82c */
extern s32 g_dwTrackRouteGates;

/** @ghidraAddress USA: 0x8028b830 */
extern s32 g_dwTrackRouteLaps;

/** The places of the race as text, "1st" to "20th". @ghidraAddress USA: 0x8028babc */
extern const char *g_apszRaceProgressStrings[RACER_SLOTS];

/**
 * Find the point a distance along the route from the start of a section, counting the gates and
 * laps on the way, into g_vTrackRoutePoint and the variables beside it.
 *
 * @param section The section to start from.
 * @param distance The distance along the route.
 * @ghidraAddress USA: 0x80228e4c
 */
void FindTrackRoutePoint(TrackSection *section, f32 distance);

/**
 * Move a distance along the route from a point of a section, into g_vTrackRoutePoint and the
 * variables beside it.
 *
 * @param section The section.
 * @param pointIndex The point of the section to start from.
 * @param fraction The share of the segment after the point still ahead.
 * @param distance The distance to move.
 * @ghidraAddress USA: 0x802290c4
 */
void AdvanceAlongTrackRoute(TrackSection *section, s32 pointIndex, f32 fraction, f32 distance);

/**
 * Count the gates and laps a racer crossed this frame, with its lap records and messages, and the
 * time an arcade checkpoint adds.
 *
 * @param slot The racer.
 * @ghidraAddress USA: 0x8022a0e0
 */
void UpdateRaceProgress(RacerSlot *slot);

#endif

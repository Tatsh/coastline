#ifndef COASTLINE_RACERS_H
#define COASTLINE_RACERS_H

/**
 * @file
 * The racers of a race: their progress through the gates and laps, their places, and the sharing
 * of the computer's vehicles among them. A racer far from the players has no vehicle and moves
 * along the route by itself.
 */

#include <PR/ultratypes.h>

#include "common/track.h"
#include "common/vecmath.h"
#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The point the route walks last found.
 *
 * @ghidraAddress USA: 0x8031b750
 * @ghidraAddress EUR: 0x8031dc50
 * @ghidraAddress JPN: 0x8031c880
 * @ghidraAddress BRD3D: 0x10af9b38
 * @ghidraAddress BRGlide: 0x10b1ce98
 */
extern Vec3f g_vTrackRoutePoint;

/**
 * The section and point of that point.
 *
 * @ghidraAddress USA: 0x8028b824
 * @ghidraAddress EUR: 0x8028dad4
 * @ghidraAddress JPN: 0x8028d504
 * @ghidraAddress BRD3D: 0x10af988c
 * @ghidraAddress BRGlide: 0x10b1cbec
 */
extern TrackSection *g_pTrackRouteSection;

/**
 * @ghidraAddress USA: 0x8028b828
 * @ghidraAddress EUR: 0x8028dad8
 * @ghidraAddress JPN: 0x8028d508
 * @ghidraAddress BRD3D: 0x10acd490
 * @ghidraAddress BRGlide: 0x10af07f0
 */
extern s32 g_dwTrackRoutePointIndex;

/**
 * The gates and laps FindTrackRoutePoint() passed.
 *
 * @ghidraAddress USA: 0x8028b82c
 * @ghidraAddress EUR: 0x8028dadc
 * @ghidraAddress JPN: 0x8028d50c
 * @ghidraAddress BRD3D: 0x10af96c0
 * @ghidraAddress BRGlide: 0x10b1ca20
 */
extern s32 g_dwTrackRouteGates;

/**
 * @ghidraAddress USA: 0x8028b830
 * @ghidraAddress EUR: 0x8028dae0
 * @ghidraAddress JPN: 0x8028d510
 * @ghidraAddress BRD3D: 0x10af9b44
 * @ghidraAddress BRGlide: 0x10b1cea4
 */
extern s32 g_dwTrackRouteLaps;

/**
 * The places of the race as text, "1st" to "20th".
 *
 * @ghidraAddress USA: 0x8028babc
 * @ghidraAddress EUR: 0x8028dd70
 * @ghidraAddress JPN: 0x8028d7a0
 */
extern const char *g_apszRaceProgressStrings[RACER_SLOTS];

/**
 * Find the point a distance along the route from the start of a section, counting the gates and
 * laps on the way, into g_vTrackRoutePoint and the variables beside it.
 *
 * @param section The section to start from.
 * @param distance The distance along the route.
 * @ghidraAddress USA: 0x80228e4c
 * @ghidraAddress EUR: 0x8022a1c4
 * @ghidraAddress JPN: 0x802299a0
 * @ghidraAddress BRD3D: 0x10065b20
 * @ghidraAddress BRGlide: 0x1005eb90
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
 * @ghidraAddress EUR: 0x8022a43c
 * @ghidraAddress JPN: 0x80229c18
 * @ghidraAddress BRD3D: 0x10065c80
 * @ghidraAddress BRGlide: 0x1005ecf0
 */
void AdvanceAlongTrackRoute(TrackSection *section, s32 pointIndex, f32 fraction, f32 distance);

/**
 * Count the gates and laps a racer crossed this frame, with its lap records and messages, and the
 * time an arcade checkpoint adds.
 *
 * @param slot The racer.
 * @ghidraAddress USA: 0x8022a0e0
 * @ghidraAddress EUR: 0x8022b458
 * @ghidraAddress JPN: 0x8022ac10
 */
void UpdateRaceProgress(RacerSlot *slot);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACERS_H */

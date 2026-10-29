#include "common/racers.h"
#include "common/types.h"

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x10af96c0 */
s32 g_dwTrackRouteGates;
/** @ghidraAddress BRD3D: 0x10af9b44 */
s32 g_dwTrackRouteLaps;
/** @ghidraAddress BRD3D: 0x10acd490 */
s32 g_dwTrackRoutePointIndex;
/** @ghidraAddress BRD3D: 0x10af988c */
TrackSection *g_pTrackRouteSection;
#endif /* TGR_BOSS_RALLY */

/* Whether a segment crosses a gate, seen from above. The original hands the positions themselves
 * to the flat test, which reads only x and y. */
#define CROSSES_GATE(gate, from, to)                                                               \
    TestLineSegmentIntersection(&(gate)->ends[1], &(gate)->ends[0], (Vec2f *)(from), (Vec2f *)(to))

/* Count the gate after the ones counted if the segment crosses it, and a lap at the first. */
#define COUNT_ROUTE_GATE(next, from, to)                                                           \
    if (g_LoadedTrack.gateCount) {                                                                 \
        next = (g_dwTrackRouteGates + 1) % g_LoadedTrack.gateCount;                                \
        if (CROSSES_GATE(&g_LoadedTrack.gates[next], from, to)) {                                  \
            g_dwTrackRouteGates++;                                                                 \
            if (&g_LoadedTrack.gates[next] == g_LoadedTrack.gates) {                               \
                g_dwTrackRouteLaps++;                                                              \
            }                                                                                      \
        }                                                                                          \
    }

/* Skip the alternate branches from a section on. */
#define SKIP_ALTERNATE_SECTIONS(section)                                                           \
    while ((section) != NULL && ((section)->flags & TRACK_SECTION_ALTERNATE)) {                    \
        section = (section)->next;                                                                 \
    }

void FindTrackRoutePoint(TrackSection *section, f32 distance) {
#if TGR_BOSS_RALLY
    /* The libraries hold the span unrounded through the comparison and the division. */
    long double span;
#else
    f32 span;
#endif
    s32 next;
    s32 i;

    g_dwTrackRouteGates = 0;
    g_dwTrackRouteLaps = 0;
    for (;;) {
        SKIP_ALTERNATE_SECTIONS(section);
        if (section == NULL) {
            return;
        }
        /* The last point of a section reads the point after it. */
        for (i = 0; i < section->pointCount; i++) {
#if TGR_BOSS_RALLY
            span = (long double)section->points[i].distance - section->points[i + 1].distance;
#else
            span = section->points[i].distance - section->points[i + 1].distance;
#endif
            if (span < distance) {
                distance -= span;
                COUNT_ROUTE_GATE(
                    next, &section->points[i].position, &section->points[i + 1].position);
                continue;
            }
            Vec3fLerp(&g_vTrackRoutePoint,
                      &section->points[i + 1].position,
                      &section->points[i].position,
                      distance / span);
            COUNT_ROUTE_GATE(next, &section->points[i].position, &g_vTrackRoutePoint);
            g_pTrackRouteSection = section;
            g_dwTrackRoutePointIndex = i;
            return;
        }
        section = section->child;
    }
}

void AdvanceAlongTrackRoute(TrackSection *section, s32 pointIndex, f32 fraction, f32 distance) {
    f32 length;
    Vec3f debugPoint;

    /* The original reserves a point and tests a constant for a debug check it compiles out. */
    (void)debugPoint;
    for (;;) {
        SKIP_ALTERNATE_SECTIONS(section);
        if (section == NULL) {
            return;
        }
        for (; pointIndex < section->pointCount; pointIndex++) {
            length =
                (section->points[pointIndex].distance - section->points[pointIndex + 1].distance) *
                fraction;
            if (length < distance) {
                if (TRUE) {
                    fraction = 1.0f;
                    distance -= length;
                }
                continue;
            }
            Vec3fLerp(&g_vTrackRoutePoint,
                      &section->points[pointIndex].position,
                      &section->points[pointIndex + 1].position,
                      fraction);
            Vec3fLerp(&g_vTrackRoutePoint,
                      &section->points[pointIndex + 1].position,
                      &g_vTrackRoutePoint,
                      distance / length);
            g_pTrackRouteSection = section;
            g_dwTrackRoutePointIndex = pointIndex;
            return;
        }
        section = section->child;
        pointIndex = 0;
    }
}

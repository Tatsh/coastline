#include "common/game.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/vehicleaccess.h"
#include "common/vehicletrackpos.h"

#define FRAME_POSITION VEHICLE_GET(vehicle, Frame).position

s32 UpdateVehicleTrackPosition(Vehicle *vehicle) {
    f32 x;
    f32 y;
    f32 z;
    u8 cx;
    u8 cy;
    f32 e;
    TrackSection *seg;
    s32 best;
    s32 bestSeg;
    s32 k;
    s32 j;
    s32 n;
    TrackPoint *p;
    f32 bestD;
    f32 d;
    s32 check;
    f32 dist;
    Vec3f dir;
    Vec3f rel;
    Vec3f across;
    Vec3f off;
    f32 step;

    /* Keep these assignments on one line for IDO's register allocation. */
    /* clang-format off */
    x = FRAME_POSITION.x; y = FRAME_POSITION.y; z = FRAME_POSITION.z;
    /* clang-format on */
    check = 1;
    cx = VEHICLE_GET(vehicle, GridX);
    cy = VEHICLE_GET(vehicle, GridY);
    best = -1;
    bestSeg = 0;
    if ((g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8) && cx >= 0x38 && cx < 0x3B &&
        cy >= 0x17 && cy < 0x1C) {
        cx = 0x39;
        cy = 0x19;
        check = 0;
    }
    dist = VEHICLE_GET(vehicle, Distance) -
           g_pRootTrackSection->points[0].distance * VEHICLE_GET(vehicle, DistanceLaps);
    if (check &&
        !SegmentCrossesLine2D(
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].left,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].right,
            &VEHICLE_GET(vehicle, PreviousPosition),
            &VEHICLE_GET(vehicle, Frame).position) &&
        !SegmentCrossesLine2D(
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].left,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].right,
            &VEHICLE_GET(vehicle, PreviousPosition),
            &VEHICLE_GET(vehicle, Frame).position) &&
        Vec3fDistance(
            &VEHICLE_GET(vehicle, Frame).position,
            &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position) <
            64.0f) {
        seg = VEHICLE_GET(vehicle, Section);
        j = VEHICLE_GET(vehicle, PointIndex);
    } else {
        bestD = g_LoadedTrack.maxX - g_LoadedTrack.minX;
        bestD *= bestD;
        for (k = 0; k < g_LoadedTrack.sectionCount; k++) {
            if (VEHICLE_GET(vehicle, Index) < (s32)g_dwNumLocalPlayers || g_dwGameMode == 2 ||
                !(g_LoadedTrack.sections[k]->flags & 1)) {
                seg = g_LoadedTrack.sections[k];
                if (cx < seg->gridMinX || cx > seg->gridMaxX || cy < seg->gridMinY ||
                    cy > seg->gridMaxY) {
                    continue;
                }
                if (check) {
                    if ((g_pRootTrackSection->points[0].distance - seg->points[0].distance) - dist >
                        1000.f) {
                        continue;
                    }
                    if (dist - (g_pRootTrackSection->points[0].distance -
                                seg->points[seg->pointCount].distance) >
                        1000.0f) {
                        continue;
                    }
                }
                n = seg->pointCount;
                for (j = 0, p = seg->points; j < n; j++, p++) {
                    d = p->position.x - x;
                    d *= d;
                    e = p->position.y - y;
                    d += e * e;
                    e = p->position.z - z;
                    d += e * e;
                    if (d < bestD) {
                        dir.x = p->left.y - p->right.y;
                        dir.y = p->right.x - p->left.x;
                        dir.z = 0.0f;
                        Vec3fSubtract(&rel, &VEHICLE_GET(vehicle, Frame).position, &p->position);
                        if (Vec3fDot(&dir, &rel) >= 0.0f) {
                            bestD = d;
                            best = j;
                            bestSeg = k;
                        }
                    }
                }
            }
        }
        if (best == -1) {
            goto none;
        }
        seg = g_LoadedTrack.sections[bestSeg];
        j = best;
    }
    across.x = seg->points[j].left.y - seg->points[j].right.y;
    across.y = seg->points[j].right.x - seg->points[j].left.x;
    across.z = 0.0f;
    Vec3fNormalize(&across);
    Vec3fSubtract(&off, &VEHICLE_GET(vehicle, Frame).position, &seg->points[j].position);
    p = &seg->points[j];
    step = Vec3fDot(&across, &off) +
           (g_pRootTrackSection->points->distance * (VEHICLE_GET(vehicle, DistanceLaps) + 1) -
            p->distance);
    step -= VEHICLE_GET(vehicle, Distance);
    if (!check || (-1000.0f < step && step < 1000.0f)) {
        VEHICLE_GET(vehicle, Distance) += step;
    }
    VEHICLE_GET(vehicle, Section) = seg;
    VEHICLE_GET(vehicle, PointIndex) = j;
    VEHICLE_GET(vehicle, TrackDirection).x = across.x;
    VEHICLE_GET(vehicle, TrackDirection).y = across.y;
    VEHICLE_GET(vehicle, TrackDirection).z = across.z;
    return 1;
none:
    return 0;
}

#include "vehicletrackpos.h"

#include "game.h"
#include "track.h"
#include "trackselect.h"
#include "vehicleaccess.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define TRACK_INTERMEDIATE(value) ((long double)(value))
#define TRACK_LESS(a, b) (!((a) >= (b)))
#define GRID_PATCH_SECOND_TRACK 9
#define TRACK_DISTANCE_SUM(vehicle, change) ((long double)(change) + VEHICLE_GET(vehicle, Distance))
#else
#define TRACK_INTERMEDIATE(value) (value)
#define TRACK_LESS(a, b) ((a) < (b))
#define GRID_PATCH_SECOND_TRACK 8
#define TRACK_DISTANCE_SUM(vehicle, change) (VEHICLE_GET(vehicle, Distance) + (change))
#endif

/* A vehicle that has not crossed out of its point of the route keeps it while this near. */
#define POINT_KEEP_DISTANCE 64.0f

/* The farthest a section may be along the route from a vehicle to be searched, and the farthest
 * the distance may jump in a frame. */
#define ROUTE_DISTANCE_WINDOW 1000.0f

/* The grid cells where the route search bypasses its usual checks. */
#define GRID_PATCH_MIN_X 56
#define GRID_PATCH_END_X 59
#define GRID_PATCH_MIN_Y 23
#define GRID_PATCH_END_Y 28
#define GRID_PATCH_X 57
#define GRID_PATCH_Y 25

/* The direction along the track at a point, across from its left edge to its right. */
static void TrackPointDirection(Vec3f *out, TrackPoint *point) {
    out->x = point->left.y - point->right.y;
    out->y = point->right.x - point->left.x;
    out->z = 0.0f;
}

s32 UpdateVehicleTrackPosition(Vehicle *vehicle) {
    TrackSection *section;
    TrackPoint *point;
    Vec3f direction, offset;
    f32 x = VEHICLE_GET(vehicle, Frame).position.x;
    f32 y = VEHICLE_GET(vehicle, Frame).position.y;
    f32 z = VEHICLE_GET(vehicle, Frame).position.z;
    f32 lapLength, intoLap, best, distance, change;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    f32 dx, dy, dz;
#endif
    s32 checked = TRUE;
    s32 bestPoint = -1;
    s32 bestSection = 0;
    s32 i, p, count;
    u8 gridX = VEHICLE_GET(vehicle, GridX);
    u8 gridY = VEHICLE_GET(vehicle, GridY);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double rawSpan, rawDx, rawDy, rawDz, rawDistance, rawAlong;
    f32 squareXY;
#endif

    if (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == GRID_PATCH_SECOND_TRACK) {
        if (gridX >= GRID_PATCH_MIN_X && gridX < GRID_PATCH_END_X && gridY >= GRID_PATCH_MIN_Y &&
            gridY < GRID_PATCH_END_Y) {
            gridX = GRID_PATCH_X;
            gridY = GRID_PATCH_Y;
            checked = FALSE;
        }
    }
    intoLap = TRACK_INTERMEDIATE(VEHICLE_GET(vehicle, Distance)) -
#if TGR_PLATFORM == TGR_PLATFORM_WIN
              TRACK_INTERMEDIATE(g_LoadedTrack.rootSection->points[0].distance) *
                  VEHICLE_GET(vehicle, DistanceLaps);
#else
              g_LoadedTrack.rootSection->points[0].distance *
                  (f32)VEHICLE_GET(vehicle, DistanceLaps);
#endif

    /* Keep the point while the vehicle has not crossed out of it and stays near it. */
    if (checked) {
        if (!SegmentCrossesLine2D(
                &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].left,
                &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].right,
                &VEHICLE_GET(vehicle, PreviousPosition),
                &VEHICLE_GET(vehicle, Frame).position) &&
            !SegmentCrossesLine2D(
                &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].left,
                &VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex) + 1].right,
                &VEHICLE_GET(vehicle, PreviousPosition),
                &VEHICLE_GET(vehicle, Frame).position) &&
            TRACK_LESS(Vec3fDistance(&VEHICLE_GET(vehicle, Frame).position,
                                     &VEHICLE_GET(vehicle, Section)
                                          ->points[VEHICLE_GET(vehicle, PointIndex)]
                                          .position),
                       POINT_KEEP_DISTANCE)) {
            section = VEHICLE_GET(vehicle, Section);
            p = VEHICLE_GET(vehicle, PointIndex);
            goto found;
        }
    }

    /* The nearest point ahead of the vehicle, of the sections of its cell near it on the route. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    rawSpan = (long double)g_LoadedTrack.maxX - g_LoadedTrack.minX;
    best = rawSpan * rawSpan;
#else
    best = g_LoadedTrack.maxX - g_LoadedTrack.minX;
    best *= best;
#endif
    for (i = 0; i < g_LoadedTrack.sectionCount; i++) {
        if (VEHICLE_GET(vehicle, Index) >= (s32)g_dwNumLocalPlayers &&
            g_dwGameMode != GAME_MODE_TIME_ATTACK &&
            (g_LoadedTrack.sections[i]->flags & TRACK_SECTION_ALTERNATE)) {
            continue;
        }
        section = g_LoadedTrack.sections[i];
        if (gridX < section->gridMinX || section->gridMaxX < gridX || gridY < section->gridMinY ||
            section->gridMaxY < gridY) {
            continue;
        }
        if (checked) {
            lapLength = g_LoadedTrack.rootSection->points[0].distance;
            if (ROUTE_DISTANCE_WINDOW <
                TRACK_INTERMEDIATE(lapLength) - section->points[0].distance - intoLap) {
                continue;
            }
            if (ROUTE_DISTANCE_WINDOW <
                TRACK_INTERMEDIATE(intoLap) - (TRACK_INTERMEDIATE(lapLength) -
                                               section->points[section->pointCount].distance)) {
                continue;
            }
        }
        count = section->pointCount;
        for (p = 0; p < count; p++) {
            point = &section->points[p];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            /* The y/z products use one rounded operand; the x/y sum spills before adding z. */
            rawDx = (long double)point->position.x - x;
            rawDy = (long double)point->position.y - y;
            rawDz = (long double)point->position.z - z;
            squareXY = rawDy * (f32)rawDy + rawDx * rawDx;
            rawDistance = rawDz * (f32)rawDz + squareXY;
            distance = rawDistance;
            if (TRACK_LESS(rawDistance, best)) {
#else
            dx = point->position.x - x;
            dy = point->position.y - y;
            dz = point->position.z - z;
            distance = dx * dx + dy * dy + dz * dz;
            if (distance < best) {
#endif
                TrackPointDirection(&direction, point);
                Vec3fSubtract(&offset, &VEHICLE_GET(vehicle, Frame).position, &point->position);
                if (0.0f <= Vec3fDot(&direction, &offset)) {
                    best = distance;
                    bestPoint = p;
                    bestSection = i;
                }
            }
        }
    }
    if (bestPoint == -1) {
        return 0;
    }
    section = g_LoadedTrack.sections[bestSection];
    p = bestPoint;

found:
    /* The distance moves by how far along the track the vehicle is from the point. */
    point = &section->points[p];
    TrackPointDirection(&direction, point);
    Vec3fNormalize(&direction);
    Vec3fSubtract(&offset, &VEHICLE_GET(vehicle, Frame).position, &point->position);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    rawAlong = Vec3fDot(&direction, &offset);
    change = (((long double)g_LoadedTrack.rootSection->points[0].distance *
                   (s32)((u32)VEHICLE_GET(vehicle, DistanceLaps) + 1) -
               point->distance) +
              rawAlong) -
             VEHICLE_GET(vehicle, Distance);
#else
    change = Vec3fDot(&direction, &offset) + (g_LoadedTrack.rootSection->points[0].distance *
                                                  (f32)(VEHICLE_GET(vehicle, DistanceLaps) + 1) -
                                              point->distance);
    change -= VEHICLE_GET(vehicle, Distance);
#endif
    if (!checked || (-ROUTE_DISTANCE_WINDOW < change && change < ROUTE_DISTANCE_WINDOW)) {
        VEHICLE_SET(vehicle, Distance, TRACK_DISTANCE_SUM(vehicle, change));
    }
    VEHICLE_SET(vehicle, Section, section);
    VEHICLE_SET(vehicle, PointIndex, p);
    VEHICLE_GET(vehicle, TrackDirection).x = direction.x;
    VEHICLE_GET(vehicle, TrackDirection).y = direction.y;
    VEHICLE_GET(vehicle, TrackDirection).z = direction.z;
    return 1;
}

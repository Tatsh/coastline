#include "common/driving.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY
#include <string.h>
#else
#include <PR/os_libc.h>
#endif

#if TGR_BOSS_RALLY
#define TRACE_REAL(value) ((long double)(value))

s32 g_dwBestTraceDepth;
TrackSection *g_pBestTraceSection;
s32 g_dwBestTracePoint;
s32 g_dwBestTraceLength;
s32 g_dwTraceUnusedA;
s32 g_dwTraceUnusedB;
s32 g_bTraceBlocked;
s32 g_bAiNudgeLeft;
s32 g_bAiNudgeRight;
Vec3f g_avBestLeftLane[TRACE_DEPTH];
Vec3f g_avBestCenterLine[TRACE_DEPTH];
Vec3f g_avBestRightLane[TRACE_DEPTH];
Vec3f g_avTraceLeftLane[TRACE_DEPTH];
Vec3f g_avTraceLeftHalf[TRACE_DEPTH];
Vec3f g_avTraceCenterLine[TRACE_DEPTH];
Vec3f g_avTraceRightHalf[TRACE_DEPTH];
Vec3f g_avTraceRightLane[TRACE_DEPTH];
Vec3f g_vTraceFarPoint;
Vec3f g_vTraceHalfwayPoint;
#else
#define TRACE_REAL(value) (value)
#endif /* TGR_BOSS_RALLY */

#define TRACE_LANE_SHARE 0.2f

/* The flat crossing test is handed the positions themselves and reads their x and y. */
s32 TraceTrackSplineRecursive(Vehicle *vehicle, s32 depth, s32 pointIndex, TrackSection *section) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    s32 found;
    s32 nextDepth;
    Vec3f middle;
    s32 level;
#if defined(__sgi)
    s32 weather, cars;
#endif
    Vec3f *left, *right;
#if defined(__sgi)
    /* IDO reserves eight stack bytes for these otherwise unused pointers. */
    Vec3f *unusedLeft, *unusedRight;
#endif

    found = FALSE;
    if (section == NULL) {
        section = g_LoadedTrack.rootSection;
        pointIndex = 0;
    }
    if (depth == 0) {
        g_dwBestTraceDepth = 0;
        g_bTraceBlocked = g_dwTraceUnusedB = g_dwTraceUnusedA = 0;
    } else {
        if (depth > TRACE_DEPTH || (section->flags & TRACK_SECTION_ALTERNATE)) {
            return FALSE;
        }
        Vec3fLerp(&g_avTraceRightLane[depth - 1],
                  &section->points[pointIndex].left,
                  &section->points[pointIndex].right,
                  TRACE_LANE_SHARE);
        g_avTraceCenterLine[depth - 1].x = section->points[pointIndex].position.x;
        g_avTraceCenterLine[depth - 1].y = section->points[pointIndex].position.y;
        g_avTraceCenterLine[depth - 1].z = section->points[pointIndex].position.z;
        Vec3fLerp(&g_avTraceLeftLane[depth - 1],
                  &section->points[pointIndex].right,
                  &section->points[pointIndex].left,
                  TRACE_LANE_SHARE);
        Vec3fMidpoint(&g_avTraceLeftHalf[depth - 1],
                      &g_avTraceLeftLane[depth - 1],
                      &g_avTraceCenterLine[depth - 1]);
        Vec3fMidpoint(&g_avTraceRightHalf[depth - 1],
                      &g_avTraceRightLane[depth - 1],
                      &g_avTraceCenterLine[depth - 1]);
        do {
            middle.x =
                (section->points[pointIndex].left.x + section->points[pointIndex].right.x) * 0.5f;
            middle.y =
                (section->points[pointIndex].left.y + section->points[pointIndex].right.y) * 0.5f;
        } while (0);
        left = &g_avTraceLeftLane[1];
        right = &g_avTraceRightLane[1];
        for (level = 1; level < depth - 1; level++, left++, right++) {
            if (!TestLineSegmentIntersection((Vec2f *)&VEHICLE_GET(vehicle, Frame).position,
                                             (Vec2f *)&middle,
                                             (Vec2f *)left,
                                             (Vec2f *)right)) {
                g_bTraceBlocked = TRUE;
                goto next;
            }
        }
        if (depth > g_dwBestTraceDepth) {
            g_dwBestTraceDepth = depth;
            g_pBestTraceSection = section;
            g_dwBestTracePoint = pointIndex;
#if defined(__sgi)
            weather = g_dwCurrentWeather;
            cars = g_dwNumActiveVehicles;
#endif
            if (depth > 2) {
                if (TestLineSegmentIntersection((Vec2f *)&VEHICLE_GET(vehicle, Frame).position,
                                                (Vec2f *)&g_avTraceCenterLine[2],
                                                (Vec2f *)&g_avTraceLeftHalf[1],
                                                (Vec2f *)&g_avTraceLeftLane[1])) {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = TRUE;
                } else if (TestLineSegmentIntersection(
                               (Vec2f *)&VEHICLE_GET(vehicle, Frame).position,
                               (Vec2f *)&g_avTraceCenterLine[2],
                               (Vec2f *)&g_avTraceRightHalf[1],
                               (Vec2f *)&g_avTraceRightLane[1])) {
                    g_bAiNudgeLeft = TRUE;
                    g_bAiNudgeRight = FALSE;
                } else {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = FALSE;
                }
            } else {
#if defined(__sgi)
                /* IDO eliminates this check after assigning register lifetimes. */
                if (weather + cars + pointIndex + g_avTraceLeftHalf[0].x +
                        g_avTraceRightHalf[0].x !=
                    0.0f) {
                }
#endif
                g_bAiNudgeLeft = FALSE;
                g_bAiNudgeRight = FALSE;
            }
            g_vTraceFarPoint.x = g_avTraceCenterLine[depth - 1].x;
            g_vTraceFarPoint.y = g_avTraceCenterLine[depth - 1].y;
            g_vTraceFarPoint.z = g_avTraceCenterLine[depth - 1].z;
            g_vTraceHalfwayPoint.x = g_avTraceCenterLine[depth >> 1].x;
            g_vTraceHalfwayPoint.y = g_avTraceCenterLine[depth >> 1].y;
            g_vTraceHalfwayPoint.z = g_avTraceCenterLine[depth >> 1].z;
            memcpy(g_avBestLeftLane, g_avTraceLeftLane, depth * sizeof(Vec3f));
            memcpy(g_avBestCenterLine, g_avTraceCenterLine, depth * sizeof(Vec3f));
            memcpy(g_avBestRightLane, g_avTraceRightLane, depth * sizeof(Vec3f));
            found = TRUE;
            g_dwBestTraceLength = depth;
        }
    }

next:
    if (++pointIndex == section->pointCount) {
        section = section->child;
        if (section == NULL) {
            section = g_LoadedTrack.rootSection;
        }
        if (section != NULL) {
            nextDepth = depth + 1;
            do {
                found |= TraceTrackSplineRecursive(vehicle, nextDepth, 0, section);
                section = section->next;
            } while (section != NULL);
        }
    } else {
        found |= TraceTrackSplineRecursive(vehicle, depth + 1, pointIndex, section);
    }
    return found;
#else
    TrackPoint *point;
    TrackSection *child;
    Vec2f middle;
    s32 found = FALSE;
    s32 last;
    s32 i;

    if (section == NULL) {
        section = g_LoadedTrack.rootSection;
        pointIndex = 0;
    }
    if (depth == 0) {
        g_dwBestTraceDepth = 0;
#if TGR_BOSS_RALLY
        g_dwTraceUnusedA = 0;
        g_dwTraceUnusedB = 0;
#else
        g_dwTraceUnusedB = 0;
        g_dwTraceUnusedA = 0;
#endif
        g_bTraceBlocked = FALSE;
    } else {
        if (depth > TRACE_DEPTH || (section->flags & TRACK_SECTION_ALTERNATE)) {
            return FALSE;
        }
        last = depth - 1;
        point = &section->points[pointIndex];
        Vec3fLerp(&g_avTraceRightLane[last], &point->left, &point->right, TRACE_LANE_SHARE);
        g_avTraceCenterLine[last].x = point->position.x;
        g_avTraceCenterLine[last].y = point->position.y;
        g_avTraceCenterLine[last].z = point->position.z;
        Vec3fLerp(&g_avTraceLeftLane[last], &point->right, &point->left, TRACE_LANE_SHARE);
        Vec3fMidpoint(
            &g_avTraceLeftHalf[last], &g_avTraceLeftLane[last], &g_avTraceCenterLine[last]);
        Vec3fMidpoint(
            &g_avTraceRightHalf[last], &g_avTraceRightLane[last], &g_avTraceCenterLine[last]);
        middle.x = (TRACE_REAL(point->right.x) + point->left.x) * 0.5f;
        middle.y = (TRACE_REAL(point->right.y) + point->left.y) * 0.5f;
        for (i = 1; i < last; i++) {
            if (!TestLineSegmentIntersection((Vec2f *)&VEHICLE_GET(vehicle, Frame).position,
                                             &middle,
                                             (Vec2f *)&g_avTraceLeftLane[i],
                                             (Vec2f *)&g_avTraceRightLane[i])) {
                g_bTraceBlocked = TRUE;
                goto next;
            }
        }
        if (g_dwBestTraceDepth < depth) {
            /* Store the line as the deepest so far, and nudge the computer away from the side the
             * line passes close to. */
            g_dwBestTraceDepth = depth;
            g_pBestTraceSection = section;
            g_dwBestTracePoint = pointIndex;
            if (depth >= 3) {
                if (TestLineSegmentIntersection((Vec2f *)&VEHICLE_GET(vehicle, Frame).position,
                                                (Vec2f *)&g_avTraceCenterLine[2],
                                                (Vec2f *)&g_avTraceLeftHalf[1],
                                                (Vec2f *)&g_avTraceLeftLane[1])) {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = TRUE;
                } else if (TestLineSegmentIntersection(
                               (Vec2f *)&VEHICLE_GET(vehicle, Frame).position,
                               (Vec2f *)&g_avTraceCenterLine[2],
                               (Vec2f *)&g_avTraceRightHalf[1],
                               (Vec2f *)&g_avTraceRightLane[1])) {
                    g_bAiNudgeLeft = TRUE;
                    g_bAiNudgeRight = FALSE;
                } else {
                    g_bAiNudgeLeft = FALSE;
                    g_bAiNudgeRight = FALSE;
                }
            } else {
                g_bAiNudgeLeft = FALSE;
                g_bAiNudgeRight = FALSE;
            }
            g_vTraceHalfwayPoint.x = g_avTraceCenterLine[depth >> 1].x;
            g_vTraceHalfwayPoint.y = g_avTraceCenterLine[depth >> 1].y;
            g_vTraceHalfwayPoint.z = g_avTraceCenterLine[depth >> 1].z;
            g_vTraceFarPoint.x = g_avTraceCenterLine[depth - 1].x;
            g_vTraceFarPoint.y = g_avTraceCenterLine[depth - 1].y;
            g_vTraceFarPoint.z = g_avTraceCenterLine[depth - 1].z;
            memcpy(g_avBestLeftLane, g_avTraceLeftLane, depth * sizeof(Vec3f));
            memcpy(g_avBestCenterLine, g_avTraceCenterLine, depth * sizeof(Vec3f));
            memcpy(g_avBestRightLane, g_avTraceRightLane, depth * sizeof(Vec3f));
            g_dwBestTraceLength = depth;
            found = TRUE;
        }
    }

next:
    pointIndex++;
    if (pointIndex == section->pointCount) {
        child = section->child;
        if (child == NULL) {
            child = g_LoadedTrack.rootSection;
        }
        for (; child != NULL; child = child->next) {
            found |= TraceTrackSplineRecursive(vehicle, depth + 1, 0, child);
        }
    } else {
        found |= TraceTrackSplineRecursive(vehicle, depth + 1, pointIndex, section);
    }
    return found;
#endif
}

#include "camera.h"
#include "tricache.h"
#include "vehicleaccess.h"

#define TRACK_CLEARANCE 0.1f
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define TRACK_CAMERA_CLEARANCE 0.3f
#define STORE_TRACK_VALUE(target, expression) (target) = (f32)(precise = (expression))
#define PRECISE_TRACK_VALUE(value) precise
/* The x87 tests take an unordered value as less, and as equal to zero. */
#define TRACK_FACING_AWAY(value) ((value) >= 0.0f)
#define TRACK_LESS(a, b) (!((a) >= (b)))
#define TRACK_NONZERO(value) ((value) < 0.0f || (value) > 0.0f)
#else
#define TRACK_CAMERA_CLEARANCE TRACK_CLEARANCE
#define STORE_TRACK_VALUE(target, expression) (target) = (expression)
#define PRECISE_TRACK_VALUE(value) (value)
#define TRACK_FACING_AWAY(value) (!((value) < 0.0f))
#define TRACK_LESS(a, b) ((a) < (b))
#define TRACK_NONZERO(value) ((value) != 0.0f)
#endif

/*
 * Find the nearest triangle of the cached slots that the line from start along direction crosses
 * between 0 and limit, and the point where it does, in hit and hitPoint.
 */
#define FIND_TRACK_HIT(start)                                                                      \
    {                                                                                              \
        hit = NULL;                                                                                \
        for (i = 0; i < slotCount; i++) {                                                          \
            triangle = g_aTriCacheTriangles[slots[i]];                                             \
            end = triangle + g_awTriCacheTriCount[slots[i]];                                       \
            for (; triangle != end; triangle++) {                                                  \
                STORE_TRACK_VALUE(facing, Vec3fDot(&direction, &triangle->normal));                \
                if (TRACK_FACING_AWAY(PRECISE_TRACK_VALUE(facing))) {                              \
                    continue;                                                                      \
                }                                                                                  \
                Vec3fSubtract(&toVertex, triangle->vertices[0], start);                            \
                STORE_TRACK_VALUE(t, Vec3fDot(&toVertex, &triangle->normal) / facing);             \
                if (!(0.0f < PRECISE_TRACK_VALUE(t)) || !(t < limit)) {                            \
                    continue;                                                                      \
                }                                                                                  \
                Vec3fMulAdd(&point, start, &direction, t);                                         \
                if (PointInTriangle3D(&point,                                                      \
                                      triangle->vertices[0],                                       \
                                      triangle->vertices[1],                                       \
                                      triangle->vertices[2],                                       \
                                      &triangle->normal)) {                                        \
                    limit = t;                                                                     \
                    hit = triangle;                                                                \
                    hitPoint = point;                                                              \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    }

void ResolveVehicleTrackCollision(Vehicle *vehicle, CameraFrame *camera, Vec3f *target) {
    Vec3f direction, point, toVertex;
    s32 i;
    s32 slots[2];
    s32 slotCount;
    Vec3f hitPoint, offset;
    TrackTriangle *hit, *triangle, *end;
    f32 limit, distance, newLength, facing, t;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double length, precise;
#else
    f32 length;
#endif

    g_bVehicleTrackCollisionCorrected = FALSE;
    slots[0] =
        BuildTrackTriangleCache(VEHICLE_GET(vehicle, LookAt).x, VEHICLE_GET(vehicle, LookAt).y);
    slots[1] = BuildTrackTriangleCache(camera->position.x, camera->position.y);
    slotCount = slots[0] == slots[1] ? 1 : 2;

    /* The line from the look-at point to just beyond the camera. */
    Vec3fSubtract(&direction, &camera->position, &VEHICLE_GET(vehicle, LookAt));
    length = Vec3fLength(&direction);
    limit = TRACK_NONZERO(length) ? (length + TRACK_CLEARANCE) / length : 1.0f;
    FIND_TRACK_HIT(&VEHICLE_GET(vehicle, LookAt));
    if (hit == NULL) {
        return;
    }

    /* The line from the target to the camera. */
    Vec3fSubtract(&direction, &camera->position, target);
    limit = 1.0f;
    FIND_TRACK_HIT(target);
    if (hit == NULL) {
        return;
    }
    distance = Vec3fDistance(&camera->position, &VEHICLE_GET(vehicle, LookAt));
    Vec3fMulAdd(&camera->position, &hitPoint, &hit->normal, TRACK_CAMERA_CLEARANCE);
    Vec3fSubtract(&offset, &camera->position, &VEHICLE_GET(vehicle, LookAt));
    newLength = Vec3fLength(&offset);
    if (TRACK_LESS(distance, newLength) && TRACK_NONZERO(newLength)) {
        Vec3fScaleInPlace(&offset, distance / newLength);
        Vec3fAdd(&camera->position, &VEHICLE_GET(vehicle, LookAt), &offset);
    }
    g_bVehicleTrackCollisionCorrected = TRUE;
}

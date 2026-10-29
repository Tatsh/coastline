#include "win32/terrain.h"

#include "common/physics.h"
#include "common/tricache.h"

float RaycastTerrainHeight(Vec3f *position) {
    float nearest = 100.0f;
    s16 slot = BuildTrackTriangleCache(position->x, position->y);
    TrackTriangle *triangle = g_aTriCacheTriangles[slot];
    for (unsigned count = g_awTriCacheTriCount[slot]; count != 0; --count, ++triangle) {
        long double distance = PlaneDistance(&triangle->normal, triangle->planeOffset, position);
        if (!(distance > -2.0) || !(distance < 2.0)) {
            continue;
        }
        long double facing = (long double)triangle->normal.z * -1.0f;
        float roundedFacing = (float)facing;
        if (!((facing < 0.0f ? -facing : facing) > 0.001)) {
            continue;
        }
        long double plane = ((long double)triangle->normal.x * position->x +
                             (long double)triangle->normal.z * position->z) +
                            (long double)triangle->normal.y * position->y + triangle->planeOffset;
        distance = -(plane / roundedFacing);
        float roundedDistance = (float)distance;
        Vec3f hit;
        hit.x = 0.0f + position->x;
        hit.y = position->y + 0.0f;
        // The intersection uses the quotient before its float store.
        hit.z = (float)(distance * -1.0f + position->z);
        if (!(roundedDistance > -2.0) || !(roundedDistance < 2.0) || !(roundedDistance < nearest) ||
            !(triangle->normal.z > 0.2)) {
            continue;
        }
        if (IsPointInTrackTriangle(triangle, &hit)) {
            nearest = roundedDistance;
        }
    }
    return nearest;
}

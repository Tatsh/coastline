#include "common/physics.h"
#include "common/track.h"
#include "common/trianglebox.h"
#include "common/tricache.h"

s32 CastRayAgainstTrack(Vec3f *outPoint,
                        Vec3f *outNormal,
                        Vec3f *originPoint,
                        u16 *groundInstances,
                        s32 *groundInstanceCount,
                        u16 *nearbyInstances,
                        s32 *nearbyCount,
                        f32 *groundDistance,
                        s32 *groundSurface) {
    s32 groundHits;
    s32 groundCount;
    s32 nearby;
    f32 range;
    Vec3f direction;
    f32 ceilingZ;
    f32 groundZ;
    f32 nearestGround;
    f32 nearestCeiling;
    Vec3f toVertex;
    f32 t;
    f32 denominator;
    /* These unused slots preserve the original IDO frame layout. */
    s32 unusedWordA TGR_MATCHING_UNUSED;
    Vec3f hit;
    Vec3f origin;
    f32 distance;
    s32 unusedWordB TGR_MATCHING_UNUSED;
    u16 ceilingInstance;
    u16 ceilingTriangle;
    TrackTriangle *triangle;
    s32 unusedWordC TGR_MATCHING_UNUSED;
    s32 neighborIndex;
    s32 slot;
    f32 x;
    f32 y;
    Vec3f ceilingNormal;
    Vec3f groundNormal;
    s32 unusedWordsA[2] TGR_MATCHING_UNUSED;
    TrackTriangle *end;
    s32 unusedWordsB[5] TGR_MATCHING_UNUSED;

    ceilingNormal = g_vRayCeilingNormalInit;
    groundNormal = g_vRayGroundNormalInit;
    ceilingZ = originPoint->z;
    groundZ = originPoint->z;
    range = g_LoadedTrack.maxZ - g_LoadedTrack.minZ;
    nearestGround = nearestCeiling = range * range + 1.0f;
    ceilingInstance = 0;
    ceilingTriangle = 0;
    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 1.f;
    /* clang-format off */
    x = originPoint->x; y = originPoint->y;
    /* clang-format on */
    origin.y = y;
    origin.x = x;
    origin.z = 1.0;
    groundHits = 0;
    *groundSurface = 0;
    groundCount = 0;
    nearby = 0;
    slot = BuildTrackTriangleCache(origin.x, origin.y);
    triangle = g_aTriCacheTriangles[slot];
    end = triangle + g_awTriCacheTriCount[slot];
    for (; triangle != end; triangle++) {
        if (triangle->normal.z < 0.0f) {
            continue;
        }
        denominator = Vec3fDot(&direction, &triangle->normal);
        if (denominator == 0) {
            continue;
        }
        Vec3fSubtract(&toVertex, triangle->vertices[0], &origin);
        t = Vec3fDot(&toVertex, &triangle->normal) / denominator;
        Vec3fMulAdd(&hit, &origin, &direction, t);
        if (!PointInTriangle3D(&hit,
                               triangle->vertices[0],
                               triangle->vertices[1],
                               triangle->vertices[2],
                               &triangle->normal)) {
            continue;
        }
        distance = originPoint->z + 1.5f - hit.z;
        if (distance >= 0.0f) {
            groundHits++;
            if (distance < nearestGround) {
                nearestGround = distance;
                *groundSurface = triangle->index;
                groundZ = hit.z;
                if (triangle->normal.z < 0.0f) {
                    Vec3fNegate(&groundNormal, &triangle->normal);
                } else {
                    groundNormal.x = triangle->normal.x;
                    groundNormal.y = triangle->normal.y;
                    groundNormal.z = triangle->normal.z;
                }
                if (groundCount < 32) {
                    groundInstances[groundCount++] = groundInstances[0];
                }
                groundInstances[0] = g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (distance < 5.0f) {
                    neighborIndex = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                    if (g_LoadedTrack.nearbyInstances[neighborIndex] != 0) {
                        do {
                            if (nearby < 32) {
                                nearbyInstances[nearby++] = nearbyInstances[0];
                            }
                            nearbyInstances[0] = g_LoadedTrack.nearbyInstances[neighborIndex];
                            neighborIndex++;
                        } while (g_LoadedTrack.nearbyInstances[neighborIndex] != 0);
                    }
                }
            } else if (groundCount < 32) {
                groundInstances[groundCount++] =
                    g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (distance < 5.0f) {
                    neighborIndex = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                    if (g_LoadedTrack.nearbyInstances[neighborIndex] != 0) {
                        do {
                            if (nearby < 32) {
                                nearbyInstances[nearby++] =
                                    g_LoadedTrack.nearbyInstances[neighborIndex];
                            }
                            neighborIndex++;
                        } while (g_LoadedTrack.nearbyInstances[neighborIndex] != 0);
                    }
                }
            }
        }
        distance -= 1.5f;
        if (!(distance <= 0.0f && distance < nearestCeiling)) {
            continue;
        }
        ceilingInstance = g_LoadedTrack.surfaces[triangle->index].instance + 1;
        if (-1.0f < distance) {
            nearestCeiling = distance;
            ceilingTriangle = triangle->index;
            ceilingZ = hit.z;
            if (triangle->normal.z < 0.0f) {
                Vec3fNegate(&ceilingNormal, &triangle->normal);
            } else {
                ceilingNormal.x = triangle->normal.x;
                ceilingNormal.y = triangle->normal.y;
                ceilingNormal.z = triangle->normal.z;
            }
        }
    }
    if (groundHits != 0) {
        if (outPoint != 0) {
            outPoint->z = groundZ;
            *groundDistance = nearestGround - 1.5f;
        }
        if (outNormal != 0) {
            outNormal->x = groundNormal.x;
            outNormal->y = groundNormal.y;
            outNormal->z = groundNormal.z;
        }
    } else {
        neighborIndex = g_LoadedTrack.triangleNearbyInstances[ceilingTriangle];
        while (g_LoadedTrack.nearbyInstances[neighborIndex] != 0) {
            if (nearby < 32) {
                nearbyInstances[nearby++] = g_LoadedTrack.nearbyInstances[neighborIndex];
            } else {
                nearbyInstances[31] = g_LoadedTrack.nearbyInstances[neighborIndex];
                break;
            }
            neighborIndex++;
        }
        if (groundCount < 32) {
            groundInstances[groundCount] = ceilingInstance;
        } else {
            groundInstances[31] = ceilingInstance;
        }
        if (outPoint != 0) {
            outPoint->z = ceilingZ;
        }
        if (outNormal != 0) {
            outNormal->x = ceilingNormal.x;
            outNormal->y = ceilingNormal.y;
            outNormal->z = ceilingNormal.z;
        }
    }
    *groundInstanceCount = 1;
    *nearbyCount = nearby;
    return groundHits;
}

#include "tricache.h"

#include "physics.h"
#include "rigidbody.h"
#include "track.h"
#include "trianglebox.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The count of the uses of the cache.
 *
 * @ghidraAddress USA: 0x8037eaa0
 * @ghidraAddress EUR: 0x80380fa0
 * @ghidraAddress JPN: 0x8037fc10
 * @ghidraAddress BRD3D: 0x117554e0
 * @ghidraAddress BRGlide: 0x11778840
 */
static u32 g_dwTriCacheLruCounter;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#define RAY_HEADROOM 1.5f
#define RAY_NEARBY_DISTANCE 5.0f
#define RAY_CEILING_DEPTH -1.0f

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RAY_LESS(a, b) (!((a) >= (b)))
#else
#define RAY_LESS(a, b) ((a) < (b))
#endif

s32 CastRayAgainstTrack(Vec3f *outPoint,
                        Vec3f *outNormal,
                        Vec3f *origin,
                        u16 *groundInstances,
                        s32 *groundInstanceCount,
                        u16 *nearbyInstances,
                        s32 *nearbyCount,
                        f32 *groundDistance,
                        s32 *groundSurface) {
    TrackTriangle *triangle, *end;
    Vec3f ceilingNormal = {0.0f, 0.0f, 1.0f};
    Vec3f groundNormal = {0.0f, 0.0f, 1.0f};
    Vec3f line, direction, toVertex, hit;
    f32 nearest, nearestCeiling, groundZ, ceilingZ, denominator, below;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highRange, highBelow, highDenominator;
#endif
    s32 slot, groundHits = 0, groundCount = 0, nearby = 0, j;
    s16 ceilingTriangle = 0;
    u16 ceilingInstance = 0, instance;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    highRange = (long double)g_LoadedTrack.maxZ - g_LoadedTrack.minZ;
    nearest = highRange;
    nearest = highRange * nearest + 1.0f;
#else
    nearest = g_LoadedTrack.maxZ - g_LoadedTrack.minZ;
    nearest = nearest * nearest + 1.0f;
#endif
    nearestCeiling = nearest;
    groundZ = origin->z;
    ceilingZ = origin->z;
    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 1.0f;
    line.x = origin->x;
    line.y = origin->y;
    line.z = 1.0f;
    *groundSurface = 0;

    slot = BuildTrackTriangleCache(line.x, line.y);
    triangle = g_aTriCacheTriangles[slot];
    end = triangle + g_awTriCacheTriCount[slot];
    for (; triangle != end; triangle++) {
        /* The vertical line meets the plane of each triangle facing up. */
        if (RAY_LESS(triangle->normal.z, 0.0f)) {
            continue;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        highDenominator = Vec3fDot(&direction, &triangle->normal);
        denominator = highDenominator;
        if (!(highDenominator < 0.0L) && !(highDenominator > 0.0L)) {
#else
        denominator = Vec3fDot(&direction, &triangle->normal);
        if (denominator == 0.0f) {
#endif
            continue;
        }
        Vec3fSubtract(&toVertex, triangle->vertices[0], &line);
        Vec3fMulAdd(&hit, &line, &direction, Vec3fDot(&toVertex, &triangle->normal) / denominator);
        if (!PointInTriangle3D(&hit,
                               triangle->vertices[0],
                               triangle->vertices[1],
                               triangle->vertices[2],
                               &triangle->normal)) {
            continue;
        }

        /* Ground triangles are inserted with the nearest first. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        highBelow = (long double)origin->z + RAY_HEADROOM - hit.z;
        below = highBelow;
        if (0.0L <= highBelow) {
#else
        below = origin->z + RAY_HEADROOM - hit.z;
        if (0.0f <= below) {
#endif
            groundHits++;
            if (RAY_LESS(below, nearest)) {
                *groundSurface = triangle->index;
                nearest = below;
                groundZ = hit.z;
                if (RAY_LESS(triangle->normal.z, 0.0f)) {
                    Vec3fNegate(&groundNormal, &triangle->normal);
                } else {
                    groundNormal.x = triangle->normal.x;
                    groundNormal.y = triangle->normal.y;
                    groundNormal.z = triangle->normal.z;
                }
                if (groundCount < RAY_INSTANCES) {
                    groundInstances[groundCount++] = groundInstances[0];
                }
                groundInstances[0] = g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (RAY_LESS(below, RAY_NEARBY_DISTANCE)) {
                    for (j = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                         g_LoadedTrack.nearbyInstances[j] != 0;
                         j++) {
                        if (nearby < RAY_INSTANCES) {
                            nearbyInstances[nearby++] = nearbyInstances[0];
                        }
                        nearbyInstances[0] = g_LoadedTrack.nearbyInstances[j];
                    }
                }
            } else if (groundCount < RAY_INSTANCES) {
                groundInstances[groundCount++] =
                    g_LoadedTrack.surfaces[triangle->index].instance + 1;
                if (RAY_LESS(below, RAY_NEARBY_DISTANCE)) {
                    j = g_LoadedTrack.triangleNearbyInstances[triangle->index];
                    for (instance = g_LoadedTrack.nearbyInstances[j]; instance != 0;
                         instance = g_LoadedTrack.nearbyInstances[++j]) {
                        if (nearby < RAY_INSTANCES) {
                            nearbyInstances[nearby++] = instance;
                        }
                    }
                }
            }
        }

        /* Ceiling selection follows the original signed distance order. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        highBelow = (long double)below - RAY_HEADROOM;
        below = highBelow;
        if (!(highBelow > 0.0L) && RAY_LESS(below, nearestCeiling)) {
#else
        below -= RAY_HEADROOM;
        if (below <= 0.0f && below < nearestCeiling) {
#endif
            ceilingInstance = g_LoadedTrack.surfaces[triangle->index].instance + 1;
            if (RAY_CEILING_DEPTH < below) {
                ceilingTriangle = triangle->index;
                nearestCeiling = below;
                ceilingZ = hit.z;
                if (RAY_LESS(triangle->normal.z, 0.0f)) {
                    Vec3fNegate(&ceilingNormal, &triangle->normal);
                } else {
                    ceilingNormal.x = triangle->normal.x;
                    ceilingNormal.y = triangle->normal.y;
                    ceilingNormal.z = triangle->normal.z;
                }
            }
        }
    }

    if (groundHits != 0) {
        if (outPoint != NULL) {
            outPoint->z = groundZ;
            *groundDistance = nearest - RAY_HEADROOM;
        }
        if (outNormal != NULL) {
            outNormal->x = groundNormal.x;
            outNormal->y = groundNormal.y;
            outNormal->z = groundNormal.z;
        }
    } else {
        /* Without ground, the ceiling stands in for it. */
        j = g_LoadedTrack.triangleNearbyInstances[(u16)ceilingTriangle];
        for (instance = g_LoadedTrack.nearbyInstances[j]; instance != 0;
             instance = g_LoadedTrack.nearbyInstances[++j]) {
            if (nearby >= RAY_INSTANCES) {
                nearbyInstances[RAY_INSTANCES - 1] = instance;
                break;
            }
            nearbyInstances[nearby++] = instance;
        }
        if (groundCount < RAY_INSTANCES) {
            groundInstances[groundCount] = ceilingInstance;
        } else {
            groundInstances[RAY_INSTANCES - 1] = ceilingInstance;
        }
        if (outPoint != NULL) {
            outPoint->z = ceilingZ;
        }
        if (outNormal != NULL) {
            outNormal->x = ceilingNormal.x;
            outNormal->y = ceilingNormal.y;
            outNormal->z = ceilingNormal.z;
        }
    }
    *groundInstanceCount = 1;
    *nearbyCount = nearby;
    return groundHits;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
s32 g_bTrackCollisionFrameToggle;
#endif

s32 CollectTrianglesInBox(RigidBody *body, f32 box[4][4]) {
    TrackTriangle *triangles;
    Vec3f corners[3], normal;
    s32 slot, count, i;
    s32 found = 0;

    slot = BuildTrackTriangleCache(body->orientation[3][0], body->orientation[3][1]);
    count = g_awTriCacheTriCount[slot];
    triangles = g_aTriCacheTriangles[slot];
    if (g_bTrackCollisionFrameToggle) {
        for (i = count - 1; i >= 0; i--) {
            TriangleInBox(corners, &normal, box, &triangles[i]);
            if (IsTriangleInUnitCube(corners, &normal)) {
                PushCollisionTriangle(&triangles[i]);
                found++;
            }
        }
    } else {
        for (i = 0; i < count; i++) {
            TriangleInBox(corners, &normal, box, &triangles[i]);
            if (IsTriangleInUnitCube(corners, &normal)) {
                PushCollisionTriangle(&triangles[i]);
                found++;
            }
        }
    }
    return found;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* The original converts through 64 bits, so NaN and the infinities give the cell of 0. */
#define GRID_COORDINATE(v) ((s32)(s64)(v))
#else
#define GRID_COORDINATE(v) ((s32)(v))
#endif
#define GRID_SECTOR(x, y) (GRID_COORDINATE(y) / 32 * 64 + GRID_COORDINATE(x) / 32)

#define ABS(x) ((x) < 0.0f ? -(x) : (x))
#define COMPONENT(v, axis) ((&(v)->x)[axis])
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define TRI_PRODUCT(a, b) ((long double)(a) * (b))
#else
#define TRI_PRODUCT(a, b) ((a) * (b))
#endif

s16 IsPointInTrackTriangle(TrackTriangle *triangle, Vec3f *point) {
    f32 u0, v0, pu, pv, au, av, bu, bv, beta;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double alpha, quotient;
#else
    f32 alpha, quotient;
#endif
    s32 axis, i1, i2;
    s32 inside = 0;

    /* Drop the main axis of the normal, and solve for the triangle there. */
    if (ABS(triangle->normal.y) < ABS(triangle->normal.x)) {
        axis = ABS(triangle->normal.z) < ABS(triangle->normal.x) ? 0 : 2;
    } else {
        axis = ABS(triangle->normal.z) < ABS(triangle->normal.y) ? 1 : 2;
    }
    i1 = (axis + 1) % 3;
    i2 = (axis + 2) % 3;
    u0 = COMPONENT(triangle->vertices[0], i1);
    v0 = COMPONENT(triangle->vertices[0], i2);
    pu = COMPONENT(point, i1) - u0;
    pv = COMPONENT(point, i2) - v0;
    au = COMPONENT(triangle->vertices[1], i1) - u0;
    av = COMPONENT(triangle->vertices[1], i2) - v0;
    bu = COMPONENT(triangle->vertices[2], i1) - u0;
    bv = COMPONENT(triangle->vertices[2], i2) - v0;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* The libraries test equality through the flag an unordered comparison also sets. */
    if (!(au < 0.0f || au > 0.0f)) {
#else
    if (au == 0.0f) {
#endif
        beta = quotient = TRI_PRODUCT(pu, 1.0f) / bu;
        if (0.0f <= quotient && beta <= 1.0f) {
            alpha = (pv - TRI_PRODUCT(beta, bv)) / av;
            inside = 0.0f <= alpha && alpha + beta <= 1.0f;
        }
    } else {
        beta = quotient = (TRI_PRODUCT(pv, au) - TRI_PRODUCT(pu, av)) /
                          (TRI_PRODUCT(bv, au) - TRI_PRODUCT(bu, av));
        if (0.0f <= quotient && beta <= 1.0f) {
            alpha = (pu - TRI_PRODUCT(beta, bu)) / au;
            inside = 0.0f <= alpha && alpha + beta <= 1.0f;
        }
    }
    return inside;
}

/* The original slot remains unset when every timestamp is at least 2^30. */
#pragma GCC diagnostic push
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#pragma GCC diagnostic ignored "-Wuninitialized"
s16 BuildTrackTriangleCache(f32 x, f32 y) {
    TrackTriangle *triangle;
    TrackGridCursor cursor;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double e1[3], e2[3];
#else
    f32 e1[3], e2[3];
#endif
    u32 oldest, cell;
    s16 sector;
    s32 slot, count;
    s32 best;
    u16 index;

    g_dwTriCacheLruCounter++;
    sector = GRID_SECTOR(x, y);
    oldest = 0x40000000;
    for (slot = 0; slot < TRI_CACHE_SLOTS; slot++) {
        if (sector == g_awTriCacheSectorId[slot]) {
            g_adwTriCacheLruStamp[slot] = g_dwTriCacheLruCounter;
            return slot;
        }
        if (g_adwTriCacheLruStamp[slot] < oldest) {
            best = slot;
            oldest = g_adwTriCacheLruStamp[slot];
        }
    }
    g_adwTriCacheLruStamp[best] = g_dwTriCacheLruCounter;
    g_awTriCacheSectorId[best] = sector;
    triangle = g_aTriCacheTriangles[best];
    count = 0;
    cell = LookupTrackGridCell(x, y);
    cursor.start = cell;
    cursor.count = cell >> 16;
    if (cell != 0) {
        while ((index = NextGridCellTriangle(&cursor)) != 0) {
            /* The original reads the track arrays and the corners afresh for each one. */
            triangle->vertices[0] =
                &g_LoadedTrack.vertices[g_LoadedTrack.surfaces[index].vertices[0]];
            triangle->vertices[1] =
                &g_LoadedTrack.vertices[g_LoadedTrack.surfaces[index].vertices[1]];
            triangle->vertices[2] =
                &g_LoadedTrack.vertices[g_LoadedTrack.surfaces[index].vertices[2]];
            triangle->index = index;
            triangle->surface = g_LoadedTrack.triangleFlags[index] & 7;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            e1[0] = (long double)triangle->vertices[1]->x - triangle->vertices[0]->x;
            e1[1] = (long double)triangle->vertices[1]->y - triangle->vertices[0]->y;
            e1[2] = (long double)triangle->vertices[1]->z - triangle->vertices[0]->z;
            e2[0] = (f32)(triangle->vertices[2]->x - triangle->vertices[0]->x);
            e2[1] = (f32)(triangle->vertices[2]->y - triangle->vertices[0]->y);
            e2[2] = (long double)triangle->vertices[2]->z - triangle->vertices[0]->z;
#else
            e1[0] = triangle->vertices[1]->x - triangle->vertices[0]->x;
            e1[1] = triangle->vertices[1]->y - triangle->vertices[0]->y;
            e1[2] = triangle->vertices[1]->z - triangle->vertices[0]->z;
            e2[0] = triangle->vertices[2]->x - triangle->vertices[0]->x;
            e2[1] = triangle->vertices[2]->y - triangle->vertices[0]->y;
            e2[2] = triangle->vertices[2]->z - triangle->vertices[0]->z;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
            triangle->normal.x = e1[1] * e2[2] - e2[1] * e1[2];
            triangle->normal.y = e1[2] * e2[0] - e2[2] * e1[0];
            triangle->normal.z = e1[0] * e2[1] - e2[0] * e1[1];
            Vec3fNormalizeFast(&triangle->normal);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            triangle->planeOffset = -((long double)triangle->vertices[0]->z * triangle->normal.z +
                                      ((long double)triangle->normal.x * triangle->vertices[0]->x +
                                       (long double)triangle->normal.y * triangle->vertices[0]->y));
#else
            triangle->planeOffset = -(triangle->vertices[0]->z * triangle->normal.z +
                                      (triangle->normal.x * triangle->vertices[0]->x +
                                       triangle->normal.y * triangle->vertices[0]->y));
#endif
            count++;
            triangle++;
        }
    }
    g_awTriCacheTriCount[best] = count;
    return best;
}
#pragma GCC diagnostic pop

void ResetCollisionCaches(void) {
    TrackTriangle *triangle;
    s32 slot, i;

    /* Every slot then holds cell 0, empty, until it is reused. */
    for (slot = 0; slot < TRI_CACHE_SLOTS; slot++) {
        g_awTriCacheSectorId[slot] = 0;
        g_adwTriCacheLruStamp[slot] = 0;
        g_awTriCacheTriCount[slot] = 0;
        for (i = 0; i < TRI_CACHE_TRIANGLES; i++) {
            triangle = &g_aTriCacheTriangles[slot][i];
            triangle->normal.x = 0.0f;
            triangle->normal.y = 0.0f;
            triangle->normal.z = 0.0f;
            triangle->planeOffset = 0.0f;
            triangle->vertices[0] = NULL;
            triangle->vertices[1] = NULL;
            triangle->vertices[2] = NULL;
        }
    }
    for (slot = 0; slot < COLLISION_TRIANGLE_NODES; slot++) {
        g_aCollisionTriangleNodes[slot].triangle = NULL;
        g_aCollisionTriangleNodes[slot].next = NULL;
    }
    g_pCollisionTriangleList = NULL;
    g_pNextCollisionTriangleNode = NULL;
    g_dwTriCacheLruCounter = 0;
}

void PushCollisionTriangle(TrackTriangle *triangle) {
    CollisionTriangleNode *node = g_pNextCollisionTriangleNode++;

    node->triangle = triangle;
    node->next = g_pCollisionTriangleList;
    g_pCollisionTriangleList = node;
}

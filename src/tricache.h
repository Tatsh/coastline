#ifndef COASTLINE_TRICACHE_H
#define COASTLINE_TRICACHE_H

/**
 * @file
 * The cache of the triangles of the track around a point.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"

/** The areas of the track the cache holds at once. */
#define TRI_CACHE_SLOTS 4

/** The triangles a slot of the cache holds. */
#define TRI_CACHE_TRIANGLES 150

/** A triangle of the track. Its fields join this structure as they are reconstructed. */
typedef struct {
    Vec3f normal;
    f32 unreconstructedC;
    Vec3f *vertices[3];
    /** The index of the triangle in the track, for its TrackSurface and its nearby models. */
    s16 index;
    u16 unreconstructed1E;
} TrackTriangle;

/** @ghidraAddress USA: 0x80379f80 */
extern TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];

/** The triangles in each slot. @ghidraAddress USA: 0x8037ea88 */
extern u16 g_awTriCacheTriCount[TRI_CACHE_SLOTS];

/**
 * Find or fill the slot of the cache that holds the triangles of the area under a point.
 *
 * @return The slot that holds them.
 * @ghidraAddress USA: 0x8025f18c
 */
s32 BuildTrackTriangleCache(f32 x, f32 y);

#endif

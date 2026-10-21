#ifndef COASTLINE_TRICACHE_H
#define COASTLINE_TRICACHE_H

/**
 * @file
 * The cache of the triangles of the track around a point, one cell of the 32-unit grid of the
 * track in each slot.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"

/** The cells of the grid the cache holds at once. */
#define TRI_CACHE_SLOTS 4

/** The triangles a slot of the cache holds. */
#define TRI_CACHE_TRIANGLES 150

/** A triangle of the track with its plane. */
typedef struct TrackTriangle {
    Vec3f normal;
    /** The plane is where the normal dotted with a point plus this is 0. */
    f32 planeOffset;
    Vec3f *vertices[3];
    /** The index of the triangle in the track, for its TrackSurface and its nearby models. */
    s16 index;
    /** The surface of the triangle, from the low 3 bits of its flags. */
    u8 surface;
} TrackTriangle;

/** @ghidraAddress USA: 0x80379f80 */
extern TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];

/** The cell each slot holds, as its row times 64 plus its column. @ghidraAddress USA: 0x8037ea80 */
extern u16 g_awTriCacheSectorId[TRI_CACHE_SLOTS];

/** The triangles in each slot. @ghidraAddress USA: 0x8037ea88 */
extern u16 g_awTriCacheTriCount[TRI_CACHE_SLOTS];

/** When each slot was last used. @ghidraAddress USA: 0x8037ea90 */
extern u32 g_adwTriCacheLruStamp[TRI_CACHE_SLOTS];

/** The count of the uses of the cache. @ghidraAddress USA: 0x8037eaa0 */
extern u32 g_dwTriCacheLruCounter;

/**
 * Empty the slot that holds the cell under a point.
 *
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return Whether a slot held it.
 * @ghidraAddress USA: 0x8025f0a4
 */
s32 InvalidateTriCacheSectorAt(f32 x, f32 y);

/**
 * Find or fill the slot of the cache that holds the triangles of the cell under a point, filling
 * the slot used longest ago.
 *
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return The slot that holds them.
 * @ghidraAddress USA: 0x8025f18c
 */
s16 BuildTrackTriangleCache(f32 x, f32 y);

#endif

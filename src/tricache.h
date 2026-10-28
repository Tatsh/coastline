#ifndef COASTLINE_TRICACHE_H
#define COASTLINE_TRICACHE_H

/**
 * @file
 * The cache of the triangles of the track around a point, one cell of the 32-unit grid of the
 * track in each slot.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The cells of the grid the cache holds at once. */
#define TRI_CACHE_SLOTS 4

/** The triangles a slot of the cache holds. */
#define TRI_CACHE_TRIANGLES 150

/** A triangle of the track with its plane. */
typedef struct TrackTriangle {
    Vec3f normal;       /*!< The unit normal of the plane of the triangle. */
    f32 planeOffset;    /*!< The plane is where the normal dotted with a point plus this is 0. */
    Vec3f *vertices[3]; /*!< The three corners of the triangle, in the vertices of the track. */
    s16 index;  /*!< The index of the triangle in the track, for its TrackSurface and its nearby
                     models. */
    u8 surface; /*!< The surface of the triangle, from the low 3 bits of its flags. */
} TrackTriangle;

/**
 * @ghidraAddress USA: 0x80379f80
 * @ghidraAddress EUR: 0x8037c480
 * @ghidraAddress JPN: 0x8037b0f0
 * @ghidraAddress BRD3D: 0x11750338
 * @ghidraAddress BRGlide: 0x11773698
 */
extern TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];

/**
 * The cell each slot holds, as its row times 64 plus its column.
 *
 * @ghidraAddress USA: 0x8037ea80
 * @ghidraAddress EUR: 0x80380f80
 * @ghidraAddress JPN: 0x8037fbf0
 * @ghidraAddress BRD3D: 0x117554d8
 * @ghidraAddress BRGlide: 0x11778838
 */
extern u16 g_awTriCacheSectorId[TRI_CACHE_SLOTS];

/**
 * The triangles in each slot.
 *
 * @ghidraAddress USA: 0x8037ea88
 * @ghidraAddress EUR: 0x80380f88
 * @ghidraAddress JPN: 0x8037fbf8
 * @ghidraAddress BRD3D: 0x117554a0
 * @ghidraAddress BRGlide: 0x11778800
 */
extern u16 g_awTriCacheTriCount[TRI_CACHE_SLOTS];

/**
 * When each slot was last used.
 *
 * @ghidraAddress USA: 0x8037ea90
 * @ghidraAddress EUR: 0x80380f90
 * @ghidraAddress JPN: 0x8037fc00
 * @ghidraAddress BRD3D: 0x117554c8
 * @ghidraAddress BRGlide: 0x11778828
 */
extern u32 g_adwTriCacheLruStamp[TRI_CACHE_SLOTS];

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The count of the uses of the cache.
 *
 * @ghidraAddress USA: 0x8037eaa0
 * @ghidraAddress EUR: 0x80380fa0
 * @ghidraAddress JPN: 0x8037fc10
 * @ghidraAddress BRD3D: 0x117554e0
 */
extern u32 g_dwTriCacheLruCounter;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

/**
 * Empty the slot that holds the cell under a point.
 *
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return Whether a slot held it.
 * @ghidraAddress USA: 0x8025f0a4
 * @ghidraAddress EUR: 0x80260580
 * @ghidraAddress JPN: 0x80260030
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
 * @ghidraAddress EUR: 0x80260668
 * @ghidraAddress JPN: 0x80260118
 * @ghidraAddress BRD3D: 0x1006f720
 * @ghidraAddress BRGlide: 0x100686d0
 */
s16 BuildTrackTriangleCache(f32 x, f32 y);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TRICACHE_H */

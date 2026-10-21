#ifndef COASTLINE_TRACK_H
#define COASTLINE_TRACK_H

/**
 * @file
 * The track data a race loads below the program: its triangles and placed models, and the route
 * the vehicles follow.
 *
 * The route is a tree of sections. Each section is a line of points across the track, with the
 * edges of the track at each point and the distance left from it to the finish. A section can
 * branch into others, which follow it as its children.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "vecmath.h"

/** A point of the route, across the track. */
typedef struct {
    /** The left edge of the track. */
    Vec2f left;
    f32 unreconstructed8;
    /** The middle of the track. */
    Vec3f position;
    /** The right edge of the track. */
    Vec2f right;
    f32 unreconstructed20;
    /** The distance left from the point to the finish line. */
    f32 distance;
} TrackPoint;

/** A section of the route. Its fields join this structure as they are reconstructed. */
typedef struct TrackSection {
    /** The sections that branch from this one. */
    struct TrackSection *child;
    /** The next branch of the same parent. */
    struct TrackSection *next;
    u8 unreconstructed8[0x10 - 0x8];
    /** The cells of the 32-unit grid of the track the section covers, inclusive. */
    u8 gridMinX;
    u8 gridMinY;
    u8 gridMaxX;
    u8 gridMaxY;
    u16 pointCount;
    /** The TRACK_SECTION_ flags. */
    u16 flags;
    u8 unreconstructed18[0x40 - 0x18];
    /** The points, and the point at the end of the last. */
    TrackPoint points[];
} TrackSection;

/** The flag of a branch the computer drivers do not take, and that the walks do not enter. */
#define TRACK_SECTION_ALTERNATE 0x0001

/** The flag a walk sets on the sections it has visited. */
#define TRACK_SECTION_VISITED 0x8000

/** A model placed on the track. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0x4C];
    /** The TRACK_INSTANCE_ flags. */
    u16 flags;
    u8 unreconstructed4E[0x54 - 0x4E];
} TrackInstance;

/** The flag of a placed model that keeps rain and snow off a camera under it, and echoes. */
#define TRACK_INSTANCE_SHELTER 0x0010

/** The surface whose tyre smoke is pale and that throws spray only in the rain: the paved road. */
#define TRACK_SURFACE_PAVED 3

/** The surface that slows a vehicle, except in snow. */
#define TRACK_SURFACE_SLOW 4

/** A triangle of the track: its corners and what it belongs to. */
typedef struct {
    /** The indices of its corners in the vertices of the track. */
    u16 vertices[3];
    /** The placed model the triangle belongs to, less one. */
    u16 instance;
} TrackSurface;

/** The triangles of a cell of the grid of the track: where they start in the list, and how many. */
typedef struct {
    u16 start;
    u16 count;
} TrackGridCursor;

/** The header of the loaded track. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0xC];
    /** What each triangle belongs to, by its index. */
    TrackSurface *surfaces;
    u8 unreconstructed10[0x14 - 0x10];
    Vec3f *vertices;
    u8 unreconstructed18[0x20 - 0x18];
    /** The triangles of each cell of the 64 by 64 grid, and where the list of each cell starts. */
    u16 *gridTriangles;
    u16 *gridCellStarts;
    /** The extent of the track. Lightning strikes from minZ. */
    f32 minX;
    f32 maxX;
    u8 unreconstructed30[0x38 - 0x30];
    f32 minZ;
    f32 maxZ;
    u8 unreconstructed40[0x50 - 0x40];
    /** The display list of the sky, or NULL for a sky of the fog colour. */
    Gfx *skyDisplayList;
    u8 unreconstructed54[0x60 - 0x54];
    /** The placed models. */
    TrackInstance *instances;
    s32 instanceCount;
    u8 unreconstructed68[0x70 - 0x68];
    /** The section at the start line. The distance of its first point is the length of a lap. */
    TrackSection *rootSection;
    u8 unreconstructed74[0x78 - 0x74];
    /** Every section, for a search. */
    TrackSection **sections;
    s32 sectionCount;
    u8 unreconstructed80[0x8C - 0x80];
    /** Lists of placed models near the triangles, each ending in 0. */
    u16 *nearbyInstances;
    /** Where in nearbyInstances the list of each triangle starts, by its index. */
    u16 *triangleNearbyInstances;
    /** The flags of each triangle, by its index; the low 3 bits are its surface. */
    u8 *triangleFlags;
} TrackHeader;

/**
 * The loaded track. The menus use the same memory for their music. The link script places it.
 *
 * @ghidraAddress USA: 0x80025c00
 */
extern TrackHeader g_LoadedTrack;

/** The placed models drawn this frame, by index. @ghidraAddress USA: 0x80352580 */
extern u16 g_awVisibleInstanceList[];

/** The opaque models at the start of the list. @ghidraAddress USA: 0x8028c744 */
extern s32 g_nOpaqueVisibleInstanceCount;

/** The placed model left undrawn while debugging. @ghidraAddress USA: 0x8028c754 */
extern s32 g_nDebugHiddenInstanceIndex;

/** The most placed models CastRayAgainstTrack() lists. */
#define RAY_INSTANCES 32

/**
 * Drop a vertical line through a point onto the track. The ground is the nearest triangle facing
 * up at most 1.5 above the point, and failing one, the nearest within a unit above that.
 *
 * @param outPoint Receives the height of the ground in its z, unless it is NULL.
 * @param outNormal Receives the normal of the ground, unless it is NULL.
 * @param groundInstances Receives the placed models of the ground, the nearest first.
 * @param done Set to 1.
 * @param nearbyInstances Receives the placed models near the ground, the latest first.
 * @param nearbyCount Receives the number of nearbyInstances.
 * @param groundDistance Receives how far the ground is below the point.
 * @param groundSurface Receives the index of the triangle of the ground.
 * @return The number of triangles below the point.
 * @ghidraAddress USA: 0x8021f380
 */
s32 CastRayAgainstTrack(Vec3f *outPoint,
                        Vec3f *outNormal,
                        Vec3f *origin,
                        u16 *groundInstances,
                        s32 *done,
                        u16 *nearbyInstances,
                        s32 *nearbyCount,
                        f32 *groundDistance,
                        s32 *groundSurface);

/**
 * The triangles of the cell of the grid of the track under a point.
 *
 * @return The count of the triangles in the upper half and where they start in the lower, or 0
 * outside the grid.
 * @ghidraAddress USA: 0x8021e620
 */
u32 LookupTrackGridCell(f32 x, f32 y);

/**
 * Take the next triangle of a cell of the grid.
 *
 * @return Its index, or 0 when the cell has no more.
 * @ghidraAddress USA: 0x8021ea90
 */
u16 NextGridCellTriangle(TrackGridCursor *cursor);

#endif

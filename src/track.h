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
    /** Where and how the model is placed. */
    f32 matrix[4][4];
    u8 unreconstructed40[0x44 - 0x40];
    Gfx *displayList;
    /** The groups the model is in, hidden while the camera is near a model that hides them. */
    u16 groups;
    /** The groups the model hides while a ray from the camera hits it. */
    u16 hidesGroups;
    /** The TRACK_INSTANCE_ flags. */
    u16 flags;
    /** The vertices, triangles, and polygons of the model, for the counts of a frame. */
    u16 vertexCount;
    u16 triangleCount;
    u16 polygonCount;
} TrackInstance;

/** The bits of the flags of a placed model that pick its light from g_adwLightRampColors. */
#define TRACK_INSTANCE_LIGHT_RAMP 0x0003

/** The flag of a placed model drawn from both sides. */
#define TRACK_INSTANCE_TWO_SIDED 0x0004

/** The flag of a placed model drawn after the others, the farthest first. */
#define TRACK_INSTANCE_TRANSLUCENT 0x0008

/** The flag of a placed model that keeps rain and snow off a camera under it, and echoes. */
#define TRACK_INSTANCE_SHELTER 0x0010

/**
 * The flag of a placed model drawn without fog, and hidden in the snow, the rain, and at night
 * except on the tracks whose spray shows in the snow.
 */
#define TRACK_INSTANCE_CLEAR_SKY 0x0020

/** The flag of a placed model drawn with flat shading. */
#define TRACK_INSTANCE_FLAT 0x0080

/** The flag of a placed model lit at half the light of the sun at night. */
#define TRACK_INSTANCE_NIGHT_LIT 0x0100

/** The flag of a placed model that the vehicles cast no shadow on. */
#define TRACK_INSTANCE_NO_SHADOW 0x0200

/** The flag of a placed model lit by its light ramp instead of the sun. */
#define TRACK_INSTANCE_RAMPED 0x0400

/** The flag of a placed model shown only at night or in the rain. */
#define TRACK_INSTANCE_DARK_OR_WET 0x0800

/** The flag of a placed model whose matrix holds only its position and a scale. */
#define TRACK_INSTANCE_UNROTATED 0x2000

/** The flag of a placed model shown only in arcade races and the first season. */
#define TRACK_INSTANCE_ARCADE 0x4000

/** The flags of a placed model that change the modes it is drawn with. */
#define TRACK_INSTANCE_MODE_FLAGS                                                                  \
    (TRACK_INSTANCE_TWO_SIDED | TRACK_INSTANCE_CLEAR_SKY | TRACK_INSTANCE_FLAT |                   \
     TRACK_INSTANCE_RAMPED)

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
    u8 unreconstructed54[0x5C - 0x54];
    /** The names of the placed models, by index, or NULL. */
    const char **instanceNames;
    /** The placed models. */
    TrackInstance *instances;
    s32 instanceCount;
    /** The placed models of each cell of the 64 by 64 grid, and where the list of each starts. */
    u16 *gridInstances;
    u16 *gridInstanceStarts;
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
 * @param origin The point the line drops through.
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
 * @param x The x of the point, in the units of the world.
 * @param y The y of the point.
 * @return The count of the triangles in the upper half and where they start in the lower, or 0
 * outside the grid.
 * @ghidraAddress USA: 0x8021e620
 */
u32 LookupTrackGridCell(f32 x, f32 y);

/**
 * Take the next triangle of a cell of the grid.
 *
 * @param cursor Where the triangles of the cell go on and how many are left, which this advances.
 * @return Its index, or 0 when the cell has no more.
 * @ghidraAddress USA: 0x8021ea90
 */
u16 NextGridCellTriangle(TrackGridCursor *cursor);

/**
 * The placed models of a cell of the grid of the track.
 *
 * @param column The column of the cell, from 0 to 63.
 * @param row The row of the cell, from 0 to 63.
 * @return The count of the models in the upper half and where they start in the lower, or 0
 * outside the grid.
 * @ghidraAddress USA: 0x8021ea0c
 */
u32 LookupInstanceGridCell(s32 column, s32 row);

/**
 * Take the next placed model of a cell of the grid.
 *
 * @param cursor Where the models of the cell go on and how many are left, which this advances.
 * @return Its index, or 0 when the cell has no more.
 * @ghidraAddress USA: 0x8021eadc
 */
u16 NextGridCellInstance(TrackGridCursor *cursor);

/**
 * The placed model an animation flies along a spline, such as the airplane, or 0.
 *
 * @ghidraAddress USA: 0x8026ff64
 */
extern s32 g_dwSplineAnimInstance;

#endif

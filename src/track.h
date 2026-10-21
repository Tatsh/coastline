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

#include "render.h"
#include "vecmath.h"

/** A point of the route, across the track. */
typedef struct {
    /** The left edge of the track. */
    Vec3f left;
    /** The middle of the track. */
    Vec3f position;
    /** The right edge of the track. */
    Vec3f right;
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
    /** One over the length of the x axis of the matrix: the inverse of the scale of the model. */
    f32 invScale;
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

/** The surface a wheel and its spray take before the wheel touches the track. */
#define TRACK_SURFACE_START 2

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

/** The special objects a track header lists. */
#define TRACK_SPECIALS 16

/** The kinds of the special objects of a track. */
enum {
    /** Placed models that turn each frame about the z, x or y axis. */
    TRACK_SPECIAL_SPIN_Z,
    TRACK_SPECIAL_SPIN_X,
    TRACK_SPECIAL_SPIN_Y,
    /** The placed model of the airplane, which flies along its path once triggered. */
    TRACK_SPECIAL_AIRPLANE,
    /** The left and right edges of the path of the airplane, a point for each node. */
    TRACK_SPECIAL_AIRPLANE_PATH_LEFT,
    TRACK_SPECIAL_AIRPLANE_PATH_RIGHT,
    /** The placed model a vehicle must pass over on the last lap to send the airplane off. */
    TRACK_SPECIAL_AIRPLANE_TRIGGER,
    /** A placed model that sounds like a waterfall. */
    TRACK_SPECIAL_WATERFALL,
};

/** A special object of a track. */
typedef struct {
    /** The placed model, or for the path of the airplane its points. */
    union {
        s32 instance;
        Vec3f *path;
    } target;
    /** The angle a spinning model turns each frame, or the number of points of the path. */
    union {
        f32 spinAngle;
        s32 pathLength;
    } param;
    /** The kind, by the TRACK_SPECIAL_ values. */
    u8 type;
} TrackSpecial;

/** The gates a track can have. */
#define TRACK_GATES 10

/** A gate of a lap: a line across the route, seen from above. */
typedef struct {
    Vec2f ends[2];
    /** A time the debug output prints when a racer first crosses the gate. */
    f32 seconds;
} TrackGate;

/** The header of the loaded track. Its fields join this structure as they are reconstructed. */
typedef struct {
    /** The bytes of the track once decompressed; the loading first keeps its size here. */
    u32 size;
    /** The size of this header, which must be TRACK_HEADER_SIZE. */
    s32 headerSize;
    u8 unreconstructed8[0xC - 0x8];
    /** What each triangle belongs to, by its index. */
    TrackSurface *surfaces;
    u8 unreconstructed10[0x14 - 0x10];
    Vec3f *vertices;
    /** The textures of the placed models. */
    s32 textureCount;
    ModelTexture *textures;
    /** The triangles of each cell of the 64 by 64 grid, and where the list of each cell starts. */
    u16 *gridTriangles;
    u16 *gridCellStarts;
    /** The extent of the track. Lightning strikes from minZ. */
    f32 minX;
    f32 maxX;
    u8 unreconstructed30[0x38 - 0x30];
    f32 minZ;
    f32 maxZ;
    /** Where the grid starts, and the heading of the vehicles on it, in radians. */
    Vec3f startPosition;
    f32 startHeading;
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
    /** The colour of the fog of the track, in parts. */
    u8 fogColor[3];
    u8 unreconstructed83[0x84 - 0x83];
    /** The points the trackside camera of a replay may stand at. */
    Vec3f *cameraPoints;
    s32 cameraPointCount;
    /** Lists of placed models near the triangles, each ending in 0. */
    u16 *nearbyInstances;
    /** Where in nearbyInstances the list of each triangle starts, by its index. */
    u16 *triangleNearbyInstances;
    /** The flags of each triangle, by its index; the low 3 bits are its surface. */
    u8 *triangleFlags;
    /** The gates of a lap, in order; the first is the line of the lap. */
    TrackGate gates[TRACK_GATES];
    s32 gateCount;
    /** The objects of the track that move or sound, by the TRACK_SPECIAL_ values. */
    TrackSpecial specials[TRACK_SPECIALS];
    s32 specialCount;
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
 * @param groundInstanceCount Set to 1, however many groundInstances were found, so that its readers
 * take only the nearest.
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
                        s32 *groundInstanceCount,
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
 * Decompress a track and its sections from the cartridge into g_LoadedTrack, and prepare it.
 *
 * @param track The index of the track among the descriptors of g_apTrackMenuItems.
 * @ghidraAddress USA: 0x8021de5c
 */
void LoadTrackGeometryFromRom(s32 track);

/** The size of the header of a track. */
#define TRACK_HEADER_SIZE 0x230

/** The most bytes a track may take, and the most placed models it may have. */
#define TRACK_MAX_SIZE 0x171400
#define TRACK_MAX_INSTANCES 0x800

/** The header and the data of the two sections of the route of the loaded track. */
#define TRACK_SECTION_HEADER_SIZE 0x20
#define TRACK_SECTION_DATA_SIZE 0x800

/** @ghidraAddress USA: 0x80373eb0 */
extern u8 g_abTrackSection1Header[TRACK_SECTION_HEADER_SIZE];

/** @ghidraAddress USA: 0x803736b0 */
extern u8 g_abTrackSection1Data[TRACK_SECTION_DATA_SIZE];

/** @ghidraAddress USA: 0x803746d0 */
extern u8 g_abTrackSection2Header[TRACK_SECTION_HEADER_SIZE];

/** @ghidraAddress USA: 0x80373ed0 */
extern u8 g_abTrackSection2Data[TRACK_SECTION_DATA_SIZE];

/** Where the loaded track is on the cartridge. @ghidraAddress USA: 0x8028a87c */
extern u32 g_dwCurrentTrackRomOffset;

/**
 * Where the textures of the loaded track are on the cartridge, after its compressed geometry.
 *
 * @ghidraAddress USA: 0x8028a880
 */
extern u32 g_dwTrackTextureRomOffset;

/** The segment of textures last streamed, or -1. @ghidraAddress USA: 0x8028aa7c */
extern s32 g_dwStreamDmaPrevSegment;

/** The snow mode the textures were last streamed for, or -1. @ghidraAddress USA: 0x8028aa88 */
extern s32 g_dwStreamedSnowMode;

/**
 * Two words of the loaded track, 0x3960 and 0x3968 bytes into it, that the loading prints. The
 * link script places them.
 */
extern u16 g_awTrackDebugWords[5];

/**
 * The triangles of a cell of the grid of the track by its column and row.
 *
 * @param column The column of the cell, from 0 to 63.
 * @param row The row of the cell, from 0 to 63.
 * @return As LookupTrackGridCell().
 * @ghidraAddress USA: 0x8021e998
 */
u32 FindTrackGridCellByIndex(s32 column, s32 row);

/**
 * The placed models of the cell of the grid of the track under a point.
 *
 * @param x The x of the point, in the units of the world.
 * @param y The y of the point.
 * @return As LookupInstanceGridCell().
 * @ghidraAddress USA: 0x8021e7dc
 */
u32 LookupInstanceCellAtPoint(f32 x, f32 y);

/**
 * The triangles left in a cell.
 *
 * @param cursor The cursor of the cell.
 * @return Its count.
 * @ghidraAddress USA: 0x8021ea80
 */
u16 GetTrackGridCellCount(TrackGridCursor *cursor);

/**
 * The placed models left in a cell.
 *
 * @param cursor The cursor of the cell.
 * @return Its count.
 * @ghidraAddress USA: 0x8021ea88
 */
u16 GetInstanceGridCellCount(TrackGridCursor *cursor);

#endif

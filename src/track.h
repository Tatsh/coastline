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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/gbicommand.h"
#endif

/** A point of the route, across the track. */
typedef struct {
    Vec3f left;     /*!< The left edge of the track. */
    Vec3f position; /*!< The middle of the track. */
    Vec3f right;    /*!< The right edge of the track. */
    f32 distance;   /*!< The distance remaining from the point to the finish line. */
} TrackPoint;

/** A section of the route. Its fields join this structure as they are reconstructed. */
typedef struct TrackSection {
    struct TrackSection *child;  /*!< The first section that branches from this one, or NULL. */
    struct TrackSection *next;   /*!< The next branch of the same parent, or NULL. */
    struct TrackSection *parent; /*!< The first preceding section, linked through reverseNext. */
    struct TrackSection *reverseNext; /*!< The next section in a list of preceding sections. */
    u8 gridMinX;    /*!< The first column of the 32-unit grid of the track the section covers. */
    u8 gridMinY;    /*!< The first row of the grid the section covers. */
    u8 gridMaxX;    /*!< The last column of the grid the section covers, inclusive. */
    u8 gridMaxY;    /*!< The last row of the grid the section covers, inclusive. */
    u16 pointCount; /*!< The number of points, without the point at the end of the last. */
    u16 flags;      /*!< The TRACK_SECTION_ flags. */
    TrackPoint previousPoint; /*!< The route point preceding the first point of the section. */
    TrackPoint points[];      /*!< The points, and the point at the end of the last. */
} TrackSection;

/** The flag of a branch the computer drivers do not take, and that the walks do not enter. */
#define TRACK_SECTION_ALTERNATE 0x0001

/** The flag a walk sets on the sections it has visited. */
#define TRACK_SECTION_VISITED 0x8000

/** A model placed on the track. Its fields join this structure as they are reconstructed. */
typedef struct {
    f32 matrix[4][4]; /*!< Where and how the model is placed. */
    f32 invScale; /*!< One over the length of the x axis of the matrix, the inverse of the scale. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    GbiCommand *displayList; /*!< The display list that draws the model. */
#else
    Gfx *displayList; /*!< The display list that draws the model. */
#endif
    u16 groups;        /*!< The groups of the model, hidden near a model that hides them. */
    u16 hidesGroups;   /*!< The groups the model hides while a ray from the camera hits it. */
    u16 flags;         /*!< The TRACK_INSTANCE_ flags. */
    u16 vertexCount;   /*!< The number of vertices of the model, for the counts of a frame. */
    u16 triangleCount; /*!< The number of triangles of the model, for the counts of a frame. */
    u16 polygonCount;  /*!< The number of polygons of the model, for the counts of a frame. */
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
    u16 vertices[3]; /*!< The indices of its corners in the vertices of the track. */
    u16 instance;    /*!< The placed model the triangle belongs to, less one. */
} TrackSurface;

/** The triangles of a cell of the grid of the track: where they start in the list, and how many. */
typedef struct {
    u16 start; /*!< The index of the next entry in the list of the grid. */
    u16 count; /*!< The number of entries remaining in the cell. */
} TrackGridCursor;

/** The special objects a track header lists. */
#define TRACK_SPECIALS 16

/** The kinds of the special objects of a track. */
enum {
    TRACK_SPECIAL_SPIN_Z,             /*!< A placed model that turns each frame about the z axis. */
    TRACK_SPECIAL_SPIN_X,             /*!< A placed model that turns each frame about the x axis. */
    TRACK_SPECIAL_SPIN_Y,             /*!< A placed model that turns each frame about the y axis. */
    TRACK_SPECIAL_AIRPLANE,           /*!< The airplane, flying along its path once triggered. */
    TRACK_SPECIAL_AIRPLANE_PATH_LEFT, /*!< The left edge of the path of the airplane. */
    TRACK_SPECIAL_AIRPLANE_PATH_RIGHT, /*!< The right edge of the path of the airplane. */
    TRACK_SPECIAL_AIRPLANE_TRIGGER, /*!< The model that sends the airplane off on the last lap. */
    TRACK_SPECIAL_WATERFALL,        /*!< A placed model that sounds like a waterfall. */
};

/** A special object of a track. */
typedef struct {
    union {
        s32 instance; /*!< The index of the placed model. */
        Vec3f *path;  /*!< The points of an edge of the path of the airplane, one for each node. */
    } target;         /*!< The placed model, or for the path of the airplane its points. */
    union {
        f32 spinAngle;  /*!< The angle a spinning model turns each frame, in degrees. */
        s32 pathLength; /*!< The number of nodes of the path of the airplane. */
    } param;            /*!< The angle of a spinning model, or the length of the path. */
    u8 type;            /*!< The kind, by the TRACK_SPECIAL_ values. */
} TrackSpecial;

/** The gates a track can have. */
#define TRACK_GATES 10

/** A gate of a lap: a line across the route, seen from above. */
typedef struct {
    Vec2f ends[2]; /*!< The ends of the line across the route. */
    f32 seconds;   /*!< A time the debug output prints when a racer first crosses the gate. */
} TrackGate;

/** The header of the loaded track. Its fields join this structure as they are reconstructed. */
typedef struct {
    u32 size; /*!< The bytes of the track once decompressed, first written here by the loading. */
    s32 headerSize;         /*!< The size of this header, which must be #TRACK_HEADER_SIZE. */
    s32 surfaceCount;       /*!< The number of triangle surfaces. */
    TrackSurface *surfaces; /*!< What each triangle belongs to, by its index. */
    s32 vertexCount;        /*!< The number of triangle corners. */
    Vec3f *vertices;        /*!< The corners of the triangles. */
    s32 textureCount;       /*!< The number of textures. */
    ModelTexture *textures; /*!< The textures of the placed models. */
    u16 *gridTriangles;     /*!< The triangles of each cell of the 64 by 64 grid, cell by cell. */
    u16 *gridCellStarts;    /*!< Where the list of each cell starts in gridTriangles, by cell. */
    f32 minX;               /*!< The least x of the track. */
    f32 maxX;               /*!< The greatest x of the track. */
    f32 minY;
    f32 maxY;
    f32 minZ;            /*!< The least z of the track, where lightning strikes from. */
    f32 maxZ;            /*!< The greatest z of the track. */
    Vec3f startPosition; /*!< The point the vehicles of the starting grid are placed from. */
    f32 startHeading;    /*!< The heading of the vehicles on the grid of the start, in radians. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    GbiCommand *skyDisplayList;
#else
    Gfx *skyDisplayList; /*!< The display list of the sky, or NULL for a sky of the fog colour. */
#endif
    u8 *reservedSkyData[2];     /* +0x54 */
    const char **instanceNames; /*!< The names of the placed models, by index, or NULL. */
    TrackInstance *instances;   /*!< The placed models. */
    s32 instanceCount;          /*!< The number of placed models. */
    u16 *gridInstances; /*!< The placed models of each cell of the 64 by 64 grid, cell by cell. */
    u16 *gridInstanceStarts;   /*!< Where the list of each cell starts in gridInstances, by cell. */
    TrackSection *rootSection; /*!< The section at the start line, whose first point gives the
                                    distance of a lap. */
    u8 *reservedRouteData;     /* +0x74 */
    TrackSection **sections;   /*!< Every section, for a search. */
    s32 sectionCount;          /*!< The number of sections. */
    u8 fogColor[3]; /*!< The colour of the fog of the track, red, green, and blue from 0 to 255. */
    u8 unreconstructed83[0x84 - 0x83]; /*!< Not yet reconstructed. */
    Vec3f *cameraPoints;  /*!< The points the trackside camera of a replay may stand at. */
    s32 cameraPointCount; /*!< The number of cameraPoints. */
    u16 *nearbyInstances; /*!< Lists of placed models near the triangles, each ending in 0. */
    u16 *triangleNearbyInstances; /*!< Where in nearbyInstances the list of each triangle starts,
                                       by its index. */
    u8 *triangleFlags;            /*!< The flags of each triangle, its surface in the low 3 bits. */
    TrackGate gates[TRACK_GATES]; /*!< The gates of a lap in order, the first at the lap line. */
    s32 gateCount;                /*!< The number of gates. */
    TrackSpecial specials[TRACK_SPECIALS]; /*!< The objects of the track that move or sound. */
    s32 specialCount;                      /*!< The number of specials. */
    u8 unreconstructed228[0x230 - 0x228];  /* +0x228 */
} TrackHeader;

/**
 * The loaded track. The menus use the same memory for their music. The link script places it.
 *
 * @ghidraAddress USA: 0x80025c00
 * @ghidraAddress EUR: 0x80025c00
 * @ghidraAddress JPN: 0x80025c00
 * @ghidraAddress BRD3D: 0x106c7c48
 */
extern TrackHeader g_LoadedTrack;

/**
 * The placed models drawn this frame, by index.
 *
 * @ghidraAddress USA: 0x80352580
 * @ghidraAddress EUR: 0x80354a80
 * @ghidraAddress JPN: 0x803536f0
 * @ghidraAddress BRD3D: 0x10362f28
 * @ghidraAddress BRGlide: 0x1035e710
 */
extern u16 g_awVisibleInstanceList[];

/**
 * The opaque models at the start of the list.
 *
 * @ghidraAddress USA: 0x8028c744
 * @ghidraAddress EUR: 0x8028eea4
 * @ghidraAddress JPN: 0x8028e8f4
 * @ghidraAddress BRD3D: 0x103643b4
 * @ghidraAddress BRGlide: 0x1035fb9c
 */
extern s32 g_nOpaqueVisibleInstanceCount;

/**
 * The placed model left undrawn while debugging.
 *
 * @ghidraAddress USA: 0x8028c754
 * @ghidraAddress EUR: 0x8028eeb4
 * @ghidraAddress JPN: 0x8028e904
 * @ghidraAddress BRD3D: 0x1039b6c0
 * @ghidraAddress BRGlide: 0x10396ea8
 */
extern s32 g_nDebugHiddenInstanceIndex;

/** The most placed models CastRayAgainstTrack() lists. */
#define RAY_INSTANCES 32

#ifdef __cplusplus
extern "C" {
#endif
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
 * @ghidraAddress EUR: 0x80220680
 * @ghidraAddress JPN: 0x8021fe70
 * @ghidraAddress BRD3D: 0x100759d0
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
#ifdef __cplusplus
}
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
extern "C" {
#endif

/**
 * The triangles of the cell of the grid of the track under a point.
 *
 * @param x The x of the point, in the units of the world.
 * @param y The y of the point.
 * @return The count of the triangles in the upper half and where they start in the lower, or 0
 * outside the grid.
 * @ghidraAddress USA: 0x8021e620
 * @ghidraAddress EUR: 0x8021f920
 * @ghidraAddress JPN: 0x8021f110
 * @ghidraAddress BRD3D: 0x10002de0
 */
u32 LookupTrackGridCell(f32 x, f32 y);

/**
 * Take the next triangle of a cell of the grid.
 *
 * @param cursor Where the triangles of the cell go on and how many are left, which this advances.
 * @return Its index, or 0 when the cell has no more.
 * @ghidraAddress USA: 0x8021ea90
 * @ghidraAddress EUR: 0x8021fd90
 * @ghidraAddress JPN: 0x8021f580
 * @ghidraAddress BRD3D: 0x10002ef0
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
 * @ghidraAddress EUR: 0x8021fd0c
 * @ghidraAddress JPN: 0x8021f4fc
 * @ghidraAddress BRD3D: 0x10002e90
 */
u32 LookupInstanceGridCell(s32 column, s32 row);

/**
 * Take the next placed model of a cell of the grid.
 *
 * @param cursor Where the models of the cell go on and how many are left, which this advances.
 * @return Its index, or 0 when the cell has no more.
 * @ghidraAddress USA: 0x8021eadc
 * @ghidraAddress EUR: 0x8021fddc
 * @ghidraAddress JPN: 0x8021f5cc
 * @ghidraAddress BRD3D: 0x10002f40
 */
u16 NextGridCellInstance(TrackGridCursor *cursor);
#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
}
#endif

/**
 * Decompress a track and its sections from the cartridge into g_LoadedTrack, and prepare it.
 *
 * @param track The index of the track among the descriptors of g_apTrackMenuItems.
 * @ghidraAddress USA: 0x8021de5c
 * @ghidraAddress EUR: 0x8021f05c
 * @ghidraAddress JPN: 0x8021e92c
 */
void LoadTrackGeometryFromRom(s32 track);

/** The size of the header of a track. */
#define TRACK_HEADER_SIZE 0x230

/* The limits of a track. */
#define TRACK_MAX_SIZE 0x171400   /*!< The most bytes a track may take. */
#define TRACK_MAX_INSTANCES 0x800 /*!< The most placed models a track may have. */

/** The bytes of the header of each of the two sections of the route of the loaded track. */
#define TRACK_SECTION_HEADER_SIZE 0x20
/** The bytes of the data of each of the two sections of the route of the loaded track. */
#define TRACK_SECTION_DATA_SIZE 0x800

/**
 * @ghidraAddress USA: 0x80373eb0
 * @ghidraAddress EUR: 0x803763b0
 * @ghidraAddress JPN: 0x80375020
 */
extern u8 g_abTrackSection1Header[TRACK_SECTION_HEADER_SIZE];

/**
 * @ghidraAddress USA: 0x803736b0
 * @ghidraAddress EUR: 0x80375bb0
 * @ghidraAddress JPN: 0x80374820
 */
extern u8 g_abTrackSection1Data[TRACK_SECTION_DATA_SIZE];

/**
 * @ghidraAddress USA: 0x803746d0
 * @ghidraAddress EUR: 0x80376bd0
 * @ghidraAddress JPN: 0x80375840
 */
extern u8 g_abTrackSection2Header[TRACK_SECTION_HEADER_SIZE];

/**
 * @ghidraAddress USA: 0x80373ed0
 * @ghidraAddress EUR: 0x803763d0
 * @ghidraAddress JPN: 0x80375040
 */
extern u8 g_abTrackSection2Data[TRACK_SECTION_DATA_SIZE];

/**
 * Where the loaded track is on the cartridge.
 *
 * @ghidraAddress USA: 0x8028a87c
 * @ghidraAddress EUR: 0x8028ca7c
 * @ghidraAddress JPN: 0x8028c54c
 */
extern u32 g_dwCurrentTrackRomOffset;

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * Where the textures of the loaded track are on the cartridge, after its compressed geometry.
 *
 * @ghidraAddress USA: 0x8028a880
 * @ghidraAddress EUR: 0x8028ca80
 * @ghidraAddress JPN: 0x8028c550
 */
extern u32 g_dwTrackTextureRomOffset;
#endif

/**
 * The segment of textures last streamed, or -1.
 *
 * @ghidraAddress USA: 0x8028aa7c
 * @ghidraAddress EUR: 0x8028cc84
 * @ghidraAddress JPN: 0x8028c754
 */
extern s32 g_dwStreamDmaPrevSegment;

/**
 * The snow mode the textures were last streamed for, or -1.
 *
 * @ghidraAddress USA: 0x8028aa88
 * @ghidraAddress EUR: 0x8028cc90
 * @ghidraAddress JPN: 0x8028c760
 * @ghidraAddress BRD3D: 0x100aa8a0
 */
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
 * @ghidraAddress EUR: 0x8021fc98
 * @ghidraAddress JPN: 0x8021f488
 */
u32 FindTrackGridCellByIndex(s32 column, s32 row);

/**
 * The placed models of the cell of the grid of the track under a point.
 *
 * @param x The x of the point, in the units of the world.
 * @param y The y of the point.
 * @return As LookupInstanceGridCell().
 * @ghidraAddress USA: 0x8021e7dc
 * @ghidraAddress EUR: 0x8021fadc
 * @ghidraAddress JPN: 0x8021f2cc
 */
u32 LookupInstanceCellAtPoint(f32 x, f32 y);

/**
 * The triangles left in a cell.
 *
 * @param cursor The cursor of the cell.
 * @return Its count.
 * @ghidraAddress USA: 0x8021ea80
 * @ghidraAddress EUR: 0x8021fd80
 * @ghidraAddress JPN: 0x8021f570
 */
u16 GetTrackGridCellCount(TrackGridCursor *cursor);

/**
 * The placed models left in a cell.
 *
 * @param cursor The cursor of the cell.
 * @return Its count.
 * @ghidraAddress USA: 0x8021ea88
 * @ghidraAddress EUR: 0x8021fd88
 * @ghidraAddress JPN: 0x8021f578
 */
u16 GetInstanceGridCellCount(TrackGridCursor *cursor);

#endif

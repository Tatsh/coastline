#ifndef COASTLINE_RENDER_H
#define COASTLINE_RENDER_H

/**
 * @file
 * The state the renderer shares between the parts of a frame.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "displaylistptr.h"
#include "vecmath.h"

/**
 * A frame of reference: three axes and a position, the rows of a 4 by 4 matrix whose last column
 * FinishFrameMatrix() sets, and a field of view. The world is z up. A vehicle and each of its
 * cameras has one.
 */
typedef struct {
    Vec3f forward;  /*!< The axis pointing forward. */
    f32 forwardW;   /*!< The fourth column of the forward row, 0 after FinishFrameMatrix(). */
    Vec3f side;     /*!< The axis pointing to the side. */
    f32 sideW;      /*!< The fourth column of the side row, 0 after FinishFrameMatrix(). */
    Vec3f up;       /*!< The axis pointing up. */
    f32 upW;        /*!< The fourth column of the up row, 0 after FinishFrameMatrix(). */
    Vec3f position; /*!< The position in the world. */
    f32 positionW;  /*!< The fourth column of the position row, 1 after FinishFrameMatrix(). */
    f32 fov;        /*!< The field of view of a camera, in radians. */
} CameraFrame;

/**
 * A frame of an animated texture: the time it starts at, in milliseconds, and where its texels and
 * its palette are on the cartridge, from g_dwTrackTextureRomOffset, or -1 for none.
 */
typedef struct {
    u32 time;       /*!< The time the frame starts at, in milliseconds. */
    u32 imageRom;   /*!< The offset of the texels from g_dwTrackTextureRomOffset, or -1. */
    u32 paletteRom; /*!< The offset of the palette from g_dwTrackTextureRomOffset, or -1. */
} TextureAnimationFrame;

/** The frames of an animated texture. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0x2 - 0x0]; /*!< Bytes not yet reconstructed. */
    u16 frameCount;                 /*!< The number of frames. */
    u8 unreconstructed4[0x8 - 0x4]; /*!< Bytes not yet reconstructed. */
    TextureAnimationFrame frames[]; /*!< The frames, in order of their start times. */
} TextureAnimation;

/**
 * A texture of a model and its palette. Its fields join this structure as they are reconstructed.
 */
typedef struct {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    union {
        u8 *image;             /*!< The texels before device texture creation. */
        unsigned textureEntry; /*!< The loaded entry after device texture creation. */
    };
#else
    u8 *image; /*!< The texels. */
#endif
    u16 *palette; /*!< The 16 colors of the palette, in RGBA 5551. */
    union {
        u32 paletteRomIndex; /*!< The palette offset, in units of 32 bytes in the low 12 bits. */
        TextureAnimation *animation; /*!< The frames of an animated texture. */
    } source;   /*!< The source of the palette, or of the frames of an animated texture. */
    u16 width;  /*!< The width in texels. */
    u16 height; /*!< The height in texels. */
    s16 tileT0; /*!< The first texel row the tile covers. */
    s16 tileS0; /*!< The first texel column the tile covers. */
    s16 tileT1; /*!< The last texel row the tile covers. */
    s16 tileS1; /*!< The last texel column the tile covers. */
    u8 unreconstructed18[0x20 - 0x18]; /*!< Bytes not yet reconstructed. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u32 textureFlags;
#else
    unsigned mirrorS : 1;  /*!< Whether the texture repeats mirrored in s. */
    unsigned mirrorT : 1;  /*!< Whether the texture repeats mirrored in t. */
    unsigned clampS : 1;   /*!< Whether the texture clamps in s rather than wrapping. */
    unsigned clampT : 1;   /*!< Whether the texture clamps in t rather than wrapping. */
    unsigned format : 4;   /*!< The format, a TEXTURE_FORMAT_* value. */
    unsigned : 3;          /*!< Bits not yet reconstructed. */
    unsigned animated : 1; /*!< Whether source points to the frames of an animated texture. */
    unsigned : 2;          /*!< Bits not yet reconstructed. */
    unsigned size : 18;    /*!< The length of the texels in bytes. */
#endif
} ModelTexture;

/* The formats of a texture of a model. */
#define TEXTURE_FORMAT_CI4 1 /*!< 4-bit texels with a palette of 16 colors. */
#define TEXTURE_FORMAT_I8 4  /*!< 8-bit intensity texels. */

/** The format of an animated texture of the track that stays on its first frame in the snow. */
#define TEXTURE_FORMAT_SNOW_STILL 0xB

/** The palette of the texture memory a 4-bit texture of a model loads its colors into. */
#define TEXTURE_CI4_PALETTE 15

/** The clipping ratios from 1 to 6, each a display list that sets it. */
#define CLIP_RATIOS 6

/** The rectangle of the screen a view fills. */
typedef struct {
    s32 x;            /*!< The left edge, in screen pixels. */
    s32 y;            /*!< The top edge, in screen pixels. */
    s32 width;        /*!< The width, in screen pixels. */
    s32 height;       /*!< The height, in screen pixels. */
    s32 vehicleIndex; /*!< The vehicle of the player the view follows. */
} ViewportRect;

/**
 * The projection of the current view.
 *
 * @ghidraAddress USA: 0x8028a878
 * @ghidraAddress EUR: 0x8028ca78
 * @ghidraAddress JPN: 0x8028c548
 */
extern Mtx *g_pProjectionMtx;

/**
 * A matrix the renderer builds before converting it.
 *
 * @ghidraAddress USA: 0x8031ab10
 * @ghidraAddress EUR: 0x8031d010
 * @ghidraAddress JPN: 0x8031bc40
 */
extern f32 g_afMvpMatrix[4][4];

/**
 * The perspective of the current view.
 *
 * @ghidraAddress USA: 0x8031aa90
 * @ghidraAddress EUR: 0x8031cf90
 * @ghidraAddress JPN: 0x8031bbc0
 */
extern f32 g_afPerspectiveMatrix[4][4];

/**
 * A matrix the renderer fills as an operand.
 *
 * @ghidraAddress USA: 0x8031ab50
 * @ghidraAddress EUR: 0x8031d050
 * @ghidraAddress JPN: 0x8031bc80
 */
extern f32 g_afScratchMatrix[4][4];

/**
 * The texture filter of the other modes.
 *
 * @ghidraAddress USA: 0x8028a898
 * @ghidraAddress EUR: 0x8028ca9c
 * @ghidraAddress JPN: 0x8028c56c
 * @ghidraAddress BRD3D: 0x106c0258
 */
extern u32 g_dwTextureFilterMode;

/**
 * Whether the track is drawn mirrored.
 *
 * @ghidraAddress USA: 0x8028a8ac
 * @ghidraAddress EUR: 0x8028cab0
 * @ghidraAddress JPN: 0x8028c580
 */
extern s32 g_dwTrackMirrored;

/**
 * Whether the current view is mirrored.
 *
 * @ghidraAddress USA: 0x8028a8a8
 * @ghidraAddress EUR: 0x8028caac
 * @ghidraAddress JPN: 0x8028c57c
 */
extern s32 g_dwViewportMirrored;

/**
 * The camera of the view being drawn.
 *
 * @ghidraAddress USA: 0x8028aaf4
 * @ghidraAddress EUR: 0x8028ccfc
 * @ghidraAddress JPN: 0x8028c7cc
 * @ghidraAddress BRD3D: 0x106c6490
 */
extern CameraFrame *g_pActiveCamera;

/**
 * The field of view of the cameras, in radians.
 *
 * @ghidraAddress USA: 0x8028aac0
 * @ghidraAddress EUR: 0x8028ccc8
 * @ghidraAddress JPN: 0x8028c798
 * @ghidraAddress BRD3D: 0x100aa8ac
 */
extern f32 g_fCameraFovRadians;

/**
 * The distance the far planes of the cameras are taken from; the stages of the menus see a fifth of
 * it.
 *
 * @ghidraAddress USA: 0x8028aac8
 * @ghidraAddress EUR: 0x8028ccd0
 * @ghidraAddress JPN: 0x8028c7a0
 */
extern f32 g_fCameraProjectionScale;

/**
 * The views on the screen, one for each player.
 *
 * @ghidraAddress USA: 0x8028ab0c
 * @ghidraAddress EUR: 0x8028cd14
 * @ghidraAddress JPN: 0x8028c7e4
 * @ghidraAddress BRD3D: 0x100aa8b4
 */
extern s32 g_dwNumViewports;

/**
 * The rectangles of the views.
 *
 * @ghidraAddress USA: 0x8031b2c8
 * @ghidraAddress EUR: 0x8031d7c8
 * @ghidraAddress JPN: 0x8031c3f8
 */
extern ViewportRect g_aViewportRects[];

/**
 * The view being drawn.
 *
 * @ghidraAddress USA: 0x8028aaec
 * @ghidraAddress EUR: 0x8028ccf4
 * @ghidraAddress JPN: 0x8028c7c4
 */
extern s32 g_dwCurrentSplitScreenIndex;

/**
 * The depth scale of the viewport SetViewport() last set.
 *
 * @ghidraAddress USA: 0x8028ab64
 * @ghidraAddress EUR: 0x8028cd6c
 * @ghidraAddress JPN: 0x8028c83c
 */
extern s32 g_dwViewportScaleZ;

/**
 * The depth offset of the viewport SetViewport() last set.
 *
 * @ghidraAddress USA: 0x8028ab68
 * @ghidraAddress EUR: 0x8028cd70
 * @ghidraAddress JPN: 0x8028c840
 */
extern s32 g_dwViewportTransZ;

/**
 * The view and projection of the view being drawn.
 *
 * @ghidraAddress USA: 0x8031aa50
 * @ghidraAddress EUR: 0x8031cf50
 * @ghidraAddress JPN: 0x8031bb80
 * @ghidraAddress BRD3D: 0x106c29a8
 */
extern f32 g_afViewProjMatrix[4][4];

/**
 * The normalisation of the perspective of the projection.
 *
 * @ghidraAddress USA: 0x8028a874
 * @ghidraAddress EUR: 0x8028ca74
 * @ghidraAddress JPN: 0x8028c544
 */
extern u16 g_wPerspNorm;

/**
 * The distance of the far plane of the cameras.
 *
 * @ghidraAddress USA: 0x8028aacc
 * @ghidraAddress EUR: 0x8028ccd4
 * @ghidraAddress JPN: 0x8028c7a4
 */
extern f32 gFrustumFar;

/**
 * The angle SetupFrustumCorners() was last given.
 *
 * @ghidraAddress USA: 0x8028aac4
 * @ghidraAddress EUR: 0x8028cccc
 * @ghidraAddress JPN: 0x8028c79c
 */
extern f32 g_fFrustumFovAngle;

/**
 * The view of the current view, before its perspective.
 *
 * @ghidraAddress USA: 0x8031aad0
 * @ghidraAddress EUR: 0x8031cfd0
 * @ghidraAddress JPN: 0x8031bc00
 */
extern f32 g_ViewLookAtMtx[4][4];

/**
 * @ghidraAddress USA: 0x8028a8c0
 * @ghidraAddress EUR: 0x8028cac8
 * @ghidraAddress JPN: 0x8028c598
 */
extern Mtx g_mtxIdentity;

/**
 * The vertices, triangles, and polygons drawn this frame.
 *
 * @ghidraAddress USA: 0x8028aa08
 * @ghidraAddress EUR: 0x8028cc10
 * @ghidraAddress JPN: 0x8028c6e0
 */
extern s32 g_dwStatVerticesDrawn;

/**
 * @ghidraAddress USA: 0x8028aa10
 * @ghidraAddress EUR: 0x8028cc18
 * @ghidraAddress JPN: 0x8028c6e8
 */
extern s32 g_dwStatTrianglesDrawn;

/**
 * @ghidraAddress USA: 0x8028aa38
 * @ghidraAddress EUR: 0x8028cc40
 * @ghidraAddress JPN: 0x8028c710
 */
extern s32 g_dwStatPolygonsDrawn;

/**
 * Whether the level of detail of the textures of the track is on.
 *
 * @ghidraAddress USA: 0x8028aa44
 * @ghidraAddress EUR: 0x8028cc4c
 * @ghidraAddress JPN: 0x8028c71c
 */
extern s32 g_bTrackTextureLod;

/**
 * Whether the track is drawn with smooth shading.
 *
 * @ghidraAddress USA: 0x8028aa48
 * @ghidraAddress EUR: 0x8028cc50
 * @ghidraAddress JPN: 0x8028c720
 */
extern s32 g_bSmoothShading;

/**
 * The colour of the light of the sun and of the ambient light.
 *
 * @ghidraAddress USA: 0x8028ab58
 * @ghidraAddress EUR: 0x8028cd60
 * @ghidraAddress JPN: 0x8028c830
 * @ghidraAddress BRD3D: 0x106c5ab0
 */
extern u32 g_dwDirLightColor;

/**
 * @ghidraAddress USA: 0x8028ab5c
 * @ghidraAddress EUR: 0x8028cd64
 * @ghidraAddress JPN: 0x8028c834
 * @ghidraAddress BRD3D: 0x106c29e8
 */
extern u32 g_dwAmbientLightColor;

/**
 * The lights the vehicles and the track start from.
 *
 * @ghidraAddress USA: 0x8028a9f0
 * @ghidraAddress EUR: 0x8028cbf8
 * @ghidraAddress JPN: 0x8028c6c8
 */
extern Lights1 g_DefaultLights;

/**
 * The direction of the light of the sun.
 *
 * @ghidraAddress USA: 0x8031b338
 * @ghidraAddress EUR: 0x8031d828
 * @ghidraAddress JPN: 0x8031c458
 * @ghidraAddress BRD3D: 0x106c0670
 */
extern f32 g_afLightDirection[3];

/**
 * The colours of the light of the placed models lit by their light ramp, by its index.
 *
 * @ghidraAddress USA: 0x8031b360
 * @ghidraAddress EUR: 0x8031d858
 * @ghidraAddress JPN: 0x8031c488
 * @ghidraAddress BRD3D: 0x106c0950
 */
extern u32 g_adwLightRampColors[4];

/**
 * Whether each vehicle is in the view being drawn.
 *
 * @ghidraAddress USA: 0x80351c70
 * @ghidraAddress EUR: 0x80354170
 * @ghidraAddress JPN: 0x80352de0
 */
extern s32 g_adwVehicleVisible[];

/**
 * The display lists that set each clipping ratio, from 1.
 *
 * @ghidraAddress USA: 0x8028a900
 * @ghidraAddress EUR: 0x8028cb08
 * @ghidraAddress JPN: 0x8028c5d8
 */
extern Gfx g_aClipRatioDisplayLists[CLIP_RATIOS][5];

/**
 * The colour of the light of the sun, in parts.
 *
 * @ghidraAddress USA: 0x8028ab40
 * @ghidraAddress EUR: 0x8028cd48
 * @ghidraAddress JPN: 0x8028c818
 * @ghidraAddress BRD3D: 0x106c1580
 */
extern u8 g_bDirLightR;

/**
 * @ghidraAddress USA: 0x8028ab44
 * @ghidraAddress EUR: 0x8028cd4c
 * @ghidraAddress JPN: 0x8028c81c
 * @ghidraAddress BRD3D: 0x106c335c
 */
extern u8 g_bDirLightG;

/**
 * @ghidraAddress USA: 0x8028ab48
 * @ghidraAddress EUR: 0x8028cd50
 * @ghidraAddress JPN: 0x8028c820
 * @ghidraAddress BRD3D: 0x106c0968
 */
extern u8 g_bDirLightB;

/**
 * The colour of the ambient light, in parts.
 *
 * @ghidraAddress USA: 0x8028ab4c
 * @ghidraAddress EUR: 0x8028cd54
 * @ghidraAddress JPN: 0x8028c824
 * @ghidraAddress BRD3D: 0x10690bf0
 */
extern u8 g_bAmbientLightR;

/**
 * @ghidraAddress USA: 0x8028ab50
 * @ghidraAddress EUR: 0x8028cd58
 * @ghidraAddress JPN: 0x8028c828
 * @ghidraAddress BRD3D: 0x106c0960
 */
extern u8 g_bAmbientLightG;

/**
 * @ghidraAddress USA: 0x8028ab54
 * @ghidraAddress EUR: 0x8028cd5c
 * @ghidraAddress JPN: 0x8028c82c
 * @ghidraAddress BRD3D: 0x106c65bc
 */
extern u8 g_bAmbientLightB;

/**
 * The least detailed level the vehicles are drawn at, from 0.
 *
 * @ghidraAddress USA: 0x8028aa9c
 * @ghidraAddress EUR: 0x8028cca4
 * @ghidraAddress JPN: 0x8028c774
 */
extern s32 g_dwVehicleLodBias;

/**
 * Whether the car of the camera is drawn from its cockpit and mirror cameras too.
 *
 * @ghidraAddress USA: 0x8028aa70
 * @ghidraAddress EUR: 0x8028cc78
 * @ghidraAddress JPN: 0x8028c748
 */
extern s32 g_dwVehicleCullingDisabled;

/**
 * The triangles of the vehicles drawn this frame.
 *
 * @ghidraAddress USA: 0x8028aa3c
 * @ghidraAddress EUR: 0x8028cc44
 * @ghidraAddress JPN: 0x8028c714
 */
extern s32 g_dwStatVehicleTriangles;

/**
 * Counts of a frame that ClearRenderStats() clears. No routine reads them.
 *
 * @ghidraAddress USA: 0x8028aa0c
 * @ghidraAddress EUR: 0x8028cc14
 * @ghidraAddress JPN: 0x8028c6e4
 */
extern s32 g_dwStatUnused0;

/**
 * @ghidraAddress USA: 0x8028aa2c
 * @ghidraAddress EUR: 0x8028cc34
 * @ghidraAddress JPN: 0x8028c704
 */
extern s32 g_dwStatUnused1;

/**
 * @ghidraAddress USA: 0x8028aa30
 * @ghidraAddress EUR: 0x8028cc38
 * @ghidraAddress JPN: 0x8028c708
 */
extern s32 g_dwStatUnused2;

/**
 * @ghidraAddress USA: 0x8028aa34
 * @ghidraAddress EUR: 0x8028cc3c
 * @ghidraAddress JPN: 0x8028c70c
 */
extern s32 g_dwStatUnused3;

/**
 * The address in the texture memory DrawTexturedTile() loads to.
 *
 * @ghidraAddress USA: 0x8028ab1c
 * @ghidraAddress EUR: 0x8028cd24
 * @ghidraAddress JPN: 0x8028c7f4
 */
extern u32 g_dwTextureTmemAddress;

/**
 * The tile DrawTexturedTile() sets up to draw with.
 *
 * @ghidraAddress USA: 0x8028ab18
 * @ghidraAddress EUR: 0x8028cd20
 * @ghidraAddress JPN: 0x8028c7f0
 */
extern u32 g_dwTextureRenderTile;

/**
 * The depths the fog starts and ends at, from 0 at the near plane to 1000 at the far.
 *
 * @ghidraAddress USA: 0x8028ab30
 * @ghidraAddress EUR: 0x8028cd38
 * @ghidraAddress JPN: 0x8028c808
 * @ghidraAddress BRD3D: 0x106c3398
 */
extern s32 g_dwFogMinDistance;

/**
 * @ghidraAddress USA: 0x8028ab34
 * @ghidraAddress EUR: 0x8028cd3c
 * @ghidraAddress JPN: 0x8028c80c
 * @ghidraAddress BRD3D: 0x106c64d8
 */
extern s32 g_dwFogMaxDistance;

/**
 * The offset and the scale that turn the depth of a point into its fog, from 0 to 255, as the
 * microcode does.
 *
 * @ghidraAddress USA: 0x8028ab38
 * @ghidraAddress EUR: 0x8028cd40
 * @ghidraAddress JPN: 0x8028c810
 * @ghidraAddress BRD3D: 0x106c1618
 */
extern s32 g_dwFogFactorOffset;

/**
 * @ghidraAddress USA: 0x8028ab3c
 * @ghidraAddress EUR: 0x8028cd44
 * @ghidraAddress JPN: 0x8028c814
 * @ghidraAddress BRD3D: 0x106c2cf4
 */
extern s32 g_dwFogFactorScale;

/**
 * The light ramps of the placed models, in parts, by the index of the ramp.
 *
 * @ghidraAddress USA: 0x8031b350
 * @ghidraAddress EUR: 0x8031d848
 * @ghidraAddress JPN: 0x8031c478
 * @ghidraAddress BRD3D: 0x10690ff8
 */
extern u8 g_abLightRampR[4];

/**
 * @ghidraAddress USA: 0x8031b354
 * @ghidraAddress EUR: 0x8031d84c
 * @ghidraAddress JPN: 0x8031c47c
 * @ghidraAddress BRD3D: 0x106c6494
 */
extern u8 g_abLightRampG[4];

/**
 * @ghidraAddress USA: 0x8031b358
 * @ghidraAddress EUR: 0x8031d850
 * @ghidraAddress JPN: 0x8031c480
 * @ghidraAddress BRD3D: 0x106c3358
 */
extern u8 g_abLightRampB[4];

/** The viewports. The display lists of the last frames may still use them. */
#define VIEWPORTS 32

/**
 * @ghidraAddress USA: 0x80318cd0
 * @ghidraAddress EUR: 0x8031c1b0
 * @ghidraAddress JPN: 0x8031adf8
 */
extern Vp g_aViewports[VIEWPORTS];

/**
 * The viewport set last.
 *
 * @ghidraAddress USA: 0x8028ab60
 * @ghidraAddress EUR: 0x8028cd68
 * @ghidraAddress JPN: 0x8028c838
 */
extern s32 g_dwViewportIndex;

/**
 * The display list of g_aClipRatioDisplayLists a frame starts with.
 *
 * @ghidraAddress USA: 0x8028a8a4
 * @ghidraAddress EUR: 0x8028caa8
 * @ghidraAddress JPN: 0x8028c578
 */
extern s32 g_dwClipRatioIndex;

/**
 * Whether the microcode clips the triangles.
 *
 * @ghidraAddress USA: 0x8028aa50
 * @ghidraAddress EUR: 0x8028cc58
 * @ghidraAddress JPN: 0x8028c728
 */
extern s32 g_bGbiClipping;

/**
 * Clear the counts of the vertices, triangles, and polygons drawn in a frame.
 *
 * @ghidraAddress USA: 0x802173c8
 * @ghidraAddress EUR: 0x802183e8
 * @ghidraAddress JPN: 0x80217d38
 */
void ClearRenderStats(void);

/**
 * Blank the screen and clear both frame buffers in the low resolution.
 *
 * @ghidraAddress USA: 0x8021a540
 * @ghidraAddress EUR: 0x8021b630
 * @ghidraAddress JPN: 0x8021af0c
 */
void ClearFramebuffersLowRes(void);

/**
 * Scale a whole matrix down by the largest magnitude of its first three rows, plus 0.375, so that
 * it fits the fixed point of the matrix the lighting is forced to.
 *
 * @param m The matrix.
 * @ghidraAddress USA: 0x802174b4
 * @ghidraAddress EUR: 0x802184d4
 * @ghidraAddress JPN: 0x80217e24
 */
void NormalizeLightMatrix(f32 m[4][4]);

/**
 * Load a texture of a model into the texture memory and set up the tiles to draw it.
 *
 * @param index The index of the texture.
 * @param textures The textures of the model.
 * @ghidraAddress USA: 0x80217734
 * @ghidraAddress EUR: 0x80218754
 * @ghidraAddress JPN: 0x802180a4
 */
void DrawTexturedTile(s32 index, ModelTexture *textures);

#ifdef __cplusplus
extern "C" {
#endif
/**
 * How thick the fog is at a point of the world, in the view being drawn.
 *
 * @param point The point.
 * @return The fog, from 0 to 1, or 0 when there is no weather.
 * @ghidraAddress USA: 0x80218c8c
 * @ghidraAddress EUR: 0x80219cac
 * @ghidraAddress JPN: 0x802195fc
 * @ghidraAddress BRD3D: 0x10031d3f
 */
f32 ComputeFogFactorAtPoint(Vec3f *point);
#ifdef __cplusplus
}
#endif

/**
 * The largest absolute value of the first three rows of a matrix.
 *
 * @param m The matrix.
 * @return The largest absolute value of its first 12 elements.
 * @ghidraAddress USA: 0x80217420
 * @ghidraAddress EUR: 0x80218440
 * @ghidraAddress JPN: 0x80217d90
 */
f32 GetMaxAbsValue12(f32 m[4][4]);

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Set the direction and the colours of the lights, and the light ramps, for the weather.
 *
 * @ghidraAddress USA: 0x80218d5c
 * @ghidraAddress EUR: 0x80219d7c
 * @ghidraAddress JPN: 0x802196cc
 * @ghidraAddress BRD3D: 0x10031dcf
 */
void SetupWeatherLights(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Set the colour and the range of the fog for the weather and the track, and add them to the
 * display list.
 *
 * @ghidraAddress USA: 0x802182a8
 * @ghidraAddress EUR: 0x802192c8
 * @ghidraAddress JPN: 0x80218c18
 * @ghidraAddress BRD3D: 0x10031866
 */
void SetupFog(void);
#ifdef __cplusplus
}
#endif

/**
 * Load the viewport of the view being drawn again.
 *
 * @ghidraAddress USA: 0x80219f04
 * @ghidraAddress EUR: 0x8021af24
 * @ghidraAddress JPN: 0x8021a87c
 */
void RestoreCurrentViewport(void);

/**
 * Fill a rectangle of the screen with a colour.
 *
 * @param x The left edge, in the pixels of the low resolution, which high resolution doubles.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height.
 * @param red The red of the colour, from 0 to 255, of which the upper 5 bits count.
 * @param green The green of the colour.
 * @param blue The blue of the colour.
 * @ghidraAddress USA: 0x80218104
 * @ghidraAddress EUR: 0x80219124
 * @ghidraAddress JPN: 0x80218a74
 */
void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue);

/**
 * Do nothing.
 *
 * @ghidraAddress USA: 0x80217c8c
 * @ghidraAddress EUR: 0x80218cac
 * @ghidraAddress JPN: 0x802185fc
 */
void StubEmpty(void);

/**
 * Set the scissor to a rectangle, and the viewport to the whole of the low-resolution screen.
 *
 * @param x The left edge of the scissor.
 * @param y The upper edge of the scissor.
 * @param width The width of the scissor, of which only the size counts.
 * @param height The height of the scissor.
 * @param setScissor Whether the scissor is set.
 * @ghidraAddress USA: 0x80219df0
 * @ghidraAddress EUR: 0x8021ae10
 * @ghidraAddress JPN: 0x8021a768
 */
void SetupViewportAndScissor(s32 x, s32 y, s32 width, s32 height, s32 setScissor);

/**
 * Outline a rectangle of the screen with lines a pixel wide.
 *
 * @param x The left edge.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height.
 * @ghidraAddress USA: 0x80219f6c
 * @ghidraAddress EUR: 0x8021af8c
 * @ghidraAddress JPN: 0x8021a8e4
 */
void DrawRdpOutlineRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Frame a rectangle of the screen, such as the rear-view mirror, with a dark border and light
 * rings.
 *
 * @param x The left edge.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height.
 * @ghidraAddress USA: 0x8021a0f8
 * @ghidraAddress EUR: 0x8021b118
 * @ghidraAddress JPN: 0x8021aa70
 */
void DrawRdpBorderRect(s32 x, s32 y, s32 width, s32 height);

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * Whether BlankDisplay() blanked the screen and UnblankDisplay() has not yet shown it.
 *
 * @ghidraAddress EUR: 0x8028ce14
 * @ghidraAddress JPN: 0x8028c844
 */
extern u32 g_dwDisplayBlanked;

/**
 * Blank the screen. The European release first sets the video mode of the resolution in use
 * from the tables of libultra, unless the screen is already blank.
 *
 * @ghidraAddress EUR: 0x8021b4e8
 * @ghidraAddress JPN: 0x8021ae40
 */
void BlankDisplay(void);

/**
 * Show the screen again. The European release sets the video mode of the resolution in use from
 * the copies of the game when BlankDisplay() blanked the screen.
 *
 * @ghidraAddress EUR: 0x8021b54c
 * @ghidraAddress JPN: 0x8021ae68
 */
void UnblankDisplay(void);
#endif

#if TGR_REGION == TGR_REGION_US
#define BLANK_SCREEN() osViBlack(TRUE)
#else
#define BLANK_SCREEN() BlankDisplay()
#endif

/**
 * Black out the screen and clear both frame buffers in the high resolution.
 *
 * @ghidraAddress USA: 0x8021a4c8
 * @ghidraAddress EUR: 0x8021b5b0
 * @ghidraAddress JPN: 0x8021ae8c
 */
void ClearFramebuffersHighRes(void);

/**
 * Copy the current frames of the animated textures of the track, and their snow versions, from the
 * cartridge.
 *
 * @ghidraAddress USA: 0x8021a5b8
 * @ghidraAddress EUR: 0x8021b6d0
 * @ghidraAddress JPN: 0x8021afac
 */
void StreamAnimatedTrackTextures(void);

#endif

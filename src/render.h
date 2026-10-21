#ifndef COASTLINE_RENDER_H
#define COASTLINE_RENDER_H

/**
 * @file
 * The state the renderer shares between the parts of a frame.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "vecmath.h"

/**
 * A frame of reference: three axes and a position, the rows of a 4 by 4 matrix whose last column
 * FinishFrameMatrix() sets, and a field of view. The world is z up. A vehicle and each of its
 * cameras has one.
 */
typedef struct {
    Vec3f forward;
    f32 forwardW;
    Vec3f side;
    f32 sideW;
    Vec3f up;
    f32 upW;
    Vec3f position;
    f32 positionW;
    /** The field of view of a camera, in radians. */
    f32 fov;
} CameraFrame;

/**
 * A frame of an animated texture: the time it starts at, in milliseconds, and where its texels and
 * its palette are on the cartridge, from g_dwTrackTextureRomOffset, or -1 for none.
 */
typedef struct {
    u32 time;
    u32 imageRom;
    u32 paletteRom;
} TextureAnimationFrame;

/** The frames of an animated texture. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0x2 - 0x0];
    u16 frameCount;
    u8 unreconstructed4[0x8 - 0x4];
    TextureAnimationFrame frames[];
} TextureAnimation;

/**
 * A texture of a model and its palette. Its fields join this structure as they are reconstructed.
 */
typedef struct {
    /** The texels. */
    u8 *image;
    /** The 16 colors of the palette, in RGBA 5551. */
    u16 *palette;
    /**
     * Where the palette is on the cartridge, in units of 32 bytes in the low 12 bits, or the frames
     * of an animated texture.
     */
    union {
        u32 paletteRomIndex;
        TextureAnimation *animation;
    } source;
    /** The size, in texels. */
    u16 width;
    u16 height;
    /** The texels the tile covers, in t and s, from the first to the last. */
    s16 tileT0;
    s16 tileS0;
    s16 tileT1;
    s16 tileS1;
    u8 unreconstructed18[0x20 - 0x18];
    /** How the texture wraps and repeats, by bits from the highest. */
    unsigned mirrorS : 1;
    unsigned mirrorT : 1;
    unsigned clampS : 1;
    unsigned clampT : 1;
    /** The TEXTURE_FORMAT_ values. */
    unsigned format : 4;
    unsigned : 3;
    unsigned animated : 1;
    unsigned : 2;
    /** The bytes of the texels. */
    unsigned size : 18;
} ModelTexture;

/** The formats of a texture of a model: 4-bit with a palette of 16, and 8-bit intensity. */
#define TEXTURE_FORMAT_CI4 1
#define TEXTURE_FORMAT_I8 4

/** The format of an animated texture of the track that stays on its first frame in the snow. */
#define TEXTURE_FORMAT_SNOW_STILL 0xB

/** The palette of the texture memory a 4-bit texture of a model loads its colors into. */
#define TEXTURE_CI4_PALETTE 15

/** The clipping ratios from 1 to 6, each a display list that sets it. */
#define CLIP_RATIOS 6

/** The rectangle of the screen a view fills. */
typedef struct {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    /** The vehicle of the player the view follows. */
    s32 vehicleIndex;
} ViewportRect;

/** The next command of the display list. @ghidraAddress USA: 0x8028a858 */
extern Gfx *gDisplayListPtr;

/** The projection of the current view. @ghidraAddress USA: 0x8028a878 */
extern Mtx *g_pProjectionMtx;

/** A matrix the renderer builds before converting it. @ghidraAddress USA: 0x8031ab10 */
extern f32 g_afMvpMatrix[4][4];

/** The perspective of the current view. @ghidraAddress USA: 0x8031aa90 */
extern f32 g_afPerspectiveMatrix[4][4];

/** A matrix the renderer fills as an operand. @ghidraAddress USA: 0x8031ab50 */
extern f32 g_afScratchMatrix[4][4];

/** The texture filter of the other modes. @ghidraAddress USA: 0x8028a898 */
extern u32 g_dwTextureFilterMode;

/** Whether the track is drawn mirrored. @ghidraAddress USA: 0x8028a8ac */
extern s32 g_dwTrackMirrored;

/** Whether the current view is mirrored. @ghidraAddress USA: 0x8028a8a8 */
extern s32 g_dwViewportMirrored;

/** The camera of the view being drawn. @ghidraAddress USA: 0x8028aaf4 */
extern CameraFrame *g_pActiveCamera;

/** The field of view of the cameras, in radians. @ghidraAddress USA: 0x8028aac0 */
extern f32 g_fCameraFovRadians;

/**
 * The distance the far planes of the cameras are taken from; the stages of the menus see a fifth of
 * it.
 *
 * @ghidraAddress USA: 0x8028aac8
 */
extern f32 g_fCameraProjectionScale;

/** The views on the screen, one for each player. @ghidraAddress USA: 0x8028ab0c */
extern s32 g_dwNumViewports;

/** The rectangles of the views. @ghidraAddress USA: 0x8031b2c8 */
extern ViewportRect g_aViewportRects[];

/** The view being drawn. @ghidraAddress USA: 0x8028aaec */
extern s32 g_dwCurrentSplitScreenIndex;

/** The depth scale of the viewport SetViewport() last set. @ghidraAddress USA: 0x8028ab64 */
extern s32 g_dwViewportScaleZ;

/** The depth offset of the viewport SetViewport() last set. @ghidraAddress USA: 0x8028ab68 */
extern s32 g_dwViewportTransZ;

/** The view and projection of the view being drawn. @ghidraAddress USA: 0x8031aa50 */
extern f32 g_afViewProjMatrix[4][4];

/** The normalisation of the perspective of the projection. @ghidraAddress USA: 0x8028a874 */
extern u16 g_wPerspNorm;

/** The distance of the far plane of the cameras. @ghidraAddress USA: 0x8028aacc */
extern f32 gFrustumFar;

/** The angle SetupFrustumCorners() was last given. @ghidraAddress USA: 0x8028aac4 */
extern f32 g_fFrustumFovAngle;

/** The view of the current view, before its perspective. @ghidraAddress USA: 0x8031aad0 */
extern f32 g_ViewLookAtMtx[4][4];

/** @ghidraAddress USA: 0x8028a8c0 */
extern Mtx g_mtxIdentity;

/** The vertices, triangles, and polygons drawn this frame. @ghidraAddress USA: 0x8028aa08 */
extern s32 g_dwStatVerticesDrawn;

/** @ghidraAddress USA: 0x8028aa10 */
extern s32 g_dwStatTrianglesDrawn;

/** @ghidraAddress USA: 0x8028aa38 */
extern s32 g_dwStatPolygonsDrawn;

/**
 * Whether the level of detail of the textures of the track is on.
 *
 * @ghidraAddress USA: 0x8028aa44
 */
extern s32 g_bTrackTextureLod;

/** Whether the track is drawn with smooth shading. @ghidraAddress USA: 0x8028aa48 */
extern s32 g_bSmoothShading;

/** The colour of the light of the sun and of the ambient light. @ghidraAddress USA: 0x8028ab58 */
extern u32 g_dwDirLightColor;

/** @ghidraAddress USA: 0x8028ab5c */
extern u32 g_dwAmbientLightColor;

/** The lights the vehicles and the track start from. @ghidraAddress USA: 0x8028a9f0 */
extern Lights1 g_DefaultLights;

/** The direction of the light of the sun. @ghidraAddress USA: 0x8031b338 */
extern f32 g_afLightDirection[3];

/**
 * The colours of the light of the placed models lit by their light ramp, by its index.
 *
 * @ghidraAddress USA: 0x8031b360
 */
extern u32 g_adwLightRampColors[4];

/** Whether each vehicle is in the view being drawn. @ghidraAddress USA: 0x80351c70 */
extern s32 g_adwVehicleVisible[];

/** The display lists that set each clipping ratio, from 1. @ghidraAddress USA: 0x8028a900 */
extern Gfx g_aClipRatioDisplayLists[CLIP_RATIOS][5];

/** The colour of the light of the sun, in parts. @ghidraAddress USA: 0x8028ab40 */
extern u8 g_bDirLightR;

/** @ghidraAddress USA: 0x8028ab44 */
extern u8 g_bDirLightG;

/** @ghidraAddress USA: 0x8028ab48 */
extern u8 g_bDirLightB;

/** The colour of the ambient light, in parts. @ghidraAddress USA: 0x8028ab4c */
extern u8 g_bAmbientLightR;

/** @ghidraAddress USA: 0x8028ab50 */
extern u8 g_bAmbientLightG;

/** @ghidraAddress USA: 0x8028ab54 */
extern u8 g_bAmbientLightB;

/** The least detailed level the vehicles are drawn at, from 0. @ghidraAddress USA: 0x8028aa9c */
extern s32 g_dwVehicleLodBias;

/**
 * Whether the car of the camera is drawn from its cockpit and mirror cameras too.
 *
 * @ghidraAddress USA: 0x8028aa70
 */
extern s32 g_dwVehicleCullingDisabled;

/** The triangles of the vehicles drawn this frame. @ghidraAddress USA: 0x8028aa3c */
extern s32 g_dwStatVehicleTriangles;

/**
 * Counts of a frame that ClearRenderStats() clears. No routine reads them.
 *
 * @ghidraAddress USA: 0x8028aa0c
 */
extern s32 g_dwStatUnused0;

/** @ghidraAddress USA: 0x8028aa2c */
extern s32 g_dwStatUnused1;

/** @ghidraAddress USA: 0x8028aa30 */
extern s32 g_dwStatUnused2;

/** @ghidraAddress USA: 0x8028aa34 */
extern s32 g_dwStatUnused3;

/** The address in the texture memory DrawTexturedTile() loads to. @ghidraAddress USA: 0x8028ab1c */
extern u32 g_dwTextureTmemAddress;

/** The tile DrawTexturedTile() sets up to draw with. @ghidraAddress USA: 0x8028ab18 */
extern u32 g_dwTextureRenderTile;

/**
 * The depths the fog starts and ends at, from 0 at the near plane to 1000 at the far.
 *
 * @ghidraAddress USA: 0x8028ab30
 */
extern s32 g_dwFogMinDistance;

/** @ghidraAddress USA: 0x8028ab34 */
extern s32 g_dwFogMaxDistance;

/**
 * The offset and the scale that turn the depth of a point into its fog, from 0 to 255, as the
 * microcode does.
 *
 * @ghidraAddress USA: 0x8028ab38
 */
extern s32 g_dwFogFactorOffset;

/** @ghidraAddress USA: 0x8028ab3c */
extern s32 g_dwFogFactorScale;

/**
 * The light ramps of the placed models, in parts, by the index of the ramp.
 *
 * @ghidraAddress USA: 0x8031b350
 */
extern u8 g_abLightRampR[4];

/** @ghidraAddress USA: 0x8031b354 */
extern u8 g_abLightRampG[4];

/** @ghidraAddress USA: 0x8031b358 */
extern u8 g_abLightRampB[4];

/** The viewports. The display lists of the last frames may still use them. */
#define VIEWPORTS 32

/** @ghidraAddress USA: 0x80318cd0 */
extern Vp g_aViewports[VIEWPORTS];

/** The viewport set last. @ghidraAddress USA: 0x8028ab60 */
extern s32 g_dwViewportIndex;

/**
 * The display list of g_aClipRatioDisplayLists a frame starts with.
 *
 * @ghidraAddress USA: 0x8028a8a4
 */
extern s32 g_dwClipRatioIndex;

/** Whether the microcode clips the triangles. @ghidraAddress USA: 0x8028aa50 */
extern s32 g_bGbiClipping;

/**
 * Clear the counts of the vertices, triangles, and polygons drawn in a frame.
 *
 * @ghidraAddress USA: 0x802173c8
 */
void ClearRenderStats(void);

/**
 * Blank the screen and clear both frame buffers in the low resolution.
 *
 * @ghidraAddress USA: 0x8021a540
 */
void ClearFramebuffersLowRes(void);

/**
 * Scale a whole matrix down by the largest magnitude of its first three rows, plus 0.375, so that
 * it fits the fixed point of the matrix the lighting is forced to.
 *
 * @param m The matrix.
 * @ghidraAddress USA: 0x802174b4
 */
void NormalizeLightMatrix(f32 m[4][4]);

/**
 * Load a texture of a model into the texture memory and set up the tiles to draw it.
 *
 * @param index The index of the texture.
 * @param textures The textures of the model.
 * @ghidraAddress USA: 0x80217734
 */
void DrawTexturedTile(s32 index, ModelTexture *textures);

/**
 * How thick the fog is at a point of the world, in the view being drawn.
 *
 * @param point The point.
 * @return The fog, from 0 to 1, or 0 when there is no weather.
 * @ghidraAddress USA: 0x80218c8c
 */
f32 ComputeFogFactorAtPoint(Vec3f *point);

/**
 * The largest absolute value of the first three rows of a matrix.
 *
 * @param m The matrix.
 * @return The largest absolute value of its first 12 elements.
 * @ghidraAddress USA: 0x80217420
 */
f32 GetMaxAbsValue12(f32 m[4][4]);

/**
 * Set the direction and the colours of the lights, and the light ramps, for the weather.
 *
 * @ghidraAddress USA: 0x80218d5c
 */
void SetupWeatherLights(void);

/**
 * Set the colour and the range of the fog for the weather and the track, and add them to the
 * display list.
 *
 * @ghidraAddress USA: 0x802182a8
 */
void SetupFog(void);

/**
 * Load the viewport of the view being drawn again.
 *
 * @ghidraAddress USA: 0x80219f04
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
 */
void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue);

/**
 * Do nothing.
 *
 * @ghidraAddress USA: 0x80217c8c
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
 */
void DrawRdpBorderRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Black out the screen and clear both frame buffers in the high resolution.
 *
 * @ghidraAddress USA: 0x8021a4c8
 */
void ClearFramebuffersHighRes(void);

/**
 * Copy the current frames of the animated textures of the track, and their snow versions, from the
 * cartridge.
 *
 * @ghidraAddress USA: 0x8021a5b8
 */
void StreamAnimatedTrackTextures(void);

#endif

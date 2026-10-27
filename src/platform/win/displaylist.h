#pragma once

/**
 * @file
 * The interpreter of the display lists of the game in the PC port. Each handler runs one command
 * and returns the command to run next.
 */

#include <array>

#include "platform/win/gbicommand.h"
#include "platform/win/textureupload.h"
#include "vecmath.h"

#if TGR_RENDERER == TGR_RENDERER_D3D
#include <d3d.h>
#include <windows.h>
#endif

/** The number of entries of g_apGbiDisplayListReturnStack. */
#define GBI_DISPLAY_LIST_STACK_SIZE 10

/** The depth g_dwGbiMatrixStackDepth takes when a pop empties it. */
#define GBI_MATRIX_STACK_RESET_DEPTH 10

/**
 * The scale of the width of a texture rectangle.
 *
 * @ghidraAddress BRD3D: 0x100a79e8
 * @ghidraAddress BRGlide: 0x100a9a54
 */
extern float g_fTexRectScaleW;

/**
 * The depth of a texture rectangle.
 *
 * @ghidraAddress BRD3D: 0x104c5174
 * @ghidraAddress BRGlide: 0x105d17c4
 */
extern float g_fTexRectDepth;

/**
 * The first word of the combine mode set last.
 *
 * @ghidraAddress BRD3D: 0x104c5158
 * @ghidraAddress BRGlide: 0x105d17ac
 */
extern unsigned g_dwGbiCombineModeHi;

/**
 * The second word of the combine mode set last.
 *
 * @ghidraAddress BRD3D: 0x104c515c
 * @ghidraAddress BRGlide: 0x105d17b0
 */
extern unsigned g_dwGbiCombineModeLo;

/**
 * The geometry mode.
 *
 * @ghidraAddress BRD3D: 0x104c5178
 * @ghidraAddress BRGlide: 0x105d17c8
 */
extern unsigned g_dwGbiGeometryMode;

/**
 * The geometry mode before the last change.
 *
 * @ghidraAddress BRD3D: 0x104c517c
 * @ghidraAddress BRGlide: 0x105d17cc
 */
extern unsigned g_dwGbiPrevGeometryMode;

/**
 * The depth of the stack of matrices.
 *
 * @ghidraAddress BRD3D: 0x100a79dc
 * @ghidraAddress BRGlide: 0x100a9a50
 */
extern int g_dwGbiMatrixStackDepth;

/**
 * The number of entries of g_apGbiDisplayListReturnStack in use.
 *
 * @ghidraAddress BRD3D: 0x104c01a4
 * @ghidraAddress BRGlide: 0x105ccfe8
 */
extern int g_dwGbiDisplayListStackDepth;

/**
 * The commands that follow the calls of display lists, to return to.
 *
 * @ghidraAddress BRD3D: 0x104c16a8
 * @ghidraAddress BRGlide: 0x105ce2e8
 */
extern GbiCommand *g_apGbiDisplayListReturnStack[GBI_DISPLAY_LIST_STACK_SIZE];

/**
 * Choose the routine that submits vertices for a combine mode, and the colour of the combine modes
 * that draw in one colour. One combine mode takes the handler of lit vertices that modulates by the
 * environment colour, and every other gives it up.
 *
 * @param dwCombineHi The first word of the combine mode.
 * @param dwCombineLo The second word of the combine mode.
 * @ghidraAddress BRD3D: 0x1001c820
 * @ghidraAddress BRGlide: 0x1001e7a0
 * @stub{BRGlide}
 */
void SelectVertexSubmitterForCombineMode(unsigned dwCombineHi, unsigned dwCombineLo);

/**
 * Apply the geometry mode after a change: the culling and the comparison of depth of the device
 * when they changed, and the handlers of vertices and triangles in g_apfnGbiHandlers.
 *
 * @ghidraAddress BRD3D: 0x1001e7c0
 * @ghidraAddress BRGlide: 0x1001fd70
 * @stub{BRGlide}
 */
void ApplyGeometryModeChanges(void);

/**
 * Set the scale of the width of a texture rectangle.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cd60
 * @ghidraAddress BRGlide: 0x1001eb10
 */
GbiCommand *HandleGbiSetTexRectW(GbiCommand *pCommand);

/**
 * Set the depth of a texture rectangle.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cd80
 * @ghidraAddress BRGlide: 0x1001eb30
 */
GbiCommand *HandleGbiSetTexRectDepth(GbiCommand *pCommand);

/**
 * Set the combine mode and choose the routine that submits vertices for it.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001c7f0
 * @ghidraAddress BRGlide: 0x1001e770
 */
GbiCommand *HandleGbiSetCombine(GbiCommand *pCommand);

/**
 * Clear bits of the geometry mode.
 *
 * @param pCommand The command, the bits in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001e790
 * @ghidraAddress BRGlide: 0x1001fd40
 */
GbiCommand *HandleGbiClearGeometryMode(GbiCommand *pCommand);

/**
 * Set bits of the geometry mode.
 *
 * @param pCommand The command, the bits in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020f20
 * @ghidraAddress BRGlide: 0x100211e0
 */
GbiCommand *HandleGbiSetGeometryMode(GbiCommand *pCommand);

/**
 * Pop the stack of matrices. A pop that empties it sets its depth to #GBI_MATRIX_STACK_RESET_DEPTH.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020ef0
 * @ghidraAddress BRGlide: 0x100211b0
 */
GbiCommand *HandleGbiPopMatrix(GbiCommand *pCommand);

/**
 * Branch to a display list, or call it when bits 16 to 23 of the first word are clear. A call that
 * fills the last entry of g_apGbiDisplayListReturnStack exits the game.
 *
 * @param pCommand The command.
 * @return The first command of the display list.
 * @ghidraAddress BRD3D: 0x10020d60
 * @ghidraAddress BRGlide: 0x10021020
 */
GbiCommand *HandleGbiDisplayList(GbiCommand *pCommand);

/**
 * Skip a command.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x100243d0
 * @ghidraAddress BRGlide: 0x10021240
 */
GbiCommand *HandleGbiNoOp(GbiCommand *pCommand);

/**
 * The render mode of the console the display lists set last.
 *
 * @ghidraAddress BRD3D: 0x104c1694
 * @ghidraAddress BRGlide: 0x105ce2d4
 */
extern unsigned g_dwGbiRenderMode;

/**
 * Apply the blending, the depth, and the test of alpha of a render mode of the console.
 *
 * @param dwRenderMode The render mode.
 * @ghidraAddress BRD3D: 0x10020fa0
 * @ghidraAddress BRGlide: 0x10021270
 */
void ApplyRdpRenderMode(unsigned dwRenderMode);

/**
 * Set the render mode.
 *
 * @param pCommand The command, the render mode in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020f80
 * @ghidraAddress BRGlide: 0x10021250
 */
GbiCommand *HandleGbiSetRenderMode(GbiCommand *pCommand);

/**
 * Set the low word of the other modes. Only the render mode is applied, and the comparison of
 * alpha and every other mode are skipped.
 *
 * @param pCommand The command, the shift of the mode in bits 8 to 15 of its first word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020f50
 * @ghidraAddress BRGlide: 0x10021210
 */
GbiCommand *HandleGbiSetOtherModeL(GbiCommand *pCommand);

/**
 * Fill a rectangle of the screen with the fill colour.
 *
 * @param nLeft The left edge.
 * @param nTop The top edge.
 * @param nRight The right edge.
 * @param nBottom The bottom edge.
 * @ghidraAddress BRD3D: 0x1001be90
 * @ghidraAddress BRGlide: 0x1001e380
 */
void DrawFillRectangle(int nLeft, int nTop, int nRight, int nBottom);

/**
 * Fill a rectangle given in whole pixels of the console, whose rows run up from the bottom of the
 * screen of the PC.
 *
 * @param pCommand The command, the lower right corner in its first word and the upper left in its
 * second, as signed fields of 12 bits.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001c7a0
 * @ghidraAddress BRGlide: 0x1001e720
 */
GbiCommand *HandleGbiFillRectangleIntCoords(GbiCommand *pCommand);

/**
 * Fill a rectangle given in pixels of the console with two bits of fraction, whose rows run up from
 * the bottom of the screen of the PC. The fractions are dropped.
 *
 * @param pCommand The command, the lower right corner in its first word and the upper left in its
 * second.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001be30
 * @ghidraAddress BRGlide: 0x1001e320
 */
GbiCommand *HandleGbiFillRectangle(GbiCommand *pCommand);

/**
 * The red of the fill colour.
 *
 * @ghidraAddress BRD3D: 0x104bbf00
 * @ghidraAddress BRGlide: 0x105ccd40
 */
extern unsigned char g_bGbiFillColorR;

/**
 * The green of the fill colour.
 *
 * @ghidraAddress BRD3D: 0x104bc194
 * @ghidraAddress BRGlide: 0x105ccfd8
 */
extern unsigned char g_bGbiFillColorG;

/**
 * The blue of the fill colour.
 *
 * @ghidraAddress BRD3D: 0x104c5150
 * @ghidraAddress BRGlide: 0x105d17a0
 */
extern unsigned char g_bGbiFillColorB;

/**
 * The alpha of the fill colour, 0 or 255.
 *
 * @ghidraAddress BRD3D: 0x104c15cc
 * @ghidraAddress BRGlide: 0x105ce208
 */
extern unsigned char g_bGbiFillColorA;

/**
 * Set the fill colour from the RGBA 5551 colour in the low 16 bits of the command.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cc00
 * @ghidraAddress BRGlide: 0x1001e9f0
 */
GbiCommand *HandleGbiSetFillColor(GbiCommand *pCommand);

/**
 * The red of the environment colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104bbf04
 * @ghidraAddress BRGlide: 0x105ccd44
 */
extern float g_fGbiEnvColorR;

/**
 * The green of the environment colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104c0bac
 * @ghidraAddress BRGlide: 0x105cd9f4
 */
extern float g_fGbiEnvColorG;

/**
 * The blue of the environment colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104bbeb8
 * @ghidraAddress BRGlide: 0x105cccf8
 */
extern float g_fGbiEnvColorB;

/**
 * The alpha of the environment colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104bbe2c
 * @ghidraAddress BRGlide: 0x105ccc74
 */
extern float g_fGbiEnvColorA;

/**
 * The red of the primitive colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104c5154
 * @ghidraAddress BRGlide: 0x105d17a4
 */
extern float g_fGbiPrimColorR;

/**
 * The green of the primitive colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104c5160
 * @ghidraAddress BRGlide: 0x105d17b4
 */
extern float g_fGbiPrimColorG;

/**
 * The blue of the primitive colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104c1690
 * @ghidraAddress BRGlide: 0x105ce2d0
 */
extern float g_fGbiPrimColorB;

/**
 * The alpha of the primitive colour, from 0 to 1.
 *
 * @ghidraAddress BRD3D: 0x104c0ba8
 * @ghidraAddress BRGlide: 0x105cd9f0
 */
extern float g_fGbiPrimColorA;

/**
 * Set the environment colour from the RGBA colour of the command.
 *
 * @param pCommand The command, the colour in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cb40
 * @ghidraAddress BRGlide: 0x1001e930
 */
GbiCommand *HandleGbiSetEnvColor(GbiCommand *pCommand);

/**
 * Draw a rectangle of a tile of texture.
 *
 * @param nUpperLeftX The left edge.
 * @param nUpperLeftY The top edge.
 * @param nLowerRightX The right edge.
 * @param nLowerRightY The bottom edge.
 * @param nTile The tile.
 * @ghidraAddress BRD3D: 0x10021560
 * @ghidraAddress BRGlide: 0x100215c0
 */
void DrawTextureRectangle(
    int nUpperLeftX, int nUpperLeftY, int nLowerRightX, int nLowerRightY, int nTile);

/**
 * Draw a texture rectangle of a command of one word pair, its corners in pixels with two bits of
 * fraction, as DrawTextureRectangle() reads them with four.
 *
 * @param pCommand The command, the lower right corner in its first word, and the tile and the upper
 * left corner in its second.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10021b80
 * @ghidraAddress BRGlide: 0x100219d0
 */
GbiCommand *HandleGbiTextureRectangle(GbiCommand *pCommand);

/**
 * Draw a texture rectangle of the console, which takes three commands, its corners as the console
 * gives them.
 *
 * @param pCommand The first command, the lower right corner in its first word, and the tile and the
 * upper left corner in its second.
 * @return The command after the three.
 * @ghidraAddress BRD3D: 0x10021510
 * @ghidraAddress BRGlide: 0x10021570
 */
GbiCommand *HandleGbiTextureRectangleNative(GbiCommand *pCommand);

/**
 * The scale of x of the viewport, in pixels.
 *
 * @ghidraAddress BRD3D: 0x104bbf08
 * @ghidraAddress BRGlide: 0x105ccd48
 */
extern float g_fViewportScaleX;

/**
 * The scale of y of the viewport, in pixels, negative for Direct3D.
 *
 * @ghidraAddress BRD3D: 0x104bc198
 * @ghidraAddress BRGlide: 0x105ccfdc
 */
extern float g_fViewportScaleY;

/**
 * The translation of x of the viewport, in pixels.
 *
 * @ghidraAddress BRD3D: 0x104c0bb0
 * @ghidraAddress BRGlide: 0x105cd9f8
 */
extern float g_fViewportTranslateX;

/**
 * The translation of y of the viewport, in pixels, from the bottom of the screen for Glide.
 *
 * @ghidraAddress BRD3D: 0x104c0bb8
 * @ghidraAddress BRGlide: 0x105cd9fc
 */
extern float g_fViewportTranslateY;

/**
 * Set the viewport. Direct3D turns its y over through the scale, and Glide through the
 * translation from the height of the screen.
 *
 * @param pCommand The command, the viewport in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10024260
 * @ghidraAddress BRGlide: 0x10023920
 */
GbiCommand *HandleGbiSetViewport(GbiCommand *pCommand);

/** A light of the display lists, in the shared console format. */
using GbiLight = Light;

/** The number of entries of g_aGbiLights. */
#define GBI_LIGHT_COUNT 8

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * @ghidraAddress BRD3D: 0x1002c190
 */
void HalveGbiTriangleVertexIndices(GbiCommand *pCommand);

/**
 * @ghidraAddress BRD3D: 0x1002c1b0
 */
void HalveGbiTrianglePairVertexIndices(GbiCommand *pCommand);
#else
/**
 * Halve each channel of the second copy of the colour of a light.
 *
 * @param pLight The light.
 * @ghidraAddress BRGlide: 0x10019250
 */
void HalveAmbientLightColor(GbiLight *pLight);

/**
 * Halve each channel of both copies of the colour of a light.
 *
 * @param pLight The light.
 * @ghidraAddress BRGlide: 0x10019270
 */
void HalveLightColors(GbiLight *pLight);
#endif

/**
 * The number of lights the display lists set, as their command gives it.
 *
 * @ghidraAddress BRD3D: 0x104bc190
 * @ghidraAddress BRGlide: 0x105ccfd0
 */
extern int g_dwGbiNumLights;

/**
 * The lights of the display lists.
 *
 * @ghidraAddress BRD3D: 0x104bbe38
 * @ghidraAddress BRGlide: 0x105ccc78
 */
extern GbiLight g_aGbiLights[GBI_LIGHT_COUNT];

/**
 * Whether the lights were transformed since they last changed.
 *
 * @ghidraAddress BRD3D: 0x104c5180
 * @ghidraAddress BRGlide: 0x105d17d0
 */
extern int g_bGbiLightsTransformed;

/**
 * Set the number of lights or the colour of a light. Every other word is skipped.
 *
 * @param pCommand The command, the index of the word in the low byte of its first word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x100242f0
 * @ghidraAddress BRGlide: 0x100239c0
 */
GbiCommand *HandleGbiMoveWord(GbiCommand *pCommand);

/**
 * The matrix that takes a vertex of the display lists to the screen.
 *
 * @ghidraAddress BRD3D: 0x104c4d10
 * @ghidraAddress BRGlide: 0x105d1760
 */
extern float g_GbiMvpMatrix[4][4];

#if TGR_RENDERER == TGR_RENDERER_D3D
/** @ghidraAddress BRD3D: 0x104bbec0 */
extern float g_GbiProjectionMatrix[4][4];

/** @ghidraAddress BRD3D: 0x104bbf10 */
extern float g_aGbiModelViewStack[GBI_MATRIX_STACK_RESET_DEPTH][4][4];
#endif

/**
 * The second vector of the look-at of the display lists, as a move memory command gives it.
 *
 * @ghidraAddress BRD3D: 0x104c1698
 * @ghidraAddress BRGlide: 0x105ce2d8
 */
extern const Light *g_pGbiLookAtY;

/**
 * The first vector of the look-at of the display lists, as a move memory command gives it.
 *
 * @ghidraAddress BRD3D: 0x104c169c
 * @ghidraAddress BRGlide: 0x105ce2dc
 */
extern const Light *g_pGbiLookAtX;

/**
 * Copy the matrix of the command into g_GbiMvpMatrix.
 *
 * @param pCommand The command, the matrix in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10024240
 * @ghidraAddress BRGlide: 0x10023900
 */
GbiCommand *HandleGbiLoadMatrix(GbiCommand *pCommand);

/**
 * Set the viewport, a vector of the look-at, a light, or g_GbiMvpMatrix from memory. Every other
 * index is skipped.
 *
 * @param pCommand The command, the index in bits 16 to 23 of its first word and the size of a light
 * in its low 16 bits.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10024150
 * @ghidraAddress BRGlide: 0x10023810
 */
GbiCommand *HandleGbiMoveMem(GbiCommand *pCommand);

/**
 * The left edge of the tile, in texels with two bits of fraction.
 *
 * @ghidraAddress BRD3D: 0x118aa080
 * @ghidraAddress BRGlide: 0x118ed198
 */
extern int g_dwTileSizeUls;

/**
 * The top edge of the tile, in texels with two bits of fraction.
 *
 * @ghidraAddress BRD3D: 0x11829838
 * @ghidraAddress BRGlide: 0x1186c950
 */
extern int g_dwTileSizeUlt;

/**
 * The right edge of the tile, in texels with two bits of fraction.
 *
 * @ghidraAddress BRD3D: 0x1182983c
 * @ghidraAddress BRGlide: 0x1186c954
 */
extern int g_dwTileSizeLrs;

/**
 * The bottom edge of the tile, in texels with two bits of fraction.
 *
 * @ghidraAddress BRD3D: 0x118a9870
 * @ghidraAddress BRGlide: 0x118ec988
 */
extern int g_dwTileSizeLrt;

/**
 * The width of the tile, in whole texels.
 *
 * @ghidraAddress BRD3D: 0x11829840
 * @ghidraAddress BRGlide: 0x1186c958
 */
extern int g_dwTileSizeWidth;

/**
 * The height of the tile, in whole texels.
 *
 * @ghidraAddress BRD3D: 0x118aa094
 * @ghidraAddress BRGlide: 0x118ed1ac
 */
extern int g_dwTileSizeHeight;

/**
 * Set the edges of the tile, and its width and height in whole texels.
 *
 * @param pCommand The command, the upper left corner in its first word and the lower right in its
 * second, as signed fields of 12 bits.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cf30
 * @ghidraAddress BRGlide: 0x1001ec30
 */
GbiCommand *HandleGbiSetTileSize(GbiCommand *pCommand);

/**
 * Upload a texture the game loaded to its surface, set by InitializeTextureSubsystem().
 *
 * @ghidraAddress BRD3D: 0x118aa0b8
 * @ghidraAddress BRGlide: 0x118ed1d0
 */
extern void (*g_pfnUploadTextureEntryToSurface)(int nEntry, TextureUploadData payload);

/**
 * Upload a texture the game loaded to its surface.
 *
 * @param pCommand The command, the entry of the texture in the low 24 bits of its first word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001be10
 * @ghidraAddress BRGlide: 0x1001e300
 */
GbiCommand *HandleGbiLoadPreparedTexture(GbiCommand *pCommand);

#if TGR_RENDERER == TGR_RENDERER_D3D
/** The vertex a command of the display lists loaded. The layout is not recovered in full. */
struct GbiVertexSource {
    GbiVertexSource *pNext;
    float fX;
    float fY;
    float fZ;
    float fS; /*!< The s of the texture before g_fTexCoordScaleS. */
    float fT; /*!< The t of the texture before g_fTexCoordScaleT. */
    float fW;
    float fRed;
    float fGreen;
    float fBlue;
};

/** A vertex of the display lists after its transform. The layout is not recovered in full. */
struct GbiVertex {
    float fX;               /*!< The x on the screen. */
    float fY;               /*!< The y on the screen. */
    float fZ;               /*!< The depth. */
    float fRed;             /*!< The red, from 0 to 1. */
    float fGreen;           /*!< The green, from 0 to 1. */
    float fBlue;            /*!< The blue, from 0 to 1. */
    char abUnknown18[8];    /* +0x18 */
    float fRhw;             /*!< The reciprocal of the homogeneous w. */
    float fU;               /*!< The u of the texture. */
    float fV;               /*!< The v of the texture. */
    char abUnknown2C[0x10]; /* +0x2C */
    int nClipFlags;         /*!< The planes of the view the vertex lies outside, a bit for each. */
    GbiVertexSource source; /*!< The vertex as the command loaded it. */
    int nRenderBufferIndex; /*!< The entry of g_renderBuffer of the vertex, -1 for none. */
};

/** @ghidraAddress BRD3D: 0x104c15d0 */
extern float g_fGbiLight0ColorR;
/** @ghidraAddress BRD3D: 0x104c15d4 */
extern float g_fGbiLight0ColorG;
/** @ghidraAddress BRD3D: 0x104c15d8 */
extern float g_fGbiLight0ColorB;
/** @ghidraAddress BRD3D: 0x104c15dc */
extern Vec3f g_GbiLight0Direction;
/** @ghidraAddress BRD3D: 0x104c15e8 */
extern float g_fGbiAmbientColorR;
/** @ghidraAddress BRD3D: 0x104c15ec */
extern float g_fGbiAmbientColorG;
/** @ghidraAddress BRD3D: 0x104c15f0 */
extern float g_fGbiAmbientColorB;

/**
 * Light a vertex normal with the first directional light and the ambient colour.
 *
 * @ghidraAddress BRD3D: 0x10022350
 */
void ComputeVertexDirectionalLight(const ConvertedModelVertex *pVertex, GbiVertexSource *pDest);

/**
 * @ghidraAddress BRD3D: 0x100a79e0
 */
extern float g_fDepthScale;
/**
 * @ghidraAddress BRD3D: 0x100a79e4
 */
extern float g_fDepthOffset;

/**
 * @ghidraAddress BRD3D: 0x10022dc0
 */
int ComputeVertexClipCodes(const GbiVertexSource *pVertex);

/**
 * @ghidraAddress BRD3D: 0x100222d0
 */
void TransformVertexToScreen(
    GbiVertex *pDest, const GbiVertexSource *pSource, float fRed, float fGreen, float fBlue);

/** The number of entries of g_aGbiVertexCache and g_apVertexPool. */
#define GBI_VERTEX_CACHE_SIZE 128

#define GBI_CLIP_VERTEX_POOL_SIZE 64

/** @ghidraAddress BRD3D: 0x104c01a8 */
extern GbiVertexSource g_aClipVertexPool[GBI_CLIP_VERTEX_POOL_SIZE];

/** @ghidraAddress BRD3D: 0x104c0bbc */
extern GbiVertexSource *g_pVertexFreeList;

/**
 * Take a clipping vertex from the free list and interpolate its attributes.
 *
 * @ghidraAddress BRD3D: 0x1001d940
 */
GbiVertexSource *
LerpVertexFromPool(const GbiVertexSource *pFirst, const GbiVertexSource *pSecond, float fFraction);

/** A circular list of vertices undergoing homogeneous clipping. */
struct GbiClipPolygon {
    GbiVertexSource *pHead;
    int nCount;
};

/** @ghidraAddress BRD3D: 0x1001d810 */
void ClipPolygonPositiveWPlane(GbiClipPolygon *pPolygon);
/** @ghidraAddress BRD3D: 0x1001d9f0 */
void ClipPolygonLeftPlane(GbiClipPolygon *pPolygon);
/** @ghidraAddress BRD3D: 0x1001db30 */
void ClipPolygonRightPlane(GbiClipPolygon *pPolygon);
/** @ghidraAddress BRD3D: 0x1001dc70 */
void ClipPolygonBottomPlane(GbiClipPolygon *pPolygon);
/** @ghidraAddress BRD3D: 0x1001ddb0 */
void ClipPolygonTopPlane(GbiClipPolygon *pPolygon);
/** @ghidraAddress BRD3D: 0x1001def0 */
void ClipPolygonNearZPlane(GbiClipPolygon *pPolygon);
/** @ghidraAddress BRD3D: 0x1001e030 */
void ClipPolygonFarZPlane(GbiClipPolygon *pPolygon);

/** @ghidraAddress BRD3D: 0x104c5184 */
extern int g_bRdpBlendStateOverride;

/** The number of entries of g_renderBuffer and g_awRenderIndexBuffer. */
#define RENDER_BUFFER_SIZE 512

/**
 * The vertices the display lists loaded.
 *
 * @ghidraAddress BRD3D: 0x104c16d0
 */
extern GbiVertex g_aGbiVertexCache[GBI_VERTEX_CACHE_SIZE];

/**
 * The vertices of g_aGbiVertexCache copied into g_renderBuffer since the last flush.
 *
 * @ghidraAddress BRD3D: 0x104c0bc0
 */
extern GbiVertex *g_apVertexPool[GBI_VERTEX_CACHE_SIZE];

/**
 * The number of entries of g_apVertexPool in use.
 *
 * @ghidraAddress BRD3D: 0x104c5190
 */
extern int g_nVertexPoolCount;

/**
 * The vertices waiting to be drawn.
 *
 * @ghidraAddress BRD3D: 0x104bc1a0
 */
extern D3DTLVERTEX g_renderBuffer[RENDER_BUFFER_SIZE];

/**
 * The number of entries of g_renderBuffer in use.
 *
 * @ghidraAddress BRD3D: 0x104c5188
 */
extern int g_nRenderBufferIndex;

/**
 * The indices of the triangles waiting to be drawn, into g_renderBuffer.
 *
 * @ghidraAddress BRD3D: 0x104c4d50
 */
extern WORD g_awRenderIndexBuffer[RENDER_BUFFER_SIZE];

/**
 * The number of entries of g_awRenderIndexBuffer in use.
 *
 * @ghidraAddress BRD3D: 0x104c518c
 */
extern int g_nRenderIndexCount;

/**
 * Empty g_renderBuffer and g_awRenderIndexBuffer, and mark each vertex of g_apVertexPool as not in
 * g_renderBuffer.
 *
 * @ghidraAddress BRD3D: 0x1001c640
 */
void ResetRenderBufferAndVertexPool(void);

/**
 * The scale from the reciprocal of w of a vertex to the alpha of its fog.
 *
 * @ghidraAddress BRD3D: 0x104c15c8
 */
extern float g_fVertexFogRange;

/**
 * The alpha of fog below which a vertex takes none.
 *
 * @ghidraAddress BRD3D: 0x104bbe30
 */
extern int g_dwFogAlphaCutoff;

/**
 * Set the device fog state and the vertex fog range for the active weather.
 *
 * @ghidraAddress BRD3D: 0x100243e0
 */
void UpdateD3DFogRenderState(void);

/**
 * Copy a vertex into g_renderBuffer with its colour. A vertex of g_aGbiVertexCache already copied
 * since the last flush is not copied again.
 *
 * @param pVertex The vertex.
 * @return The entry of g_renderBuffer of the vertex.
 * @ghidraAddress BRD3D: 0x1001bc90
 */
int SubmitVertexToRenderBuffer(GbiVertex *pVertex);

/**
 * Copy a vertex into g_renderBuffer with its colour and its fog, as SubmitVertexToRenderBuffer()
 * does.
 *
 * @param pVertex The vertex.
 * @return The entry of g_renderBuffer of the vertex.
 * @ghidraAddress BRD3D: 0x1001c690
 */
int SubmitVertexToRenderBufferEx(GbiVertex *pVertex);

/**
 * Copy a vertex into g_renderBuffer with g_dwVertexConstantColor, as SubmitVertexToRenderBuffer()
 * does.
 *
 * @param pVertex The vertex.
 * @return The entry of g_renderBuffer of the vertex.
 * @ghidraAddress BRD3D: 0x1001ca10
 */
int SubmitVertexToRenderBufferConstColor(GbiVertex *pVertex);

/**
 * Copy a vertex into g_renderBuffer with g_dwVertexConstantColor and its fog, as
 * SubmitVertexToRenderBuffer() does.
 *
 * @param pVertex The vertex.
 * @return The entry of g_renderBuffer of the vertex.
 * @ghidraAddress BRD3D: 0x1001ca90
 */
int SubmitVertexToRenderBufferFogged(GbiVertex *pVertex);

/**
 * The routine that copies a vertex into g_renderBuffer, chosen for the combine mode.
 *
 * @ghidraAddress BRD3D: 0x100a79ec
 */
extern int (*g_pfnSubmitVertexToRenderBuffer)(GbiVertex *pVertex);

/**
 * The scale from the s of a vertex to its u.
 *
 * @ghidraAddress BRD3D: 0x118aa08c
 */
extern float g_fTexCoordScaleS;

/**
 * The scale from the t of a vertex to its v.
 *
 * @ghidraAddress BRD3D: 0x118aa090
 */
extern float g_fTexCoordScaleT;

/**
 * Scale the texture coordinates of a vertex as it was loaded into those of the vertex.
 *
 * @param pVertex The vertex.
 * @param pSource The vertex as it was loaded.
 * @ghidraAddress BRD3D: 0x1001e4b0
 */
void ScaleVertexTexCoords(GbiVertex *pVertex, const GbiVertexSource *pSource);

/**
 * Clip a triangle to the planes of the view and submit what remains.
 *
 * @param pVertex0 The first vertex.
 * @param pVertex1 The second vertex.
 * @param pVertex2 The third vertex.
 * @ghidraAddress BRD3D: 0x1001d420
 */
void ClipAndSubmitTriangle(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2);

/**
 * Clip and submit a triangle with one colour for all surviving vertices.
 *
 * @ghidraAddress BRD3D: 0x1001eeb0
 */
void ClipPolygonAndSubmitFlat(GbiVertex *pVertex0,
                              GbiVertex *pVertex1,
                              GbiVertex *pVertex2,
                              float fRed,
                              float fGreen,
                              float fBlue);

/**
 * Clip and submit a triangle with one colour at the fixed far depth.
 *
 * @ghidraAddress BRD3D: 0x1001f7e0
 */
void ClipPolygonAndSubmitDepthScaled(GbiVertex *pVertex0,
                                     GbiVertex *pVertex1,
                                     GbiVertex *pVertex2,
                                     float fRed,
                                     float fGreen,
                                     float fBlue);

/**
 * Submit a triangle to g_renderBuffer, its vertices from the last, and draw the triangles waiting
 * when either buffer has no room for three more entries.
 *
 * @param pVertex0 The first vertex.
 * @param pVertex1 The second vertex.
 * @param pVertex2 The third vertex.
 * @ghidraAddress BRD3D: 0x1001e4e0
 */
void SubmitTriangleToRenderBuffer(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2);

/**
 * Submit a triangle of g_aGbiVertexCache. A triangle outside one plane of the view is dropped, one
 * across a plane is clipped, and the texture coordinates of one inside every plane are scaled
 * first.
 *
 * @param nVertex0 The entry of the first vertex.
 * @param nVertex1 The entry of the second vertex.
 * @param nVertex2 The entry of the third vertex.
 * @ghidraAddress BRD3D: 0x1001e5d0
 */
void SubmitIndexedTriangle(int nVertex0, int nVertex1, int nVertex2);

/**
 * Clip a triangle to the planes of the view and submit what remains at the far depth.
 *
 * @param pVertex0 The first vertex.
 * @param pVertex1 The second vertex.
 * @param pVertex2 The third vertex.
 * @ghidraAddress BRD3D: 0x1001ff90
 */
void ClipAndSubmitTriangleAtFarDepth(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2);

/**
 * Submit a triangle of g_aGbiVertexCache as SubmitIndexedTriangle() does, clipping one across a
 * plane of the view through ClipAndSubmitTriangleAtFarDepth().
 *
 * @param nVertex0 The entry of the first vertex.
 * @param nVertex1 The entry of the second vertex.
 * @param nVertex2 The entry of the third vertex.
 * @ghidraAddress BRD3D: 0x10020ba0
 */
void SubmitIndexedTriangleRasterized(int nVertex0, int nVertex1, int nVertex2);

/**
 * Draw the triangles waiting in g_renderBuffer, when there are any, and empty it.
 *
 * @ghidraAddress BRD3D: 0x1001d3a0
 */
void FlushIndexedRenderBuffer(void);

/**
 * Give the device each render state marked in g_dwDirtyRenderStateMask, after drawing the triangles
 * waiting, and clear the mask.
 *
 * @ghidraAddress BRD3D: 0x10024850
 */
void FlushDirtyRenderStates(void);

/** The size of g_szDebugOverlayText, the room before the next datum. */
#define DEBUG_OVERLAY_TEXT_SIZE 0x408

/**
 * The text drawn over each frame. The game never writes it.
 *
 * @ghidraAddress BRD3D: 0x118ab478
 * @ghidraAddress BRGlide: 0x118ee590
 */
extern char g_szDebugOverlayText[DEBUG_OVERLAY_TEXT_SIZE];

/**
 * The comparison of the test of alpha the display lists start a frame with.
 *
 * @ghidraAddress BRD3D: 0x104bbe28
 */
extern DWORD g_dwD3DAlphaFunc;

/** The number of entries of g_adwFpsFrameTimeSamples. */
#define FPS_FRAME_TIME_SAMPLE_COUNT 120

/**
 * The number of frames the frame rate is averaged over.
 *
 * @ghidraAddress BRD3D: 0x100b5420
 * @ghidraAddress BRGlide: 0x100b4c28
 */
extern int g_dwFpsSampleFrames;

/**
 * The entry of g_adwFpsFrameTimeSamples written last, negative before the first frame.
 *
 * @ghidraAddress BRD3D: 0x100b5424
 * @ghidraAddress BRGlide: 0x100b4c2c
 */
extern int g_dwFpsFrameCountdown;

/**
 * The times of the last frames, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x10b4ffe8
 * @ghidraAddress BRGlide: 0x10b73348
 */
extern DWORD g_adwFpsFrameTimeSamples[FPS_FRAME_TIME_SAMPLE_COUNT];

/**
 * The time the last frame was shown, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x104c5194
 * @ghidraAddress BRGlide: 0x105d17dc
 */
extern DWORD g_dwGbiLastFrameTime;

/**
 * The time between the last two frames shown, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x104c5198
 */
extern DWORD g_dwGbiFrameDeltaTime;

/**
 * Draw a frame of the game with Direct3D: start the scene, set the render states the display lists
 * start from, run the display list and the text over the frame, end the scene, show the frame, and
 * record its time. Surfaces lost are restored, and the frame is dropped when the scene cannot start
 * or end.
 *
 * @param pDisplayList The display list of the frame.
 * @ghidraAddress BRD3D: 0x10024550
 */
void RenderGbiDisplayListFrame(GbiCommand *pDisplayList);

/**
 * Bind a texture the game loaded, set by InitializeTextureSubsystem().
 *
 * @ghidraAddress BRD3D: 0x118aa0b4
 */
extern void (*g_pfnBindTextureEntry)(int nEntry);
#endif

/**
 * Draw the triangles waiting, and bind a texture the game loaded in place of the commands that load
 * it on the console.
 *
 * @param pCommand The command, the entry of g_pTextureEntries in the low 24 bits of its first word
 * and the number of commands it replaces in its second.
 * @return The command after those it replaces.
 * @ghidraAddress BRD3D: 0x1001bd70
 * @ghidraAddress BRGlide: 0x1001e2e0
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiBindTextureEntry(GbiCommand *pCommand);

/**
 * Set the primitive colour from the RGBA colour of the command.
 *
 * @param pCommand The command, the colour in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cca0
 * @ghidraAddress BRGlide: 0x1001ea80
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiSetPrimColor(GbiCommand *pCommand);

/**
 * Set the colour of the fog of the device from the RGBA colour of the command.
 *
 * @param pCommand The command, the colour in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cc70
 * @ghidraAddress BRGlide: 0x1001ea60
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiSetFogColor(GbiCommand *pCommand);

/**
 * The left edge of the scissor, in pixels.
 *
 * @ghidraAddress BRD3D: 0x104c516c
 * @ghidraAddress BRGlide: 0x105d17bc
 */
extern int g_dwScissorLeft;

/**
 * The top edge of the scissor, in pixels from the bottom of the screen.
 *
 * @ghidraAddress BRD3D: 0x104c01a0
 * @ghidraAddress BRGlide: 0x105ccfe0
 */
extern int g_dwScissorTop;

/**
 * The right edge of the scissor, in pixels.
 *
 * @ghidraAddress BRD3D: 0x104c5164
 * @ghidraAddress BRGlide: 0x105d17b8
 */
extern int g_dwScissorRight;

/**
 * The bottom edge of the scissor, in pixels from the bottom of the screen.
 *
 * @ghidraAddress BRD3D: 0x104c5170
 * @ghidraAddress BRGlide: 0x105d17c0
 */
extern int g_dwScissorBottom;

/**
 * Set the scissor from corners in pixels of the console with two bits of fraction, dropping the
 * fractions, and clip the device to it.
 *
 * @param pCommand The command, the upper left corner in its first word and the lower right in its
 * second.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cda0
 * @ghidraAddress BRGlide: 0x1001eb50
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiSetScissor(GbiCommand *pCommand);

/**
 * Set the scissor from corners in whole pixels of the console, and clip the device to it.
 *
 * @param pCommand The command, the upper left corner in its first word and the lower right in its
 * second.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001ce70
 * @ghidraAddress BRGlide: 0x1001ebc0
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiSetScissorIntCoords(GbiCommand *pCommand);

/**
 * Return from a display list to the command after its call.
 *
 * @param pCommand The command.
 * @return The command after the call, or `nullptr` at the end of the display list of the frame.
 * @ghidraAddress BRD3D: 0x10020da0
 * @ghidraAddress BRGlide: 0x10021060
 */
GbiCommand *HandleGbiEndDisplayList(GbiCommand *pCommand);

/**
 * Multiply or load the projection or the model view matrix, and update g_GbiMvpMatrix.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020dc0
 * @ghidraAddress BRGlide: 0x10021080
 */
GbiCommand *HandleGbiMatrix(GbiCommand *pCommand);

/**
 * Load vertices without lighting.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10021bd0
 * @ghidraAddress BRGlide: 0x10021a20
 */
GbiCommand *HandleGbiVertexUnlit(GbiCommand *pCommand);

/**
 * Draw one triangle with smooth shading, after giving the device the dirty render states.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of the triangle in the low three
 * bytes of its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cff0
 * @ghidraAddress BRGlide: 0x1001ecf0
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri1Smooth(GbiCommand *pCommand);

/**
 * Draw two triangles with smooth shading, after giving the device the dirty render states.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of each triangle in the low three
 * bytes of each word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001e170
 * @ghidraAddress BRGlide: 0x1001fa30
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri2Smooth(GbiCommand *pCommand);

/**
 * Load vertices with lighting.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10021e80
 */
GbiCommand *HandleGbiVertexLit(GbiCommand *pCommand);

/**
 * Load vertices with lighting modulated by the environment colour.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10022480
 * @ghidraAddress BRGlide: 0x100221d0
 */
GbiCommand *HandleGbiVertexLitEnvColorModulated(GbiCommand *pCommand);

/**
 * Load vertices that generate their texture coordinates.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x100228f0
 * @ghidraAddress BRGlide: 0x10022600
 */
GbiCommand *HandleGbiVertexTexGen(GbiCommand *pCommand);

/**
 * Generate tile-relative texture coordinates from the transformed vertex normal.
 *
 * @param pInput The converted model vertex.
 * @param pSource The vertex that receives the texture coordinates.
 * @ghidraAddress BRD3D: 0x10022e70
 */
void GenerateGbiVertexTextureCoordinates(const ConvertedModelVertex *pInput,
                                         GbiVertexSource *pSource);

/**
 * Generate texture coordinates with the linear-mode vertical arcsine mapping.
 *
 * @param pInput The converted model vertex.
 * @param pSource The vertex that receives the texture coordinates.
 * @ghidraAddress BRD3D: 0x100236a0
 */
void GenerateGbiVertexTextureCoordinatesLinear(const ConvertedModelVertex *pInput,
                                               GbiVertexSource *pSource);

/**
 * Load vertices that generate their texture coordinates linearly.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x100231d0
 * @ghidraAddress BRGlide: 0x10022bf0
 */
GbiCommand *HandleGbiVertexTexGenLinear(GbiCommand *pCommand);

/**
 * Load vertices with lighting without the depth buffer.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10023cc0
 * @ghidraAddress BRGlide: 0x10023360
 */
GbiCommand *HandleGbiVertexLitNoZBuffer(GbiCommand *pCommand);

/**
 * Load vertices without lighting without the depth buffer.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10023a10
 * @ghidraAddress BRGlide: 0x10023110
 */
GbiCommand *HandleGbiVertexUnlitNoZBuffer(GbiCommand *pCommand);

/**
 * Draw a triangle of g_aGbiVertexCache in the colour of its first vertex.
 *
 * @param nVertex0 The entry of the first vertex, whose colour the triangle takes.
 * @param nVertex1 The entry of the second vertex.
 * @param nVertex2 The entry of the third vertex.
 * @ghidraAddress BRD3D: 0x1001ec20
 * @ghidraAddress BRGlide: 0x1001ff60
 */
void DrawFlatShadedTriangle(int nVertex0, int nVertex1, int nVertex2);

/**
 * Draw one triangle with flat shading, after giving the device the dirty render states. The top
 * byte of the second word chooses the vertex whose colour the triangle takes.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of the triangle in the low three
 * bytes of its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001e980
 * @ghidraAddress BRGlide: 0x1001fef0
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri1Flat(GbiCommand *pCommand);

/**
 * Draw two triangles with flat shading, after giving the device the dirty render states.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of each triangle in the low three
 * bytes of each word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020380
 * @ghidraAddress BRGlide: 0x10020cf0
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri2Flat(GbiCommand *pCommand);

/**
 * Draw a triangle of g_aGbiVertexCache in the colour of its first vertex, without the depth buffer.
 *
 * @param nVertex0 The entry of the first vertex, whose colour the triangle takes.
 * @param nVertex1 The entry of the second vertex.
 * @param nVertex2 The entry of the third vertex.
 * @ghidraAddress BRD3D: 0x1001f550
 * @ghidraAddress BRGlide: 0x10020460
 */
void DrawFlatShadedTriangleDepthScaled(int nVertex0, int nVertex1, int nVertex2);

/**
 * Draw one triangle with smooth shading without the depth buffer, after giving the device the
 * dirty render states.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of the triangle in the low three
 * bytes of its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001fbe0
 * @ghidraAddress BRGlide: 0x10020900
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri1SmoothNoZBuffer(GbiCommand *pCommand);

/**
 * Draw two triangles with smooth shading without the depth buffer, after giving the device the
 * dirty render states.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of each triangle in the low three
 * bytes of each word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020860
 * @ghidraAddress BRGlide: 0x10020d70
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri2SmoothNoZBuffer(GbiCommand *pCommand);

/**
 * Draw one triangle with flat shading without the depth buffer, after giving the device the dirty
 * render states. The top byte of the second word chooses the vertex whose colour the triangle
 * takes.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of the triangle in the low three
 * bytes of its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001f2b0
 * @ghidraAddress BRGlide: 0x100203f0
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri1FlatNoZBuffer(GbiCommand *pCommand);

/**
 * Draw two triangles with flat shading without the depth buffer, after giving the device the dirty
 * render states.
 *
 * @param pCommand The command, the entries of g_aGbiVertexCache of each triangle in the low three
 * bytes of each word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x100205f0
 * @ghidraAddress BRGlide: 0x10020d30
 * @stub{BRGlide}
 */
GbiCommand *HandleGbiTri2FlatNoZBuffer(GbiCommand *pCommand);

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * The comparison of depth of the geometry mode.
 *
 * @ghidraAddress BRD3D: 0x104c16a0
 */
extern DWORD g_dwD3DZFunc;

/**
 * Whether the combine mode takes the routine that modulates the lighting by the environment colour.
 *
 * @ghidraAddress BRD3D: 0x104c0dc0
 */
extern int g_bCombineModeSpecialSubmitter;

/**
 * The colour of the vertices of the combine modes that draw in one colour.
 *
 * @ghidraAddress BRD3D: 0x104c0bb4
 */
extern D3DCOLOR g_dwVertexConstantColor;
#endif

/** The opcodes of the commands of the display lists that have handlers of their own. */
enum GbiOpcode {
    GBI_OP_MATRIX = 0x01,                    /*!< HandleGbiMatrix(). */
    GBI_OP_MOVE_MEM = 0x03,                  /*!< HandleGbiMoveMem(). */
    GBI_OP_VERTEX = 0x04,                    /*!< A handler of vertices. */
    GBI_OP_DISPLAY_LIST = 0x06,              /*!< HandleGbiDisplayList(). */
    GBI_OP_TRI2 = 0xb1,                      /*!< A handler of two triangles. */
    GBI_OP_CLEAR_GEOMETRY_MODE = 0xb6,       /*!< HandleGbiClearGeometryMode(). */
    GBI_OP_SET_GEOMETRY_MODE = 0xb7,         /*!< HandleGbiSetGeometryMode(). */
    GBI_OP_END_DISPLAY_LIST = 0xb8,          /*!< HandleGbiEndDisplayList(). */
    GBI_OP_SET_OTHER_MODE_L = 0xb9,          /*!< HandleGbiSetOtherModeL(). */
    GBI_OP_MOVE_WORD = 0xbc,                 /*!< HandleGbiMoveWord(). */
    GBI_OP_POP_MATRIX = 0xbd,                /*!< HandleGbiPopMatrix(). */
    GBI_OP_TRI1 = 0xbf,                      /*!< A handler of one triangle. */
    GBI_OP_BIND_TEXTURE_ENTRY = 0xdc,        /*!< HandleGbiBindTextureEntry(). */
    GBI_OP_LOAD_PREPARED_TEXTURE = 0xdd,     /*!< HandleGbiLoadPreparedTexture(). */
    GBI_OP_SET_TEX_RECT_W = 0xde,            /*!< HandleGbiSetTexRectW(). */
    GBI_OP_SET_TEX_RECT_DEPTH = 0xdf,        /*!< HandleGbiSetTexRectDepth(). */
    GBI_OP_FILL_RECTANGLE_INT_COORDS = 0xe1, /*!< HandleGbiFillRectangleIntCoords(). */
    GBI_OP_SET_SCISSOR_INT_COORDS = 0xe2,    /*!< HandleGbiSetScissorIntCoords(). */
    GBI_OP_TEXTURE_RECTANGLE = 0xe3,         /*!< HandleGbiTextureRectangle(). */
    GBI_OP_TEXTURE_RECTANGLE_NATIVE = 0xe4,  /*!< HandleGbiTextureRectangleNative(). */
    GBI_OP_SET_SCISSOR = 0xed,               /*!< HandleGbiSetScissor(). */
    GBI_OP_SET_TILE_SIZE = 0xf2,             /*!< HandleGbiSetTileSize(). */
    GBI_OP_FILL_RECTANGLE = 0xf6,            /*!< HandleGbiFillRectangle(). */
    GBI_OP_SET_FILL_COLOR = 0xf7,            /*!< HandleGbiSetFillColor(). */
    GBI_OP_SET_FOG_COLOR = 0xf8,             /*!< HandleGbiSetFogColor(). */
    GBI_OP_SET_PRIM_COLOR = 0xfa,            /*!< HandleGbiSetPrimColor(). */
    GBI_OP_SET_ENV_COLOR = 0xfb,             /*!< HandleGbiSetEnvColor(). */
    GBI_OP_SET_COMBINE = 0xfc,               /*!< HandleGbiSetCombine(). */
};

/** A handler of a command of a display list. */
typedef GbiCommand *(*GbiHandler)(GbiCommand *pCommand);

/** The number of entries of g_apfnGbiHandlers, one for each opcode. */
#define GBI_HANDLER_COUNT 256

/**
 * The handlers of the commands of the display lists, by the opcode in the high byte of the first
 * word. Every opcode without a handler of its own takes HandleGbiNoOp().
 *
 * @ghidraAddress BRD3D: 0x100a79f0
 * @ghidraAddress BRGlide: 0x100a9a58
 */
extern std::array<GbiHandler, GBI_HANDLER_COUNT> g_apfnGbiHandlers;

/**
 * Run a display list through g_apfnGbiHandlers until a handler returns `nullptr`.
 *
 * @param pCommand The first command, or `nullptr` for none.
 * @ghidraAddress BRD3D: 0x10024a90
 * @ghidraAddress BRGlide: 0x10023c90
 */
void ExecuteGbiDisplayList(GbiCommand *pCommand);

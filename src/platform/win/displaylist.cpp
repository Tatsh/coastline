#include "platform/win/displaylist.h"

#include <cstdlib>

#include "platform/win/settings.h"

#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/renderer.h"
#endif

float g_fTexRectScaleW = 1.0f;
float g_fTexRectDepth;
unsigned g_dwGbiCombineModeHi;
unsigned g_dwGbiCombineModeLo;
unsigned g_dwGbiGeometryMode;
unsigned g_dwGbiPrevGeometryMode;
int g_dwGbiMatrixStackDepth = 1;
int g_dwGbiDisplayListStackDepth;
GbiCommand *g_apGbiDisplayListReturnStack[GBI_DISPLAY_LIST_STACK_SIZE];

// The bits of the first word of a display list command that mark a branch rather than a call.
#define GBI_DISPLAY_LIST_BRANCH_MASK 0xff0000

GbiCommand *HandleGbiSetTexRectW(GbiCommand *pCommand) {
    g_fTexRectScaleW = pCommand->f1;
    return pCommand + 1;
}

GbiCommand *HandleGbiSetTexRectDepth(GbiCommand *pCommand) {
    g_fTexRectDepth = pCommand->f1;
    return pCommand + 1;
}

unsigned char g_bGbiFillColorR;
unsigned char g_bGbiFillColorG;
unsigned char g_bGbiFillColorB;
unsigned char g_bGbiFillColorA;

static inline int GbiPixelField10(unsigned dwWord, int nShift) {
    return dwWord >> (nShift + 2) & 0x3ff;
}

GbiCommand *HandleGbiFillRectangle(GbiCommand *pCommand) {
    int nLowerRightY = GbiPixelField10(pCommand->w0, 0);
    int nLowerRightX = GbiPixelField10(pCommand->w0, 12);
    int nUpperLeftY = GbiPixelField10(pCommand->w1, 0);
    int nUpperLeftX = GbiPixelField10(pCommand->w1, 12);
    DrawFillRectangle(nUpperLeftX,
                      g_dwConfigFramebufferHeight - nLowerRightY - 1,
                      nLowerRightX + 1,
                      g_dwConfigFramebufferHeight - nUpperLeftY);
    return pCommand + 1;
}

static inline unsigned char WidenFillColorChannel5(unsigned dwChannel) {
    return static_cast<unsigned char>(dwChannel << 3 | dwChannel >> 2);
}

GbiCommand *HandleGbiSetFillColor(GbiCommand *pCommand) {
    unsigned dwColor = pCommand->w1;
    g_bGbiFillColorR = WidenFillColorChannel5(dwColor >> 11 & 0x1f);
    g_bGbiFillColorG = WidenFillColorChannel5(dwColor >> 6 & 0x1f);
    g_bGbiFillColorB = WidenFillColorChannel5(dwColor >> 1 & 0x1f);
    g_bGbiFillColorA = (dwColor & 1) ? 0xff : 0;
    return pCommand + 1;
}

float g_fGbiEnvColorR;
float g_fGbiEnvColorG;
float g_fGbiEnvColorB;
float g_fGbiEnvColorA;
float g_fGbiPrimColorR;
float g_fGbiPrimColorG;
float g_fGbiPrimColorB;
float g_fGbiPrimColorA;

static inline void
SetGbiColorChannels(unsigned dwRgba, float &fRed, float &fGreen, float &fBlue, float &fAlpha) {
    fRed = static_cast<float>(dwRgba >> 24) * (1.0f / 255.0f);
    fGreen = static_cast<float>(dwRgba >> 16 & 0xff) * (1.0f / 255.0f);
    fBlue = static_cast<float>(dwRgba >> 8 & 0xff) * (1.0f / 255.0f);
    fAlpha = static_cast<float>(dwRgba & 0xff) * (1.0f / 255.0f);
}

GbiCommand *HandleGbiSetEnvColor(GbiCommand *pCommand) {
    SetGbiColorChannels(
        pCommand->w1, g_fGbiEnvColorR, g_fGbiEnvColorG, g_fGbiEnvColorB, g_fGbiEnvColorA);
    return pCommand + 1;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
GbiCommand *HandleGbiSetPrimColor(GbiCommand *pCommand) {
    SetGbiColorChannels(
        pCommand->w1, g_fGbiPrimColorR, g_fGbiPrimColorG, g_fGbiPrimColorB, g_fGbiPrimColorA);
    return pCommand + 1;
}
#endif

// The commands of a texture rectangle of the console: the rectangle and its two halves of texture
// coordinates.
#define GBI_TEXTURE_RECTANGLE_COMMANDS 3

static inline int GbiField12(unsigned dwWord, int nShift) {
    return dwWord >> nShift & 0xfff;
}

static inline int GbiTile(unsigned dwWord) {
    return dwWord >> 24 & 7;
}

GbiCommand *HandleGbiTextureRectangleNative(GbiCommand *pCommand) {
    DrawTextureRectangle(GbiField12(pCommand->w1, 12),
                         GbiField12(pCommand->w1, 0),
                         GbiField12(pCommand->w0, 12),
                         GbiField12(pCommand->w0, 0),
                         GbiTile(pCommand->w1));
    return pCommand + GBI_TEXTURE_RECTANGLE_COMMANDS;
}

GbiCommand *HandleGbiTextureRectangle(GbiCommand *pCommand) {
    // The corners take two more bits of fraction than the console gives, and the command is one.
    DrawTextureRectangle(GbiField12(pCommand->w1, 12) << 2,
                         GbiField12(pCommand->w1, 0) << 2,
                         GbiField12(pCommand->w0, 12) << 2,
                         GbiField12(pCommand->w0, 0) << 2,
                         GbiTile(pCommand->w1));
    return pCommand + 1;
}

float g_fViewportScaleX;
float g_fViewportScaleY;
float g_fViewportTranslateX;
float g_fViewportTranslateY;

// The pixels of a unit of a viewport of the console.
#define GBI_VIEWPORT_UNIT 0.25f

GbiCommand *HandleGbiSetViewport(GbiCommand *pCommand) {
    const GbiViewport *pViewport = pCommand->pViewport;
#if TGR_RENDERER == TGR_RENDERER_D3D
    g_fViewportScaleX = static_cast<float>(pViewport->anScale[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportScaleY = static_cast<float>(pViewport->anScale[1]) * -GBI_VIEWPORT_UNIT;
    g_fViewportTranslateX = static_cast<float>(pViewport->anTranslate[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportTranslateY = static_cast<float>(pViewport->anTranslate[1]) * GBI_VIEWPORT_UNIT;
#else
    float fHeight = static_cast<float>(g_dwConfigFramebufferHeight);
    g_fViewportScaleX = static_cast<float>(pViewport->anScale[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportScaleY = static_cast<float>(pViewport->anScale[1]) * GBI_VIEWPORT_UNIT;
    g_fViewportTranslateX = static_cast<float>(pViewport->anTranslate[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportTranslateY =
        fHeight - static_cast<float>(pViewport->anTranslate[1]) * GBI_VIEWPORT_UNIT;
#endif
    return pCommand + 1;
}

static inline int GbiSignedField12(unsigned dwWord, int nShift) {
    return static_cast<int>(dwWord << (20 - nShift)) >> 20;
}

GbiCommand *HandleGbiFillRectangleIntCoords(GbiCommand *pCommand) {
    int nLowerRightY = GbiSignedField12(pCommand->w0, 0);
    int nLowerRightX = GbiSignedField12(pCommand->w0, 12);
    int nUpperLeftY = GbiSignedField12(pCommand->w1, 0);
    int nUpperLeftX = GbiSignedField12(pCommand->w1, 12);
    DrawFillRectangle(nUpperLeftX,
                      g_dwConfigFramebufferHeight - nLowerRightY - 1,
                      nLowerRightX + 1,
                      g_dwConfigFramebufferHeight - nUpperLeftY);
    return pCommand + 1;
}

GbiCommand *HandleGbiSetCombine(GbiCommand *pCommand) {
    g_dwGbiCombineModeHi = pCommand->w0;
    g_dwGbiCombineModeLo = pCommand->w1;
    SelectVertexSubmitterForCombineMode(g_dwGbiCombineModeHi, g_dwGbiCombineModeLo);
    return pCommand + 1;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
GbiCommand *HandleGbiSetFogColor(GbiCommand *pCommand) {
    unsigned dwRgba = pCommand->w1;
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_FOGCOLOR, dwRgba << 24 | dwRgba >> 8);
    return pCommand + 1;
}
#endif

GbiCommand *HandleGbiClearGeometryMode(GbiCommand *pCommand) {
    g_dwGbiPrevGeometryMode = g_dwGbiGeometryMode;
    g_dwGbiGeometryMode &= ~pCommand->w1;
    ApplyGeometryModeChanges();
    return pCommand + 1;
}

GbiCommand *HandleGbiDisplayList(GbiCommand *pCommand) {
    if (!(pCommand->w0 & GBI_DISPLAY_LIST_BRANCH_MASK)) {
        if (g_dwGbiDisplayListStackDepth + 1 == GBI_DISPLAY_LIST_STACK_SIZE) {
            exit(1);
        }
        g_apGbiDisplayListReturnStack[g_dwGbiDisplayListStackDepth++] = pCommand + 1;
    }
    return pCommand->pTarget;
}

GbiCommand *HandleGbiPopMatrix(GbiCommand *pCommand) {
    // A pop to depth 0 sets the depth to 10 rather than 0.
    if (g_dwGbiMatrixStackDepth != 0 && --g_dwGbiMatrixStackDepth == 0) {
        g_dwGbiMatrixStackDepth = GBI_MATRIX_STACK_RESET_DEPTH;
    }
    return pCommand + 1;
}

// The shifts of the modes of the low word of the other modes.
#define GBI_MODE_SHIFT_ALPHA_COMPARE 0
#define GBI_MODE_SHIFT_RENDER_MODE 3

GbiCommand *HandleGbiSetOtherModeL(GbiCommand *pCommand) {
    int nShift = static_cast<signed char>(pCommand->w0 >> 8);
    if (nShift == GBI_MODE_SHIFT_ALPHA_COMPARE) {
        return HandleGbiNoOp(pCommand);
    }
    if (nShift == GBI_MODE_SHIFT_RENDER_MODE) {
        return HandleGbiSetRenderMode(pCommand);
    }
    return pCommand + 1;
}

GbiCommand *HandleGbiSetGeometryMode(GbiCommand *pCommand) {
    g_dwGbiPrevGeometryMode = g_dwGbiGeometryMode;
    g_dwGbiGeometryMode |= pCommand->w1;
    ApplyGeometryModeChanges();
    return pCommand + 1;
}

GbiCommand *HandleGbiNoOp(GbiCommand *pCommand) {
    return pCommand + 1;
}

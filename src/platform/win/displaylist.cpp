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

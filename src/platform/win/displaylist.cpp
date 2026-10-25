#include "platform/win/displaylist.h"

#include <cstdlib>

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

GbiCommand *HandleGbiSetGeometryMode(GbiCommand *pCommand) {
    g_dwGbiPrevGeometryMode = g_dwGbiGeometryMode;
    g_dwGbiGeometryMode |= pCommand->w1;
    ApplyGeometryModeChanges();
    return pCommand + 1;
}

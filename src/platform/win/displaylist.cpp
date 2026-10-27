#include "platform/win/displaylist.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>

#include <PR/gu.h>

#include "clipplane.h"
#include "displaylistptr.h"
#include "platform/win/boot/app.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/timer.h"
#include "weather.h"

#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/renderer.h"
#endif

// The headers of Windows name the drawing of text of GDI DrawText.
#undef DrawText
#include "text.h"

// The bits of the geometry mode of the console the handlers read.
#define GBI_GEOMETRY_Z_BUFFER 0x1
#define GBI_GEOMETRY_SHADING_SMOOTH 0x200
#define GBI_GEOMETRY_CULL_FRONT 0x1000
#define GBI_GEOMETRY_CULL_BACK 0x2000
#define GBI_GEOMETRY_LIGHTING 0x20000
#define GBI_GEOMETRY_TEXTURE_GEN 0x40000
#define GBI_GEOMETRY_TEXTURE_GEN_LINEAR 0x80000

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
    g_fViewportScaleX = static_cast<float>(pViewport->vp.vscale[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportScaleY = static_cast<float>(pViewport->vp.vscale[1]) * -GBI_VIEWPORT_UNIT;
    g_fViewportTranslateX = static_cast<float>(pViewport->vp.vtrans[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportTranslateY = static_cast<float>(pViewport->vp.vtrans[1]) * GBI_VIEWPORT_UNIT;
#else
    float fHeight = static_cast<float>(g_dwConfigFramebufferHeight);
    g_fViewportScaleX = static_cast<float>(pViewport->vp.vscale[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportScaleY = static_cast<float>(pViewport->vp.vscale[1]) * GBI_VIEWPORT_UNIT;
    g_fViewportTranslateX = static_cast<float>(pViewport->vp.vtrans[0]) * GBI_VIEWPORT_UNIT;
    g_fViewportTranslateY =
        fHeight - static_cast<float>(pViewport->vp.vtrans[1]) * GBI_VIEWPORT_UNIT;
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

unsigned g_dwGbiRenderMode;

GbiCommand *HandleGbiSetRenderMode(GbiCommand *pCommand) {
    g_dwGbiRenderMode = pCommand->w1;
    ApplyRdpRenderMode(g_dwGbiRenderMode);
    return pCommand + 1;
}

int g_dwGbiNumLights;
GbiLight g_aGbiLights[GBI_LIGHT_COUNT];
int g_bGbiLightsTransformed;

// The words a move word command sets, by the index in the low byte of its first word.
#define GBI_MOVE_WORD_NUM_LIGHT 2
#define GBI_MOVE_WORD_FOG 8
#define GBI_MOVE_WORD_LIGHT_COLOR 10
#define GBI_MOVE_WORD_PERSPECTIVE_NORMALIZE 14

// The bytes of the offset of a move word command for each light.
#define GBI_MOVE_WORD_LIGHT_STRIDE 32

static inline void SetGbiLightColor(unsigned char *pColor, unsigned dwRgba) {
    pColor[0] = static_cast<unsigned char>(dwRgba >> 24);
    pColor[1] = static_cast<unsigned char>(dwRgba >> 16);
    pColor[2] = static_cast<unsigned char>(dwRgba >> 8);
}

#if TGR_RENDERER == TGR_RENDERER_D3D
void HalveGbiTriangleVertexIndices(GbiCommand *pCommand) {
    pCommand->w1 = (pCommand->w1 & 0xff000000) | (pCommand->w1 >> 1 & 0x7f7f7f);
}

void HalveGbiTrianglePairVertexIndices(GbiCommand *pCommand) {
    pCommand->w0 = (pCommand->w0 & 0xff000000) | (pCommand->w0 >> 1 & 0x7f7f7f);
    HalveGbiTriangleVertexIndices(pCommand);
}
#else
void HalveAmbientLightColor(GbiLight *pLight) {
    pLight->l.colc[2] >>= 1;
    pLight->l.colc[1] >>= 1;
    pLight->l.colc[0] >>= 1;
}

void HalveLightColors(GbiLight *pLight) {
    pLight->l.col[2] >>= 1;
    pLight->l.col[1] >>= 1;
    pLight->l.col[0] >>= 1;
    HalveAmbientLightColor(pLight);
}
#endif

GbiCommand *HandleGbiMoveWord(GbiCommand *pCommand) {
    unsigned dwOffset;

    switch (static_cast<signed char>(pCommand->w0)) {
    case GBI_MOVE_WORD_NUM_LIGHT:
        g_dwGbiNumLights = pCommand->w1 >> 5 & 0xf;
        break;
    case GBI_MOVE_WORD_LIGHT_COLOR:
        dwOffset = pCommand->w0 >> 8 & 0xffff;
        if (dwOffset & 0xf) {
            SetGbiLightColor(g_aGbiLights[dwOffset / GBI_MOVE_WORD_LIGHT_STRIDE].l.colc,
                             pCommand->w1);
        } else {
            SetGbiLightColor(g_aGbiLights[dwOffset / GBI_MOVE_WORD_LIGHT_STRIDE].l.col,
                             pCommand->w1);
        }
        g_bGbiLightsTransformed = false;
        break;
    // The fog and the normalisation of the perspective have cases of their own that do nothing.
    case GBI_MOVE_WORD_FOG:
    case GBI_MOVE_WORD_PERSPECTIVE_NORMALIZE:
    default:
        break;
    }
    return pCommand + 1;
}

float g_GbiMvpMatrix[4][4];
const Light *g_pGbiLookAtY;
const Light *g_pGbiLookAtX;

GbiCommand *HandleGbiLoadMatrix(GbiCommand *pCommand) {
    memcpy(g_GbiMvpMatrix, pCommand->pData, sizeof(g_GbiMvpMatrix));
    return pCommand + 1;
}

// The memory a move memory command sets, by the index in bits 16 to 23 of its first word.
#define GBI_MOVE_MEM_VIEWPORT 0x80
#define GBI_MOVE_MEM_LOOK_AT_Y 0x82
#define GBI_MOVE_MEM_LOOK_AT_X 0x84
#define GBI_MOVE_MEM_LIGHT_0 0x86
#define GBI_MOVE_MEM_LIGHT_1 0x88
#define GBI_MOVE_MEM_LIGHT_2 0x8a
#define GBI_MOVE_MEM_LIGHT_3 0x8c
#define GBI_MOVE_MEM_LIGHT_4 0x8e
#define GBI_MOVE_MEM_LIGHT_5 0x90
#define GBI_MOVE_MEM_LIGHT_6 0x92
#define GBI_MOVE_MEM_LIGHT_7 0x94
#define GBI_MOVE_MEM_MATRIX 0x9e

GbiCommand *HandleGbiMoveMem(GbiCommand *pCommand) {
    unsigned dwIndex = pCommand->w0 >> 16 & 0xff;

    switch (dwIndex) {
    case GBI_MOVE_MEM_VIEWPORT:
        return HandleGbiSetViewport(pCommand);
    case GBI_MOVE_MEM_LOOK_AT_Y:
        g_pGbiLookAtY = pCommand->pLight;
        break;
    case GBI_MOVE_MEM_LOOK_AT_X:
        g_pGbiLookAtX = pCommand->pLight;
        break;
    case GBI_MOVE_MEM_LIGHT_0:
    case GBI_MOVE_MEM_LIGHT_1:
    case GBI_MOVE_MEM_LIGHT_2:
    case GBI_MOVE_MEM_LIGHT_3:
    case GBI_MOVE_MEM_LIGHT_4:
    case GBI_MOVE_MEM_LIGHT_5:
    case GBI_MOVE_MEM_LIGHT_6:
    case GBI_MOVE_MEM_LIGHT_7:
        memcpy(&g_aGbiLights[(dwIndex - GBI_MOVE_MEM_LIGHT_0) / 2],
               pCommand->pData,
               pCommand->w0 & 0xffff);
        g_bGbiLightsTransformed = false;
        break;
    case GBI_MOVE_MEM_MATRIX:
        return HandleGbiLoadMatrix(pCommand);
    default:
        break;
    }
    return pCommand + 1;
}

int g_dwTileSizeUls;
int g_dwTileSizeUlt;
int g_dwTileSizeLrs;
int g_dwTileSizeLrt;
int g_dwTileSizeWidth;
int g_dwTileSizeHeight;

GbiCommand *HandleGbiSetTileSize(GbiCommand *pCommand) {
    g_dwTileSizeUls = GbiSignedField12(pCommand->w0, 12);
    g_dwTileSizeUlt = GbiSignedField12(pCommand->w0, 0);
    g_dwTileSizeLrs = GbiSignedField12(pCommand->w1, 12);
    g_dwTileSizeLrt = GbiSignedField12(pCommand->w1, 0);
    // The edges are inclusive, so the size counts one texel more than their difference.
    g_dwTileSizeWidth = (g_dwTileSizeLrs - g_dwTileSizeUls + 4) >> 2;
    g_dwTileSizeHeight = (g_dwTileSizeLrt - g_dwTileSizeUlt + 4) >> 2;
    return pCommand + 1;
}

// The bits of the first word of a command that give the entry of a texture the game loaded.
#define GBI_TEXTURE_ENTRY_MASK 0xffffff

#if TGR_RENDERER == TGR_RENDERER_D3D
GbiVertex g_aGbiVertexCache[GBI_VERTEX_CACHE_SIZE];
GbiVertex *g_apVertexPool[GBI_VERTEX_CACHE_SIZE];
int g_nVertexPoolCount;
D3DTLVERTEX g_renderBuffer[RENDER_BUFFER_SIZE];
int g_nRenderBufferIndex;
WORD g_awRenderIndexBuffer[RENDER_BUFFER_SIZE];
int g_nRenderIndexCount;

void ResetRenderBufferAndVertexPool(void) {
    for (int i = 0; i < g_nVertexPoolCount; i++) {
        g_apVertexPool[i]->nRenderBufferIndex = -1;
    }
    g_nRenderIndexCount = 0;
    g_nRenderBufferIndex = 0;
    g_nVertexPoolCount = 0;
}

void FlushIndexedRenderBuffer(void) {
    if (g_nRenderIndexCount == 0) {
        return;
    }
    g_pD3DDevice2->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                        D3DVT_TLVERTEX,
                                        g_renderBuffer,
                                        g_nRenderBufferIndex,
                                        g_awRenderIndexBuffer,
                                        g_nRenderIndexCount,
                                        D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
    ResetRenderBufferAndVertexPool();
}

static inline void ApplyDirtyRenderState(DWORD dwDirtyBit,
                                         D3DRENDERSTATETYPE nState,
                                         DWORD dwPending,
                                         DWORD *pdwApplied) {
    if (g_dwDirtyRenderStateMask & dwDirtyBit) {
        g_pD3DDevice2->SetRenderState(nState, dwPending);
        *pdwApplied = dwPending;
    }
}

void FlushDirtyRenderStates(void) {
    if (g_dwDirtyRenderStateMask == 0) {
        return;
    }
    FlushIndexedRenderBuffer();
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_Z_WRITE_ENABLE,
                          D3DRENDERSTATE_ZWRITEENABLE,
                          g_dwPendingRS_ZWriteEnable,
                          &g_dwAppliedRS_ZWriteEnable);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_ALPHA_TEST_ENABLE,
                          D3DRENDERSTATE_ALPHATESTENABLE,
                          g_dwPendingRS_AlphaTestEnable,
                          &g_dwAppliedRS_AlphaTestEnable);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_SRC_BLEND,
                          D3DRENDERSTATE_SRCBLEND,
                          g_dwPendingRS_SrcBlend,
                          &g_dwAppliedRS_SrcBlend);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_DEST_BLEND,
                          D3DRENDERSTATE_DESTBLEND,
                          g_dwPendingRS_DestBlend,
                          &g_dwAppliedRS_DestBlend);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_CULL_MODE,
                          D3DRENDERSTATE_CULLMODE,
                          g_dwPendingRS_CullMode,
                          &g_dwAppliedRS_CullMode);
    ApplyDirtyRenderState(
        RENDER_STATE_DIRTY_Z_FUNC, D3DRENDERSTATE_ZFUNC, g_dwPendingRS_ZFunc, &g_dwAppliedRS_ZFunc);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_ALPHA_REF,
                          D3DRENDERSTATE_ALPHAREF,
                          g_dwPendingRS_AlphaRef,
                          &g_dwAppliedRS_AlphaRef);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_ALPHA_FUNC,
                          D3DRENDERSTATE_ALPHAFUNC,
                          g_dwPendingRS_AlphaFunc,
                          &g_dwAppliedRS_AlphaFunc);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_ADDRESS_U,
                          D3DRENDERSTATE_TEXTUREADDRESSU,
                          g_dwPendingRS_AddressU,
                          &g_dwAppliedRS_AddressU);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_ADDRESS_V,
                          D3DRENDERSTATE_TEXTUREADDRESSV,
                          g_dwPendingRS_AddressV,
                          &g_dwAppliedRS_AddressV);
    ApplyDirtyRenderState(RENDER_STATE_DIRTY_ALPHA_BLEND_ENABLE,
                          D3DRENDERSTATE_ALPHABLENDENABLE,
                          g_dwPendingRS_AlphaBlendEnable,
                          &g_dwAppliedRS_AlphaBlendEnable);
    g_dwDirtyRenderStateMask = 0;
}

char g_szDebugOverlayText[DEBUG_OVERLAY_TEXT_SIZE];
DWORD g_dwD3DAlphaFunc;
int g_dwFpsSampleFrames = 5;
int g_dwFpsFrameCountdown = -1;
DWORD g_adwFpsFrameTimeSamples[FPS_FRAME_TIME_SAMPLE_COUNT];
DWORD g_dwGbiLastFrameTime;
DWORD g_dwGbiFrameDeltaTime;

// The bytes of the display list of the text drawn over a frame.
#define OVERLAY_DISPLAY_LIST_SIZE 0x8000
// The size and the position of the text drawn over a frame.
#define OVERLAY_TEXT_SIZE 20
#define OVERLAY_TEXT_X 0
#define OVERLAY_TEXT_Y 60
// The first word of the command that ends a display list.
#define GBI_END_DISPLAY_LIST_W0 0xb8000000

// Start or end the scene, restoring the surfaces as long as they are lost.
static inline bool RunSceneCall(HRESULT (STDMETHODCALLTYPE IDirect3DDevice2::*pfnCall)()) {
    HRESULT hResult = (g_pD3DDevice2->*pfnCall)();
    while (hResult == DDERR_SURFACELOST) {
        while (g_pMainRenderWindow->RestoreLostSurfaces() == DDERR_SURFACELOST) {
        }
        hResult = (g_pD3DDevice2->*pfnCall)();
    }
    return hResult == D3D_OK;
}

static inline void ResetFrameRenderStates(void) {
    g_dwPendingRS_AlphaTestEnable = false;
    g_dwPendingRS_AlphaBlendEnable = false;
    g_dwPendingRS_ZWriteEnable = true;
    g_dwPendingRS_SrcBlend = D3DBLEND_ONE;
    g_dwPendingRS_DestBlend = D3DBLEND_ZERO;
    g_dwPendingRS_CullMode = D3DCULL_CCW;
    g_dwD3DZFunc = g_dwPendingRS_ZFunc = D3DCMP_LESS;
    g_dwPendingRS_AlphaRef = 8;
    g_dwD3DAlphaFunc = g_dwPendingRS_AlphaFunc = D3DCMP_GREATEREQUAL;
    g_dwPendingRS_AddressU = D3DTADDRESS_WRAP;
    g_dwPendingRS_AddressV = D3DTADDRESS_WRAP;
    g_dwDirtyRenderStateMask = 0xffffffff;
    FlushDirtyRenderStates();
    g_dwGbiGeometryMode = GBI_GEOMETRY_Z_BUFFER | GBI_GEOMETRY_CULL_BACK;
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, false);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_ZENABLE, true);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_DITHERENABLE, true);
    if (!g_bD3DAlphaCompare) {
        g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, true);
    }
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_WRAPU, false);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_WRAPV, false);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, true);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_LINEAR);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR);
}

static inline void RecordFrameTime(void) {
    DWORD dwNow = GetHighResolutionTime();
    DWORD dwDelta = dwNow - g_dwGbiLastFrameTime;
    int nEntry = g_dwFpsFrameCountdown;

    g_dwGbiLastFrameTime = dwNow;
    g_dwGbiFrameDeltaTime = dwDelta;
    if (nEntry < 0) {
        // The first frame fills every sample with its time.
        nEntry = 0;
        if (g_dwFpsSampleFrames > 0) {
            for (int i = 0; i < g_dwFpsSampleFrames; i++) {
                g_adwFpsFrameTimeSamples[i] = dwDelta;
            }
            nEntry = g_dwFpsSampleFrames;
        }
    }
    if (++nEntry >= g_dwFpsSampleFrames) {
        nEntry = 0;
    }
    g_dwFpsFrameCountdown = nEntry;
    g_adwFpsFrameTimeSamples[nEntry] = dwDelta;
}

void RenderGbiDisplayListFrame(GbiCommand *pDisplayList) {
    GbiCommand aOverlayDisplayList[OVERLAY_DISPLAY_LIST_SIZE / sizeof(GbiCommand)];

    ClearRenderTargetOnce();
    if (!RunSceneCall(&IDirect3DDevice2::BeginScene)) {
        return;
    }
    ResetFrameRenderStates();
    g_nRenderIndexCount = 0;
    g_nRenderBufferIndex = 0;
    g_nVertexPoolCount = 0;
    g_dwGbiDisplayListStackDepth = 0;
    ProcessTextureUploadQueueEntry();
    ExecuteGbiDisplayList(pDisplayList);
    gDisplayListPtr = aOverlayDisplayList;
    SetTextSize(OVERLAY_TEXT_SIZE);
    DrawText(g_szDebugOverlayText, OVERLAY_TEXT_X, OVERLAY_TEXT_Y);
    gDisplayListPtr->w0 = GBI_END_DISPLAY_LIST_W0;
    gDisplayListPtr->w1 = 0;
    gDisplayListPtr++;
    ExecuteGbiDisplayList(aOverlayDisplayList);
    FlushIndexedRenderBuffer();
    if (!RunSceneCall(&IDirect3DDevice2::EndScene)) {
        return;
    }
    g_pfnPresentFrame();
    RecordFrameTime();
}

void (*g_pfnBindTextureEntry)(int nEntry);

GbiCommand *HandleGbiBindTextureEntry(GbiCommand *pCommand) {
    int nEntry = pCommand->w0 & GBI_TEXTURE_ENTRY_MASK;
    FlushIndexedRenderBuffer();
    g_pfnBindTextureEntry(nEntry);
    return pCommand + pCommand->w1;
}
#endif

void (*g_pfnUploadTextureEntryToSurface)(int nEntry, TextureUploadData payload);

GbiCommand *HandleGbiLoadPreparedTexture(GbiCommand *pCommand) {
    g_pfnUploadTextureEntryToSurface(pCommand->w0 & GBI_TEXTURE_ENTRY_MASK, pCommand->w1);
    return pCommand + 1;
}

int g_dwScissorLeft;
int g_dwScissorTop;
int g_dwScissorRight;
int g_dwScissorBottom;

#if TGR_RENDERER == TGR_RENDERER_D3D
static inline void
SetScissorAndClipStatus(int nUpperLeftX, int nUpperLeftY, int nLowerRightX, int nLowerRightY) {
    D3DCLIPSTATUS clipStatus;
    int nHeight = g_dwConfigFramebufferHeight;

    clipStatus.dwFlags = D3DCLIPSTATUS_EXTENTS2;
    clipStatus.dwStatus = D3DSTATUS_CLIPINTERSECTIONALL;
    clipStatus.minz = 0.0f;
    clipStatus.maxz = 1.0f;
    g_dwScissorLeft = nUpperLeftX;
    g_dwScissorTop = nHeight - nUpperLeftY;
    g_dwScissorRight = nLowerRightX;
    g_dwScissorBottom = nHeight - nLowerRightY;
    clipStatus.minx = static_cast<float>(g_dwScissorLeft);
    clipStatus.maxx = static_cast<float>(g_dwScissorRight);
    // The edges are turned over twice, back to the rows of the console.
    clipStatus.miny = static_cast<float>(nHeight - g_dwScissorTop);
    clipStatus.maxy = static_cast<float>(nHeight - g_dwScissorBottom);
    g_pD3DDevice2->SetClipStatus(&clipStatus);
}

GbiCommand *HandleGbiSetScissor(GbiCommand *pCommand) {
    SetScissorAndClipStatus(GbiPixelField10(pCommand->w0, 12),
                            GbiPixelField10(pCommand->w0, 0),
                            GbiPixelField10(pCommand->w1, 12),
                            GbiPixelField10(pCommand->w1, 0));
    return pCommand + 1;
}

GbiCommand *HandleGbiSetScissorIntCoords(GbiCommand *pCommand) {
    SetScissorAndClipStatus(GbiField12(pCommand->w0, 12),
                            GbiField12(pCommand->w0, 0),
                            GbiField12(pCommand->w1, 12),
                            GbiField12(pCommand->w1, 0));
    return pCommand + 1;
}
#endif

GbiCommand *HandleGbiEndDisplayList(GbiCommand *pCommand) {
    if (g_dwGbiDisplayListStackDepth == 0) {
        return nullptr;
    }
    return g_apGbiDisplayListReturnStack[--g_dwGbiDisplayListStackDepth];
}

static constexpr std::array<GbiHandler, GBI_HANDLER_COUNT> MakeGbiHandlers() {
    std::array<GbiHandler, GBI_HANDLER_COUNT> apfnHandlers{};

    for (GbiHandler &pfnHandler : apfnHandlers) {
        pfnHandler = HandleGbiNoOp;
    }
    apfnHandlers[GBI_OP_MATRIX] = HandleGbiMatrix;
    apfnHandlers[GBI_OP_MOVE_MEM] = HandleGbiMoveMem;
    apfnHandlers[GBI_OP_VERTEX] = HandleGbiVertexUnlit;
    apfnHandlers[GBI_OP_DISPLAY_LIST] = HandleGbiDisplayList;
    apfnHandlers[GBI_OP_TRI2] = HandleGbiTri2Smooth;
    apfnHandlers[GBI_OP_CLEAR_GEOMETRY_MODE] = HandleGbiClearGeometryMode;
    apfnHandlers[GBI_OP_SET_GEOMETRY_MODE] = HandleGbiSetGeometryMode;
    apfnHandlers[GBI_OP_END_DISPLAY_LIST] = HandleGbiEndDisplayList;
    apfnHandlers[GBI_OP_SET_OTHER_MODE_L] = HandleGbiSetOtherModeL;
    apfnHandlers[GBI_OP_MOVE_WORD] = HandleGbiMoveWord;
    apfnHandlers[GBI_OP_POP_MATRIX] = HandleGbiPopMatrix;
    apfnHandlers[GBI_OP_TRI1] = HandleGbiTri1Smooth;
    apfnHandlers[GBI_OP_BIND_TEXTURE_ENTRY] = HandleGbiBindTextureEntry;
    apfnHandlers[GBI_OP_LOAD_PREPARED_TEXTURE] = HandleGbiLoadPreparedTexture;
    apfnHandlers[GBI_OP_SET_TEX_RECT_W] = HandleGbiSetTexRectW;
    apfnHandlers[GBI_OP_SET_TEX_RECT_DEPTH] = HandleGbiSetTexRectDepth;
    apfnHandlers[GBI_OP_FILL_RECTANGLE_INT_COORDS] = HandleGbiFillRectangleIntCoords;
    apfnHandlers[GBI_OP_SET_SCISSOR_INT_COORDS] = HandleGbiSetScissorIntCoords;
    apfnHandlers[GBI_OP_TEXTURE_RECTANGLE] = HandleGbiTextureRectangle;
    apfnHandlers[GBI_OP_TEXTURE_RECTANGLE_NATIVE] = HandleGbiTextureRectangleNative;
    apfnHandlers[GBI_OP_SET_SCISSOR] = HandleGbiSetScissor;
    apfnHandlers[GBI_OP_SET_TILE_SIZE] = HandleGbiSetTileSize;
    apfnHandlers[GBI_OP_FILL_RECTANGLE] = HandleGbiFillRectangle;
    apfnHandlers[GBI_OP_SET_FILL_COLOR] = HandleGbiSetFillColor;
    apfnHandlers[GBI_OP_SET_FOG_COLOR] = HandleGbiSetFogColor;
    apfnHandlers[GBI_OP_SET_PRIM_COLOR] = HandleGbiSetPrimColor;
    apfnHandlers[GBI_OP_SET_ENV_COLOR] = HandleGbiSetEnvColor;
    apfnHandlers[GBI_OP_SET_COMBINE] = HandleGbiSetCombine;
    return apfnHandlers;
}

std::array<GbiHandler, GBI_HANDLER_COUNT> g_apfnGbiHandlers = MakeGbiHandlers();

void ExecuteGbiDisplayList(GbiCommand *pCommand) {
    while (pCommand != nullptr) {
        pCommand = g_apfnGbiHandlers[pCommand->w0 >> 24](pCommand);
    }
}

#if TGR_RENDERER == TGR_RENDERER_D3D

DWORD g_dwD3DZFunc;
int g_bCombineModeSpecialSubmitter;

static inline void
SetPendingRenderState(DWORD *pdwPending, DWORD dwApplied, DWORD dwValue, DWORD dwDirtyBit) {
    *pdwPending = dwValue;
    if (dwApplied == dwValue) {
        g_dwDirtyRenderStateMask &= ~dwDirtyBit;
    } else {
        g_dwDirtyRenderStateMask |= dwDirtyBit;
    }
}

int g_nBoundTextureEntry;

void BindTextureEntry(int nEntry) {
    if (nEntry == g_nBoundTextureEntry) {
        return;
    }
    const TextureEntry &entry = g_pTextureEntries[nEntry];
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                  g_ppTextureIdTable[entry.nTextureId]->hTexture);
    SetPendingRenderState(&g_dwPendingRS_AddressU,
                          g_dwAppliedRS_AddressU,
                          entry.data.bClampS == 1 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP,
                          RENDER_STATE_DIRTY_ADDRESS_U);
    SetPendingRenderState(&g_dwPendingRS_AddressV,
                          g_dwAppliedRS_AddressV,
                          entry.data.bClampT == 1 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP,
                          RENDER_STATE_DIRTY_ADDRESS_V);
    g_fTexCoordScaleS = entry.fTextureScaleS;
    g_fTexCoordScaleT = entry.fTextureScaleT;
    const TexScanTile &tile = entry.data.aTiles[entry.data.nTileIndex];
    g_dwTileSizeUls = tile.nUls;
    g_dwTileSizeUlt = tile.nUlt;
    g_dwTileSizeLrs = tile.nLrs;
    g_dwTileSizeLrt = tile.nLrt;
    g_nBoundTextureEntry = nEntry;
}

void ApplyRdpRenderMode(unsigned dwRenderMode) {
    auto zWrite = [](DWORD value) {
        SetPendingRenderState(&g_dwPendingRS_ZWriteEnable,
                              g_dwAppliedRS_ZWriteEnable,
                              value,
                              RENDER_STATE_DIRTY_Z_WRITE_ENABLE);
    };
    auto alphaTest = [](DWORD value) {
        SetPendingRenderState(&g_dwPendingRS_AlphaTestEnable,
                              g_dwAppliedRS_AlphaTestEnable,
                              value,
                              RENDER_STATE_DIRTY_ALPHA_TEST_ENABLE);
    };
    auto alphaBlend = [](DWORD value) {
        SetPendingRenderState(&g_dwPendingRS_AlphaBlendEnable,
                              g_dwAppliedRS_AlphaBlendEnable,
                              value,
                              RENDER_STATE_DIRTY_ALPHA_BLEND_ENABLE);
    };
    auto blend = [](DWORD source, DWORD destination) {
        SetPendingRenderState(
            &g_dwPendingRS_SrcBlend, g_dwAppliedRS_SrcBlend, source, RENDER_STATE_DIRTY_SRC_BLEND);
        SetPendingRenderState(&g_dwPendingRS_DestBlend,
                              g_dwAppliedRS_DestBlend,
                              destination,
                              RENDER_STATE_DIRTY_DEST_BLEND);
    };
    auto alphaFunction = [](DWORD reference, DWORD function) {
        SetPendingRenderState(&g_dwPendingRS_AlphaRef,
                              g_dwAppliedRS_AlphaRef,
                              reference,
                              RENDER_STATE_DIRTY_ALPHA_REF);
        g_dwD3DAlphaFunc = function;
        SetPendingRenderState(&g_dwPendingRS_AlphaFunc,
                              g_dwAppliedRS_AlphaFunc,
                              function,
                              RENDER_STATE_DIRTY_ALPHA_FUNC);
    };
    auto zFunction = [](DWORD function) {
        g_dwD3DZFunc = function;
        SetPendingRenderState(
            &g_dwPendingRS_ZFunc, g_dwAppliedRS_ZFunc, function, RENDER_STATE_DIRTY_Z_FUNC);
    };

    zWrite(TRUE);
    if (dwRenderMode == 0x504f50 || dwRenderMode == 4 || dwRenderMode == 0xc184240 ||
        dwRenderMode == 0x504240) {
        alphaFunction(8, D3DCMP_GREATEREQUAL);
        if (!g_bRdpBlendStateOverride) {
            blend(D3DBLEND_SRCALPHA,
                  dwRenderMode == 4 ? g_dwD3DDestBlendMode : D3DBLEND_INVSRCALPHA);
        }
        alphaTest(TRUE);
        alphaBlend(TRUE);
        if (dwRenderMode == 0x504f50 || dwRenderMode == 4) {
            zFunction(D3DCMP_EQUAL);
        }
        return;
    }
    if (dwRenderMode == 3) {
        g_bRdpBlendStateOverride = false;
        return;
    }
    if (dwRenderMode == 1) {
        zFunction(D3DCMP_LESSEQUAL);
        zWrite(FALSE);
        return;
    }
    if (dwRenderMode == 0) {
        zFunction(D3DCMP_LESS);
        alphaTest(TRUE);
        alphaBlend(FALSE);
        return;
    }
    if (dwRenderMode == 0xd1849d8) {
        blend(D3DBLEND_SRCCOLOR, D3DBLEND_ONE);
        zWrite(FALSE);
        alphaBlend(TRUE);
        g_bRdpBlendStateOverride = true;
        return;
    }
    if (dwRenderMode & 0x1800) {
        if (dwRenderMode & 0x10000) {
            if (!g_bD3DAlphaCompare) {
                return;
            }
            alphaFunction(128, D3DCMP_GREATEREQUAL);
        } else {
            alphaFunction(1, D3DCMP_GREATEREQUAL);
            if (!g_bRdpBlendStateOverride) {
                blend(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
            }
            zWrite(FALSE);
        }
        alphaTest(TRUE);
        alphaBlend(TRUE);
        return;
    }
    if (!g_bRdpBlendStateOverride) {
        blend(D3DBLEND_ONE, D3DBLEND_ZERO);
    }
    g_dwD3DAlphaFunc = D3DCMP_ALWAYS;
    SetPendingRenderState(&g_dwPendingRS_AlphaFunc,
                          g_dwAppliedRS_AlphaFunc,
                          g_dwD3DAlphaFunc,
                          RENDER_STATE_DIRTY_ALPHA_FUNC);
    alphaTest(FALSE);
    alphaBlend(FALSE);
}

void DrawTextureRectangle(
    int nUpperLeftX, int nUpperLeftY, int nLowerRightX, int nLowerRightY, int nTile) {
    float width = g_dwConfigFramebufferWidth;
    float height = g_dwConfigFramebufferHeight;
    float halfWidth = static_cast<long double>(width) / 2.0f;
    float halfHeight = static_cast<long double>(height) / 2.0f;
    auto xCoordinate = [halfWidth](int coordinate) {
        return static_cast<float>(
            (static_cast<long double>(static_cast<float>(static_cast<unsigned>(coordinate))) /
                 4.0f -
             halfWidth) /
            halfWidth / g_fTexRectScaleW);
    };
    auto yCoordinate = [height, halfHeight](int coordinate) {
        return static_cast<float>(
            (static_cast<long double>(height) -
             static_cast<long double>(static_cast<float>(static_cast<unsigned>(coordinate))) /
                 4.0f -
             halfHeight) /
            halfHeight / g_fTexRectScaleW);
    };
    float left = xCoordinate(nUpperLeftX);
    float right = xCoordinate(nLowerRightX);
    float top = yCoordinate(nUpperLeftY);
    float bottom = yCoordinate(nLowerRightY);
    float depth = static_cast<long double>(g_fTexRectDepth) / g_fTexRectScaleW;
    float w = 1.0L / g_fTexRectScaleW;
    float sLeft = static_cast<long double>(static_cast<float>(g_dwTileSizeUls)) * 8.0f;
    float sRight = static_cast<long double>(static_cast<float>(g_dwTileSizeLrs)) * 8.0f;
    float tTop = static_cast<long double>(static_cast<float>(g_dwTileSizeUlt)) * 8.0f;
    float tBottom = static_cast<long double>(static_cast<float>(g_dwTileSizeLrt)) * 8.0f;
    GbiVertex vertices[4];
    const float xs[4] = {right, left, right, left};
    const float ys[4] = {top, bottom, bottom, top};
    const float ss[4] = {sRight, sLeft, sRight, sLeft};
    const float ts[4] = {tBottom, tTop, tTop, tBottom};
    for (int i = 0; i < 4; i++) {
        GbiVertexSource &source = vertices[i].source;
        source.fX = xs[i];
        source.fY = ys[i];
        source.fZ = depth;
        source.fW = w;
        source.fS = ss[i];
        source.fT = ts[i];
        if (g_bCombineModeSpecialSubmitter) {
            bool bTop = i == 0 || i == 3;
            source.fRed = bTop ? g_fGbiEnvColorR : g_fGbiPrimColorR;
            source.fGreen = bTop ? g_fGbiEnvColorG : g_fGbiPrimColorG;
            source.fBlue = bTop ? g_fGbiEnvColorB : g_fGbiPrimColorB;
        } else {
            source.fRed = source.fGreen = source.fBlue = 1.0f;
        }
    }
    FlushDirtyRenderStates();
    if (g_dwGbiGeometryMode & GBI_GEOMETRY_CULL_FRONT) {
        ClipAndSubmitTriangle(&vertices[3], &vertices[0], &vertices[1]);
        ClipAndSubmitTriangle(&vertices[0], &vertices[2], &vertices[1]);
    } else {
        ClipAndSubmitTriangle(&vertices[1], &vertices[0], &vertices[3]);
        ClipAndSubmitTriangle(&vertices[1], &vertices[2], &vertices[0]);
    }
}

void ApplyGeometryModeChanges(void) {
    unsigned dwChanged = g_dwGbiPrevGeometryMode ^ g_dwGbiGeometryMode;
    unsigned dwMode = g_dwGbiGeometryMode;

    if (dwChanged & (GBI_GEOMETRY_CULL_FRONT | GBI_GEOMETRY_CULL_BACK)) {
        DWORD dwCullMode = D3DCULL_NONE;
        if (dwMode & GBI_GEOMETRY_CULL_FRONT) {
            dwCullMode = D3DCULL_CW;
        } else if (dwMode & GBI_GEOMETRY_CULL_BACK) {
            dwCullMode = D3DCULL_CCW;
        }
        SetPendingRenderState(&g_dwPendingRS_CullMode,
                              g_dwAppliedRS_CullMode,
                              dwCullMode,
                              RENDER_STATE_DIRTY_CULL_MODE);
    }
    if (dwChanged & GBI_GEOMETRY_Z_BUFFER) {
        g_dwD3DZFunc = (dwMode & GBI_GEOMETRY_Z_BUFFER) ? D3DCMP_LESS : D3DCMP_ALWAYS;
        SetPendingRenderState(
            &g_dwPendingRS_ZFunc, g_dwAppliedRS_ZFunc, g_dwD3DZFunc, RENDER_STATE_DIRTY_Z_FUNC);
    }
    bool bSmooth = (dwMode & GBI_GEOMETRY_SHADING_SMOOTH) != 0;
    if (dwMode & GBI_GEOMETRY_Z_BUFFER) {
        if (dwMode & GBI_GEOMETRY_LIGHTING) {
            g_apfnGbiHandlers[GBI_OP_VERTEX] = g_bCombineModeSpecialSubmitter ?
                                                   HandleGbiVertexLitEnvColorModulated :
                                                   HandleGbiVertexLit;
        } else {
            g_apfnGbiHandlers[GBI_OP_VERTEX] = HandleGbiVertexUnlit;
        }
        if (dwMode & GBI_GEOMETRY_TEXTURE_GEN) {
            g_apfnGbiHandlers[GBI_OP_VERTEX] = (dwMode & GBI_GEOMETRY_TEXTURE_GEN_LINEAR) ?
                                                   HandleGbiVertexTexGenLinear :
                                                   HandleGbiVertexTexGen;
        }
        g_apfnGbiHandlers[GBI_OP_TRI1] = bSmooth ? HandleGbiTri1Smooth : HandleGbiTri1Flat;
        g_apfnGbiHandlers[GBI_OP_TRI2] = bSmooth ? HandleGbiTri2Smooth : HandleGbiTri2Flat;
    } else {
        g_apfnGbiHandlers[GBI_OP_VERTEX] = (dwMode & GBI_GEOMETRY_LIGHTING) ?
                                               HandleGbiVertexLitNoZBuffer :
                                               HandleGbiVertexUnlitNoZBuffer;
        g_apfnGbiHandlers[GBI_OP_TRI1] =
            bSmooth ? HandleGbiTri1SmoothNoZBuffer : HandleGbiTri1FlatNoZBuffer;
        g_apfnGbiHandlers[GBI_OP_TRI2] =
            bSmooth ? HandleGbiTri2SmoothNoZBuffer : HandleGbiTri2FlatNoZBuffer;
    }
}

D3DCOLOR g_dwVertexConstantColor;
float g_fVertexFogRange;
int g_dwFogAlphaCutoff;

float g_fGbiLight0ColorR;
float g_fGbiLight0ColorG;
float g_fGbiLight0ColorB;
Vec3f g_GbiLight0Direction;
float g_fGbiAmbientColorR;
float g_fGbiAmbientColorG;
float g_fGbiAmbientColorB;

static void
ShadeGbiVertex(const ConvertedModelVertex *pVertex, GbiVertexSource *pDest, bool bTexGen) {
    if (!g_dwGbiNumLights) {
        pDest->fRed = g_fGbiPrimColorR;
        pDest->fGreen = g_fGbiPrimColorG;
        pDest->fBlue = g_fGbiPrimColorB;
        return;
    }
    long double x = static_cast<long double>(pVertex->normal.x) * g_GbiLight0Direction.x;
    long double y = static_cast<long double>(pVertex->normal.y) * g_GbiLight0Direction.y;
    long double z = static_cast<long double>(pVertex->normal.z) * g_GbiLight0Direction.z;
    float dot = bTexGen ? (z + y) + x : z + (x + y);
    if (!(dot >= 0.0f)) {
        pDest->fRed = g_fGbiAmbientColorR;
        pDest->fGreen = g_fGbiAmbientColorG;
        pDest->fBlue = g_fGbiAmbientColorB;
        return;
    }
    auto channel = [dot](float light, float ambient) {
        float color = static_cast<long double>(dot) * light + ambient;
        return color > 1.0f ? 1.0f : color;
    };
    pDest->fRed = channel(g_fGbiLight0ColorR, g_fGbiAmbientColorR);
    pDest->fGreen = channel(g_fGbiLight0ColorG, g_fGbiAmbientColorG);
    pDest->fBlue = channel(g_fGbiLight0ColorB, g_fGbiAmbientColorB);
}

void ComputeVertexDirectionalLight(const ConvertedModelVertex *pVertex, GbiVertexSource *pDest) {
    ShadeGbiVertex(pVertex, pDest, false);
}

float g_GbiProjectionMatrix[4][4];
float g_aGbiModelViewStack[GBI_MATRIX_STACK_RESET_DEPTH][4][4];

GbiCommand *HandleGbiMatrix(GbiCommand *pCommand) {
    if (pCommand->w0 & 0x10000) {
        if (pCommand->w0 & 0x20000) {
            memcpy(g_GbiProjectionMatrix, pCommand->pMatrix, sizeof(g_GbiProjectionMatrix));
        } else {
            guMtxCatF(pCommand->pMatrix, g_GbiProjectionMatrix, g_GbiProjectionMatrix);
        }
    } else {
        float product[4][4];
        float (*pSource)[4] = pCommand->pMatrix;
        if (!(pCommand->w0 & 0x20000)) {
            guMtxCatF(pSource,
                      g_dwGbiMatrixStackDepth ? g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1] :
                                                nullptr,
                      product);
            pSource = product;
        }
        if (pCommand->w0 & 0x40000) {
            if (g_dwGbiMatrixStackDepth == GBI_MATRIX_STACK_RESET_DEPTH) {
                g_dwGbiMatrixStackDepth = 0;
            }
            ++g_dwGbiMatrixStackDepth;
        }
        memcpy(g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1], pSource, sizeof(product));
        g_bGbiLightsTransformed = false;
    }
    guMtxCatF(g_dwGbiMatrixStackDepth ? g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1] : nullptr,
              g_GbiProjectionMatrix,
              g_GbiMvpMatrix);
    return pCommand + 1;
}

GbiVertexSource *
LerpVertexFromPool(const GbiVertexSource *pFirst, const GbiVertexSource *pSecond, float fFraction) {
    GbiVertexSource *pVertex = g_pVertexFreeList;
    if (pVertex) {
        g_pVertexFreeList = pVertex->pNext;
    }
    auto interpolate = [fFraction](float fFirst, float fSecond) {
        return static_cast<float>((static_cast<long double>(fSecond) - fFirst) * fFraction +
                                  fFirst);
    };
    pVertex->fX = interpolate(pFirst->fX, pSecond->fX);
    pVertex->fY = interpolate(pFirst->fY, pSecond->fY);
    pVertex->fZ = interpolate(pFirst->fZ, pSecond->fZ);
    pVertex->fS = interpolate(pFirst->fS, pSecond->fS);
    pVertex->fT = interpolate(pFirst->fT, pSecond->fT);
    pVertex->fW = interpolate(pFirst->fW, pSecond->fW);
    pVertex->fRed = interpolate(pFirst->fRed, pSecond->fRed);
    pVertex->fGreen = interpolate(pFirst->fGreen, pSecond->fGreen);
    pVertex->fBlue = interpolate(pFirst->fBlue, pSecond->fBlue);
    return pVertex;
}

void UpdateD3DFogRenderState(void) {
    if (g_dwWeatherActive) {
        g_fVertexFogRange = !g_dwRainMode && !g_dwSnowMode && g_dwNightMode ? 16384.0f : 8192.0f;
        g_dwFogAlphaCutoff = 8;
        g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_FOGENABLE, true);
        g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, 0);
    } else {
        g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_FOGENABLE, false);
    }
}

static void RecycleClipVertex(GbiVertexSource *pVertex) {
    if (std::greater_equal<>{}(pVertex, g_aClipVertexPool) &&
        std::less<>{}(pVertex, g_aClipVertexPool + GBI_CLIP_VERTEX_POOL_SIZE)) {
        pVertex->pNext = g_pVertexFreeList;
        g_pVertexFreeList = pVertex;
    }
}

template <typename Distance>
static void ClipPolygonPlane(GbiClipPolygon *pPolygon, Distance distance) {
    ClipLinkedPolygonPlane(pPolygon, distance, LerpVertexFromPool, RecycleClipVertex);
}

void ClipPolygonPositiveWPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon, [](const GbiVertexSource *pVertex) { return pVertex->fW; });
}

void ClipPolygonLeftPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon,
                     [](const GbiVertexSource *pVertex) { return pVertex->fW + pVertex->fX; });
}

void ClipPolygonRightPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon,
                     [](const GbiVertexSource *pVertex) { return pVertex->fW - pVertex->fX; });
}

void ClipPolygonBottomPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon,
                     [](const GbiVertexSource *pVertex) { return pVertex->fY + pVertex->fW; });
}

void ClipPolygonTopPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon,
                     [](const GbiVertexSource *pVertex) { return pVertex->fW - pVertex->fY; });
}

void ClipPolygonNearZPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon,
                     [](const GbiVertexSource *pVertex) { return pVertex->fZ + pVertex->fW; });
}

void ClipPolygonFarZPlane(GbiClipPolygon *pPolygon) {
    ClipPolygonPlane(pPolygon,
                     [](const GbiVertexSource *pVertex) { return pVertex->fW - pVertex->fZ; });
}

int ComputeVertexClipCodes(const GbiVertexSource *pVertex) {
    int nFlags = 0;
    // The x87 comparisons also classify unordered values as outside.
    if (!(pVertex->fW >= 0.0f)) {
        nFlags |= 1;
    }
    if (!(pVertex->fZ + pVertex->fW >= 0.0f)) {
        nFlags |= 2;
    }
    if (!(pVertex->fW - pVertex->fZ >= 0.0f)) {
        nFlags |= 4;
    }
    if (!(pVertex->fX + pVertex->fW >= 0.0f)) {
        nFlags |= 8;
    }
    if (!(pVertex->fW - pVertex->fX >= 0.0f)) {
        nFlags |= 16;
    }
    if (!(pVertex->fY + pVertex->fW >= 0.0f)) {
        nFlags |= 32;
    }
    if (!(pVertex->fW - pVertex->fY >= 0.0f)) {
        nFlags |= 64;
    }
    return nFlags;
}

void TransformVertexToScreen(
    GbiVertex *pDest, const GbiVertexSource *pSource, float fRed, float fGreen, float fBlue) {
    // The x87 arithmetic retains extended precision until each float is stored.
    pDest->fRhw = 1.0L / pSource->fW;
    pDest->fX = static_cast<long double>(pSource->fX) * g_fViewportScaleX * pDest->fRhw +
                g_fViewportTranslateX;
    pDest->fY = static_cast<long double>(pSource->fY) * g_fViewportScaleY * pDest->fRhw +
                g_fViewportTranslateY;
    pDest->fZ =
        static_cast<long double>(pSource->fZ) * g_fDepthScale * pDest->fRhw + g_fDepthOffset;
    pDest->fRed = fRed;
    pDest->fGreen = fGreen;
    pDest->fBlue = fBlue;
}

static inline float TransformGbiVertexComponent(const Vec3f &position, int nColumn) {
    long double fYz = static_cast<long double>(position.y) * g_GbiMvpMatrix[1][nColumn] +
                      static_cast<long double>(position.z) * g_GbiMvpMatrix[2][nColumn];
    return fYz + static_cast<long double>(position.x) * g_GbiMvpMatrix[0][nColumn] +
           g_GbiMvpMatrix[3][nColumn];
}

enum class LightTransformMode { Standard, EnvModulated, NoZBuffer, TexGen, TexGenLinear };

static float
TransformGbiLightDirectionRow(const float row[4], float x, float y, float z, int nLast) {
    long double mx = static_cast<long double>(row[0]) * x;
    long double my = static_cast<long double>(row[1]) * y;
    long double mz = static_cast<long double>(row[2]) * z;
    long double sum;
    if (nLast == 0) {
        sum = (my + mz) + mx;
    } else if (nLast == 1) {
        sum = (mx + mz) + my;
    } else {
        sum = (my + mx) + mz;
    }
    return sum / 128.0f;
}

static void UpdateGbiDirectionalLight(LightTransformMode mode = LightTransformMode::Standard) {
    if (g_bGbiLightsTransformed) {
        return;
    }
    if (g_dwGbiNumLights) {
        const Light_t &light = g_aGbiLights[0].l;
        g_fGbiLight0ColorR = static_cast<float>(light.col[0]) * (1.0f / 255.0f);
        g_fGbiLight0ColorG = static_cast<float>(light.col[1]) * (1.0f / 255.0f);
        g_fGbiLight0ColorB = static_cast<float>(light.col[2]) * (1.0f / 255.0f);
        float (*matrix)[4] =
            g_dwGbiMatrixStackDepth ? g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1] : nullptr;
        float x = light.dir[0];
        float y = light.dir[1];
        float z = light.dir[2];
        if (mode == LightTransformMode::TexGenLinear) {
            // This handler multiplies the second matrix column by the light's z byte.
            y = light.dir[2];
            z = light.dir[1];
        }
        g_GbiLight0Direction.x = TransformGbiLightDirectionRow(
            matrix[0],
            x,
            y,
            z,
            mode == LightTransformMode::Standard || mode == LightTransformMode::TexGenLinear ? 0 :
                                                                                               1);
        g_GbiLight0Direction.y = TransformGbiLightDirectionRow(
            matrix[1],
            x,
            y,
            z,
            mode == LightTransformMode::TexGen || mode == LightTransformMode::TexGenLinear ? 0 :
            mode == LightTransformMode::NoZBuffer                                          ? 1 :
                                                                                             2);
        g_GbiLight0Direction.z = TransformGbiLightDirectionRow(
            matrix[2],
            x,
            y,
            z,
            mode == LightTransformMode::EnvModulated || mode == LightTransformMode::TexGen ||
                    mode == LightTransformMode::TexGenLinear ?
                1 :
            mode == LightTransformMode::NoZBuffer ? 0 :
                                                    2);
        Vec3fNormalize(&g_GbiLight0Direction);
        const Light_t &ambient = g_aGbiLights[1].l;
        g_fGbiAmbientColorR = static_cast<float>(ambient.col[0]) * (1.0f / 255.0f);
        g_fGbiAmbientColorG = static_cast<float>(ambient.col[1]) * (1.0f / 255.0f);
        g_fGbiAmbientColorB = static_cast<float>(ambient.col[2]) * (1.0f / 255.0f);
    }
    g_bGbiLightsTransformed = true;
}

static inline GbiCommand *LoadGbiVertices(GbiCommand *pCommand,
                                          bool bUseZBuffer,
                                          bool bLighting,
                                          bool bEnvModulated = false,
                                          int nTexGen = 0) {
    if (bLighting) {
        UpdateGbiDirectionalLight(nTexGen == 2  ? LightTransformMode::TexGenLinear :
                                  nTexGen == 1  ? LightTransformMode::TexGen :
                                  bEnvModulated ? LightTransformMode::EnvModulated :
                                  bUseZBuffer   ? LightTransformMode::Standard :
                                                  LightTransformMode::NoZBuffer);
    }
    FlushIndexedRenderBuffer();
    int nFirst = pCommand->w0 >> 16 & 0xff;
    int nCount = pCommand->w0 >> 10 & 0x3f;
    const ConvertedModelVertex *pVertices = pCommand->vertexData.pConverted;
    for (int nVertex = 0; nVertex < nCount; nVertex++) {
        GbiVertex &vertex = g_aGbiVertexCache[nFirst + nVertex];
        const ConvertedModelVertex &input = pVertices[nVertex];
        vertex.source.fX = TransformGbiVertexComponent(input.position, 0);
        vertex.source.fY = TransformGbiVertexComponent(input.position, 1);
        vertex.source.fZ = TransformGbiVertexComponent(input.position, 2);
        vertex.source.fW = TransformGbiVertexComponent(input.position, 3);
        vertex.source.fS = input.texture.x;
        vertex.source.fT = input.texture.y;
        if (nTexGen == 2) {
            GenerateGbiVertexTextureCoordinatesLinear(&input, &vertex.source);
        } else if (nTexGen == 1) {
            GenerateGbiVertexTextureCoordinates(&input, &vertex.source);
        }
        if (bLighting) {
            ShadeGbiVertex(&input, &vertex.source, nTexGen == 1);
        } else {
            vertex.source.fRed = static_cast<long double>(input.normal.x) * (1.0f / 255.0f);
            vertex.source.fGreen = static_cast<long double>(input.normal.y) * (1.0f / 255.0f);
            vertex.source.fBlue = static_cast<long double>(input.normal.z) * (1.0f / 255.0f);
        }
        if (bEnvModulated) {
            vertex.source.fRed = static_cast<long double>(g_fGbiEnvColorR) * vertex.source.fRed;
            vertex.source.fGreen = static_cast<long double>(g_fGbiEnvColorG) * vertex.source.fGreen;
            vertex.source.fBlue = static_cast<long double>(g_fGbiEnvColorB) * vertex.source.fBlue;
        }
        vertex.nClipFlags = ComputeVertexClipCodes(&vertex.source);
        if (vertex.nClipFlags != 0) {
            continue;
        }
        if (bUseZBuffer) {
            TransformVertexToScreen(&vertex,
                                    &vertex.source,
                                    vertex.source.fRed,
                                    vertex.source.fGreen,
                                    vertex.source.fBlue);
        } else {
            float fRhw = 1.0L / vertex.source.fW;
            vertex.fX = static_cast<long double>(g_fViewportScaleX) * fRhw * vertex.source.fX +
                        g_fViewportTranslateX;
            vertex.fY = static_cast<long double>(g_fViewportScaleY) * fRhw * vertex.source.fY +
                        g_fViewportTranslateY;
            vertex.fZ = 0.999f;
            vertex.fRhw = 1.0f / 65535.0f;
            vertex.fRed = vertex.source.fRed;
            vertex.fGreen = vertex.source.fGreen;
            vertex.fBlue = vertex.source.fBlue;
        }
    }
    return pCommand + 1;
}

static void GenerateGbiTextureCoordinates(const ConvertedModelVertex *pInput,
                                          GbiVertexSource *pSource,
                                          bool bLinear) {
    const float (*matrix)[4] =
        g_dwGbiMatrixStackDepth != 0 ? g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1] : nullptr;
    Vec3f normal;
    float *components[3] = {&normal.x, &normal.y, &normal.z};
    for (int column = 0; column < 3; column++) {
        *components[column] = (static_cast<long double>(matrix[1][column]) * pInput->normal.y +
                               static_cast<long double>(matrix[2][column]) * pInput->normal.z) +
                              static_cast<long double>(matrix[0][column]) * pInput->normal.x;
    }
    Vec3fNormalize(&normal);
    const Light_t &lookX = g_pGbiLookAtX->l;
    const Light_t &lookY = g_pGbiLookAtY->l;
    long double sx = static_cast<long double>(lookX.dir[0]) * normal.x;
    long double sy = static_cast<long double>(lookX.dir[1]) * normal.y;
    long double sz = static_cast<long double>(lookX.dir[2]) * normal.z;
    float s = (bLinear ? (sx + sz) + sy : (sx + sy) + sz) / 128.0f;
    long double tDot = ((static_cast<long double>(lookY.dir[0]) * normal.x +
                         static_cast<long double>(lookY.dir[2]) * normal.z) +
                        static_cast<long double>(lookY.dir[1]) * normal.y) /
                       128.0f;
    float t;
    if (bLinear) {
        float angle = asinl(static_cast<double>(tDot));
        t = static_cast<long double>(angle) / 3.1415927f;
    } else {
        t = tDot;
    }
    long double generatedS =
        (static_cast<long double>(s) * static_cast<float>(g_dwTileSizeWidth) + 128.0f) /
        (static_cast<long double>(g_fTexCoordScaleS) * 256.0f -
         static_cast<float>(g_dwTileSizeUls) * (bLinear ? 0.5L : 1.0L));
    if (g_dwTileSizeWidth < g_dwTileSizeHeight) {
        generatedS *= static_cast<float>(g_dwTileSizeHeight / g_dwTileSizeWidth);
    }
    pSource->fS = generatedS;
    long double generatedT =
        (static_cast<long double>(t) * static_cast<float>(g_dwTileSizeHeight) + 128.0f) /
        (static_cast<long double>(g_fTexCoordScaleT) * 256.0f -
         static_cast<float>(g_dwTileSizeUlt) * (bLinear ? 0.5L : 1.0L));
    if (g_dwTileSizeWidth > g_dwTileSizeHeight) {
        generatedT *= static_cast<float>(g_dwTileSizeWidth / g_dwTileSizeHeight);
    }
    pSource->fT = generatedT;
}

void GenerateGbiVertexTextureCoordinates(const ConvertedModelVertex *pInput,
                                         GbiVertexSource *pSource) {
    GenerateGbiTextureCoordinates(pInput, pSource, false);
}

void GenerateGbiVertexTextureCoordinatesLinear(const ConvertedModelVertex *pInput,
                                               GbiVertexSource *pSource) {
    GenerateGbiTextureCoordinates(pInput, pSource, true);
}

GbiCommand *HandleGbiVertexUnlit(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, true, false);
}

GbiCommand *HandleGbiVertexUnlitNoZBuffer(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, false, false);
}

GbiCommand *HandleGbiVertexLit(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, true, true);
}

GbiCommand *HandleGbiVertexLitEnvColorModulated(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, true, true, true);
}

GbiCommand *HandleGbiVertexLitNoZBuffer(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, false, true);
}

GbiCommand *HandleGbiVertexTexGen(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, true, true, false, 1);
}

GbiCommand *HandleGbiVertexTexGenLinear(GbiCommand *pCommand) {
    return LoadGbiVertices(pCommand, true, true, false, 2);
}

// The scale from a channel of the colour of a vertex to a byte.
#define VERTEX_COLOR_SCALE 255.0f

// Take an entry of g_renderBuffer for a vertex, or report the entry it already has.
static inline D3DTLVERTEX *ClaimRenderBufferEntry(GbiVertex *pVertex, int *pnIndex) {
    if (std::greater_equal<>{}(pVertex, g_aGbiVertexCache) &&
        std::less<>{}(pVertex, g_aGbiVertexCache + GBI_VERTEX_CACHE_SIZE)) {
        if (pVertex->nRenderBufferIndex != -1) {
            *pnIndex = pVertex->nRenderBufferIndex;
            return nullptr;
        }
        pVertex->nRenderBufferIndex = g_nRenderBufferIndex;
        g_apVertexPool[g_nVertexPoolCount++] = pVertex;
    }
    *pnIndex = g_nRenderBufferIndex++;
    return &g_renderBuffer[*pnIndex];
}

static inline void CopyVertexPosition(D3DTLVERTEX *pOut, const GbiVertex *pVertex) {
    pOut->sx = pVertex->fX;
    pOut->sy = pVertex->fY;
    pOut->sz = pVertex->fZ;
    pOut->rhw = pVertex->fRhw;
}

static inline void CopyVertexTexCoords(D3DTLVERTEX *pOut, const GbiVertex *pVertex) {
    pOut->tu = pVertex->fU;
    pOut->tv = pVertex->fV;
}

static inline D3DCOLOR VertexColor(const GbiVertex *pVertex) {
    // The channels round to the nearest byte and are not limited, so one above 255 runs into the
    // channel above it.
    DWORD dwRed = static_cast<DWORD>(std::lrintf(pVertex->fRed * VERTEX_COLOR_SCALE));
    DWORD dwGreen = static_cast<DWORD>(std::lrintf(pVertex->fGreen * VERTEX_COLOR_SCALE));
    DWORD dwBlue = static_cast<DWORD>(std::lrintf(pVertex->fBlue * VERTEX_COLOR_SCALE));
    return ((dwRed | 0xffffff00) << 8 | dwGreen) << 8 | dwBlue;
}

static inline D3DCOLOR VertexFog(const GbiVertex *pVertex) {
    int nAlpha = static_cast<int>(g_fVertexFogRange * pVertex->fRhw);
    if (nAlpha < g_dwFogAlphaCutoff) {
        nAlpha = 0;
    }
    if (nAlpha > 0xff) {
        nAlpha = 0xff;
    }
    return static_cast<D3DCOLOR>(nAlpha) << 24;
}

int SubmitVertexToRenderBuffer(GbiVertex *pVertex) {
    int nIndex;
    D3DTLVERTEX *pOut = ClaimRenderBufferEntry(pVertex, &nIndex);
    if (pOut != nullptr) {
        CopyVertexPosition(pOut, pVertex);
        pOut->color = VertexColor(pVertex);
        CopyVertexTexCoords(pOut, pVertex);
    }
    return nIndex;
}

int SubmitVertexToRenderBufferEx(GbiVertex *pVertex) {
    int nIndex;
    D3DTLVERTEX *pOut = ClaimRenderBufferEntry(pVertex, &nIndex);
    if (pOut != nullptr) {
        CopyVertexPosition(pOut, pVertex);
        pOut->color = VertexColor(pVertex);
        pOut->specular = VertexFog(pVertex);
        CopyVertexTexCoords(pOut, pVertex);
    }
    return nIndex;
}

int SubmitVertexToRenderBufferConstColor(GbiVertex *pVertex) {
    int nIndex;
    D3DTLVERTEX *pOut = ClaimRenderBufferEntry(pVertex, &nIndex);
    if (pOut != nullptr) {
        CopyVertexPosition(pOut, pVertex);
        pOut->color = g_dwVertexConstantColor;
        CopyVertexTexCoords(pOut, pVertex);
    }
    return nIndex;
}

int SubmitVertexToRenderBufferFogged(GbiVertex *pVertex) {
    int nIndex;
    D3DTLVERTEX *pOut = ClaimRenderBufferEntry(pVertex, &nIndex);
    if (pOut != nullptr) {
        CopyVertexPosition(pOut, pVertex);
        pOut->color = g_dwVertexConstantColor;
        pOut->specular = VertexFog(pVertex);
        CopyVertexTexCoords(pOut, pVertex);
    }
    return nIndex;
}

int (*g_pfnSubmitVertexToRenderBuffer)(GbiVertex *pVertex) = SubmitVertexToRenderBuffer;
float g_fTexCoordScaleS;
float g_fTexCoordScaleT;

// The entries a triangle takes in each buffer.
#define TRIANGLE_VERTICES 3

void SubmitTriangleToRenderBuffer(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2) {
    g_awRenderIndexBuffer[g_nRenderIndexCount++] =
        static_cast<WORD>(g_pfnSubmitVertexToRenderBuffer(pVertex2));
    g_awRenderIndexBuffer[g_nRenderIndexCount++] =
        static_cast<WORD>(g_pfnSubmitVertexToRenderBuffer(pVertex1));
    g_awRenderIndexBuffer[g_nRenderIndexCount++] =
        static_cast<WORD>(g_pfnSubmitVertexToRenderBuffer(pVertex0));
    if (g_nRenderBufferIndex + TRIANGLE_VERTICES > RENDER_BUFFER_SIZE ||
        g_nRenderIndexCount + TRIANGLE_VERTICES > RENDER_BUFFER_SIZE) {
        FlushIndexedRenderBuffer();
    }
}

void ScaleVertexTexCoords(GbiVertex *pVertex, const GbiVertexSource *pSource) {
    pVertex->fU = pSource->fS * g_fTexCoordScaleS;
    pVertex->fV = pSource->fT * g_fTexCoordScaleT;
}

template <bool FarDepth, bool FlatColor = false>
static void ClipAndSubmitTriangleImpl(GbiVertex *pVertex0,
                                      GbiVertex *pVertex1,
                                      GbiVertex *pVertex2,
                                      float fRed = 0,
                                      float fGreen = 0,
                                      float fBlue = 0) {
    pVertex0->source.pNext = &pVertex1->source;
    pVertex1->source.pNext = &pVertex2->source;
    pVertex2->source.pNext = &pVertex0->source;
    GbiClipPolygon polygon = {&pVertex0->source, 3};
    using PlaneClipper = void (*)(GbiClipPolygon *);
    const PlaneClipper clippers[] = {ClipPolygonNearZPlane,
                                     ClipPolygonLeftPlane,
                                     ClipPolygonRightPlane,
                                     ClipPolygonTopPlane,
                                     ClipPolygonFarZPlane,
                                     ClipPolygonBottomPlane,
                                     ClipPolygonPositiveWPlane};
    for (auto clip : clippers) {
        clip(&polygon);
        if (polygon.nCount < TRIANGLE_VERTICES) {
            for (int i = 0; i < polygon.nCount; ++i) {
                GbiVertexSource *pVertex = polygon.pHead;
                polygon.pHead = pVertex->pNext;
                RecycleClipVertex(pVertex);
            }
            return;
        }
    }
    GbiVertex vertices[9];
    for (int i = 0; i < polygon.nCount; ++i) {
        GbiVertexSource *pVertex = polygon.pHead;
        polygon.pHead = pVertex->pNext;
        float fVertexRed = FlatColor ? fRed : pVertex->fRed;
        float fVertexGreen = FlatColor ? fGreen : pVertex->fGreen;
        float fVertexBlue = FlatColor ? fBlue : pVertex->fBlue;
        if constexpr (FarDepth) {
            float fRhw = 1.0L / pVertex->fW;
            vertices[i].fX = static_cast<long double>(pVertex->fX) * g_fViewportScaleX * fRhw +
                             g_fViewportTranslateX;
            vertices[i].fY = static_cast<long double>(pVertex->fY) * g_fViewportScaleY * fRhw +
                             g_fViewportTranslateY;
            vertices[i].fZ = 0.999f;
            vertices[i].fRhw = 1.0f / 65535.0f;
            vertices[i].fRed = fVertexRed;
            vertices[i].fGreen = fVertexGreen;
            vertices[i].fBlue = fVertexBlue;
        } else {
            TransformVertexToScreen(&vertices[i], pVertex, fVertexRed, fVertexGreen, fVertexBlue);
        }
        ScaleVertexTexCoords(&vertices[i], pVertex);
        RecycleClipVertex(pVertex);
    }
    for (int i = 2; i < polygon.nCount; ++i) {
        SubmitTriangleToRenderBuffer(&vertices[0], &vertices[i - 1], &vertices[i]);
    }
}

void ClipAndSubmitTriangle(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2) {
    ClipAndSubmitTriangleImpl<false>(pVertex0, pVertex1, pVertex2);
}

void ClipAndSubmitTriangleAtFarDepth(GbiVertex *pVertex0,
                                     GbiVertex *pVertex1,
                                     GbiVertex *pVertex2) {
    ClipAndSubmitTriangleImpl<true>(pVertex0, pVertex1, pVertex2);
}

void ClipPolygonAndSubmitFlat(GbiVertex *pVertex0,
                              GbiVertex *pVertex1,
                              GbiVertex *pVertex2,
                              float fRed,
                              float fGreen,
                              float fBlue) {
    ClipAndSubmitTriangleImpl<false, true>(pVertex0, pVertex1, pVertex2, fRed, fGreen, fBlue);
}

void ClipPolygonAndSubmitDepthScaled(GbiVertex *pVertex0,
                                     GbiVertex *pVertex1,
                                     GbiVertex *pVertex2,
                                     float fRed,
                                     float fGreen,
                                     float fBlue) {
    ClipAndSubmitTriangleImpl<true, true>(pVertex0, pVertex1, pVertex2, fRed, fGreen, fBlue);
}

template <bool FarDepth>
static void DrawFlatShadedTriangleImpl(int nVertex0, int nVertex1, int nVertex2) {
    GbiVertex *pVertex0 = &g_aGbiVertexCache[nVertex0];
    GbiVertex *pVertex1 = &g_aGbiVertexCache[nVertex1];
    GbiVertex *pVertex2 = &g_aGbiVertexCache[nVertex2];
    if (pVertex0->nClipFlags & pVertex2->nClipFlags & pVertex1->nClipFlags) {
        return;
    }
    if (pVertex0->nClipFlags | pVertex2->nClipFlags | pVertex1->nClipFlags) {
        auto clip = FarDepth ? ClipPolygonAndSubmitDepthScaled : ClipPolygonAndSubmitFlat;
        clip(pVertex0,
             pVertex1,
             pVertex2,
             pVertex0->source.fRed,
             pVertex0->source.fGreen,
             pVertex0->source.fBlue);
        return;
    }
    ScaleVertexTexCoords(pVertex0, &pVertex0->source);
    ScaleVertexTexCoords(pVertex1, &pVertex1->source);
    ScaleVertexTexCoords(pVertex2, &pVertex2->source);
    Vec3f color1 = {pVertex1->fRed, pVertex1->fGreen, pVertex1->fBlue};
    Vec3f color2 = {pVertex2->fRed, pVertex2->fGreen, pVertex2->fBlue};
    pVertex1->fRed = pVertex2->fRed = pVertex0->fRed;
    pVertex1->fGreen = pVertex2->fGreen = pVertex0->fGreen;
    pVertex1->fBlue = pVertex2->fBlue = pVertex0->fBlue;
    SubmitTriangleToRenderBuffer(pVertex0, pVertex1, pVertex2);
    pVertex1->fRed = color1.x;
    pVertex1->fGreen = color1.y;
    pVertex1->fBlue = color1.z;
    pVertex2->fRed = color2.x;
    pVertex2->fGreen = color2.y;
    pVertex2->fBlue = color2.z;
}

void DrawFlatShadedTriangle(int nVertex0, int nVertex1, int nVertex2) {
    DrawFlatShadedTriangleImpl<false>(nVertex0, nVertex1, nVertex2);
}

void DrawFlatShadedTriangleDepthScaled(int nVertex0, int nVertex1, int nVertex2) {
    DrawFlatShadedTriangleImpl<true>(nVertex0, nVertex1, nVertex2);
}

static unsigned char ConvertFillColorChannel(float fChannel) {
    long double value = static_cast<long double>(fChannel) * 255.0f;
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    return static_cast<unsigned char>(static_cast<long long>(value));
}

void DrawFillRectangle(int nLeft, int nTop, int nRight, int nBottom) {
    if (nLeft < g_dwScissorLeft) {
        nLeft = g_dwScissorLeft;
    }
    if (nTop < g_dwScissorBottom) {
        nTop = g_dwScissorBottom;
    }
    if (nRight > g_dwScissorRight) {
        nRight = g_dwScissorRight;
    }
    if (nBottom > g_dwScissorTop) {
        nBottom = g_dwScissorTop;
    }
    unsigned char red = g_bGbiFillColorR;
    unsigned char green = g_bGbiFillColorG;
    unsigned char blue = g_bGbiFillColorB;
    unsigned char alpha = g_bGbiFillColorA;
    if (g_dwGbiCombineModeHi == 0xfcffffff && g_dwGbiCombineModeLo == 0xfffdf6fb) {
        red = ConvertFillColorChannel(g_fGbiPrimColorR);
        green = ConvertFillColorChannel(g_fGbiPrimColorG);
        blue = ConvertFillColorChannel(g_fGbiPrimColorB);
        alpha = ConvertFillColorChannel(g_fGbiPrimColorA);
    }
    int nScreenTop = static_cast<int>(static_cast<unsigned>(g_dwConfigFramebufferHeight) -
                                      static_cast<unsigned>(nBottom));
    int nScreenBottom = static_cast<int>(static_cast<unsigned>(g_dwConfigFramebufferHeight) -
                                         static_cast<unsigned>(nTop));
    if (g_dwGbiRenderMode != 0x504340) {
        FlushIndexedRenderBuffer();
        D3DMaterial *pMaterial = g_pMainRenderWindow->pMaterial;
        pMaterial->desc.diffuse.r = static_cast<float>(red) * (1.0f / 255.0f);
        pMaterial->desc.diffuse.g = static_cast<float>(green) * (1.0f / 255.0f);
        pMaterial->desc.diffuse.b = static_cast<float>(blue) * (1.0f / 255.0f);
        pMaterial->desc.dwRampSize = 1;
        pMaterial->bDirty = true;
        pMaterial->CommitMaterialIfDirty();
        g_pMainRenderWindow->pViewport->SetBackground(pMaterial->hMaterial);
        D3DRECT rect;
        rect.x1 = nLeft;
        rect.y1 = nScreenTop;
        rect.x2 = nRight;
        rect.y2 = nScreenBottom;
        g_pMainRenderWindow->pViewport->Clear(1, &rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
        return;
    }
    g_pfnSubmitVertexToRenderBuffer =
        g_dwWeatherActive ? SubmitVertexToRenderBufferEx : SubmitVertexToRenderBuffer;
    SetPendingRenderState(&g_dwPendingRS_SrcBlend,
                          g_dwAppliedRS_SrcBlend,
                          D3DBLEND_SRCALPHA,
                          RENDER_STATE_DIRTY_SRC_BLEND);
    SetPendingRenderState(&g_dwPendingRS_DestBlend,
                          g_dwAppliedRS_DestBlend,
                          D3DBLEND_INVSRCALPHA,
                          RENDER_STATE_DIRTY_DEST_BLEND);
    SetPendingRenderState(
        &g_dwPendingRS_AlphaRef, g_dwAppliedRS_AlphaRef, 0, RENDER_STATE_DIRTY_ALPHA_REF);
    g_dwD3DAlphaFunc = D3DCMP_ALWAYS;
    SetPendingRenderState(&g_dwPendingRS_AlphaFunc,
                          g_dwAppliedRS_AlphaFunc,
                          g_dwD3DAlphaFunc,
                          RENDER_STATE_DIRTY_ALPHA_FUNC);
    SetPendingRenderState(&g_dwPendingRS_AlphaTestEnable,
                          g_dwAppliedRS_AlphaTestEnable,
                          false,
                          RENDER_STATE_DIRTY_ALPHA_TEST_ENABLE);
    SetPendingRenderState(&g_dwPendingRS_AlphaBlendEnable,
                          g_dwAppliedRS_AlphaBlendEnable,
                          true,
                          RENDER_STATE_DIRTY_ALPHA_BLEND_ENABLE);
    g_dwD3DZFunc = D3DCMP_ALWAYS;
    SetPendingRenderState(
        &g_dwPendingRS_ZFunc, g_dwAppliedRS_ZFunc, g_dwD3DZFunc, RENDER_STATE_DIRTY_Z_FUNC);
    SetPendingRenderState(&g_dwPendingRS_ZWriteEnable,
                          g_dwAppliedRS_ZWriteEnable,
                          false,
                          RENDER_STATE_DIRTY_Z_WRITE_ENABLE);
    FlushDirtyRenderStates();
    D3DCOLOR color = static_cast<D3DCOLOR>(alpha) << 24 | static_cast<D3DCOLOR>(red) << 16 |
                     static_cast<D3DCOLOR>(green) << 8 | blue;
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    auto makeVertex = [color](float x, float y, float u, float v) {
        D3DTLVERTEX vertex;
        vertex.sx = x;
        vertex.sy = y;
        vertex.sz = 0.0f;
        vertex.rhw = 1.0f;
        vertex.color = color;
        vertex.specular = 0xff0000ff;
        vertex.tu = u;
        vertex.tv = v;
        return vertex;
    };
    D3DTLVERTEX triangle[3] = {makeVertex(nLeft, nScreenTop, 0, 0),
                               makeVertex(nRight, nScreenBottom, 1, 1),
                               makeVertex(nLeft, nScreenBottom, 0, 1)};
    g_pD3DDevice2->DrawPrimitive(
        D3DPT_TRIANGLELIST, D3DVT_TLVERTEX, triangle, 3, D3DDP_WAIT | D3DDP_DONOTUPDATEEXTENTS);
    triangle[1] = makeVertex(nRight, nScreenTop, 1, 0);
    triangle[2] = makeVertex(nRight, nScreenBottom, 1, 1);
    g_pD3DDevice2->DrawPrimitive(
        D3DPT_TRIANGLELIST, D3DVT_TLVERTEX, triangle, 3, D3DDP_WAIT | D3DDP_DONOTUPDATEEXTENTS);
    g_pD3DDevice2->SetRenderState(D3DRENDERSTATE_CULLMODE, g_dwAppliedRS_CullMode);
}

// A routine that clips a triangle of g_aGbiVertexCache and submits what remains.
typedef void (*TriangleClipper)(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2);

// A routine that draws a triangle of g_aGbiVertexCache by the entries of its vertices.
typedef void (*IndexedTriangleDrawer)(int nVertex0, int nVertex1, int nVertex2);

static inline void
SubmitIndexedTriangleClipped(int nVertex0, int nVertex1, int nVertex2, TriangleClipper pfnClip) {
    GbiVertex *pVertex0 = &g_aGbiVertexCache[nVertex0];
    GbiVertex *pVertex1 = &g_aGbiVertexCache[nVertex1];
    GbiVertex *pVertex2 = &g_aGbiVertexCache[nVertex2];

    if (pVertex0->nClipFlags & pVertex2->nClipFlags & pVertex1->nClipFlags) {
        return;
    }
    if (pVertex0->nClipFlags | pVertex2->nClipFlags | pVertex1->nClipFlags) {
        pfnClip(pVertex0, pVertex1, pVertex2);
        return;
    }
    ScaleVertexTexCoords(pVertex0, &pVertex0->source);
    ScaleVertexTexCoords(pVertex1, &pVertex1->source);
    ScaleVertexTexCoords(pVertex2, &pVertex2->source);
    SubmitTriangleToRenderBuffer(pVertex0, pVertex1, pVertex2);
}

void SubmitIndexedTriangle(int nVertex0, int nVertex1, int nVertex2) {
    SubmitIndexedTriangleClipped(nVertex0, nVertex1, nVertex2, ClipAndSubmitTriangle);
}

void SubmitIndexedTriangleRasterized(int nVertex0, int nVertex1, int nVertex2) {
    SubmitIndexedTriangleClipped(nVertex0, nVertex1, nVertex2, ClipAndSubmitTriangleAtFarDepth);
}

static inline int GbiTriangleVertex(unsigned dwWord, int nByte) {
    return dwWord >> (nByte * 8) & 0xff;
}

// Draw the triangle of the entries of g_aGbiVertexCache in the low three bytes of a word, from the
// third.
static inline void DrawGbiTriangleWord(unsigned dwWord, IndexedTriangleDrawer pfnDraw) {
    pfnDraw(
        GbiTriangleVertex(dwWord, 2), GbiTriangleVertex(dwWord, 1), GbiTriangleVertex(dwWord, 0));
}

static inline GbiCommand *DrawGbiTri1(GbiCommand *pCommand, IndexedTriangleDrawer pfnDraw) {
    FlushDirtyRenderStates();
    DrawGbiTriangleWord(pCommand->w1, pfnDraw);
    return pCommand + 1;
}

static inline GbiCommand *DrawGbiTri2(GbiCommand *pCommand, IndexedTriangleDrawer pfnDraw) {
    FlushDirtyRenderStates();
    DrawGbiTriangleWord(pCommand->w0, pfnDraw);
    DrawGbiTriangleWord(pCommand->w1, pfnDraw);
    return pCommand + 1;
}

static inline GbiCommand *DrawGbiTri1Flat(GbiCommand *pCommand, IndexedTriangleDrawer pfnDraw) {
    unsigned dwWord = pCommand->w1;

    FlushDirtyRenderStates();
    // The vertex the top byte chooses comes first, in the order of the console.
    switch (GbiTriangleVertex(dwWord, 3)) {
    case 0:
        pfnDraw(GbiTriangleVertex(dwWord, 2),
                GbiTriangleVertex(dwWord, 1),
                GbiTriangleVertex(dwWord, 0));
        break;
    case 1:
        pfnDraw(GbiTriangleVertex(dwWord, 1),
                GbiTriangleVertex(dwWord, 0),
                GbiTriangleVertex(dwWord, 2));
        break;
    default:
        pfnDraw(GbiTriangleVertex(dwWord, 0),
                GbiTriangleVertex(dwWord, 2),
                GbiTriangleVertex(dwWord, 1));
        break;
    }
    return pCommand + 1;
}

GbiCommand *HandleGbiTri1Smooth(GbiCommand *pCommand) {
    return DrawGbiTri1(pCommand, SubmitIndexedTriangle);
}

GbiCommand *HandleGbiTri2Smooth(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, SubmitIndexedTriangle);
}

GbiCommand *HandleGbiTri1Flat(GbiCommand *pCommand) {
    return DrawGbiTri1Flat(pCommand, DrawFlatShadedTriangle);
}

GbiCommand *HandleGbiTri2Flat(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, DrawFlatShadedTriangle);
}

GbiCommand *HandleGbiTri1SmoothNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri1(pCommand, SubmitIndexedTriangleRasterized);
}

GbiCommand *HandleGbiTri2SmoothNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, SubmitIndexedTriangleRasterized);
}

GbiCommand *HandleGbiTri1FlatNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri1Flat(pCommand, DrawFlatShadedTriangleDepthScaled);
}

GbiCommand *HandleGbiTri2FlatNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, DrawFlatShadedTriangleDepthScaled);
}

// The colours of the combine modes that draw in one colour.
#define VERTEX_CONSTANT_COLOR_WHITE 0xffffffff
#define VERTEX_CONSTANT_COLOR_BLACK 0xff000000

static inline bool
IsCombineMode(unsigned dwHi, unsigned dwLo, unsigned dwModeHi, unsigned dwModeLo) {
    return dwHi == dwModeHi && dwLo == dwModeLo;
}

// Take the routine that copies a vertex with its fog in weather, and with its colour alone in
// sunshine.
static inline void SelectWeatherVertexSubmitter(void) {
    g_pfnSubmitVertexToRenderBuffer =
        g_dwWeatherActive ? SubmitVertexToRenderBufferEx : SubmitVertexToRenderBuffer;
}

static inline void SelectConstantColorVertexSubmitter(D3DCOLOR dwColor) {
    g_dwVertexConstantColor = dwColor;
    g_pfnSubmitVertexToRenderBuffer =
        g_dwWeatherActive ? SubmitVertexToRenderBufferFogged : SubmitVertexToRenderBufferConstColor;
}

void SelectVertexSubmitterForCombineMode(unsigned dwCombineHi, unsigned dwCombineLo) {
    bool bSpecial = false;

    g_bCombineModeSpecialSubmitter = false;
    if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xfffcf87c)) {
        SelectConstantColorVertexSubmitter(VERTEX_CONSTANT_COLOR_WHITE);
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xfffe793c)) {
        SelectWeatherVertexSubmitter();
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc567eac, 0xfffff3f9)) {
        SelectConstantColorVertexSubmitter(VERTEX_CONSTANT_COLOR_BLACK);
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcff97ff, 0xff2dfeff)) {
        SelectWeatherVertexSubmitter();
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xfffdf2f9)) {
        SelectWeatherVertexSubmitter();
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xffff73b9)) {
        SelectConstantColorVertexSubmitter(VERTEX_CONSTANT_COLOR_WHITE);
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc127e08, 0xf3fff2f8)) {
        // Without night or rain, the routine chosen last is kept.
        if (g_dwNightMode || g_dwRainMode) {
            SelectWeatherVertexSubmitter();
        }
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc317e02, 0x5ffef3fa) ||
               IsCombineMode(dwCombineHi, dwCombineLo, 0xfc317e02, 0x51fef3fa)) {
        SelectWeatherVertexSubmitter();
        bSpecial = true;
        g_bCombineModeSpecialSubmitter = true;
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc127fff, 0xfffff838)) {
        SelectConstantColorVertexSubmitter(VERTEX_CONSTANT_COLOR_BLACK);
    } else {
        SelectWeatherVertexSubmitter();
    }
    if (bSpecial) {
        if (g_apfnGbiHandlers[GBI_OP_VERTEX] == HandleGbiVertexLit) {
            g_apfnGbiHandlers[GBI_OP_VERTEX] = HandleGbiVertexLitEnvColorModulated;
        }
    } else if (g_apfnGbiHandlers[GBI_OP_VERTEX] == HandleGbiVertexLitEnvColorModulated) {
        g_apfnGbiHandlers[GBI_OP_VERTEX] = HandleGbiVertexLit;
    }
}
#endif

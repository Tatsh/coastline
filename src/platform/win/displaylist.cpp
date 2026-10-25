#include "platform/win/displaylist.h"

#include <cstdlib>
#include <cstring>

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

GbiCommand *HandleGbiMoveWord(GbiCommand *pCommand) {
    unsigned dwOffset;

    switch (static_cast<signed char>(pCommand->w0)) {
    case GBI_MOVE_WORD_NUM_LIGHT:
        g_dwGbiNumLights = pCommand->w1 >> 5 & 0xf;
        break;
    case GBI_MOVE_WORD_LIGHT_COLOR:
        dwOffset = pCommand->w0 >> 8 & 0xffff;
        if (dwOffset & 0xf) {
            SetGbiLightColor(g_aGbiLights[dwOffset / GBI_MOVE_WORD_LIGHT_STRIDE].abColorCopy,
                             pCommand->w1);
        } else {
            SetGbiLightColor(g_aGbiLights[dwOffset / GBI_MOVE_WORD_LIGHT_STRIDE].abColor,
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
const void *g_pGbiLookAtY;
const void *g_pGbiLookAtX;

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
        g_pGbiLookAtY = pCommand->pData;
        break;
    case GBI_MOVE_MEM_LOOK_AT_X:
        g_pGbiLookAtX = pCommand->pData;
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

void (*g_pfnBindTextureEntry)(int nEntry);

GbiCommand *HandleGbiBindTextureEntry(GbiCommand *pCommand) {
    int nEntry = pCommand->w0 & GBI_TEXTURE_ENTRY_MASK;
    FlushIndexedRenderBuffer();
    g_pfnBindTextureEntry(nEntry);
    return pCommand + pCommand->w1;
}
#endif

void (*g_pfnUploadTextureEntryToSurface)(int nEntry, unsigned dwUnused);

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

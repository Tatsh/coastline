#include "platform/win/glide/displaylist.h"

#include <cmath>
#include <functional>

#include "convertedvertex.h"
#include "displaylistops.h"
#include "displaylistptr.h"
#include "platform/win/glide/renderer.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/timer.h"
#include "screen.h"
#include "weather.h"

// Windows headers define DrawText as a GDI alias.
#undef DrawText
#include "text.h"

// The bits of the geometry mode of the console the handlers read.
#define GBI_GEOMETRY_Z_BUFFER 0x1
#define GBI_GEOMETRY_SHADING_SMOOTH 0x200
#define GBI_GEOMETRY_CULL_FRONT 0x1000
#define GBI_GEOMETRY_CULL_BACK 0x2000
#define GBI_GEOMETRY_FOG 0x10000
#define GBI_GEOMETRY_LIGHTING 0x20000
#define GBI_GEOMETRY_TEXTURE_GEN 0x40000
#define GBI_GEOMETRY_TEXTURE_GEN_LINEAR 0x80000

// The bits of the first word of a command that give the entry of a texture the game loaded.
#define GBI_TEXTURE_ENTRY_MASK 0xffffff

// The entries a triangle takes.
#define TRIANGLE_VERTICES 3

// The most vertices that remain after a triangle is clipped to the seven planes of the view.
#define CLIPPED_POLYGON_VERTICES 9

// The brightest channel of a vertex.
#define GLIDE_COLOR_MAX 255.0f

// The w a vertex drawn without the depth buffer takes.
#define GLIDE_FAR_ONE_OVER_W (1.0f / 65535.0f)

// The divisor of the coordinates and the vectors of the lights of the console.
#define GBI_LIGHT_SCALE 128.0f

// The texels a generated texture coordinate is offset by.
#define GBI_TEXTURE_GEN_OFFSET 128.0f

int g_dwGlideCullMode;
int g_dwGlideDepthFunction;
int g_dwGlideAlphaTestFunction;
int g_dwGlideBlendRgbSource;
int g_dwGlideBlendRgbDest;
int g_dwGlideBlendAlphaSource;
int g_dwGlideBlendAlphaDest;
GrFog_t g_aGlideFogTable[GR_FOG_TABLE_SIZE];
int g_nGbiVertexSnap;
GbiVertex g_aGbiVertexCache[GBI_VERTEX_CACHE_SIZE];
int g_bCombineModeSpecialSubmitter;

static inline void SetGlideDepthFunction(int nFunction) {
    g_dwGlideDepthFunction = nFunction;
    grDepthBufferFunction(nFunction);
}

// Pass the alpha of a texel of at least a reference value.
static inline void SetGlideAlphaTest(int nReference) {
    grAlphaTestReferenceValue(static_cast<GrAlpha_t>(nReference));
    g_dwGlideAlphaTestFunction = GR_CMP_GEQUAL;
    grAlphaTestFunction(GR_CMP_GEQUAL);
}

static inline void
SetGlideAlphaBlend(int nRgbSource, int nRgbDest, int nAlphaSource, int nAlphaDest) {
    g_dwGlideBlendRgbSource = nRgbSource;
    g_dwGlideBlendRgbDest = nRgbDest;
    g_dwGlideBlendAlphaSource = nAlphaSource;
    g_dwGlideBlendAlphaDest = nAlphaDest;
    grAlphaBlendFunction(nRgbSource, nRgbDest, nAlphaSource, nAlphaDest);
}

// Take the alpha of the texel, scaled by the constant alpha.
static inline void SetGlideTextureAlphaCombine(void) {
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                   GR_COMBINE_FACTOR_ONE,
                   GR_COMBINE_LOCAL_CONSTANT,
                   GR_COMBINE_OTHER_TEXTURE,
                   FXFALSE);
}

// Take the constant alpha, scaled by the alpha of the texel.
static inline void SetGlideConstantAlphaCombine(void) {
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                   GR_COMBINE_FACTOR_TEXTURE_ALPHA,
                   GR_COMBINE_LOCAL_CONSTANT,
                   GR_COMBINE_OTHER_CONSTANT,
                   FXFALSE);
}

void ApplyRdpRenderMode(unsigned dwRenderMode) {
    grDepthMask(FXTRUE);
    if (dwRenderMode == 0x504f50 || dwRenderMode == 4) {
        SetGlideAlphaTest(8);
        if (!g_bRdpBlendStateOverride) {
            SetGlideTextureAlphaCombine();
            SetGlideAlphaBlend(GR_BLEND_SRC_ALPHA,
                               dwRenderMode == 4 ? GR_BLEND_ONE : GR_BLEND_ONE_MINUS_SRC_ALPHA,
                               GR_BLEND_ONE,
                               GR_BLEND_ZERO);
        }
        SetGlideDepthFunction(GR_CMP_EQUAL);
        return;
    }
    if (dwRenderMode == 0xc184240) {
        SetGlideAlphaTest(0x80);
        return;
    }
    if (dwRenderMode == 0x504240) {
        SetGlideAlphaTest(8);
        if (!g_bRdpBlendStateOverride) {
            SetGlideTextureAlphaCombine();
            SetGlideAlphaBlend(
                GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ONE, GR_BLEND_ZERO);
        }
        return;
    }
    if (dwRenderMode == 3) {
        g_bRdpBlendStateOverride = false;
        return;
    }
    if (dwRenderMode == 1) {
        SetGlideDepthFunction(GR_CMP_LEQUAL);
        grDepthMask(FXFALSE);
        return;
    }
    if (dwRenderMode == 0) {
        SetGlideDepthFunction(GR_CMP_LESS);
        return;
    }
    if (dwRenderMode == 2 || dwRenderMode == 0xd1849d8) {
        SetGlideConstantAlphaCombine();
        SetGlideAlphaBlend(GR_BLEND_SRC_ALPHA,
                           dwRenderMode == 2 ? GR_BLEND_ONE : GR_BLEND_ONE_MINUS_SRC_ALPHA,
                           GR_BLEND_ONE,
                           GR_BLEND_ZERO);
        grDepthMask(FXFALSE);
        g_bRdpBlendStateOverride = true;
        return;
    }
    if (dwRenderMode & 0x1800) {
        if (dwRenderMode & 0x10000) {
            SetGlideAlphaTest(0x80);
            return;
        }
        SetGlideAlphaTest(1);
        if (!g_bRdpBlendStateOverride) {
            SetGlideTextureAlphaCombine();
            SetGlideAlphaBlend(
                GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ONE, GR_BLEND_ZERO);
        }
        grDepthMask(FXFALSE);
        return;
    }
    if (!g_bRdpBlendStateOverride) {
        SetGlideTextureAlphaCombine();
        SetGlideAlphaBlend(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
    }
    g_dwGlideAlphaTestFunction = GR_CMP_ALWAYS;
    grAlphaTestFunction(GR_CMP_ALWAYS);
}

void ApplyGeometryModeChanges(void) {
    if ((g_dwGbiPrevGeometryMode ^ g_dwGbiGeometryMode) & GBI_GEOMETRY_FOG) {
        grFogMode((g_dwGbiGeometryMode & GBI_GEOMETRY_FOG) ? GR_FOG_WITH_TABLE : GR_FOG_DISABLE);
    }
    if ((g_dwGbiPrevGeometryMode ^ g_dwGbiGeometryMode) &
        (GBI_GEOMETRY_CULL_FRONT | GBI_GEOMETRY_CULL_BACK)) {
        g_dwGlideCullMode = GR_CULL_DISABLE;
        if (g_dwGbiGeometryMode & GBI_GEOMETRY_CULL_FRONT) {
            g_dwGlideCullMode = GR_CULL_POSITIVE;
        } else if (g_dwGbiGeometryMode & GBI_GEOMETRY_CULL_BACK) {
            g_dwGlideCullMode = GR_CULL_NEGATIVE;
        }
        grCullMode(g_dwGlideCullMode);
    }
    if ((g_dwGbiPrevGeometryMode ^ g_dwGbiGeometryMode) & GBI_GEOMETRY_Z_BUFFER) {
        SetGlideDepthFunction((g_dwGbiGeometryMode & GBI_GEOMETRY_Z_BUFFER) ? GR_CMP_LESS :
                                                                              GR_CMP_ALWAYS);
    }
    unsigned dwMode = g_dwGbiGeometryMode;
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

GbiCommand *HandleGbiBindTextureEntry(GbiCommand *pCommand) {
    g_pfnBindTextureEntry(pCommand->w0 & GBI_TEXTURE_ENTRY_MASK);
    return pCommand + pCommand->w1;
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
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    if (g_dwGbiCombineModeHi == 0xfcffffff && g_dwGbiCombineModeLo == 0xfffdf6fb) {
        // The colour of the primitive is already from 0 to 255.
        red = static_cast<unsigned char>(static_cast<long>(g_fGbiPrimColorR));
        green = static_cast<unsigned char>(static_cast<long>(g_fGbiPrimColorG));
        blue = static_cast<unsigned char>(static_cast<long>(g_fGbiPrimColorB));
        alpha = static_cast<unsigned char>(static_cast<long>(g_fGbiPrimColorA));
    } else {
        red = g_bGbiFillColorR;
        green = g_bGbiFillColorG;
        blue = g_bGbiFillColorB;
        alpha = g_bGbiFillColorA;
    }
    if (g_dwGbiRenderMode != 0x504340) {
        grClipWindow(nLeft, nTop, nRight, nBottom);
        grBufferClear(static_cast<GrColor_t>(red) << 24 | static_cast<GrColor_t>(green) << 16 |
                          static_cast<GrColor_t>(blue) << 8 | alpha,
                      0,
                      GR_WDEPTHVALUE_FARTHEST);
        grClipWindow(g_dwScissorLeft, g_dwScissorBottom, g_dwScissorRight, g_dwScissorTop);
        return;
    }
    // The depths and the texture coordinates of the corners are not set.
    GrVertex lowerRight;
    GrVertex upperLeft;
    GrVertex lowerLeft;
    GrVertex upperRight;
    GrVertex *apCorners[4] = {&lowerRight, &upperLeft, &lowerLeft, &upperRight};
    const float afX[4] = {static_cast<float>(nRight),
                          static_cast<float>(nLeft),
                          static_cast<float>(nLeft),
                          static_cast<float>(nRight)};
    const float afY[4] = {static_cast<float>(nBottom),
                          static_cast<float>(nTop),
                          static_cast<float>(nBottom),
                          static_cast<float>(nTop)};
    for (int i = 0; i < 4; i++) {
        apCorners[i]->x = afX[i];
        apCorners[i]->y = afY[i];
        apCorners[i]->r = static_cast<float>(red);
        apCorners[i]->g = static_cast<float>(green);
        apCorners[i]->b = static_cast<float>(blue);
        apCorners[i]->a = static_cast<float>(alpha);
        apCorners[i]->oow = 1.0f;
    }
    grAlphaBlendFunction(
        GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ONE, GR_BLEND_ZERO);
    grAlphaTestFunction(GR_CMP_ALWAYS);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,
                   GR_COMBINE_FACTOR_ZERO,
                   GR_COMBINE_LOCAL_ITERATED,
                   GR_COMBINE_OTHER_CONSTANT,
                   FXFALSE);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                   GR_COMBINE_FACTOR_ZERO,
                   GR_COMBINE_LOCAL_ITERATED,
                   GR_COMBINE_OTHER_CONSTANT,
                   FXFALSE);
    grDepthBufferFunction(GR_CMP_ALWAYS);
    grDepthMask(FXFALSE);
    grClipWindow(nLeft, nTop, nRight, nBottom);
    grCullMode(GR_CULL_DISABLE);
    grDrawTriangle(&upperLeft, &lowerRight, &lowerLeft);
    grDrawTriangle(&upperLeft, &upperRight, &lowerRight);
    grClipWindow(g_dwScissorLeft, g_dwScissorBottom, g_dwScissorRight, g_dwScissorTop);
    // The cull mode and the combine of alpha go back to the default. The rest is not restored.
    grCullMode(GR_CULL_DISABLE);
    SetGlideTextureAlphaCombine();
}

// Select the combine of colour of a vertex from its colour and the texel.
static inline void SetGlideTextureColorCombine(GrCombineFactor_t nFactor, GrCombineLocal_t nLocal) {
    grColorCombine(
        GR_COMBINE_FUNCTION_SCALE_OTHER, nFactor, nLocal, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
}

// Select the combine that draws in the constant colour.
static inline void SetGlideConstantColorCombine(void) {
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
                   GR_COMBINE_FACTOR_ONE,
                   GR_COMBINE_LOCAL_CONSTANT,
                   GR_COMBINE_OTHER_CONSTANT,
                   FXFALSE);
}

static inline bool
IsCombineMode(unsigned dwHi, unsigned dwLo, unsigned dwModeHi, unsigned dwModeLo) {
    return dwHi == dwModeHi && dwLo == dwModeLo;
}

void SelectVertexSubmitterForCombineMode(unsigned dwCombineHi, unsigned dwCombineLo) {
    g_bCombineModeSpecialSubmitter = false;
    if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xfffcf87c)) {
        SetGlideTextureColorCombine(GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_CONSTANT);
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xfffe793c)) {
        grColorCombine(GR_COMBINE_FUNCTION_LOCAL,
                       GR_COMBINE_FACTOR_ZERO,
                       GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_CONSTANT,
                       FXFALSE);
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc567eac, 0xfffff3f9)) {
        grConstantColorValue(0xff);
        SetGlideConstantColorCombine();
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcff97ff, 0xff2dfeff) ||
               IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xfffdf2f9)) {
        SetGlideConstantColorCombine();
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfcffffff, 0xffff73b9)) {
        grConstantColorValue(0xffffffff);
        SetGlideConstantColorCombine();
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc127e08, 0xf3fff2f8)) {
        // Without night or rain, the combine set last remains.
        if (g_dwNightMode || g_dwRainMode) {
            grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL,
                           GR_COMBINE_FACTOR_TEXTURE_ALPHA,
                           GR_COMBINE_LOCAL_CONSTANT,
                           GR_COMBINE_OTHER_TEXTURE,
                           FXFALSE);
        }
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc317e02, 0x5ffef3fa) ||
               IsCombineMode(dwCombineHi, dwCombineLo, 0xfc317e02, 0x51fef3fa)) {
        SetGlideTextureColorCombine(GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED);
        g_bCombineModeSpecialSubmitter = true;
    } else if (IsCombineMode(dwCombineHi, dwCombineLo, 0xfc127fff, 0xfffff838)) {
        grConstantColorValue(0);
        SetGlideConstantColorCombine();
    } else {
        SetGlideTextureColorCombine(GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED);
    }
    if (g_bCombineModeSpecialSubmitter) {
        if (g_apfnGbiHandlers[GBI_OP_VERTEX] == HandleGbiVertexLit) {
            g_apfnGbiHandlers[GBI_OP_VERTEX] = HandleGbiVertexLitEnvColorModulated;
        }
    } else if (g_apfnGbiHandlers[GBI_OP_VERTEX] == HandleGbiVertexLitEnvColorModulated) {
        g_apfnGbiHandlers[GBI_OP_VERTEX] = HandleGbiVertexLit;
    }
}

GbiCommand *HandleGbiSetFogColor(GbiCommand *pCommand) {
    grFogColorValue(pCommand->w1);
    return pCommand + 1;
}

GbiCommand *HandleGbiSetPrimColor(GbiCommand *pCommand) {
    // Glide takes the colour of the primitive from 0 to 255.
    g_fGbiPrimColorR = static_cast<float>(pCommand->w1 >> 24);
    g_fGbiPrimColorG = static_cast<float>(pCommand->w1 >> 16 & 0xff);
    g_fGbiPrimColorB = static_cast<float>(pCommand->w1 >> 8 & 0xff);
    g_fGbiPrimColorA = static_cast<float>(pCommand->w1 & 0xff);
    grConstantColorValue(pCommand->w1);
    return pCommand + 1;
}

// Record the scissor of the rows of the console, turned over to those of Glide, and clip to it.
static inline void
SetScissorAndClipWindow(int nUpperLeftX, int nUpperLeftY, int nLowerRightX, int nLowerRightY) {
    int nHeight = g_dwConfigFramebufferHeight;
    g_dwScissorLeft = nUpperLeftX;
    g_dwScissorTop = nHeight - nUpperLeftY;
    g_dwScissorRight = nLowerRightX;
    g_dwScissorBottom = nHeight - nLowerRightY;
    grClipWindow(g_dwScissorLeft, g_dwScissorBottom, g_dwScissorRight, g_dwScissorTop);
}

static inline int GbiPixelField10(unsigned dwWord, int nShift) {
    return dwWord >> (nShift + 2) & 0x3ff;
}

static inline int GbiField12(unsigned dwWord, int nShift) {
    return dwWord >> nShift & 0xfff;
}

GbiCommand *HandleGbiSetScissor(GbiCommand *pCommand) {
    SetScissorAndClipWindow(GbiPixelField10(pCommand->w0, 12),
                            GbiPixelField10(pCommand->w0, 0),
                            GbiPixelField10(pCommand->w1, 12),
                            GbiPixelField10(pCommand->w1, 0));
    return pCommand + 1;
}

GbiCommand *HandleGbiSetScissorIntCoords(GbiCommand *pCommand) {
    SetScissorAndClipWindow(GbiField12(pCommand->w0, 12),
                            GbiField12(pCommand->w0, 0),
                            GbiField12(pCommand->w1, 12),
                            GbiField12(pCommand->w1, 0));
    return pCommand + 1;
}

// Round a coordinate on the screen to a quarter of a pixel through an integer.
static inline float SnapScreenCoordinate(float fCoordinate, int &nSnap) {
    float fScaled = fCoordinate * 4.0f;
    nSnap = static_cast<int>(fScaled);
    float fSnapped = static_cast<float>(nSnap);
    return fSnapped * 0.25f;
}

void TransformVertexToScreen(
    GrVertex *pDest, const GbiVertexSource *pSource, float fRed, float fGreen, float fBlue) {
    float fOneOverW = 1.0f / pSource->fW;
    pDest->oow = fOneOverW;
    pDest->x = pSource->fX * g_fViewportScaleX * fOneOverW + g_fViewportTranslateX;
    pDest->y = pSource->fY * g_fViewportScaleY * pDest->oow + g_fViewportTranslateY;
    pDest->r = fRed;
    pDest->g = fGreen;
    pDest->b = fBlue;
    pDest->x = SnapScreenCoordinate(pDest->x, g_nGbiVertexSnap);
    pDest->y = SnapScreenCoordinate(pDest->y, g_nGbiVertexSnap);
}

void TransformVertexToScreenNoZBuffer(
    GrVertex *pDest, const GbiVertexSource *pSource, float fRed, float fGreen, float fBlue) {
    int nSnap = 0;
    float fOneOverW = 1.0f / pSource->fW;
    pDest->oow = fOneOverW;
    pDest->x = pSource->fX * g_fViewportScaleX * fOneOverW + g_fViewportTranslateX;
    pDest->y = pSource->fY * g_fViewportScaleY * pDest->oow + g_fViewportTranslateY;
    pDest->r = fRed;
    // The w of the position is replaced by that of the far plane once the position is known.
    pDest->oow = GLIDE_FAR_ONE_OVER_W;
    pDest->g = fGreen;
    pDest->b = fBlue;
    pDest->x = SnapScreenCoordinate(pDest->x, nSnap);
    pDest->y = SnapScreenCoordinate(pDest->y, nSnap);
}

void ScaleVertexTexCoords(GrVertex *pVertex, const GbiVertexSource *pSource) {
    pVertex->tmuvtx[1].oow = pVertex->oow;
    pVertex->tmuvtx[0].oow = pVertex->oow;
    float fSow = pSource->fS * g_fTexCoordScaleS * pVertex->oow;
    pVertex->tmuvtx[1].sow = fSow;
    pVertex->tmuvtx[0].sow = fSow;
    float fTow = pSource->fT * g_fTexCoordScaleT * pVertex->oow;
    pVertex->tmuvtx[1].tow = fTow;
    pVertex->tmuvtx[0].tow = fTow;
}

static void RecycleClipVertex(GbiVertexSource *pVertex) {
    if (std::greater_equal<>{}(pVertex, g_aClipVertexPool) &&
        std::less<>{}(pVertex, g_aClipVertexPool + GBI_CLIP_VERTEX_POOL_SIZE)) {
        pVertex->pNext = g_pVertexFreeList;
        g_pVertexFreeList = pVertex;
    }
}

template <bool FarDepth, bool FlatColor>
static void ClipAndSubmitTriangleImpl(GbiVertex *pVertex0,
                                      GbiVertex *pVertex1,
                                      GbiVertex *pVertex2,
                                      float fRed = 0,
                                      float fGreen = 0,
                                      float fBlue = 0) {
    pVertex2->source.pNext = nullptr;
    pVertex1->source.pNext = &pVertex2->source;
    pVertex0->source.pNext = &pVertex1->source;
    pVertex2->source.pNext = &pVertex0->source;
    GbiClipPolygon polygon = {&pVertex0->source, TRIANGLE_VERTICES};
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
    GrVertex vertices[CLIPPED_POLYGON_VERTICES];
    for (int i = 0; i < polygon.nCount; ++i) {
        GbiVertexSource *pVertex = polygon.pHead;
        polygon.pHead = pVertex->pNext;
        float fVertexRed = FlatColor ? fRed : pVertex->fRed;
        float fVertexGreen = FlatColor ? fGreen : pVertex->fGreen;
        float fVertexBlue = FlatColor ? fBlue : pVertex->fBlue;
        if constexpr (FarDepth) {
            TransformVertexToScreenNoZBuffer(
                &vertices[i], pVertex, fVertexRed, fVertexGreen, fVertexBlue);
        } else {
            TransformVertexToScreen(&vertices[i], pVertex, fVertexRed, fVertexGreen, fVertexBlue);
        }
        ScaleVertexTexCoords(&vertices[i], pVertex);
        RecycleClipVertex(pVertex);
    }
    if (polygon.nCount == TRIANGLE_VERTICES) {
        grDrawTriangle(&vertices[0], &vertices[1], &vertices[2]);
    } else {
        grDrawPolygonVertexList(polygon.nCount, vertices);
    }
}

void ClipAndSubmitTriangle(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2) {
    ClipAndSubmitTriangleImpl<false, false>(pVertex0, pVertex1, pVertex2);
}

void ClipAndSubmitTriangleAtFarDepth(GbiVertex *pVertex0,
                                     GbiVertex *pVertex1,
                                     GbiVertex *pVertex2) {
    ClipAndSubmitTriangleImpl<true, false>(pVertex0, pVertex1, pVertex2);
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

// A routine that clips a triangle of g_aGbiVertexCache and submits what remains.
typedef void (*TriangleClipper)(GbiVertex *pVertex0, GbiVertex *pVertex1, GbiVertex *pVertex2);

// A routine that draws a triangle of g_aGbiVertexCache by the entries of its vertices.
typedef void (*IndexedTriangleDrawer)(int nVertex0, int nVertex1, int nVertex2);

// Draw a triangle of g_aGbiVertexCache. A triangle outside one plane of the view is dropped, one
// across a plane is clipped, and the texture coordinates of one inside every plane are scaled
// first.
static inline void
DrawIndexedTriangle(int nVertex0, int nVertex1, int nVertex2, TriangleClipper pfnClip) {
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
    ScaleVertexTexCoords(&pVertex0->vertex, &pVertex0->source);
    ScaleVertexTexCoords(&pVertex1->vertex, &pVertex1->source);
    ScaleVertexTexCoords(&pVertex2->vertex, &pVertex2->source);
    grDrawTriangle(&pVertex0->vertex, &pVertex1->vertex, &pVertex2->vertex);
}

static void DrawIndexedTriangleSmooth(int nVertex0, int nVertex1, int nVertex2) {
    DrawIndexedTriangle(nVertex0, nVertex1, nVertex2, ClipAndSubmitTriangle);
}

static void DrawIndexedTriangleSmoothNoZBuffer(int nVertex0, int nVertex1, int nVertex2) {
    DrawIndexedTriangle(nVertex0, nVertex1, nVertex2, ClipAndSubmitTriangleAtFarDepth);
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
    ScaleVertexTexCoords(&pVertex0->vertex, &pVertex0->source);
    ScaleVertexTexCoords(&pVertex1->vertex, &pVertex1->source);
    ScaleVertexTexCoords(&pVertex2->vertex, &pVertex2->source);
    Vec3f color1 = {pVertex1->vertex.r, pVertex1->vertex.g, pVertex1->vertex.b};
    Vec3f color2 = {pVertex2->vertex.r, pVertex2->vertex.g, pVertex2->vertex.b};
    pVertex1->vertex.r = pVertex2->vertex.r = pVertex0->vertex.r;
    pVertex1->vertex.g = pVertex2->vertex.g = pVertex0->vertex.g;
    pVertex1->vertex.b = pVertex2->vertex.b = pVertex0->vertex.b;
    grDrawTriangle(&pVertex0->vertex, &pVertex1->vertex, &pVertex2->vertex);
    pVertex1->vertex.r = color1.x;
    pVertex1->vertex.g = color1.y;
    pVertex1->vertex.b = color1.z;
    pVertex2->vertex.r = color2.x;
    pVertex2->vertex.g = color2.y;
    pVertex2->vertex.b = color2.z;
}

void DrawFlatShadedTriangle(int nVertex0, int nVertex1, int nVertex2) {
    DrawFlatShadedTriangleImpl<false>(nVertex0, nVertex1, nVertex2);
}

void DrawFlatShadedTriangleDepthScaled(int nVertex0, int nVertex1, int nVertex2) {
    DrawFlatShadedTriangleImpl<true>(nVertex0, nVertex1, nVertex2);
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
    DrawGbiTriangleWord(pCommand->w1, pfnDraw);
    return pCommand + 1;
}

static inline GbiCommand *DrawGbiTri2(GbiCommand *pCommand, IndexedTriangleDrawer pfnDraw) {
    DrawGbiTriangleWord(pCommand->w0, pfnDraw);
    DrawGbiTriangleWord(pCommand->w1, pfnDraw);
    return pCommand + 1;
}

static inline GbiCommand *DrawGbiTri1Flat(GbiCommand *pCommand, IndexedTriangleDrawer pfnDraw) {
    unsigned dwWord = pCommand->w1;

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
    return DrawGbiTri1(pCommand, DrawIndexedTriangleSmooth);
}

GbiCommand *HandleGbiTri2Smooth(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, DrawIndexedTriangleSmooth);
}

GbiCommand *HandleGbiTri1Flat(GbiCommand *pCommand) {
    return DrawGbiTri1Flat(pCommand, DrawFlatShadedTriangle);
}

GbiCommand *HandleGbiTri2Flat(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, DrawFlatShadedTriangle);
}

GbiCommand *HandleGbiTri1SmoothNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri1(pCommand, DrawIndexedTriangleSmoothNoZBuffer);
}

GbiCommand *HandleGbiTri2SmoothNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, DrawIndexedTriangleSmoothNoZBuffer);
}

GbiCommand *HandleGbiTri1FlatNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri1Flat(pCommand, DrawFlatShadedTriangleDepthScaled);
}

GbiCommand *HandleGbiTri2FlatNoZBuffer(GbiCommand *pCommand) {
    return DrawGbiTri2(pCommand, DrawFlatShadedTriangleDepthScaled);
}

void DrawTextureRectangle(
    int nUpperLeftX, int nUpperLeftY, int nLowerRightX, int nLowerRightY, int nTile) {
    float width = g_dwConfigFramebufferWidth;
    float height = g_dwConfigFramebufferHeight;
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    auto xCoordinate = [halfWidth](int coordinate) {
        return (static_cast<float>(static_cast<unsigned>(coordinate)) / 4.0f - halfWidth) /
               halfWidth / g_fTexRectScaleW;
    };
    auto yCoordinate = [height, halfHeight](int coordinate) {
        return (height - static_cast<float>(static_cast<unsigned>(coordinate)) / 4.0f -
                halfHeight) /
               halfHeight / g_fTexRectScaleW;
    };
    float left = xCoordinate(nUpperLeftX);
    float top = yCoordinate(nUpperLeftY);
    float right = xCoordinate(nLowerRightX);
    float bottom = yCoordinate(nLowerRightY);
    float depth = g_fTexRectDepth / g_fTexRectScaleW;
    float w = 1.0f / g_fTexRectScaleW;
    float sLeft = static_cast<float>(g_dwTileSizeUls) * 8.0f;
    float sRight = static_cast<float>(g_dwTileSizeLrs) * 8.0f;
    float tTop = static_cast<float>(g_dwTileSizeUlt) * 8.0f;
    float tBottom = static_cast<float>(g_dwTileSizeLrt) * 8.0f;
    // The vertices of the rectangle are not linked or classified against the view.
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
            // The colour of the environment is from 0 to 1, and that of the primitive from 0 to
            // 255.
            bool bTop = i == 0 || i == 3;
            source.fRed = bTop ? g_fGbiEnvColorR * GLIDE_COLOR_MAX : g_fGbiPrimColorR;
            source.fGreen = bTop ? g_fGbiEnvColorG * GLIDE_COLOR_MAX : g_fGbiPrimColorG;
            source.fBlue = bTop ? g_fGbiEnvColorB * GLIDE_COLOR_MAX : g_fGbiPrimColorB;
        } else {
            source.fRed = source.fGreen = source.fBlue = GLIDE_COLOR_MAX;
        }
    }
    if (g_dwGbiGeometryMode & GBI_GEOMETRY_CULL_FRONT) {
        ClipAndSubmitTriangle(&vertices[3], &vertices[0], &vertices[1]);
        ClipAndSubmitTriangle(&vertices[0], &vertices[2], &vertices[1]);
    } else {
        ClipAndSubmitTriangle(&vertices[1], &vertices[0], &vertices[3]);
        ClipAndSubmitTriangle(&vertices[1], &vertices[2], &vertices[0]);
    }
}

// The coordinate of a position of a vertex in the clip space of g_GbiMvpMatrix.
static inline float TransformGbiVertexComponent(const Vec3f &position, int nColumn) {
    return position.y * g_GbiMvpMatrix[1][nColumn] + position.z * g_GbiMvpMatrix[2][nColumn] +
           position.x * g_GbiMvpMatrix[0][nColumn] + g_GbiMvpMatrix[3][nColumn];
}

static inline void TransformGbiVertexPosition(GbiVertexSource &source,
                                              const ConvertedModelVertex &input) {
    source.fX = TransformGbiVertexComponent(input.position, 0);
    source.fY = TransformGbiVertexComponent(input.position, 1);
    source.fZ = TransformGbiVertexComponent(input.position, 2);
    source.fW = TransformGbiVertexComponent(input.position, 3);
}

// Take the first light of the display lists into the space of the view, and its colour and that of
// the second, the ambient light, from 0 to 255.
static void UpdateGlideDirectionalLight(void) {
    if (g_bGbiLightsTransformed) {
        return;
    }
    if (g_dwGbiNumLights) {
        const Light_t &light = g_aGbiLights[0].l;
        g_fGbiLight0ColorR = static_cast<float>(light.col[0]);
        g_fGbiLight0ColorG = static_cast<float>(light.col[1]);
        g_fGbiLight0ColorB = static_cast<float>(light.col[2]);
        float (*matrix)[4] =
            g_dwGbiMatrixStackDepth ? g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1] : nullptr;
        float x = light.dir[0];
        float y = light.dir[1];
        float z = light.dir[2];
        g_GbiLight0Direction.x =
            (matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z) / GBI_LIGHT_SCALE;
        g_GbiLight0Direction.y =
            (matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z) / GBI_LIGHT_SCALE;
        g_GbiLight0Direction.z =
            (matrix[2][0] * x + matrix[2][1] * y + matrix[2][2] * z) / GBI_LIGHT_SCALE;
        Vec3fNormalize(&g_GbiLight0Direction);
        const Light_t &ambient = g_aGbiLights[1].l;
        g_fGbiAmbientColorR = static_cast<float>(ambient.col[0]);
        g_fGbiAmbientColorG = static_cast<float>(ambient.col[1]);
        g_fGbiAmbientColorB = static_cast<float>(ambient.col[2]);
    }
    g_bGbiLightsTransformed = true;
}

// Light a vertex with the first light and the ambient light, or give it the colour of the
// primitive when the display lists set no light.
static inline void LightGlideVertex(const ConvertedModelVertex &input, GbiVertexSource &source) {
    if (!g_dwGbiNumLights) {
        source.fRed = g_fGbiPrimColorR;
        source.fGreen = g_fGbiPrimColorG;
        source.fBlue = g_fGbiPrimColorB;
        return;
    }
    float dot = input.normal.x * g_GbiLight0Direction.x +
                (input.normal.y * g_GbiLight0Direction.y + input.normal.z * g_GbiLight0Direction.z);
    if (dot < 0.0f) {
        source.fRed = g_fGbiAmbientColorR;
        source.fGreen = g_fGbiAmbientColorG;
        source.fBlue = g_fGbiAmbientColorB;
        return;
    }
    auto channel = [dot](float light, float ambient) {
        float color = dot * light + ambient;
        return color > GLIDE_COLOR_MAX ? GLIDE_COLOR_MAX : color;
    };
    source.fRed = channel(g_fGbiLight0ColorR, g_fGbiAmbientColorR);
    source.fGreen = channel(g_fGbiLight0ColorG, g_fGbiAmbientColorG);
    source.fBlue = channel(g_fGbiLight0ColorB, g_fGbiAmbientColorB);
}

// Generate the texture coordinates of a vertex from its normal in the space of the view and the
// vectors of the look-at. The linear generation takes the arc sine of the t.
static inline void GenerateGlideTextureCoordinates(const ConvertedModelVertex &input,
                                                   GbiVertexSource &source,
                                                   bool bLinear) {
    const float (*matrix)[4] =
        g_dwGbiMatrixStackDepth ? g_aGbiModelViewStack[g_dwGbiMatrixStackDepth - 1] : nullptr;
    Vec3f normal;
    normal.x = matrix[0][0] * input.normal.x + matrix[1][0] * input.normal.y +
               matrix[2][0] * input.normal.z;
    normal.y = matrix[0][1] * input.normal.x + matrix[1][1] * input.normal.y +
               matrix[2][1] * input.normal.z;
    normal.z = matrix[0][2] * input.normal.x + matrix[1][2] * input.normal.y +
               matrix[2][2] * input.normal.z;
    Vec3fNormalize(&normal);
    const Light_t &lookX = g_pGbiLookAtX->l;
    const Light_t &lookY = g_pGbiLookAtY->l;
    float s = (lookX.dir[0] * normal.x + lookX.dir[1] * normal.y + lookX.dir[2] * normal.z) /
              GBI_LIGHT_SCALE;
    float t = (lookY.dir[0] * normal.x + lookY.dir[1] * normal.y + lookY.dir[2] * normal.z) /
              GBI_LIGHT_SCALE;
    float fEdgeScale = bLinear ? 0.5f : 1.0f;
    if (bLinear) {
        t = static_cast<float>(asin(t)) / 3.1415927f;
    }
    source.fS = (s * static_cast<float>(g_dwTileSizeWidth) + GBI_TEXTURE_GEN_OFFSET -
                 static_cast<float>(g_dwTileSizeUls) * fEdgeScale) /
                g_fTexCoordScaleS;
    source.fT = (t * static_cast<float>(g_dwTileSizeHeight) + GBI_TEXTURE_GEN_OFFSET -
                 static_cast<float>(g_dwTileSizeUlt) * fEdgeScale) /
                g_fTexCoordScaleT;
}

// How a vertex handler colours its vertices.
enum class GlideVertexColor { Unlit, Lit, LitEnvModulated, TexGen, TexGenLinear };

template <GlideVertexColor Color, bool UseZBuffer>
static GbiCommand *LoadGlideVertices(GbiCommand *pCommand) {
    if constexpr (Color != GlideVertexColor::Unlit) {
        UpdateGlideDirectionalLight();
    }
    int nFirst = pCommand->w0 >> 16 & 0xff;
    int nCount = pCommand->w0 >> 10 & 0x3f;
    const ConvertedModelVertex *pVertices = pCommand->vertexData.pConverted;
    for (int nVertex = 0; nVertex < nCount; nVertex++) {
        GbiVertex &vertex = g_aGbiVertexCache[nFirst + nVertex];
        const ConvertedModelVertex &input = pVertices[nVertex];
        TransformGbiVertexPosition(vertex.source, input);
        if constexpr (Color == GlideVertexColor::TexGen ||
                      Color == GlideVertexColor::TexGenLinear) {
            GenerateGlideTextureCoordinates(
                input, vertex.source, Color == GlideVertexColor::TexGenLinear);
            ComputeVertexDirectionalLight(&input, &vertex.source);
        } else {
            vertex.source.fS = input.texture.x;
            vertex.source.fT = input.texture.y;
            if constexpr (Color == GlideVertexColor::Unlit) {
                vertex.source.fRed = input.color.x;
                vertex.source.fGreen = input.color.y;
                vertex.source.fBlue = input.color.z;
            } else {
                LightGlideVertex(input, vertex.source);
            }
            if constexpr (Color == GlideVertexColor::LitEnvModulated) {
                vertex.source.fRed = g_fGbiEnvColorR * vertex.source.fRed;
                vertex.source.fGreen = g_fGbiEnvColorG * vertex.source.fGreen;
                vertex.source.fBlue = g_fGbiEnvColorB * vertex.source.fBlue;
            }
        }
        vertex.nClipFlags = ComputeVertexClipCodes(&vertex.source);
        if (vertex.nClipFlags != 0) {
            continue;
        }
        if constexpr (UseZBuffer) {
            TransformVertexToScreen(&vertex.vertex,
                                    &vertex.source,
                                    vertex.source.fRed,
                                    vertex.source.fGreen,
                                    vertex.source.fBlue);
        } else {
            TransformVertexToScreenNoZBuffer(&vertex.vertex,
                                             &vertex.source,
                                             vertex.source.fRed,
                                             vertex.source.fGreen,
                                             vertex.source.fBlue);
        }
    }
    return pCommand + 1;
}

GbiCommand *HandleGbiVertexUnlit(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::Unlit, true>(pCommand);
}

GbiCommand *HandleGbiVertexLit(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::Lit, true>(pCommand);
}

GbiCommand *HandleGbiVertexLitEnvColorModulated(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::LitEnvModulated, true>(pCommand);
}

GbiCommand *HandleGbiVertexTexGen(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::TexGen, true>(pCommand);
}

GbiCommand *HandleGbiVertexTexGenLinear(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::TexGenLinear, true>(pCommand);
}

GbiCommand *HandleGbiVertexUnlitNoZBuffer(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::Unlit, false>(pCommand);
}

GbiCommand *HandleGbiVertexLitNoZBuffer(GbiCommand *pCommand) {
    return LoadGlideVertices<GlideVertexColor::Lit, false>(pCommand);
}

// The near and far ends of the fog of each weather, in units of w.
#define FOG_RAIN_NEAR -100.0f
#define FOG_RAIN_FAR 300.0f
#define FOG_NIGHT_NEAR 60.0f
#define FOG_NIGHT_FAR 200.0f
#define FOG_CLEAR_NEAR -40.0f
#define FOG_CLEAR_FAR 220.0f

void UpdateVertexFogRange(void) {
    if (g_dwWeatherActive) {
        if (g_dwRainMode || g_dwSnowMode) {
            guFogGenerateLinear(g_aGlideFogTable, FOG_RAIN_NEAR, FOG_RAIN_FAR);
        } else if (g_dwNightMode) {
            guFogGenerateLinear(g_aGlideFogTable, FOG_NIGHT_NEAR, FOG_NIGHT_FAR);
        } else {
            guFogGenerateLinear(g_aGlideFogTable, FOG_CLEAR_NEAR, FOG_CLEAR_FAR);
        }
    }
    grFogTable(g_aGlideFogTable);
}

void InitGbiGlideRenderer(void) {
    g_pfnRenderBackendSubmitDisplayList = RenderGbiDisplayListFrame;
    g_pfnRenderBackendUpdateFog = UpdateVertexFogRange;
    g_pfnRenderBackendShutdown = NullStub;
    GbiVertexSource *pNext = nullptr;
    for (int i = GBI_CLIP_VERTEX_POOL_SIZE - 1; i >= 0; --i) {
        g_aClipVertexPool[i].pNext = pNext;
        pNext = &g_aClipVertexPool[i];
    }
    g_pVertexFreeList = pNext;
    g_dwGbiDisplayListStackDepth = 0;
    g_dwGbiMatrixStackDepth = 1;
    g_bRdpBlendStateOverride = 0;
}

// Record the time of a frame shown into the samples of the counter of frames.
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

    SetGlideTextureAlphaCombine();
    SetGlideAlphaBlend(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
    grDepthMask(FXTRUE);
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
    g_pfnPresentFrame();
    RecordFrameTime();
}

#include <PR/R4300.h>

#include "common/input.h"
#include "common/render.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/util.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "common/text.h"
#endif

#if TGR_BOSS_RALLY
#include "common/gbicommand.h"
#include "common/settings.h"
#if TGR_BOSS_RALLY
/**
 * The display list of g_aClipRatioDisplayLists a frame starts with.
 *
 * @ghidraAddress USA: 0x8028a8a4
 * @ghidraAddress EUR: 0x8028caa8
 * @ghidraAddress JPN: 0x8028c578
 * @ghidraAddress BRD3D: 0x106c65fc
 * @ghidraAddress BRGlide: 0x106ed68c
 */
static s32 g_dwClipRatioIndex;
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x106c0680 */
struct GbiCommand *gDisplayListPtr;
/** @ghidraAddress BRD3D: 0x106c0920 */
u32 g_dwAlphaDitherMode;
/** @ghidraAddress BRD3D: 0x106c0688 */
u32 g_dwColorDitherMode;
/** @ghidraAddress BRD3D: 0x106c0258 */
u32 g_dwTextureFilterMode;
#endif /* TGR_BOSS_RALLY */

#define TO_RSP_ADDRESS(p) (p)
#define FRAME_SUB(a, b) ((s32)((u32)(a) - (u32)(b)))
#else
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#define TO_RSP_ADDRESS(p) (p)
#elif defined(__GNUC__)
#define TO_RSP_ADDRESS(p) ((u32)(p) - K0BASE)
#else
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))
#endif
#define FRAME_SUB(a, b) ((a) - (b))
#endif /* TGR_BOSS_RALLY */

#define VIEWPORT_MARGIN 8
#define DISPLAY_LIST_START()                                                                       \
    (g_pDisplayListBase + g_dwFrameBufferIndex * DISPLAY_LIST_COMMANDS + DISPLAY_LIST_RESERVED)

#if TGR_BOSS_RALLY
s32 g_dwStatUnused3;
s32 g_dwStatUnused2;
s32 g_dwStatUnused1;
s32 g_dwStatVehicleTriangles;
s32 g_dwStatPolygonsDrawn;
s32 g_dwStatUnused0;
s32 g_dwStatVerticesDrawn;
s32 g_dwStatTrianglesDrawn;
#endif /* TGR_BOSS_RALLY */

void ClearRenderStats(void) {
    g_dwStatUnused1 = g_dwStatUnused2 = g_dwStatUnused3 = 0;
    g_dwStatPolygonsDrawn = g_dwStatVehicleTriangles = 0;
    g_dwStatTrianglesDrawn = g_dwStatVerticesDrawn = g_dwStatUnused0 = 0;
}

#if TGR_BOSS_RALLY
void BeginDisplayList(ViewportRect *views, s32 highRes) {
#else
void BeginDisplayList(s32 highRes) {
    ViewportRect *views = g_aViewportRects;
#endif
    if (highRes != g_dwScreenScaleShift) {
        g_dwDisplayModeChangeCountdown = 1;
        g_dwScreenScaleShift = highRes;
    }
#if TGR_BOSS_RALLY
    NullStub();
#else
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
#endif
#if TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_JP
    ResetGlyphCache();
#endif
    switch (g_dwNumViewports) {
    case 1:
#if TGR_BOSS_RALLY
        views[0].x = 0;
        views[0].y = 0;
        views[0].width = g_dwConfigFramebufferWidth;
        views[0].height = g_dwConfigFramebufferHeight;
#else
        views[0].x = VIEWPORT_MARGIN;
        views[0].y = VIEWPORT_MARGIN;
        views[0].width = g_dwScreenWidth - 2 * VIEWPORT_MARGIN;
        views[0].height = g_dwScreenHeight - 2 * VIEWPORT_MARGIN;
#endif /* TGR_BOSS_RALLY */
        break;
    case 2:
        /* The two views stop 88 pixels short of the right of the screen. */
        views[1].x = VIEWPORT_MARGIN;
        views[1].y = (g_dwScreenHeight >> 1) + 1;
        views[1].width = FRAME_SUB(g_dwScreenWidth, 96);
        views[1].height = FRAME_SUB(g_dwScreenHeight >> 1, VIEWPORT_MARGIN);
        views[0].x = VIEWPORT_MARGIN;
        views[0].y = VIEWPORT_MARGIN;
        views[0].width = FRAME_SUB(g_dwScreenWidth, 96);
        views[0].height = FRAME_SUB(g_dwScreenHeight >> 1, VIEWPORT_MARGIN);
        break;
    }
    ClearRenderStats();
    ResetFrameBlocks();
#if TGR_PLATFORM == TGR_PLATFORM_N64
    if (g_dwDisplayModeChangeCountdown != 0 && g_dwDisplayModeChangeCountdown - 1 == 0) {
        if (g_dwScreenScaleShift) {
            /* The frame buffers of the high resolution end where the first of the low starts. */
            g_apFramebuffers[0] = g_aFramebuffer0 - (g_aFramebuffer1 - g_aFramebuffer0) * 6;
            g_apFramebuffers[1] = g_aFramebuffer0 - (g_aFramebuffer1 - g_aFramebuffer0) * 2;
        } else {
            g_apFramebuffers[0] = g_aFramebuffer0;
            g_apFramebuffers[1] = g_aFramebuffer1;
        }
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
    gDisplayListPtr = DISPLAY_LIST_START();
    g_dwTextureFilterMode = g_bBilinearFiltering ? G_TF_BILERP : G_TF_POINT;
    g_dwColorDitherMode = G_CD_BAYER;
    g_dwAlphaDitherMode = G_AD_PATTERN;
    gSPSegment(gDisplayListPtr++, 0, 0);
    gDPPipeSync(gDisplayListPtr++);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      COMBINED,
                      0,
                      0,
                      0,
                      COMBINED);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTextureDetail(gDisplayListPtr++, G_TD_CLAMP);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureConvert(gDisplayListPtr++, G_TC_FILT);
    gDPSetCombineKey(gDisplayListPtr++, G_CK_NONE);
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    /* The alpha dither goes to the field of the colour dither. */
    gDPSetColorDither(gDisplayListPtr++, g_dwAlphaDitherMode);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(&g_mtxIdentity),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH | G_FOG | G_LIGHTING |
                             G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD);
    gSPSetGeometryMode(gDisplayListPtr++, G_CULL_BACK);
    if (g_bGbiClipping) {
        gSPSetGeometryMode(gDisplayListPtr++, G_CLIPPING);
    } else {
        gSPClearGeometryMode(gDisplayListPtr++, G_CLIPPING);
    }
    gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[g_dwClipRatioIndex]);
    gSPTexture(gDisplayListPtr++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
#if TGR_PLATFORM == TGR_PLATFORM_N64
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));
    gDPSetDepthImage(gDisplayListPtr++, TO_RSP_ADDRESS(g_aDepthBuffer));
    osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON);
    osViSetSpecialFeatures(OS_VI_DIVOT_ON);
    osViSetSpecialFeatures(g_dwViGammaEnabled ? OS_VI_GAMMA_ON : OS_VI_GAMMA_OFF);
#elif TGR_BOSS_RALLY
    NullStub();
    NullStub();
    NullStub();
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
}

#if TGR_BOSS_RALLY
void StartLowResFrame(ViewportRect *views) {
    BeginDisplayList(views, 0);
}
#else
void StartLowResFrame(void) {
    BeginDisplayList(0);
}
#endif

void StartHighResFrame(void) {
#if TGR_BOSS_RALLY
    BeginDisplayList(g_aViewportRects, 1);
#else
    BeginDisplayList(1);
#endif
}

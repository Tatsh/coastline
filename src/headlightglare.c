#include "fade.h"
#include "render.h"
#include "system.h"

#define GLARE_MIN_LEVEL 0.1f
#define GLARE_MAX_LEVEL 0.7f

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/combine.h"
#define GLARE_LEVEL level
#define GLARE_VIEWPORTS viewports
#define GLARE_ALPHA(value) ((s32)(s64)((long double)255.0f * (value)))
#define GLARE_ADD(left, right) ((s32)((u32)(left) + (u32)(right)))

static inline void WriteGlareCombineMode(s32 secondCycleInput) {
    PackCombineMode(gDisplayListPtr++,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_PRIMITIVE,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_PRIMITIVE,
                    0,
                    0,
                    0,
                    secondCycleInput,
                    0,
                    0,
                    0,
                    secondCycleInput);
}
#else
#define GLARE_LEVEL g_fHeadlightGlare
#define GLARE_VIEWPORTS g_aViewportRects
#define GLARE_ALPHA(value) ((s32)(255.0f * (value)))
#define GLARE_ADD(left, right) ((left) + (right))
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void RenderHeadlightGlare(const ViewportRect *viewports, f32 level) {
#else
void RenderHeadlightGlare(void) {
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!(level >= GLARE_MIN_LEVEL)) {
#else
    if (GLARE_LEVEL < GLARE_MIN_LEVEL) {
#endif
        return;
    }
    if (GLARE_LEVEL > GLARE_MAX_LEVEL) {
        GLARE_LEVEL = GLARE_MAX_LEVEL;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetRenderMode(gDisplayListPtr++,
                     IM_RD | CVG_DST_SAVE | FORCE_BL |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    WriteGlareCombineMode(1000 + G_CCMUX_PRIMITIVE);
#else
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
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE);
#endif
    gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0xFF, 0xFF, 0xFF, GLARE_ALPHA(GLARE_LEVEL));
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    gDPFillRectangle(gDisplayListPtr++,
                     GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].x,
                     GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].y,
                     GLARE_ADD(GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].x,
                               GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].width),
                     GLARE_ADD(GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].y,
                               GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].height));
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    WriteGlareCombineMode(1000 + G_CCMUX_COMBINED);
#endif
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
}

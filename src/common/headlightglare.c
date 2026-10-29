#include "common/fade.h"
#include "common/fadecommands.h"
#include "common/render.h"
#include "common/system.h"

#define GLARE_MIN_LEVEL 0.1f
#define GLARE_MAX_LEVEL 0.7f

#if TGR_PC
#define GLARE_LEVEL level
#define GLARE_VIEWPORTS viewports
#define GLARE_ALPHA(value) ((s32)(s64)((long double)255.0f * (value)))
#define GLARE_ADD(left, right) ((s32)((u32)(left) + (u32)(right)))
#else
#define GLARE_LEVEL g_fHeadlightGlare
#define GLARE_VIEWPORTS g_aViewportRects
#define GLARE_ALPHA(value) ((s32)(255.0f * (value)))
#define GLARE_ADD(left, right) ((left) + (right))
#endif /* TGR_PC */

#if TGR_PC
void RenderHeadlightGlare(const ViewportRect *viewports, f32 level) {
#else
void RenderHeadlightGlare(void) {
#endif
#if TGR_PC
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
    WriteFadeCombineMode(PRIMITIVE);
    gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0xFF, 0xFF, 0xFF, GLARE_ALPHA(GLARE_LEVEL));
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    gDPFillRectangle(gDisplayListPtr++,
                     GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].x,
                     GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].y,
                     GLARE_ADD(GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].x,
                               GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].width),
                     GLARE_ADD(GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].y,
                               GLARE_VIEWPORTS[g_dwCurrentSplitScreenIndex].height));
#if TGR_PC
    WriteFadeCombineMode(COMBINED);
#endif
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
}

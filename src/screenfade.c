#include "fade.h"
#include "fadecommands.h"
#include "screen.h"
#include "util.h"

#define BLACK_FRAMES 3

#if TGR_PC
#define FADE_EQUAL(left, right) (!((left) < (right) || (left) > (right)))
#define FADE_SHIFT(value) ((u32)(value) << ((u32)g_dwScreenScaleShift & 31))
#define FADE_DECREMENT(value) ((s32)((u32)(value) - 1))
#define FillScreenColumns(left, right)                                                             \
    gDPFillRectangle(gDisplayListPtr++,                                                            \
                     FADE_SHIFT(left),                                                             \
                     0,                                                                            \
                     FADE_SHIFT(right) - 1u,                                                       \
                     FADE_SHIFT(g_dwScreenHeight) - 1u)
#else
#define FADE_EQUAL(left, right) ((left) == (right))
#define FADE_DECREMENT(value) ((value) - 1)
/* Fill the full height of the screen with the colour set, between two columns of pixels. */
#define FillScreenColumns(left, right)                                                             \
    gDPFillRectangle(gDisplayListPtr++,                                                            \
                     (left) << g_dwScreenScaleShift,                                               \
                     0,                                                                            \
                     ((right) << g_dwScreenScaleShift) - 1,                                        \
                     (g_dwScreenHeight << g_dwScreenScaleShift) - 1)
#endif /* TGR_PC */

void ApplyScreenFade(void) {
    /* Two boot frames precede the normal three-frame clear. */
    /**
     * @ghidraAddress USA: 0x8028b788
     * @ghidraAddress EUR: 0x8028da38
     * @ghidraAddress JPN: 0x8028d468
     * @ghidraAddress BRD3D: 0x100a81bc
     * @ghidraAddress BRGlide: 0x100a7510
     */
    static s32 g_dwScreenFadeFramesRemaining = 2;
    s32 left;
    s32 right;

    if (FADE_EQUAL(g_fFadeLevel, 1.0f)) {
        return;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    WriteFadeCombineMode(PRIMITIVE);
#if TGR_PC
    /* The original shifts integer dimensions without rounding through a float. */
    gDisplayListWords(gDisplayListPtr++,
                      _SHIFTL(G_SETSCISSOR_INT_COORDS, 24, 8),
                      _SHIFTL(FADE_SHIFT(g_dwScreenWidth), 12, 12) |
                          _SHIFTL(FADE_SHIFT(g_dwScreenHeight), 0, 12));
#else
    gDPSetScissor(gDisplayListPtr++,
                  G_SC_NON_INTERLACE,
                  0,
                  0,
                  g_dwScreenWidth << g_dwScreenScaleShift,
                  g_dwScreenHeight << g_dwScreenScaleShift);
#endif /* TGR_PC */
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0, 0, 0, 0);

    if (g_dwFadeRectLeft) {
#if TGR_PC
        /* The covered edge of the other framebuffer is read and then discarded. */
        left = g_adwFadeRectLeftPerBuffer[g_dwFrameBufferIndex ^ 1];
        left = 0;
        right = g_dwFadeRectLeft;
#else
        /* Start from the covered edge of the other framebuffer to avoid clearing old black pixels.
         */
        left = g_adwFadeRectLeftPerBuffer[g_dwFrameBufferIndex ^ 1];
        if (g_dwFadeRectLeft < left) {
            left = 0;
        }
        right = g_dwFadeRectLeft;
        if (left) {
            left--;
        }
#endif /* TGR_PC */
        if (IsFadedToBlack()) {
            g_dwScreenFadeFramesRemaining = BLACK_FRAMES;
        }
#if TGR_PC
        gDPSetRenderMode(gDisplayListPtr++,
                         IM_RD | CVG_DST_SAVE | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
        WriteFadeCombineMode(PRIMITIVE);
        gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0, 0, 0, 0xFF);
#endif
        FillScreenColumns(left, right);
    }

    /* Black right of the picture, or the whole screen once faded out, for a few frames. */
    if (g_dwFadeRectRight < g_dwScreenWidth) {
        left = g_dwFadeRectRight;
        right = g_dwScreenWidth;
        if (g_dwScreenFadeFramesRemaining) {
            g_dwScreenFadeFramesRemaining = FADE_DECREMENT(g_dwScreenFadeFramesRemaining);
            FillScreenColumns(left, right);
        }
    } else if (FADE_EQUAL(g_fFadeLevel, 0.0f)) {
        right = g_dwScreenWidth;
        if (g_dwScreenFadeFramesRemaining) {
            g_dwScreenFadeFramesRemaining = FADE_DECREMENT(g_dwScreenFadeFramesRemaining);
            FillScreenColumns(0, right);
        }
    }
    gDPPipeSync(gDisplayListPtr++);
}

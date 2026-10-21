#include "fade.h"

#include "audio.h"
#include "render.h"
#include "system.h"
#include "util.h"

/* The glare shows from this level and grows no further than the next. */
#define GLARE_MIN_LEVEL 0.1f
#define GLARE_MAX_LEVEL 0.7f

/* The frames the wipe blacks the uncovered screen after the screen has faded out. */
#define BLACK_FRAMES 3

/* The volume of a fade level of 1. */
#define MAX_VOLUME 255.0

void ResetHeadlightGlare(void) {
    g_fHeadlightGlare = 0.0f;
}

void RenderHeadlightGlare(void) {
    if (g_fHeadlightGlare < GLARE_MIN_LEVEL) {
        return;
    }
    if (g_fHeadlightGlare > GLARE_MAX_LEVEL) {
        g_fHeadlightGlare = GLARE_MAX_LEVEL;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetRenderMode(gDisplayListPtr++,
                     IM_RD | CVG_DST_SAVE | FORCE_BL |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
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
    gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0xFF, 0xFF, 0xFF, (s32)(255.0f * g_fHeadlightGlare));
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    gDPFillRectangle(gDisplayListPtr++,
                     g_aViewportRects[g_dwCurrentSplitScreenIndex].x,
                     g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                     g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                         g_aViewportRects[g_dwCurrentSplitScreenIndex].width,
                     g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                         g_aViewportRects[g_dwCurrentSplitScreenIndex].height);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
}

void SetFadeTarget(float target, float seconds) {
    g_dwFadeSkipFrame = 1;
    if (target < g_fFadeLevel || target == 0.0f) {
        if (g_fFadeLevel == 1.0f || (f64)g_fFadeRate <= 0.0) {
            g_fFadeRate = -1.0f / seconds;
            g_fFadeTarget = target;
        } else {
            /* Still fading in: fade out once the picture is whole. */
            g_dwFadeAutoReverse = 1;
        }
        return;
    }
    g_fFadeTarget = target;
    g_fFadeRate = 1.0f / seconds;
}

void FadeSfxTo(float target, float seconds) {
    g_dwSfxFadeSkipFrame = 1;
    g_fSfxFadeTarget = target;
    if (g_fSfxFadeTarget < g_fSfxFadeCurrent || g_fSfxFadeTarget == 0.0f) {
        g_fSfxFadeSpeed = -1.0f / seconds;
    } else {
        g_fSfxFadeSpeed = 1.0f / seconds;
    }
}

void FadeMusicTo(float target, float seconds) {
    g_dwMusicFadeSkipFrame = 1;
    g_fMusicFadeTarget = target;
    if (g_fMusicFadeTarget < g_fMusicFadeCurrent || g_fMusicFadeTarget == 0.0f) {
        g_fMusicFadeSpeed = -1.0f / seconds;
    } else {
        g_fMusicFadeSpeed = 1.0f / seconds;
    }
}

s32 HasSfxFadeFinished(void) {
    return g_fSfxFadeTarget == g_fSfxFadeCurrent;
}

void SetSfxFadeLevelImmediate(float level) {
    g_fSfxFadeTarget = level;
    g_fSfxFadeCurrent = g_fSfxFadeTarget;
}

void SnapScreenFade(float level) {
    g_fFadeTarget = level;
    g_fFadeLevel = g_fFadeTarget;
}

s32 IsFadingIn(void) {
    return g_fFadeRate > 0.0f && !g_dwFadeAutoReverse;
}

s32 IsFadingOut(void) {
    return g_fFadeRate < 0.0f || g_dwFadeAutoReverse;
}

s32 IsFadeComplete(void) {
    return g_fFadeTarget == g_fFadeLevel && !g_dwFadeAutoReverse;
}

s32 IsFadedToBlack(void) {
    return g_fFadeRate < 0.0f && g_fFadeLevel == 0.0f && !g_dwFadeAutoReverse;
}

s32 IsFadedToWhite(void) {
    return g_fFadeRate > 0.0f && g_fFadeLevel == 1.0f && !g_dwFadeAutoReverse;
}

void UnusedScreenFadeStub(void) {
}

/* Fill the full height of the screen with the colour set, between two columns of pixels. */
#define FillScreenColumns(left, right)                                                             \
    gDPFillRectangle(gDisplayListPtr++,                                                            \
                     (left) << g_dwScreenScaleShift,                                               \
                     0,                                                                            \
                     ((right) << g_dwScreenScaleShift) - 1,                                        \
                     (g_dwScreenHeight << g_dwScreenScaleShift) - 1)

void ApplyScreenFade(void) {
    s32 left;
    s32 right;

    if (g_fFadeLevel == 1.0f) {
        return;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
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
    gDPSetScissor(gDisplayListPtr++,
                  G_SC_NON_INTERLACE,
                  0,
                  0,
                  g_dwScreenWidth << g_dwScreenScaleShift,
                  g_dwScreenHeight << g_dwScreenScaleShift);
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0, 0, 0, 0);

    /* Black from where the other framebuffer was covered to the left edge of the picture. */
    if (g_dwFadeRectLeft) {
        left = g_adwFadeRectLeftPerBuffer[g_dwFrameBufferIndex ^ 1];
        if (g_dwFadeRectLeft < left) {
            left = 0;
        }
        right = g_dwFadeRectLeft;
        if (left) {
            left--;
        }
        if (IsFadedToBlack()) {
            g_dwScreenFadeFramesRemaining = BLACK_FRAMES;
        }
        FillScreenColumns(left, right);
    }

    /* Black right of the picture, or the whole screen once faded out, for a few frames. */
    if (g_dwFadeRectRight < g_dwScreenWidth) {
        left = g_dwFadeRectRight;
        right = g_dwScreenWidth;
        if (g_dwScreenFadeFramesRemaining) {
            g_dwScreenFadeFramesRemaining--;
            FillScreenColumns(left, right);
        }
    } else if (g_fFadeLevel == 0.0f) {
        right = g_dwScreenWidth;
        if (g_dwScreenFadeFramesRemaining) {
            g_dwScreenFadeFramesRemaining--;
            FillScreenColumns(0, right);
        }
    }
    gDPPipeSync(gDisplayListPtr++);
}

void DrawScreenFadeOverlay(float level) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++,
                     IM_RD | CVG_DST_SAVE | FORCE_BL |
                         GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
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
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0, 0, 0, 0xFF - (s32)(255.0f * level));
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    gDPFillRectangle(gDisplayListPtr++,
                     0,
                     0,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     g_dwScreenHeight << g_dwScreenScaleShift);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
}

void UpdateFadeTransitions(void) {
    f32 rate;
    s32 move;

    if (!g_dwFadeSkipFrame) {
        if (g_fFadeLevel != g_fFadeTarget) {
            rate = g_fFadeRate;
            g_fFadeLevel += rate * g_fFrameDeltaTime;
            if (rate < 0.0f) {
                if (g_fFadeLevel < g_fFadeTarget) {
                    g_fFadeLevel = g_fFadeTarget;
                }
            } else if (g_fFadeTarget <= g_fFadeLevel) {
                g_fFadeLevel = g_fFadeTarget;
                if (g_dwFadeAutoReverse) {
                    g_fFadeTarget = 0.0f;
                    g_fFadeRate = -rate;
                    g_dwFadeAutoReverse = 0;
                }
            }
        }
    } else {
        g_dwFadeSkipFrame = 0;
    }

    /* Keep the edges this framebuffer is drawn with, and move them with the level. */
    rate = g_fFadeRate;
    g_adwFadeRectLeftPerBuffer[g_dwFrameBufferIndex] = g_dwFadeRectLeft;
    g_adwFadeRectRightPerBuffer[g_dwFrameBufferIndex] = g_dwFadeRectRight;
    g_dwFadeRectTop = 0;
    g_dwFadeRectBottom = g_dwScreenHeight;
    if (rate > 0.0f) {
        /* Fading in, the picture grows from the left. */
        g_dwFadeRectLeft = 0;
        g_dwFadeRectRight = ((s32)(g_dwScreenWidth * g_fFadeLevel) + 3) & ~3;
    } else if (rate < 0.0f) {
        /* Fading out, black comes in from the left and pushes the picture right. */
        move =
            (g_dwScreenWidth - (s32)(g_dwScreenWidth * g_fFadeLevel) - g_dwFadeRectLeft + 3) & ~3;
        g_dwFadeRectLeft = g_dwFadeRectLeft + move;
        g_dwFadeRectRight = g_dwFadeRectRight + move;
        if (g_dwScreenWidth < g_dwFadeRectRight) {
            g_dwFadeRectRight = g_dwScreenWidth;
        }
    } else {
        g_dwFadeRectLeft = 0;
        g_dwFadeRectRight = g_dwScreenWidth;
    }

    if (!g_dwSfxFadeSkipFrame) {
        if (g_fSfxFadeCurrent != g_fSfxFadeTarget) {
            g_fSfxFadeCurrent += g_fSfxFadeSpeed * g_fFrameDeltaTime;
            if (g_fSfxFadeSpeed < 0.0f) {
                if (g_fSfxFadeCurrent < g_fSfxFadeTarget) {
                    g_fSfxFadeCurrent = g_fSfxFadeTarget;
                }
            } else if (g_fSfxFadeTarget < g_fSfxFadeCurrent) {
                g_fSfxFadeCurrent = g_fSfxFadeTarget;
            }
        }
    } else {
        g_dwSfxFadeSkipFrame = 0;
    }
    g_bSfxVolume = (s32)((f64)g_fSfxFadeCurrent * MAX_VOLUME);

    if (!g_dwMusicFadeSkipFrame) {
        if (g_fMusicFadeCurrent != g_fMusicFadeTarget) {
            g_fMusicFadeCurrent += g_fMusicFadeSpeed * g_fFrameDeltaTime;
            if (g_fMusicFadeSpeed < 0.0f) {
                if (g_fMusicFadeCurrent < g_fMusicFadeTarget) {
                    g_fMusicFadeCurrent = g_fMusicFadeTarget;
                }
            } else if (g_fMusicFadeTarget < g_fMusicFadeCurrent) {
                g_fMusicFadeCurrent = g_fMusicFadeTarget;
            }
        }
    } else {
        g_dwMusicFadeSkipFrame = 0;
    }
    g_bMusicVolume = (s32)((f64)g_fMusicFadeCurrent * MAX_VOLUME);
}

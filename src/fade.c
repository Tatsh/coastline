#include "fade.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "audio.h"
#include "render.h"
#include "system.h"
#include "util.h"
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* The x87 queries accept unordered comparisons as less than or equal. */
#define FADE_LESS(left, right) (!((left) >= (right)))
#define FADE_EQUAL(left, right) (!((left) < (right) || (left) > (right)))
#define FADE_LESS_EQUAL(left, right) (!((left) > (right)))
#define FADE_VOLUME_LESS(left, right) FADE_LESS((long double)(left) - (right), 0.0L)
#define FADE_REAL long double

f32 g_fFadeTarget;
f32 g_fFadeLevel;
f32 g_fFadeRate;
s32 g_dwFadeAutoReverse;
s32 g_dwFadeSkipFrame;
f32 g_fMusicFadeTarget = 1.0f;
f32 g_fMusicFadeCurrent = 1.0f;
f32 g_fSfxFadeTarget = 1.0f;
f32 g_fSfxFadeCurrent = 1.0f;
f32 g_fMusicFadeSpeed;
f32 g_fSfxFadeSpeed;
s32 g_dwMusicFadeSkipFrame;
s32 g_dwSfxFadeSkipFrame;
#else
#define FADE_LESS(left, right) ((left) < (right))
#define FADE_EQUAL(left, right) ((left) == (right))
#define FADE_LESS_EQUAL(left, right) ((left) <= (right))
#define FADE_VOLUME_LESS(left, right) FADE_LESS(left, right)
#define FADE_REAL f32
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

void SetFadeTarget(float target, float seconds) {
    g_dwFadeSkipFrame = 1;
    if (FADE_LESS(target, g_fFadeLevel) || FADE_EQUAL(target, 0.0f)) {
        if (FADE_EQUAL(g_fFadeLevel, 1.0f) || FADE_LESS_EQUAL((f64)g_fFadeRate, 0.0)) {
            g_fFadeRate = (f32)((FADE_REAL)-1.0f / seconds);
            g_fFadeTarget = target;
        } else {
            /* Still fading in: fade out once the picture is whole. */
            g_dwFadeAutoReverse = 1;
        }
        return;
    }
    g_fFadeTarget = target;
    g_fFadeRate = (f32)((FADE_REAL)1.0f / seconds);
}

void FadeSfxTo(float target, float seconds) {
    g_dwSfxFadeSkipFrame = 1;
    g_fSfxFadeTarget = target;
    if (FADE_VOLUME_LESS(g_fSfxFadeTarget, g_fSfxFadeCurrent) ||
        FADE_EQUAL(g_fSfxFadeTarget, 0.0f)) {
        g_fSfxFadeSpeed = (f32)((FADE_REAL)-1.0f / seconds);
    } else {
        g_fSfxFadeSpeed = (f32)((FADE_REAL)1.0f / seconds);
    }
}

void FadeMusicTo(float target, float seconds) {
    g_dwMusicFadeSkipFrame = 1;
    g_fMusicFadeTarget = target;
    if (FADE_VOLUME_LESS(g_fMusicFadeTarget, g_fMusicFadeCurrent) ||
        FADE_EQUAL(g_fMusicFadeTarget, 0.0f)) {
        g_fMusicFadeSpeed = (f32)((FADE_REAL)-1.0f / seconds);
    } else {
        g_fMusicFadeSpeed = (f32)((FADE_REAL)1.0f / seconds);
    }
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
s32 HasSfxFadeFinished(void) {
    return g_fSfxFadeTarget == g_fSfxFadeCurrent;
}

void SetSfxFadeLevelImmediate(float level) {
    g_fSfxFadeTarget = level;
    g_fSfxFadeCurrent = g_fSfxFadeTarget;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void SnapScreenFade(float level) {
    g_fFadeTarget = level;
    g_fFadeLevel = g_fFadeTarget;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
s32 IsFadingIn(void) {
    return g_fFadeRate > 0.0f && !g_dwFadeAutoReverse;
}
#endif

s32 IsFadingOut(void) {
    return FADE_LESS(g_fFadeRate, 0.0f) || g_dwFadeAutoReverse;
}

s32 IsFadeComplete(void) {
    return FADE_EQUAL(g_fFadeTarget, g_fFadeLevel) && !g_dwFadeAutoReverse;
}

s32 IsFadedToBlack(void) {
    return FADE_LESS(g_fFadeRate, 0.0f) && FADE_EQUAL(g_fFadeLevel, 0.0f) && !g_dwFadeAutoReverse;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
s32 IsFadedToWhite(void) {
    return g_fFadeRate > 0.0f && g_fFadeLevel == 1.0f && !g_dwFadeAutoReverse;
}

void UnusedScreenFadeStub(void) {
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

#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

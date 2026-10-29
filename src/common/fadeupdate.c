#include "common/audio.h"
#include "common/fade.h"
#include "common/frametime.h"
#include "common/screen.h"
#include "common/util.h"

#if TGR_PC
/** @ghidraAddress BRD3D: 0x105754fc */
s32 g_dwFadeRectBottom;
/** @ghidraAddress BRD3D: 0x10575508 */
s32 g_dwFadeRectLeft;
/** @ghidraAddress BRD3D: 0x10575500 */
s32 g_dwFadeRectRight;
/** @ghidraAddress BRD3D: 0x1057550c */
s32 g_dwFadeRectTop;
/** @ghidraAddress BRD3D: 0x10575504 */
f32 g_fHeadlightGlare;
/** @ghidraAddress BRD3D: 0x105754f8 */
f32 g_fScreenFogPulse;
#endif /* TGR_PC */

#define MAX_VOLUME 255.0

#if TGR_PC
#define FADE_LESS(left, right) (!((left) >= (right)))
#define FADE_DIFFERENT(left, right) ((left) < (right) || (left) > (right))
#define FADE_REAL long double
#define FADE_TRUNCATE(value) ((s32)(s64)(value))
#define FADE_ADD(left, right) ((s32)((u32)(left) + (u32)(right)))
#define FADE_SUB(left, right) ((s32)((u32)(left) - (u32)(right)))
#define FADE_VOLUME(value) ((s32)(s64)((long double)(value) * MAX_VOLUME))
#define FADE_SFX_VOLUME g_bSfxFadeVolume
#else
#define FADE_LESS(left, right) ((left) < (right))
#define FADE_DIFFERENT(left, right) ((left) != (right))
#define FADE_REAL f32
#define FADE_TRUNCATE(value) ((s32)(value))
#define FADE_ADD(left, right) ((left) + (right))
#define FADE_SUB(left, right) ((left) - (right))
#define FADE_VOLUME(value) ((s32)((f64)(value) * MAX_VOLUME))
#define FADE_SFX_VOLUME g_bSfxVolume
#endif /* TGR_PC */

void UpdateFadeTransitions(void) {
    f32 rate;
    s32 move;

    if (!g_dwFadeSkipFrame) {
        if (FADE_DIFFERENT(g_fFadeLevel, g_fFadeTarget)) {
            rate = g_fFadeRate;
            g_fFadeLevel += (FADE_REAL)rate * g_fFrameDeltaTime;
            if (FADE_LESS(rate, 0.0f)) {
                if (FADE_LESS(g_fFadeLevel, g_fFadeTarget)) {
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
        g_dwFadeRectRight =
            FADE_ADD(FADE_TRUNCATE((FADE_REAL)g_dwScreenWidth * g_fFadeLevel), 3) & ~3;
    } else if (FADE_LESS(rate, 0.0f)) {
        /* Fading out, black comes in from the left and pushes the picture right. */
        move = FADE_ADD(FADE_SUB(FADE_SUB(g_dwScreenWidth,
                                          FADE_TRUNCATE((FADE_REAL)g_dwScreenWidth * g_fFadeLevel)),
                                 g_dwFadeRectLeft),
                        3) &
               ~3;
        g_dwFadeRectLeft = FADE_ADD(g_dwFadeRectLeft, move);
        g_dwFadeRectRight = FADE_ADD(g_dwFadeRectRight, move);
        if (g_dwScreenWidth < g_dwFadeRectRight) {
            g_dwFadeRectRight = g_dwScreenWidth;
        }
    } else {
        g_dwFadeRectLeft = 0;
        g_dwFadeRectRight = g_dwScreenWidth;
    }

    if (!g_dwSfxFadeSkipFrame) {
        if (FADE_DIFFERENT(g_fSfxFadeCurrent, g_fSfxFadeTarget)) {
            g_fSfxFadeCurrent += (FADE_REAL)g_fSfxFadeSpeed * g_fFrameDeltaTime;
            if (FADE_LESS(g_fSfxFadeSpeed, 0.0f)) {
                if (FADE_LESS(g_fSfxFadeCurrent, g_fSfxFadeTarget)) {
                    g_fSfxFadeCurrent = g_fSfxFadeTarget;
                }
            } else if (g_fSfxFadeTarget < g_fSfxFadeCurrent) {
                g_fSfxFadeCurrent = g_fSfxFadeTarget;
            }
        }
    } else {
        g_dwSfxFadeSkipFrame = 0;
    }
    FADE_SFX_VOLUME = FADE_VOLUME(g_fSfxFadeCurrent);

    if (!g_dwMusicFadeSkipFrame) {
        if (FADE_DIFFERENT(g_fMusicFadeCurrent, g_fMusicFadeTarget)) {
            g_fMusicFadeCurrent += (FADE_REAL)g_fMusicFadeSpeed * g_fFrameDeltaTime;
            if (FADE_LESS(g_fMusicFadeSpeed, 0.0f)) {
                if (FADE_LESS(g_fMusicFadeCurrent, g_fMusicFadeTarget)) {
                    g_fMusicFadeCurrent = g_fMusicFadeTarget;
                }
            } else if (g_fMusicFadeTarget < g_fMusicFadeCurrent) {
                g_fMusicFadeCurrent = g_fMusicFadeTarget;
            }
        }
    } else {
        g_dwMusicFadeSkipFrame = 0;
    }
    g_bMusicVolume = FADE_VOLUME(g_fMusicFadeCurrent);
}

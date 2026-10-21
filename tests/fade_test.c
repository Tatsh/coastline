/* Host check of src/fade.c. */
#include <assert.h>
#include <stdio.h>

#include "fade.c"

Gfx *gDisplayListPtr;
static Gfx gfx[64];
ViewportRect g_aViewportRects[4] = {{10, 20, 100, 50, 0}};
s32 g_dwCurrentSplitScreenIndex, g_dwScreenWidth = 320, g_dwScreenHeight = 240,
                                 g_dwScreenScaleShift;
s32 g_dwFrameBufferIndex;
u32 g_dwColorDitherMode = 0x40;
f32 g_fFrameDeltaTime;
u8 g_bSfxVolume, g_bMusicVolume;
s32 g_dwFadeRectLeft, g_dwFadeRectRight, g_dwFadeRectTop, g_dwFadeRectBottom;
f32 g_fHeadlightGlare, g_fFadeTarget, g_fFadeRate, g_fFadeLevel;
f32 g_fMusicFadeTarget, g_fMusicFadeSpeed, g_fMusicFadeCurrent;
f32 g_fSfxFadeTarget, g_fSfxFadeSpeed, g_fSfxFadeCurrent;
s32 g_dwFadeSkipFrame, g_dwMusicFadeSkipFrame, g_dwSfxFadeSkipFrame, g_dwFadeAutoReverse;
s32 g_dwScreenFadeFramesRemaining, g_adwFadeRectLeftPerBuffer[2], g_adwFadeRectRightPerBuffer[2];

static u32 Count(void) {
    return (u32)(gDisplayListPtr - gfx);
}

static void Reset(void) {
    gDisplayListPtr = gfx;
}

static void Fill(u32 i, u32 ulx, u32 uly, u32 lrx, u32 lry) {
    if (gfx[i].words.w1 != (ulx << 14 | uly << 2) ||
        gfx[i].words.w0 != (0xF6000000 | lrx << 14 | lry << 2)) {
        fprintf(stderr,
                "fill %u: %08x %08x, ul %u %u lr %u %u\n",
                i,
                gfx[i].words.w0,
                gfx[i].words.w1,
                gfx[i].words.w1 >> 14 & 0x3FF,
                gfx[i].words.w1 >> 2 & 0x3FF,
                gfx[i].words.w0 >> 14 & 0x3FF,
                gfx[i].words.w0 >> 2 & 0x3FF);
    }
    assert(gfx[i].words.w0 == (0xF6000000 | lrx << 14 | lry << 2));
    assert(gfx[i].words.w1 == (ulx << 14 | uly << 2));
}

static void Update(f32 dt) {
    g_fFrameDeltaTime = dt;
    UpdateFadeTransitions();
    g_dwFrameBufferIndex ^= 1;
}

int main(void) {
    /* The fog shows only when thick enough, and no thicker than its limit. */
    g_fHeadlightGlare = 0.05f;
    Reset();
    RenderHeadlightGlare();
    assert(Count() == 0);
    g_fHeadlightGlare = 0.9f;
    Reset();
    RenderHeadlightGlare();
    assert(g_fHeadlightGlare == 0.7f && Count() == 9);
    assert(gfx[1].words.w0 == 0xBA001402 && gfx[1].words.w1 == 0);
    assert(gfx[2].words.w0 == 0xB900031D && gfx[2].words.w1 == 0x00504340);
    assert(gfx[3].words.w0 == 0xFCFFFFFF && gfx[3].words.w1 == 0xFFFDF6FB);
    assert(gfx[4].words.w0 == 0xFA000000 &&
           gfx[4].words.w1 == (0xFFFFFF00 | (u32)(s32)(255.0f * 0.7f)));
    assert(gfx[5].words.w0 == 0xBA000602 && gfx[5].words.w1 == 0xC0);
    Fill(6, 10, 20, 110, 70);
    assert(gfx[8].words.w0 == 0xBA000602 && gfx[8].words.w1 == 0x40);
    ResetHeadlightGlare();
    assert(g_fHeadlightGlare == 0.0f);

    /* A fade in: the frame it starts holds still, then the picture grows from the left. */
    SnapScreenFade(0.0f);
    assert(g_fFadeLevel == 0.0f && g_fFadeTarget == 0.0f);
    SetFadeTarget(1.0f, 0.5f);
    assert(g_fFadeRate == 2.0f && g_fFadeTarget == 1.0f && g_dwFadeSkipFrame);
    assert(IsFadingIn() && !IsFadingOut() && !IsFadeComplete());
    Update(0.25f);
    assert(g_fFadeLevel == 0.0f && !g_dwFadeSkipFrame && g_dwFadeRectLeft == 0 &&
           g_dwFadeRectRight == 0);
    Update(0.25f);
    assert(g_fFadeLevel == 0.5f && g_dwFadeRectRight == 160 && g_dwFadeRectBottom == 240);
    assert(g_adwFadeRectRightPerBuffer[1] == 0);

    /* A fade out asked for while fading in waits until the picture is whole. */
    SetFadeTarget(0.0f, 0.5f);
    assert(g_dwFadeAutoReverse && g_fFadeRate == 2.0f && g_fFadeTarget == 1.0f);
    assert(!IsFadingIn() && IsFadingOut());
    Update(0.25f);
    Update(0.3f);
    assert(g_fFadeLevel == 1.0f && g_fFadeTarget == 0.0f && g_fFadeRate == -2.0f);
    assert(!g_dwFadeAutoReverse && IsFadingOut());
    /* The fade turned in this update, so the edges moved as fading out, by nothing. */
    assert(g_dwFadeRectLeft == 0 && g_dwFadeRectRight == 160);
    Reset();
    ApplyScreenFade();
    assert(Count() == 0);

    /* Fading out, black comes from the left and pushes the picture right. */
    Update(0.25f);
    assert(g_fFadeLevel == 0.5f && g_dwFadeRectLeft == 160 && g_dwFadeRectRight == 320);
    g_adwFadeRectLeftPerBuffer[g_dwFrameBufferIndex ^ 1] = 100;
    Reset();
    ApplyScreenFade();
    assert(Count() == 8 && gfx[7].words.w0 == 0xE7000000);
    assert(gfx[2].words.w1 == 0x0F0A4000);
    assert(gfx[4].words.w0 == (0xED000000) && gfx[4].words.w1 == ((320 * 4) << 12 | 240 * 4));
    assert(gfx[5].words.w0 == 0xFA00FFFF && gfx[5].words.w1 == 0);
    Fill(6, 99, 0, 159, 239);
    Update(0.25f);
    assert(g_fFadeLevel == 0.0f && IsFadedToBlack() && IsFadeComplete());
    assert(g_dwFadeRectLeft == 320);
    /* Faded out: the whole screen goes black for a few frames. */
    g_dwFadeRectRight = 320;
    Reset();
    ApplyScreenFade();
    assert(g_dwScreenFadeFramesRemaining == BLACK_FRAMES - 1);
    Fill(6, 159, 0, 319, 239);
    Fill(7, 0, 0, 319, 239);
    assert(gfx[8].words.w0 == 0xE7000000 && Count() == 9);
    /* While black covers the left, each frame starts the count again. */
    Reset();
    ApplyScreenFade();
    assert(g_dwScreenFadeFramesRemaining == BLACK_FRAMES - 1);
    g_dwFadeRectLeft = 0;
    Reset();
    ApplyScreenFade();
    Fill(5 + 1, 0, 0, 319, 239);
    assert(Count() == 8 && g_dwScreenFadeFramesRemaining == BLACK_FRAMES - 2);
    Reset();
    ApplyScreenFade();
    Reset();
    ApplyScreenFade();
    assert(Count() == 7 && g_dwScreenFadeFramesRemaining == 0);

    /* A fade in again leaves the right black while frames remain. */
    g_dwScreenFadeFramesRemaining = 1;
    SetFadeTarget(1.0f, 1.0f);
    Update(0.0f);
    Update(0.5f);
    assert(g_dwFadeRectLeft == 0 && g_dwFadeRectRight == 160 && IsFadingIn());
    Reset();
    ApplyScreenFade();
    Fill(6, 160, 0, 319, 239);
    assert(g_dwScreenFadeFramesRemaining == 0 && Count() == 8);
    Update(0.5f);
    assert(IsFadedToWhite() && !IsFadedToBlack());

    /* The overlay is more opaque the lower its level. */
    Reset();
    DrawScreenFadeOverlay(0.5f);
    assert(Count() == 9);
    assert(gfx[1].words.w1 == 0x00100000 && gfx[2].words.w1 == 0x0C184340);
    assert(gfx[3].words.w1 == 0xFFFDF638);
    assert(gfx[4].words.w0 == 0xFA00FFFF && gfx[4].words.w1 == (u32)(0xFF - (s32)(255.0f * 0.5f)));
    Fill(6, 0, 0, 320, 240);

    /* The volumes follow their fades, which stop at their targets. */
    SetSfxFadeLevelImmediate(1.0f);
    assert(HasSfxFadeFinished() && g_fSfxFadeCurrent == 1.0f);
    FadeSfxTo(0.0f, 1.0f);
    FadeMusicTo(1.0f, 0.5f);
    assert(g_fSfxFadeSpeed == -1.0f && g_fMusicFadeSpeed == 2.0f && !HasSfxFadeFinished());
    Update(0.25f);
    assert(g_bSfxVolume == 255 && g_bMusicVolume == 0);
    Update(0.25f);
    assert(g_bSfxVolume == 191 && g_bMusicVolume == 127);
    Update(1.0f);
    assert(g_bSfxVolume == 0 && g_bMusicVolume == 255 && HasSfxFadeFinished());
    puts("fade ok");
    return 0;
}

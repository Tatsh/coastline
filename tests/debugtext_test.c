/* Host check of DrawBitmapTextToFramebuffer() in src/n64/debugtext.c. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "debugtext.h"
#include "system.h"
#include "track.h"
#include "util.h"

s16 g_aBitmapFontWide[BITMAP_FONT_GLYPHS][BITMAP_FONT_ROWS][BITMAP_FONT_WIDE_COLUMNS];
s16 g_aBitmapFontNarrow[BITMAP_FONT_GLYPHS][BITMAP_FONT_ROWS][BITMAP_FONT_NARROW_COLUMNS];
s32 g_dwBitmapTextX, g_dwBitmapTextY, g_bBitmapTextNarrow, g_nDebugInstanceCycleStep;
u16 g_wBitmapTextColor;
s32 g_dwScreenScaleShift, g_dwScreenWidth, g_dwFrameDeltaMs, g_dwFrameBufferIndex;
u16 *g_apFramebuffers[2];
TrackHeader g_LoadedTrack;
u16 g_awVisibleInstanceList[4];
s32 g_nOpaqueVisibleInstanceCount, g_nDebugHiddenInstanceIndex;
u64 osClockRate;
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
}

#define W 64
static u16 fb[W * W];

int main(void) {
    g_dwScreenWidth = W;
    g_apFramebuffers[1] = fb;
    /* 'A' has one pixel at row 0 column 0; lower case 'a' must draw the same glyph. */
    g_aBitmapFontWide['A' - ' '][0][0] = 1;
    g_wBitmapTextColor = 0x1234;
    g_dwBitmapTextX = 2;
    g_dwBitmapTextY = 3;
    DrawBitmapTextToFramebuffer("aA");
    /* The first glyph: outline at rows 3..5, columns 2..4, centre at row 4 column 3. */
    assert(fb[3 * W + 2] == 1 && fb[3 * W + 4] == 1 && fb[5 * W + 3] == 1);
    assert(fb[4 * W + 3] == 0x1234);
    /* The second glyph starts 6 columns on. */
    assert(fb[4 * W + 9] == 0x1234 && fb[3 * W + 8] == 1);
    assert(g_dwBitmapTextX == 2 && g_dwBitmapTextY == 3);
    DrawBitmapTextToFramebuffer("\n");
    assert(g_dwBitmapTextX == 16 && g_dwBitmapTextY == 11);

    /* Stepping stops on the first index drawn this frame. */
    g_LoadedTrack.instanceCount = 10;
    g_nOpaqueVisibleInstanceCount = 2;
    g_awVisibleInstanceList[0] = 7;
    g_awVisibleInstanceList[1] = 4;
    g_nDebugHiddenInstanceIndex = 1;
    g_nDebugInstanceCycleStep = 1;
    StepDebugHiddenInstance();
    assert(g_nDebugHiddenInstanceIndex == 4 && g_nDebugInstanceCycleStep == 0);
    g_nDebugInstanceCycleStep = -1;
    g_nDebugHiddenInstanceIndex = 0;
    StepDebugHiddenInstance();
    assert(g_nDebugHiddenInstanceIndex == 7);
    puts("debugtext ok");
    return 0;
}

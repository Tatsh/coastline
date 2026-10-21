/* Host check of src/timing.c. */
#include <assert.h>
#include <stdio.h>

#include "render.h"
#include "system.h"
#include "timing.h"
#include "util.h"

TimingMarker g_aTimingMarkers[2][TIMING_BARS][TIMING_MARKERS];
s32 g_aTimingMarkerCounts[2][TIMING_BARS];
s32 g_dwTimingFrame, g_dwTimingBarScaleHold;
u32 g_dwTimingFrameStart, g_dwTimingFrameElapsed, g_dwTimingBarScale;
u32 g_dwTimingBarsRenderMode0, g_dwTimingBarsRenderMode1;
Gfx *gDisplayListPtr;
s32 g_dwScreenWidth = 320, g_dwScreenHeight = 240, g_dwScreenScaleShift, g_dwFrameBufferIndex;
u16 *g_apFramebuffers[2];
static u32 now;
u32 osGetCount(void) {
    return now;
}
void SetScissorRect(s32 x, s32 y, s32 w, s32 h) {
}

int main(void) {
    static Gfx list[256];
    u32 n;

    /* Frame 0 runs: one blue part of 1.5 frames on bar 0, then the frame ends. */
    now = 1000;
    BeginTimingFrame();
    assert(g_dwTimingFrame == 1 && g_dwTimingFrameStart == 1000);
    now = 1000 + 1171875;
    AddTimingMarker(0, 0, 0, 0xFF, 0xFF);
    assert(g_aTimingMarkers[1][0][1].color == 0x003F003F);
    assert(g_aTimingMarkers[1][0][0].time == 1171875);
    BeginTimingFrame();
    assert(g_aTimingMarkers[1][0][2].color == 0xF83EF83E);

    gDisplayListPtr = list;
    DrawTimingBars();
    n = gDisplayListPtr - list;
    /* The scale takes the longest part, and the box sits at the right of the screen. */
    assert(g_dwTimingBarScale == 1171875 && g_dwTimingBarScaleHold == 30);
    assert(list[4].words.w0 == 0xF7000000 && list[4].words.w1 == 0x00010001);
    assert(list[5].words.w0 == 0xF64B4388 && list[5].words.w1 == 0x00468038);
    /* Below the floor the bars span the default scale, so ticks fall every 69 rows. */
    assert(list[8].words.w1 == ((283u << 14) | (16u << 2)));
    assert(list[9].words.w1 == ((283u << 14) | ((16u + 69) << 2)));
    assert(list[11].words.w1 == ((283u << 14) | ((16u + 207) << 2)));
    /* Then the blue part of bar 0, from 0 to 1.5 frames of the default scale. */
    assert(list[13].words.w1 == 0x003F003F);
    assert(list[14].words.w0 == (0xF6000000u | (288u << 14) | ((16u + 103) << 2)));
    printf("timing ok, %u commands\n", n);
    return 0;
}

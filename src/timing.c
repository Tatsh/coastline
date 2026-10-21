#include "timing.h"

#include <PR/R4300.h>
#include <PR/gbi.h>
#include <PR/os.h>

#include "render.h"
#include "system.h"
#include "util.h"

/* The counts of the CPU timer in a frame at 60 Hz. */
#define FRAME_COUNTS 781250

/* The longest part must outgrow the scale by this much before the scale grows. */
#define SCALE_MARGIN 7812

/* The frames the scale holds after it changes. */
#define SCALE_HOLD 30

/* The scale shrinks only from above this, and never below the minimum. */
#define SCALE_SHRINK_FROM 0x1A7DAF
#define SCALE_FLOOR 1700000
#define SCALE_DEFAULT 2350000

/* The share of the height of the screen the bars take, as 208 of 240 rows. */
#define BAR_ROWS 208
#define SCREEN_ROWS 240

/* The colour of the box behind the bars, and of the ticks. */
#define BOX_COLOUR 0x00010001
#define TICK_COLOUR 0xFFFFFFFF

/* The colour that ends each frame on every bar. */
#define END_RED 0xFF
#define END_GREEN 0x00
#define END_BLUE 0xFF
#define END_ALPHA 0x7F

/* The RSP takes physical addresses, which are the KSEG0 addresses less its base. */
#define TO_RSP_ADDRESS(p) ((u32)(p) - K0BASE)

void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
    TimingMarker *marker =
        &g_aTimingMarkers[g_dwTimingFrame][bar][g_aTimingMarkerCounts[g_dwTimingFrame][bar]];
    u32 colour = ((red << 8) & 0xF800) | ((green << 3) & 0x07C0) | ((blue >> 2) & 0x003E) |
                 ((alpha >> 7) & 0x0001);

    g_aTimingMarkerCounts[g_dwTimingFrame][bar]++;
    marker[1].colour = colour | (colour << 16);
    marker[0].time = osGetCount() - g_dwTimingFrameStart;
}

void BeginTimingFrame(void) {
    s32 bar;

    for (bar = 0; bar < TIMING_BARS; bar++) {
        AddTimingMarker(bar, END_RED, END_GREEN, END_BLUE, END_ALPHA);
    }
    g_dwTimingFrame ^= 1;
    g_dwTimingFrameStart = osGetCount();
    g_dwTimingFrameElapsed = 0;
    for (bar = 0; bar < TIMING_BARS; bar++) {
        g_aTimingMarkerCounts[g_dwTimingFrame][bar] = 0;
        g_aTimingMarkers[g_dwTimingFrame][bar][0].time = 0;
    }
}

void DrawTimingBars(void) {
    s32 shown = g_dwTimingFrame ^ 1;
    TimingMarker *markers;
    s32 bar, i, count, length, left, top;
    u32 longest, lowered, time, y;

    gDPPipeSync(gDisplayListPtr++);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));

    /* The longest part of the frame shown, after the start of each bar. */
    longest = 0;
    for (bar = 0; bar < TIMING_BARS; bar++) {
        markers = g_aTimingMarkers[shown][bar];
        count = g_aTimingMarkerCounts[shown][bar];
        for (i = 1; i < count; i++) {
            if (longest < markers[i].time) {
                longest = markers[i].time;
            }
        }
    }

    /* The scale grows at once, and shrinks a frame at a time after holding for half a second. */
    if (longest >= g_dwTimingBarScale + SCALE_MARGIN) {
        g_dwTimingBarScale = longest;
        g_dwTimingBarScaleHold = SCALE_HOLD;
    } else if (g_dwTimingBarScaleHold != 0) {
        g_dwTimingBarScaleHold--;
        longest = g_dwTimingBarScale;
    } else if (g_dwTimingBarScale >= SCALE_SHRINK_FROM) {
        lowered = g_dwTimingBarScale - FRAME_COUNTS;
        if (longest < lowered + SCALE_MARGIN) {
            g_dwTimingBarScale = lowered;
            longest = lowered;
        } else {
            g_dwTimingBarScale = longest;
        }
        g_dwTimingBarScaleHold = SCALE_HOLD;
    }
    if (longest < SCALE_FLOOR) {
        longest = SCALE_DEFAULT;
    }

    length = (g_dwScreenHeight << g_dwScreenScaleShift) * BAR_ROWS / SCREEN_ROWS;

    gDPSetFillColor(gDisplayListPtr++, BOX_COLOUR);
    left = (g_dwScreenWidth - 16) << g_dwScreenScaleShift;
    top = 16 << g_dwScreenScaleShift;
    gDPFillRectangle(gDisplayListPtr++, left - 22, top - 2, left - 3, top + length + 2);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++, TICK_COLOUR);
    if (longest != 0) {
        for (time = 0, y = 0; time < longest; time += FRAME_COUNTS, y += length * FRAME_COUNTS) {
            gDPFillRectangle(
                gDisplayListPtr++, left - 21, top + y / longest, left - 20, top + y / longest);
        }
    }
    gDPPipeSync(gDisplayListPtr++);

    for (bar = 0; bar < TIMING_BARS; bar++) {
        markers = g_aTimingMarkers[shown][bar];
        count = g_aTimingMarkerCounts[shown][bar];
        for (i = 1; i < count; i++) {
            gDPSetFillColor(gDisplayListPtr++, markers[i].colour);
            gDPFillRectangle(gDisplayListPtr++,
                             left + bar * 4 - 18,
                             top + markers[i - 1].time * length / longest,
                             left + bar * 4 - 16,
                             top + markers[i].time * length / longest);
            gDPPipeSync(gDisplayListPtr++);
        }
    }

    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwTimingBarsRenderMode0, g_dwTimingBarsRenderMode1);
}

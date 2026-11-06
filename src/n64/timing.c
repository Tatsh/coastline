#include "common/timing.h"

#include <PR/gbi.h>
#include <PR/os.h>

#include "common/render.h"
#include "common/screendraw.h"
#include "common/system.h"
#include "common/util.h"

void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
    TimingMarker *marker;
    u32 color;

    marker = &g_aTimingMarkers[g_dwTimingFrame][bar][g_aTimingMarkerCounts[g_dwTimingFrame][bar]];
    color = ((red << 8) & 0xF800) | ((green << 3) & 0x07C0) | ((blue >> 2) & 0x003E) |
            ((alpha >> 7) & 0x0001);
    g_aTimingMarkerCounts[g_dwTimingFrame][bar]++;
    color |= color << 16;
    marker[1].color = color;
    marker[0].time = osGetCount() - g_dwTimingFrameElapsed;
}

void DrawTimingBars(void) {
#ifdef __sgi
    static s32 g_dwTimingBarScaleHold = 0;
    static u32 g_dwTimingBarScale;
#endif
    unsigned int max;
    unsigned int t;
    unsigned int y;
    Gfx *gfx;
    int h;
    int bar;
    int j;
    int n;
    TimingMarker *e;

    gDPPipeSync(gDisplayListPtr++);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     (u32)g_apFramebuffers[g_dwFrameBufferIndex] + 0x80000000);
    max = 0;
    for (bar = 0; bar < 3; bar++) {
        e = g_aTimingMarkers[g_dwTimingFrame ^ 1][bar];
        n = g_aTimingMarkerCounts[g_dwTimingFrame ^ 1][bar];
        for (j = 1; j < n; j++) {
            if (max < e[j].time) {
                max = e[j].time;
            }
        }
    }
    if (max < g_dwTimingBarScale + 7812) {
        if (g_dwTimingBarScaleHold != 0) {
            max = g_dwTimingBarScale;
            g_dwTimingBarScaleHold--;
        } else if (g_dwTimingBarScale > 1736110) {
            g_dwTimingBarScale -= 781250;
            if (max >= g_dwTimingBarScale + 7812) {
                g_dwTimingBarScale = max;
                g_dwTimingBarScaleHold = 30;
            } else {
                max = g_dwTimingBarScale;
                g_dwTimingBarScaleHold = 30;
            }
        }
    } else {
        g_dwTimingBarScale = max;
        g_dwTimingBarScaleHold = 30;
    }
    if (max < 1700000) {
        max = 2350000;
    }
    h = ((g_dwScreenHeight << g_dwScreenScaleShift) * 208) / 240;
    gDPSetFillColor(gDisplayListPtr++, 0x00010001);
    gDPFillRectangle(gDisplayListPtr++,
                     ((g_dwScreenWidth - 16) << g_dwScreenScaleShift) - 22,
                     (16 << g_dwScreenScaleShift) - 2,
                     ((g_dwScreenWidth - 16) << g_dwScreenScaleShift) - 3,
                     (16 << g_dwScreenScaleShift) + h + 2);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++, 0xFFFFFFFF);
    for (t = 0; t < max;) {
        gfx = gDisplayListPtr++;
        y = t * h / max;
        t += 781250;
        gDPFillRectangle(gfx,
                         ((g_dwScreenWidth - 16) << g_dwScreenScaleShift) - 21,
                         (16 << g_dwScreenScaleShift) + y,
                         ((g_dwScreenWidth - 16) << g_dwScreenScaleShift) - 20,
                         (16 << g_dwScreenScaleShift) + y);
    }
    gDPPipeSync(gDisplayListPtr++);
    for (bar = 0; bar < 3; bar++) {
        e = g_aTimingMarkers[g_dwTimingFrame ^ 1][bar];
        n = g_aTimingMarkerCounts[g_dwTimingFrame ^ 1][bar];
        for (j = 1; j < n; j++) {
            gDPSetFillColor(gDisplayListPtr++, e[j].color);
            gDPFillRectangle(gDisplayListPtr++,
                             ((g_dwScreenWidth - 16) << g_dwScreenScaleShift) + (bar << 2) - 18,
                             e[j - 1].time * h / max + (16 << g_dwScreenScaleShift),
                             ((g_dwScreenWidth - 16) << g_dwScreenScaleShift) + (bar << 2) - 16,
                             e[j].time * h / max + (16 << g_dwScreenScaleShift));
            gDPPipeSync(gDisplayListPtr++);
        }
    }
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwTimingBarsRenderMode0, g_dwTimingBarsRenderMode1);
}

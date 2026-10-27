#include <PR/os.h>

#include "screen.h"
#include "timing.h"

void BeginTimingFrame(void) {
    s32 bar;

    for (bar = 0; bar < TIMING_BARS; bar++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        NullStub();
#else
        AddTimingMarker(bar, 0xFF, 0, 0xFF, 0x7F);
#endif
    }
    g_dwTimingFrame ^= 1;
    g_dwTimingFrameStart = osGetCount();
    g_dwTimingFrameElapsed = 0;
    for (bar = 0; bar < TIMING_BARS; bar++) {
        g_aTimingMarkerCounts[g_dwTimingFrame][bar] = 0;
        g_aTimingMarkers[g_dwTimingFrame][bar][0].time = 0;
    }
}

#include <PR/os.h>

#include "common/screen.h"
#include "common/timing.h"

#if TGR_PC
/**
 * Reset at the start of the frame.
 *
 * @ghidraAddress USA: 0x8028bda8
 * @ghidraAddress EUR: 0x8028e508
 * @ghidraAddress JPN: 0x8028df38
 * @ghidraAddress BRD3D: 0x1067d57c
 * @ghidraAddress BRGlide: 0x105b9724
 */
[[maybe_unused, gnu::used]] static u32 g_dwTimingFrameElapsed;

/* The libraries share the empty code of this routine with NullStub(), and still pass it the five
 * arguments. */
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
    (void)bar;
    (void)red;
    (void)green;
    (void)blue;
    (void)alpha;
}
#endif /* TGR_PC */

void BeginTimingFrame(void) {
    s32 bar;

    for (bar = 0; bar < TIMING_BARS; bar++) {
        AddTimingMarker(bar, 0xFF, 0, 0xFF, 0x7F);
    }
    g_dwTimingFrame ^= 1;
    g_dwTimingFrameStart = osGetCount();
    g_dwTimingFrameElapsed = 0;
    for (bar = 0; bar < TIMING_BARS; bar++) {
        g_aTimingMarkerCounts[g_dwTimingFrame][bar] = 0;
        g_aTimingMarkers[g_dwTimingFrame][bar][0].time = 0;
    }
}

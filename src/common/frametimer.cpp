#include "common/frameclock.h"
#include "common/racepacing.h"
#include "common/snapshot.h"

namespace {

/**
 * The time of the current tick, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x118ab118
 * @ghidraAddress BRGlide: 0x118ee230
 */
unsigned g_dwFrameTickTime;

/**
 * The entry of g_adwFrameTickIncrements of the current tick.
 *
 * @ghidraAddress BRD3D: 0x118ab12c
 * @ghidraAddress BRGlide: 0x118ee244
 */
unsigned g_dwFrameTickPhase;

/**
 * The milliseconds of each tick of a period, 33, 33, and 34 to make a tenth of a second.
 *
 * @ghidraAddress BRD3D: 0x100bbac8
 * @ghidraAddress BRGlide: 0x100bb2d0
 */
const unsigned g_adwFrameTickIncrements[TICKS_PER_PERIOD] = {33, 33, 34};

/**
 * The ticks of the frames, three for each tenth of a second.
 *
 * @ghidraAddress BRD3D: 0x118ab134
 * @ghidraAddress BRGlide: 0x118ee24c
 */
[[maybe_unused]] unsigned g_dwFrameTickCounter;

} // namespace

int g_dwRaceFrameCounter;

u32 GetFrameTickCounter(void) {
    return g_dwFrameTickTime;
}

void ResetFrameTimer(void) {
    unsigned dwNow = GetHighResolutionTime();

    g_dwFrameTickPhase = 0;
    g_dwFrameTickTime = dwNow;
    g_dwFrameTickCounter =
        dwNow / TICK_PERIOD_MS * TICKS_PER_PERIOD + dwNow % TICK_PERIOD_MS / TICK_INTERVAL_MS;
}

void AdvanceFrameTickCounter(void) {
    if (++g_dwFrameTickPhase > TICKS_PER_PERIOD - 1) {
        g_dwFrameTickPhase = 0;
    }
    g_dwFrameTickTime += g_adwFrameTickIncrements[g_dwFrameTickPhase];
    g_dwFrameTickCounter++;
}

void ResetFrameTimerState(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    SnapshotPlatformFrameTimerState();
#endif
    g_dwRaceFrameCounter = -1;
    g_bFrameTimerReset = true;
    ResetFrameTimer();
}

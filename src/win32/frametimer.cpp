#include "win32/frametimer.h"

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/win32api.h"
#else
#include <mmsystem.h>
#include <windows.h>
#endif

#include "common/racepacing.h"
#include "common/snapshot.h"
#include "win32/timer.h"

namespace {

/**
 * The clock of the frames of a race.
 *
 * @ghidraAddress BRD3D: 0x106806b0
 * @ghidraAddress BRGlide: 0x105bc858
 */
FrameTimer g_FrameTimer;

/**
 * Whether the frequency of the performance counter was read for the frames.
 *
 * @ghidraAddress BRD3D: 0x118ab140
 * @ghidraAddress BRGlide: 0x118ee258
 */
int g_bFrameTimerInitialized;

/**
 * Whether the system has a performance counter for the frames.
 *
 * @ghidraAddress BRD3D: 0x118ab144
 * @ghidraAddress BRGlide: 0x118ee25c
 */
int g_bFrameTimerHasPerfCounter;

/**
 * The frequency of the performance counter for the frames.
 *
 * @ghidraAddress BRD3D: 0x118ab138
 * @ghidraAddress BRGlide: 0x118ee250
 */
LARGE_INTEGER g_liFrameTimerFrequency;

/**
 * The time of the current tick, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x118ab118
 * @ghidraAddress BRGlide: 0x118ee230
 */
DWORD g_dwFrameTickTime;

/**
 * The entry of g_adwFrameTickIncrements of the current tick.
 *
 * @ghidraAddress BRD3D: 0x118ab12c
 * @ghidraAddress BRGlide: 0x118ee244
 */
DWORD g_dwFrameTickPhase;

/**
 * The milliseconds of each tick of a period, 33, 33, and 34 to make a tenth of a second.
 *
 * @ghidraAddress BRD3D: 0x100bbac8
 * @ghidraAddress BRGlide: 0x100bb2d0
 */
const DWORD g_adwFrameTickIncrements[TICKS_PER_PERIOD] = {33, 33, 34};

/**
 * The ticks of the frames, three for each tenth of a second.
 *
 * @ghidraAddress BRD3D: 0x118ab134
 * @ghidraAddress BRGlide: 0x118ee24c
 */
[[maybe_unused]] DWORD g_dwFrameTickCounter;

} // namespace

int g_dwRaceFrameCounter;

// The frames of a second.
#define FRAMES_PER_SECOND 30
// The milliseconds of a frame without the performance counter.
#define FRAME_INTERVAL_MS 33

FrameTimer *FrameTimer::InitializeFrameTimer() {
    if (!g_bFrameTimerInitialized) {
        g_bFrameTimerHasPerfCounter = QueryPerformanceFrequency(&g_liFrameTimerFrequency);
        g_bFrameTimerInitialized = true;
    }
    if (g_bFrameTimerHasPerfCounter) {
        liFrameInterval.QuadPart = g_liFrameTimerFrequency.QuadPart / FRAMES_PER_SECOND;
    } else {
        timeBeginPeriod(1);
        dwFrameIntervalMs = FRAME_INTERVAL_MS;
    }
    SnapshotTimerState();
    return this;
}

void FrameTimer::SnapshotTimerState() {
    if (g_bFrameTimerHasPerfCounter) {
        QueryPerformanceCounter(&liCurrentCount);
        liNextInterval = liFrameInterval;
    } else {
        dwCurrentTimeMs = GetHighResolutionTime();
        dwNextIntervalMs = dwFrameIntervalMs;
    }
}

DWORD GetFrameTickCounter(void) {
    return g_dwFrameTickTime;
}

void ResetFrameTimer(void) {
    DWORD dwNow = GetHighResolutionTime();

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
    g_FrameTimer.SnapshotTimerState();
    g_dwRaceFrameCounter = -1;
    g_bFrameTimerReset = true;
    ResetFrameTimer();
}

void EndHighResolutionTimerPeriod(void) {
    if (!g_bFrameTimerHasPerfCounter) {
        timeEndPeriod(1);
    }
}

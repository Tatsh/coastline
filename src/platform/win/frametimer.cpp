#include "platform/win/frametimer.h"

#include <windows.h>
// The headers below need the types of windows.h.
#include <mmsystem.h>

#include "platform/win/timer.h"

FrameTimer g_FrameTimer;
int g_bFrameTimerInitialized;
int g_bFrameTimerHasPerfCounter;
LARGE_INTEGER g_liFrameTimerFrequency;
DWORD g_dwFrameTickTime;
DWORD g_dwFrameTickPhase;
DWORD g_dwFrameTickCounter;
int g_dwRaceFrameCounter;
int g_bFrameTimerReset = true;

// The frames of a second.
#define FRAMES_PER_SECOND 30
// The milliseconds of a frame without the performance counter.
#define FRAME_INTERVAL_MS 33
// The ticks of the frames come three to each tenth of a second.
#define TICK_PERIOD_MS 100
#define TICKS_PER_PERIOD 3
#define TICK_INTERVAL_MS 33

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

void ResetFrameTimer(void) {
    DWORD dwNow = GetHighResolutionTime();

    g_dwFrameTickPhase = 0;
    g_dwFrameTickTime = dwNow;
    g_dwFrameTickCounter =
        dwNow / TICK_PERIOD_MS * TICKS_PER_PERIOD + dwNow % TICK_PERIOD_MS / TICK_INTERVAL_MS;
}

void ResetFrameTimerState(void) {
    g_FrameTimer.SnapshotTimerState();
    g_dwRaceFrameCounter = -1;
    g_bFrameTimerReset = true;
    ResetFrameTimer();
}

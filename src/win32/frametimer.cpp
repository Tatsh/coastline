#include "win32/frametimer.h"

#include <mmsystem.h>

namespace {
/**
 * The clock of the frames of a race.
 * @ghidraAddress BRD3D: 0x106806b0
 * @ghidraAddress BRGlide: 0x105bc858
 */
FrameTimer g_FrameTimer;

/**
 * Whether the frequency of the performance counter was read for the frames.
 * @ghidraAddress BRD3D: 0x118ab140
 * @ghidraAddress BRGlide: 0x118ee258
 */
int g_bFrameTimerInitialized;

/**
 * Whether the system has a performance counter for the frames.
 * @ghidraAddress BRD3D: 0x118ab144
 * @ghidraAddress BRGlide: 0x118ee25c
 */
int g_bFrameTimerHasPerfCounter;

/**
 * The frequency of the performance counter for the frames.
 * @ghidraAddress BRD3D: 0x118ab138
 * @ghidraAddress BRGlide: 0x118ee250
 */
LARGE_INTEGER g_liFrameTimerFrequency;
} // namespace

#define FRAMES_PER_SECOND 30
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

void SnapshotPlatformFrameTimerState(void) {
    g_FrameTimer.SnapshotTimerState();
}

void EndHighResolutionTimerPeriod(void) {
    if (!g_bFrameTimerHasPerfCounter) {
        timeEndPeriod(1);
    }
}

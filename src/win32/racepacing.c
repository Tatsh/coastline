#include "win32/racepacing.h"

#include "win32/framestatistics.h"
#include "win32/snapshotops.h"
#include "win32/timer.h"

/**
 * @ghidraAddress BRD3D: 0x106909e8
 * @ghidraAddress BRGlide: 0x105ccb90
 */
static DWORD g_dwLastFrameTimestamp;
/**
 * @ghidraAddress BRD3D: 0x106909d4
 * @ghidraAddress BRGlide: 0x105ccb7c
 */
static volatile DWORD g_dwRaceAccumulatedTime;
/**
 * @ghidraAddress BRD3D: 0x106805c4
 * @ghidraAddress BRGlide: 0x105bc76c
 */
static int g_nRacePacingInterval;
/**
 * @ghidraAddress BRD3D: 0x10680948
 * @ghidraAddress BRGlide: 0x105bcaf0
 */
static DWORD g_dwNextRaceFrameTime;
/**
 * @ghidraAddress BRD3D: 0x106909f8
 * @ghidraAddress BRGlide: 0x105ccba0
 */
static int g_nRaceFramesWithoutRender;
/**
 * @ghidraAddress BRD3D: 0x106909fc
 * @ghidraAddress BRGlide: 0x105ccba4
 */
static DWORD g_dwLastRaceRenderTime;
/**
 * @ghidraAddress BRD3D: 0x100aa268
 * @ghidraAddress BRGlide: 0x100a95b8
 */
static const DWORD g_adwRaceFrameIntervals[3] = {33, 33, 34};

void UpdateRaceFrameStatistics(void) {
    DWORD now = GetHighResolutionTime();
    DWORD elapsed = now - g_dwLastFrameTimestamp;
    int sampleFrames = g_dwGameFpsSampleFrames;
    int index = g_dwGameFpsFrameCountdown;
    int i;

    g_dwLastFrameTimestamp = now;
    if (index < 0) {
        for (i = 0; i < sampleFrames; i++) {
            g_adwGameFpsFrameTimeSamples[i] = elapsed;
        }
        index = sampleFrames > 0 ? sampleFrames : 0;
    }
    g_dwRaceFrameCounter = (int)((unsigned)g_dwRaceFrameCounter + 1u);
    if (g_dwRaceFrameCounter != 0) {
        g_dwRaceAccumulatedTime += elapsed;
    } else {
        g_dwRaceAccumulatedTime = 0;
    }
    index = (int)((unsigned)index + 1u);
    if (index >= sampleFrames) {
        index = 0;
    }
    g_dwGameFpsFrameCountdown = index;
    g_adwGameFpsFrameTimeSamples[index] = elapsed;
}

void PaceRaceFrames(void) {
    int rendered;
    DWORD now;

    if (g_bFrameTimerReset) {
        g_dwNextRaceFrameTime = GetHighResolutionTime();
        g_nRacePacingInterval = 0;
        g_bFrameTimerReset = 0;
        g_nRaceFramesWithoutRender = 0;
        return;
    }
    g_dwNextRaceFrameTime += g_adwRaceFrameIntervals[g_nRacePacingInterval];
    g_nRacePacingInterval = (int)((unsigned)g_nRacePacingInterval + 1u);
    if (g_nRacePacingInterval > 2) {
        g_nRacePacingInterval = 0;
    }
    g_nRaceFramesWithoutRender = (int)((unsigned)g_nRaceFramesWithoutRender + 1u);
    if (g_nRaceFramesWithoutRender > 3) {
        if (InterpolateFrameSnapshotsAndRender(1)) {
            g_nRaceFramesWithoutRender = 0;
            g_dwLastRaceRenderTime = GetHighResolutionTime();
        }
        return;
    }
    rendered = 1;
    now = GetHighResolutionTime();
    for (;;) {
        DWORD lastRender = g_dwLastRaceRenderTime;
        if ((now >= g_dwNextRaceFrameTime && now <= lastRender + 333u) || !rendered) {
            break;
        }
        rendered = InterpolateFrameSnapshotsAndRender(lastRender + 333u < now);
        now = GetHighResolutionTime();
        if (rendered) {
            g_nRaceFramesWithoutRender = 0;
            g_dwLastRaceRenderTime = now;
        }
    }
}

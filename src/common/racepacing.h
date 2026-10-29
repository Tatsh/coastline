#ifndef COASTLINE_PLATFORM_WIN_RACE_PACING_H
#define COASTLINE_PLATFORM_WIN_RACE_PACING_H

#include "common/frameclock.h"
#include "common/snapshotops.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/framestatistics.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The frame of the race, -1 before the first.
 * @ghidraAddress BRD3D: 0x1068093c
 * @ghidraAddress BRGlide: 0x105bcae4
 */
extern int g_dwRaceFrameCounter;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The time of the preceding game loop, in milliseconds.
 * @ghidraAddress BRD3D: 0x106909e8
 * @ghidraAddress BRGlide: 0x105ccb90
 */
extern DWORD g_dwLastFrameTimestamp;
/**
 * The milliseconds the race has run since its first frame.
 * @ghidraAddress BRD3D: 0x106909d4
 * @ghidraAddress BRGlide: 0x105ccb7c
 */
extern volatile DWORD g_dwRaceAccumulatedTime;
/**
 * The entry of g_adwRaceFrameIntervals that paces the next frame.
 * @ghidraAddress BRD3D: 0x106805c4
 * @ghidraAddress BRGlide: 0x105bc76c
 */
extern int g_nRacePacingInterval;
/**
 * The time the next game frame is due, in milliseconds.
 * @ghidraAddress BRD3D: 0x10680948
 * @ghidraAddress BRGlide: 0x105bcaf0
 */
extern DWORD g_dwNextRaceFrameTime;
/**
 * The game frames run since a snapshot was last drawn.
 * @ghidraAddress BRD3D: 0x106909f8
 * @ghidraAddress BRGlide: 0x105ccba0
 */
extern int g_nRaceFramesWithoutRender;
/**
 * The time a snapshot was last drawn, in milliseconds.
 * @ghidraAddress BRD3D: 0x106909fc
 * @ghidraAddress BRGlide: 0x105ccba4
 */
extern DWORD g_dwLastRaceRenderTime;
/**
 * The milliseconds of three game frames in turn, a tenth of a second together.
 * @ghidraAddress BRD3D: 0x100aa268
 * @ghidraAddress BRGlide: 0x100a95b8
 */
extern const DWORD g_adwRaceFrameIntervals[3];

/** Record the duration of the preceding game loop. */
static inline void UpdateRaceFrameStatistics(void) {
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

/** Render interpolated snapshots until the next game frame is due. */
static inline void PaceRaceFrames(void) {
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#ifdef __cplusplus
}
#endif

#endif

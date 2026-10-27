#ifndef COASTLINE_PLATFORM_WIN_RACE_PACING_H
#define COASTLINE_PLATFORM_WIN_RACE_PACING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The frame of the race, -1 before the first.
 * @ghidraAddress BRD3D: 0x1068093c
 * @ghidraAddress BRGlide: 0x105bcae4
 */
extern int g_dwRaceFrameCounter;

/** Record the duration of the preceding game loop. */
void UpdateRaceFrameStatistics(void);
/** Render interpolated snapshots until the next game frame is due. */
void PaceRaceFrames(void);

#ifdef __cplusplus
}
#endif

#endif

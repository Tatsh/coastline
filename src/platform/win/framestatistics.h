#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The number of game-loop durations averaged for the game frame rate.
 * @ghidraAddress BRD3D: 0x100aa008
 */
extern int g_dwGameFpsSampleFrames;

/**
 * The game-loop sample written last, negative before the first loop.
 * @ghidraAddress BRD3D: 0x100aa00c
 */
extern int g_dwGameFpsFrameCountdown;

/** The number of game-loop durations in the native averaging window. */
#define GAME_FPS_FRAME_TIME_SAMPLE_COUNT 5

/**
 * Recent game-loop durations in milliseconds.
 * @ghidraAddress BRD3D: 0x10680758
 */
extern DWORD g_adwGameFpsFrameTimeSamples[GAME_FPS_FRAME_TIME_SAMPLE_COUNT];

/** The number of entries of g_adwFpsFrameTimeSamples. */
#define FPS_FRAME_TIME_SAMPLE_COUNT 120

/**
 * The number of frames the frame rate is averaged over.
 * @ghidraAddress BRD3D: 0x100b5420
 * @ghidraAddress BRGlide: 0x100b4c28
 */
extern int g_dwFpsSampleFrames;

/**
 * The entry of g_adwFpsFrameTimeSamples written last, negative before the first frame.
 * @ghidraAddress BRD3D: 0x100b5424
 * @ghidraAddress BRGlide: 0x100b4c2c
 */
extern int g_dwFpsFrameCountdown;

/**
 * The times of the last frames, in milliseconds.
 * @ghidraAddress BRD3D: 0x10b4ffe8
 * @ghidraAddress BRGlide: 0x10b73348
 */
extern DWORD g_adwFpsFrameTimeSamples[FPS_FRAME_TIME_SAMPLE_COUNT];

#ifdef __cplusplus
}
#endif

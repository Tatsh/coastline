#pragma once

/**
 * @file
 * The ticks of the frames of a race of the Boss Rally release, at 30 frames a second.
 */

#include "common/types.h"

/** The milliseconds of a period of ticks. */
#define TICK_PERIOD_MS 100
/** The ticks of a period. */
#define TICKS_PER_PERIOD 3
/** The milliseconds of a tick, rounded down. */
#define TICK_INTERVAL_MS 33

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The time since the first call, from the performance counter in milliseconds rounded to the
 * nearest, or the time of the system when there is no counter. The first call divides by the
 * frequency of the counter without checking it, as the original does.
 *
 * @return The time in milliseconds.
 * @ghidraAddress BRD3D: 0x10075020
 * @ghidraAddress BRGlide: 0x1006e280
 */
u32 GetHighResolutionTime(void);

/**
 * The number of game-loop durations averaged for the game frame rate.
 * @ghidraAddress BRD3D: 0x100aa008
 * @ghidraAddress BRGlide: 0x100a9358
 */
extern int g_dwGameFpsSampleFrames;

/**
 * The game-loop sample written last, negative before the first loop.
 * @ghidraAddress BRD3D: 0x100aa00c
 * @ghidraAddress BRGlide: 0x100a935c
 */
extern int g_dwGameFpsFrameCountdown;

/**
 * Advance the ticks by one, the time of the current tick by the milliseconds of its phase.
 *
 * @ghidraAddress BRD3D: 0x10075150
 * @ghidraAddress BRGlide: 0x1006e3b0
 */
void AdvanceFrameTickCounter(void);

/**
 * Start the ticks of the frames from the current time, at three for each tenth of a second.
 *
 * @ghidraAddress BRD3D: 0x10075100
 * @ghidraAddress BRGlide: 0x1006e360
 */
void ResetFrameTimer(void);

/**
 * Start the timing of the frames of a race again.
 *
 * @ghidraAddress BRD3D: 0x1002c4d0
 * @ghidraAddress BRGlide: 0x10019a40
 */
void ResetFrameTimerState(void);

#ifdef __cplusplus
}
#endif

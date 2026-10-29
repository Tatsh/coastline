#pragma once

/**
 * @file
 * The ticks of the frames of a race of the Boss Rally release, at 30 frames a second.
 */

#ifdef __cplusplus
extern "C" {
#endif

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

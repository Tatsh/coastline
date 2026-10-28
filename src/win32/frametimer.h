#pragma once

/**
 * @file
 * The timing of the frames of the PC port, at 30 frames a second.
 */

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/win32types.h"
#else
#include <windows.h>
#endif

/** The milliseconds of a period of ticks. */
#define TICK_PERIOD_MS 100
/** The ticks of a period. */
#define TICKS_PER_PERIOD 3
/** The milliseconds of a tick, rounded down. */
#define TICK_INTERVAL_MS 33

/** The clock of the frames, from the performance counter or in milliseconds without one. */
class FrameTimer {
private:
    LARGE_INTEGER liFrameInterval; /*!< The counts of the performance counter in a frame. */
    LARGE_INTEGER liCurrentCount;  /*!< The count read last. */
    LARGE_INTEGER liNextInterval;  /*!< Set to liFrameInterval when the clock is read. */
    DWORD dwFrameIntervalMs;       /*!< The milliseconds of a frame without the counter, 33. */
    DWORD dwCurrentTimeMs;         /*!< The time read last without the counter. */
    DWORD dwNextIntervalMs;        /*!< Set to dwFrameIntervalMs when the clock is read. */

    /**
     * Read the frequency of the performance counter once, set the interval of a frame to a
     * thirtieth of a second, or 33 milliseconds with a finer resolution of the system time
     * without the counter, and read the clock.
     *
     * @return The timer.
     * @ghidraAddress BRD3D: 0x100751d0
     * @ghidraAddress BRGlide: 0x1006e430
     */
    FrameTimer *InitializeFrameTimer();

public:
    /**
     * Read the clock and set the next interval to the interval of a frame.
     *
     * @ghidraAddress BRD3D: 0x10075190
     * @ghidraAddress BRGlide: 0x1006e3f0
     */
    void SnapshotTimerState();
};

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
 * Get the time of the current tick.
 *
 * @return The time in milliseconds.
 * @ghidraAddress BRD3D: 0x100750f0
 * @ghidraAddress BRGlide: 0x1006e350
 */
DWORD GetFrameTickCounter(void);

/**
 * End the period of the timer of one millisecond the frame timer began without the performance
 * counter.
 *
 * @ghidraAddress BRD3D: 0x10075240
 * @ghidraAddress BRGlide: 0x1006e4a0
 */
void EndHighResolutionTimerPeriod(void);

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

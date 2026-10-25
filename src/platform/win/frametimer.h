#pragma once

/**
 * @file
 * The timing of the frames of the PC port, at 30 frames a second.
 */

#include <windows.h>

/** The milliseconds of a period of ticks. */
#define TICK_PERIOD_MS 100
/** The ticks of a period. */
#define TICKS_PER_PERIOD 3
/** The milliseconds of a tick, rounded down. */
#define TICK_INTERVAL_MS 33

/** The clock of the frames, from the performance counter or in milliseconds without one. */
class FrameTimer {
public:
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

    /**
     * Read the clock and set the next interval to the interval of a frame.
     *
     * @ghidraAddress BRD3D: 0x10075190
     * @ghidraAddress BRGlide: 0x1006e3f0
     */
    void SnapshotTimerState();
};

/**
 * The clock of the frames of a race.
 *
 * @ghidraAddress BRD3D: 0x106806b0
 * @ghidraAddress BRGlide: 0x105bc858
 */
extern FrameTimer g_FrameTimer;

/**
 * Whether the frequency of the performance counter was read for the frames.
 *
 * @ghidraAddress BRD3D: 0x118ab140
 * @ghidraAddress BRGlide: 0x118ee258
 */
extern int g_bFrameTimerInitialized;

/**
 * Whether the system has a performance counter for the frames.
 *
 * @ghidraAddress BRD3D: 0x118ab144
 * @ghidraAddress BRGlide: 0x118ee25c
 */
extern int g_bFrameTimerHasPerfCounter;

/**
 * The frequency of the performance counter for the frames.
 *
 * @ghidraAddress BRD3D: 0x118ab138
 * @ghidraAddress BRGlide: 0x118ee250
 */
extern LARGE_INTEGER g_liFrameTimerFrequency;

/**
 * The time of the current tick, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x118ab118
 * @ghidraAddress BRGlide: 0x118ee230
 */
extern DWORD g_dwFrameTickTime;

/**
 * The phase of the ticks. Its meaning is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x118ab12c
 * @ghidraAddress BRGlide: 0x118ee244
 */
extern DWORD g_dwFrameTickPhase;

/**
 * The ticks of the frames, three for each tenth of a second.
 *
 * @ghidraAddress BRD3D: 0x118ab134
 * @ghidraAddress BRGlide: 0x118ee24c
 */
extern DWORD g_dwFrameTickCounter;

/**
 * The frame of the race, -1 before the first.
 *
 * @ghidraAddress BRD3D: 0x1068093c
 * @ghidraAddress BRGlide: 0x105bcae4
 */
extern int g_dwRaceFrameCounter;

/**
 * Whether the timing of the frames was started again.
 *
 * @ghidraAddress BRD3D: 0x100a66e8
 * @ghidraAddress BRGlide: 0x100a5ea8
 */
extern int g_bFrameTimerReset;

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
 * Start the timing of the frames of a race again.
 *
 * @ghidraAddress BRD3D: 0x1002c4d0
 * @ghidraAddress BRGlide: 0x10019a40
 */
void ResetFrameTimerState(void);

#ifndef COASTLINE_TIMING_H
#define COASTLINE_TIMING_H

/**
 * @file
 * The meter that times the parts of each frame on three bars at the right of the screen.
 */

#include <PR/ultratypes.h>

/** The bars of the meter. */
#define TIMING_BARS 3

/** The markers a bar holds in a frame. */
#define TIMING_MARKERS 256

/** The end of a part of a frame, and the colour of the part after it. */
typedef struct {
    u32 color; /*!< The colour of the part that ends here, as two RGBA5551 pixels for a fill. */
    u32 time;  /*!< The count of the CPU timer since the start of the frame. */
} TimingMarker;

/**
 * The markers of the two frames.
 *
 * @ghidraAddress USA: 0x8034e9b0
 * @ghidraAddress JPN: 0x8034ef30
 * @ghidraAddress EUR: 0x80350eb0
 */
extern TimingMarker g_aTimingMarkers[2][TIMING_BARS][TIMING_MARKERS];

/**
 * @ghidraAddress USA: 0x803519b0
 * @ghidraAddress JPN: 0x80351f30
 * @ghidraAddress EUR: 0x80353eb0
 */
extern s32 g_aTimingMarkerCounts[2][TIMING_BARS];

/**
 * The frame the markers go to. The meter shows the other one.
 *
 * @ghidraAddress USA: 0x8028bda0
 * @ghidraAddress JPN: 0x8028df30
 * @ghidraAddress EUR: 0x8028e500
 */
extern s32 g_dwTimingFrame;

/**
 * The timer count at the start of the frame.
 *
 * @ghidraAddress USA: 0x8028bdac
 * @ghidraAddress JPN: 0x8028df3c
 * @ghidraAddress EUR: 0x8028e50c
 */
extern u32 g_dwTimingFrameStart;

/**
 * Reset at the start of the frame.
 *
 * @ghidraAddress USA: 0x8028bda8
 * @ghidraAddress JPN: 0x8028df38
 * @ghidraAddress EUR: 0x8028e508
 */
extern u32 g_dwTimingFrameElapsed;

/**
 * The timer count the full length of the bars stands for.
 *
 * @ghidraAddress USA: 0x803519c8
 * @ghidraAddress JPN: 0x80351f48
 * @ghidraAddress EUR: 0x80353ec8
 */
extern u32 g_dwTimingBarScale;

/**
 * The frames the scale keeps before it may shrink.
 *
 * @ghidraAddress USA: 0x8028bdb0
 * @ghidraAddress JPN: 0x8028df40
 * @ghidraAddress EUR: 0x8028e510
 */
extern s32 g_dwTimingBarScaleHold;

/**
 * The render mode the meter leaves set, in two halves.
 *
 * @ghidraAddress USA: 0x8028a890
 * @ghidraAddress JPN: 0x8028c564
 * @ghidraAddress EUR: 0x8028ca94
 */
extern u32 g_dwTimingBarsRenderMode0;

/**
 * @ghidraAddress USA: 0x8028a894
 * @ghidraAddress JPN: 0x8028c568
 * @ghidraAddress EUR: 0x8028ca98
 */
extern u32 g_dwTimingBarsRenderMode1;

/**
 * Mark the time since the start of the frame on a bar, and colour the part that follows with
 * 8-bit components.
 *
 * @param bar The index of the bar.
 * @param red The red component.
 * @param green The green component.
 * @param blue The blue component.
 * @param alpha The alpha component. Only its top bit is used.
 * @ghidraAddress USA: 0x8022d7e0
 * @ghidraAddress JPN: 0x8022e4a0
 * @ghidraAddress EUR: 0x8022ecf0
 */
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha);

/**
 * End the frame of the meter with a magenta part on each bar, and start the next.
 *
 * @ghidraAddress USA: 0x8022d8ac
 * @ghidraAddress JPN: 0x8022e56c
 * @ghidraAddress EUR: 0x8022edbc
 */
void BeginTimingFrame(void);

/**
 * Draw the bars of the last frame, with a tick for each sixtieth of a second.
 *
 * @ghidraAddress USA: 0x8022d97c
 * @ghidraAddress JPN: 0x8022e63c
 * @ghidraAddress EUR: 0x8022ee8c
 */
void DrawTimingBars(void);

/**
 * How much the debugging displays show. The screens draw the timing bars from 2.
 *
 * @ghidraAddress USA: 0x8028aa98
 * @ghidraAddress JPN: 0x8028c770
 * @ghidraAddress EUR: 0x8028cca0
 */
extern s32 g_dwHudDebugLevel;

/**
 * Keep the random seed and the clock of the frames while the race is paused.
 *
 * @ghidraAddress USA: 0x8022af34
 * @ghidraAddress JPN: 0x8022ba94 (stub)
 * @ghidraAddress EUR: 0x8022c2dc (stub)
 */
void SaveClockForPause(void);

/**
 * Give the random seed and the clock of the frames back once the pause ends.
 *
 * @ghidraAddress USA: 0x8022af64
 * @ghidraAddress JPN: 0x8022bac8 (stub)
 * @ghidraAddress EUR: 0x8022c310 (stub)
 */
void RestoreClockAfterPause(void);

#endif

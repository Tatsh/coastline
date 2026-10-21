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
    /** The colour of the part that ends here, as two RGBA5551 pixels for a fill. */
    u32 color;
    /** The count of the CPU timer since the start of the frame. */
    u32 time;
} TimingMarker;

/** The markers of the two frames. @ghidraAddress USA: 0x8034e9b0 */
extern TimingMarker g_aTimingMarkers[2][TIMING_BARS][TIMING_MARKERS];

/** @ghidraAddress USA: 0x803519b0 */
extern s32 g_aTimingMarkerCounts[2][TIMING_BARS];

/** The frame the markers go to. The meter shows the other one. @ghidraAddress USA: 0x8028bda0 */
extern s32 g_dwTimingFrame;

/** The timer count at the start of the frame. @ghidraAddress USA: 0x8028bdac */
extern u32 g_dwTimingFrameStart;

/** Reset at the start of the frame. @ghidraAddress USA: 0x8028bda8 */
extern u32 g_dwTimingFrameElapsed;

/** The timer count the full length of the bars stands for. @ghidraAddress USA: 0x803519c8 */
extern u32 g_dwTimingBarScale;

/** The frames the scale keeps before it may shrink. @ghidraAddress USA: 0x8028bdb0 */
extern s32 g_dwTimingBarScaleHold;

/** The render mode the meter leaves set, in two halves. @ghidraAddress USA: 0x8028a890 */
extern u32 g_dwTimingBarsRenderMode0;

/** @ghidraAddress USA: 0x8028a894 */
extern u32 g_dwTimingBarsRenderMode1;

/**
 * Mark the time since the start of the frame on a bar, and colour the part that follows with
 * 8-bit components.
 *
 * @ghidraAddress USA: 0x8022d7e0
 */
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha);

/**
 * End the frame of the meter with a magenta part on each bar, and start the next.
 *
 * @ghidraAddress USA: 0x8022d8ac
 */
void BeginTimingFrame(void);

/**
 * Draw the bars of the last frame, with a tick for each sixtieth of a second.
 *
 * @ghidraAddress USA: 0x8022d97c
 */
void DrawTimingBars(void);

#endif

#ifndef LIBULTRA_IO_VIINT_H
#define LIBULTRA_IO_VIINT_H

/**
 * @file
 * The two video contexts the VI manager swaps on every retrace, and the manager's state.
 */

#include <ultra64.h>

/* What a context changes when the manager swaps it in. */
#define VI_STATE_MODE_UPDATED 0x01   /*!< The video mode changed. */
#define VI_STATE_XSCALE_UPDATED 0x02 /*!< The horizontal scale changed. */
#define VI_STATE_YSCALE_UPDATED 0x04 /*!< The vertical scale changed. */
#define VI_STATE_CTRL_UPDATED 0x08   /*!< The control register changed. */
#define VI_STATE_BUFFER_UPDATED 0x10 /*!< The frame buffer changed. */
#define VI_STATE_BLACK 0x20          /*!< The screen is black. */
#define VI_STATE_REPEATLINE 0x40     /*!< Every line repeats the first. */
#define VI_STATE_FADE 0x80           /*!< The screen fades. */

/* Bits of the control register. */
#define VI_CTRL_GAMMA_DITHER_ON 0x00004  /*!< Dither before the gamma correction. */
#define VI_CTRL_GAMMA_ON 0x00008         /*!< Apply the gamma correction. */
#define VI_CTRL_DIVOT_ON 0x00010         /*!< Remove the divots the anti-aliasing leaves. */
#define VI_CTRL_ANTIALIAS_MASK 0x00300   /*!< The bits of the anti-aliasing mode. */
#define VI_CTRL_DITHER_FILTER_ON 0x10000 /*!< Filter the dithering of 16-bit frame buffers. */

/* The scale registers store a 2.10 fixed-point factor in their low 12 bits. */
#define VI_SCALE_MASK 0xFFF      /*!< The bits of the scale factor. */
#define VI_2_10_FPART_MASK 0x3FF /*!< The bits of the fraction of the scale factor. */
#define VI_SUBPIXEL_SH 0x10      /*!< The shift of the subpixel offset in a scale register. */

typedef struct {
    f32 factor; /*!< The factor the nominal scale of the mode is multiplied by. */
    u16 offset; /*!< The subpixel offset, a 2.10 fixed-point fraction. */
    u32 scale;  /*!< The value of the scale register. */
} __OSViScale;

typedef struct {
    u16 state;         /*!< The VI_STATE_* bits of the changes to apply at the next retrace. */
    u16 retraceCount;  /*!< The number of retraces between event messages. */
    void *framep;      /*!< The frame buffer to show. */
    OSViMode *modep;   /*!< The video mode. */
    u32 control;       /*!< The value of the VI control register. */
    OSMesgQueue *msgq; /*!< The queue osViSetEvent() registered, or NULL. */
    OSMesg msg;        /*!< The message sent to msgq after every retraceCount retraces. */
    __OSViScale x;     /*!< The horizontal scale. */
    __OSViScale y;     /*!< The vertical scale. */
} __OSViContext;

/**
 * @ghidraAddress USA: 0x802a61c0
 * @ghidraAddress EUR: 0x802a7dc0
 * @ghidraAddress JPN: 0x802a78b0
 */
extern __OSViContext __osViContexts[2];

/**
 * @ghidraAddress USA: 0x802a6220
 * @ghidraAddress EUR: 0x802a7e20
 * @ghidraAddress JPN: 0x802a7910
 */
extern __OSViContext *__osViCurr;

/**
 * @ghidraAddress USA: 0x802a6224
 * @ghidraAddress EUR: 0x802a7e24
 * @ghidraAddress JPN: 0x802a7914
 */
extern __OSViContext *__osViNext;

/**
 * @ghidraAddress USA: 0x802a6160
 * @ghidraAddress EUR: 0x802a7c70
 * @ghidraAddress JPN: 0x802a77b0
 */
extern OSDevMgr __osViDevMgr;

/**
 * @ghidraAddress USA: 0x8037f490
 * @ghidraAddress EUR: 0x803813d0
 * @ghidraAddress JPN: 0x80380040
 */
extern OSThread viThread;

/**
 * @ghidraAddress USA: 0x8037f640
 * @ghidraAddress EUR: 0x80381580
 * @ghidraAddress JPN: 0x803801f0
 */
extern u64 viThreadStack[0x1000 / sizeof(u64)];

/**
 * @ghidraAddress USA: 0x80380640
 * @ghidraAddress EUR: 0x80382580
 * @ghidraAddress JPN: 0x803811f0
 */
extern OSMesgQueue viEventQueue;

/**
 * @ghidraAddress USA: 0x80380658
 * @ghidraAddress EUR: 0x80382598
 * @ghidraAddress JPN: 0x80381208
 */
extern OSMesg viEventBuf[5];

/**
 * @ghidraAddress USA: 0x80380670
 * @ghidraAddress EUR: 0x803825b0
 * @ghidraAddress JPN: 0x80381220
 */
extern OSIoMesg viRetraceMsg;

/**
 * @ghidraAddress USA: 0x80380688
 * @ghidraAddress EUR: 0x803825c8
 * @ghidraAddress JPN: 0x80381238
 */
extern OSIoMesg viCounterMsg;

/**
 * Retraces remaining before viMgrMain() next sends the message of osViSetEvent().
 *
 * @ghidraAddress USA: 0x803806a0
 * @ghidraAddress EUR: 0x803825e0
 * @ghidraAddress JPN: 0x80381250
 */
extern u16 viMgrRetrace;

/**
 * @ghidraAddress USA: 0x80268580
 * @ghidraAddress EUR: 0x8026d460
 * @ghidraAddress JPN: 0x8026cf10
 */
void __osViInit(void);

/**
 * @ghidraAddress USA: 0x8026c070
 * @ghidraAddress EUR: 0x8026e040
 * @ghidraAddress JPN: 0x8026daf0
 */
void __osViSwapContext(void);

/**
 * @ghidraAddress USA: 0x8026c060
 * @ghidraAddress EUR: 0x8026e030
 * @ghidraAddress JPN: 0x8026dae0
 */
__OSViContext *__osViGetCurrentContext(void);

#endif

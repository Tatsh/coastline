#ifndef LIBULTRA_IO_VIINT_H
#define LIBULTRA_IO_VIINT_H

/**
 * @file
 * The two video contexts the VI manager swaps on every retrace, and the manager's state.
 */

#include <ultra64.h>

/* What a context changes when the manager swaps it in. */
#define VI_STATE_MODE_UPDATED 0x01
#define VI_STATE_XSCALE_UPDATED 0x02
#define VI_STATE_YSCALE_UPDATED 0x04
#define VI_STATE_CTRL_UPDATED 0x08
#define VI_STATE_BUFFER_UPDATED 0x10
#define VI_STATE_BLACK 0x20
#define VI_STATE_REPEATLINE 0x40
#define VI_STATE_FADE 0x80

/* Bits of the control register. */
#define VI_CTRL_GAMMA_DITHER_ON 0x00004
#define VI_CTRL_GAMMA_ON 0x00008
#define VI_CTRL_DIVOT_ON 0x00010
#define VI_CTRL_ANTIALIAS_MASK 0x00300
#define VI_CTRL_DITHER_FILTER_ON 0x10000

/* The scale registers store a 2.10 fixed-point factor in their low 12 bits. */
#define VI_SCALE_MASK 0xFFF
#define VI_2_10_FPART_MASK 0x3FF
#define VI_SUBPIXEL_SH 0x10

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

/** @ghidraAddress USA: 0x802a61c0 */
extern __OSViContext __osViContexts[2];

/** @ghidraAddress USA: 0x802a6220 */
extern __OSViContext *__osViCurr;

/** @ghidraAddress USA: 0x802a6224 */
extern __OSViContext *__osViNext;

/** @ghidraAddress USA: 0x802a6160 */
extern OSDevMgr __osViDevMgr;

/** @ghidraAddress USA: 0x8037f490 */
extern OSThread viThread;

/** @ghidraAddress USA: 0x8037f640 */
extern u64 viThreadStack[0x1000 / sizeof(u64)];

/** @ghidraAddress USA: 0x80380640 */
extern OSMesgQueue viEventQueue;

/** @ghidraAddress USA: 0x80380658 */
extern OSMesg viEventBuf[5];

/** @ghidraAddress USA: 0x80380670 */
extern OSIoMesg viRetraceMsg;

/** @ghidraAddress USA: 0x80380688 */
extern OSIoMesg viCounterMsg;

/**
 * Retraces remaining before viMgrMain() next sends the message of osViSetEvent().
 *
 * @ghidraAddress USA: 0x803806a0
 */
extern u16 viMgrRetrace;

/** @ghidraAddress USA: 0x80268580 */
void __osViInit(void);

/** @ghidraAddress USA: 0x8026c070 */
void __osViSwapContext(void);

/** @ghidraAddress USA: 0x8026c060 */
__OSViContext *__osViGetCurrentContext(void);

/**
 * Body of the VI manager thread. It swaps the contexts on every retrace, sends the message of
 * osViSetEvent(), advances the time, and services the timers.
 *
 * @param arg The OSDevMgr of the VI manager, __osViDevMgr.
 * @ghidraAddress USA: 0x80266588
 */
void viMgrMain(void *arg);

#endif

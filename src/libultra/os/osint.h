#ifndef LIBULTRA_OS_OSINT_H
#define LIBULTRA_OS_OSINT_H

/**
 * @file
 * Scheduler state and the routines of the exception handler that the thread and message routines
 * use. The exception handler itself is hand-written assembly.
 */

#include <ultra64.h>

typedef struct {
    OSMesgQueue *messageQueue; /*!< The queue the event sends to, or NULL when none is set. */
    OSMesg message;            /*!< The message sent when the event occurs. */
} __OSEventState;

/**
 * @ghidraAddress USA: 0x802a6290
 * @ghidraAddress EUR: 0x802a7d80
 * @ghidraAddress JPN: 0x802a7870
 */
extern struct __osThreadTail {
    OSThread *next; /*!< NULL; the tail has no next thread. */
    OSPri priority; /*!< -1, below every thread. */
} __osThreadTail;

/**
 * @ghidraAddress USA: 0x802a6298
 * @ghidraAddress EUR: 0x802a7d88
 * @ghidraAddress JPN: 0x802a7878
 */
extern OSThread *__osRunQueue;

/**
 * @ghidraAddress USA: 0x802a629c
 * @ghidraAddress EUR: 0x802a7d8c
 * @ghidraAddress JPN: 0x802a787c
 */
extern OSThread *__osActiveQueue;

/**
 * @ghidraAddress USA: 0x802a62a0
 * @ghidraAddress EUR: 0x802a7d90
 * @ghidraAddress JPN: 0x802a7880
 */
extern OSThread *__osRunningThread;

/**
 * @ghidraAddress USA: 0x8037f370
 * @ghidraAddress EUR: 0x803812b0
 * @ghidraAddress JPN: 0x8037ff20
 */
extern __OSEventState __osEventStateTab[];

/**
 * @ghidraAddress USA: 0x803818a0
 * @ghidraAddress EUR: 0x80383820
 * @ghidraAddress JPN: 0x80382490
 */
extern OSTimer __osBaseTimer;

/**
 * @ghidraAddress USA: 0x802a62d0
 * @ghidraAddress EUR: 0x802a7e30
 * @ghidraAddress JPN: 0x802a7920
 */
extern OSTimer *__osTimerList;

/**
 * @ghidraAddress USA: 0x803818c0
 * @ghidraAddress EUR: 0x80383840
 * @ghidraAddress JPN: 0x803824b0
 */
extern OSTime __osCurrentTime;

/**
 * @ghidraAddress USA: 0x803818c8
 * @ghidraAddress EUR: 0x80383848
 * @ghidraAddress JPN: 0x803824b8
 */
extern u32 __osBaseCounter;

/**
 * @ghidraAddress USA: 0x803818cc
 * @ghidraAddress EUR: 0x8038384c
 * @ghidraAddress JPN: 0x803824bc
 */
extern u32 __osViIntrCount;

/**
 * @ghidraAddress USA: 0x803818d0
 * @ghidraAddress EUR: 0x80383850
 * @ghidraAddress JPN: 0x803824c0
 */
extern u32 __osTimerCounter;

/**
 * @ghidraAddress USA: 0x8026bc40
 * @ghidraAddress EUR: 0x8026dc10
 * @ghidraAddress JPN: 0x8026d6c0
 */
void __osTimerServicesInit(void);

/**
 * @ghidraAddress USA: 0x8026bccc
 * @ghidraAddress EUR: 0x8026dc9c
 * @ghidraAddress JPN: 0x8026d74c
 */
void __osTimerInterrupt(void);

/**
 * @ghidraAddress USA: 0x8026be44
 * @ghidraAddress EUR: 0x8026de14
 * @ghidraAddress JPN: 0x8026d8c4
 */
void __osSetTimerIntr(OSTime value);

/**
 * @ghidraAddress USA: 0x8026beb8
 * @ghidraAddress EUR: 0x8026de88
 * @ghidraAddress JPN: 0x8026d938
 */
OSTime __osInsertTimer(OSTimer *t);

/**
 * @ghidraAddress USA: 0x8026e6c0
 * @ghidraAddress EUR: 0x80270690
 * @ghidraAddress JPN: 0x80270140
 */
void __osSetCompare(u32 value);

/**
 * @ghidraAddress USA: 0x802a6150
 * @ghidraAddress EUR: 0x802a7c60
 * @ghidraAddress JPN: 0x802a77a0
 */
extern u32 __OSGlobalIntMask;

/**
 * @ghidraAddress USA: 0x8037f480
 * @ghidraAddress EUR: 0x803813c0
 * @ghidraAddress JPN: 0x80380030
 */
extern u32 __osFinalrom;

/**
 * @ghidraAddress USA: 0x802a62a4
 * @ghidraAddress EUR: 0x802a7d94
 * @ghidraAddress JPN: 0x802a7884
 */
extern OSThread *__osFaultedThread;

/**
 * @ghidraAddress USA: 0x8026e7d0
 * @ghidraAddress EUR: 0x802707a0
 * @ghidraAddress JPN: 0x80270250
 */
void __osSetGlobalIntMask(u32 mask);

/**
 * @ghidraAddress USA: 0x8026e6d0
 * @ghidraAddress EUR: 0x802706a0
 * @ghidraAddress JPN: 0x80270150
 */
void __osResetGlobalIntMask(u32 mask);

/**
 * @ghidraAddress USA: 0x80266390
 * @ghidraAddress EUR: 0x80267b20
 * @ghidraAddress JPN: 0x80267620
 */
OSThread *__osGetCurrFaultedThread(void);

/**
 * @ghidraAddress USA: 0x8026b9f0
 * @ghidraAddress EUR: 0x8026d9c0
 * @ghidraAddress JPN: 0x8026d470
 */
void __osSetSR(u32 value);

/**
 * @ghidraAddress USA: 0x8026ba00
 * @ghidraAddress EUR: 0x8026d9d0
 * @ghidraAddress JPN: 0x8026d480
 */
u32 __osGetSR(void);

/**
 * @ghidraAddress USA: 0x8026ba10
 * @ghidraAddress EUR: 0x8026d9e0
 * @ghidraAddress JPN: 0x8026d490
 */
u32 __osSetFpcCsr(u32 value);

/**
 * @ghidraAddress USA: 0x8026e600
 * @ghidraAddress EUR: 0x802705d0
 * @ghidraAddress JPN: 0x80270080
 */
u32 __osProbeTLB(void *vaddr);

/**
 * @ghidraAddress USA: 0x8026acc0
 * @ghidraAddress EUR: 0x8026cb50
 * @ghidraAddress JPN: 0x8026c600
 */
void __osExceptionPreamble(void);

/**
 * @ghidraAddress USA: 0x8026acd0
 * @ghidraAddress EUR: 0x8026cb60
 * @ghidraAddress JPN: 0x8026c610
 */
void __osException(void);

/**
 * @ghidraAddress USA: 0x80268540
 * @ghidraAddress EUR: 0x8026cad0
 * @ghidraAddress JPN: 0x8026c580
 */
OSIntMask __osDisableInt(void);

/**
 * @ghidraAddress USA: 0x80268560
 * @ghidraAddress EUR: 0x8026caf0
 * @ghidraAddress JPN: 0x8026c5a0
 */
void __osRestoreInt(OSIntMask im);

/**
 * @ghidraAddress USA: 0x8026b2ec
 * @ghidraAddress EUR: 0x8026d17c
 * @ghidraAddress JPN: 0x8026cc2c
 */
void __osEnqueueAndYield(OSThread **queue);

/**
 * @ghidraAddress USA: 0x8026b3ec
 * @ghidraAddress EUR: 0x8026d27c
 * @ghidraAddress JPN: 0x8026cd2c
 */
void __osEnqueueThread(OSThread **queue, OSThread *t);

/**
 * @ghidraAddress USA: 0x8026b434
 * @ghidraAddress EUR: 0x8026d2c4
 * @ghidraAddress JPN: 0x8026cd74
 */
OSThread *__osPopThread(OSThread **queue);

/**
 * @ghidraAddress USA: 0x8026ac80
 * @ghidraAddress EUR: 0x8026cb10
 * @ghidraAddress JPN: 0x8026c5c0
 */
void __osDequeueThread(OSThread **queue, OSThread *t);

/**
 * @ghidraAddress USA: 0x8026b444
 * @ghidraAddress EUR: 0x8026d2d4
 * @ghidraAddress JPN: 0x8026cd84
 */
void __osDispatchThread(void);

/**
 * @ghidraAddress USA: 0x8026b5c0
 * @ghidraAddress EUR: 0x8026d450
 * @ghidraAddress JPN: 0x8026cf00
 */
void __osCleanupThread(void);

#endif

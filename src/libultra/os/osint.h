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

/** @ghidraAddress USA: 0x802a6290 */
extern struct __osThreadTail {
    OSThread *next; /*!< NULL; the tail has no next thread. */
    OSPri priority; /*!< -1, below every thread. */
} __osThreadTail;

/** @ghidraAddress USA: 0x802a6298 */
extern OSThread *__osRunQueue;

/** @ghidraAddress USA: 0x802a629c */
extern OSThread *__osActiveQueue;

/** @ghidraAddress USA: 0x802a62a0 */
extern OSThread *__osRunningThread;

/** @ghidraAddress USA: 0x8037f370 */
extern __OSEventState __osEventStateTab[];

/** @ghidraAddress USA: 0x803818a0 */
extern OSTimer __osBaseTimer;

/** @ghidraAddress USA: 0x802a62d0 */
extern OSTimer *__osTimerList;

/** @ghidraAddress USA: 0x803818c0 */
extern OSTime __osCurrentTime;

/** @ghidraAddress USA: 0x803818c8 */
extern u32 __osBaseCounter;

/** @ghidraAddress USA: 0x803818cc */
extern u32 __osViIntrCount;

/** @ghidraAddress USA: 0x803818d0 */
extern u32 __osTimerCounter;

/** @ghidraAddress USA: 0x8026bc40 */
void __osTimerServicesInit(void);

/** @ghidraAddress USA: 0x8026bccc */
void __osTimerInterrupt(void);

/** @ghidraAddress USA: 0x8026be44 */
void __osSetTimerIntr(OSTime value);

/** @ghidraAddress USA: 0x8026beb8 */
OSTime __osInsertTimer(OSTimer *t);

/** @ghidraAddress USA: 0x8026e6c0 */
void __osSetCompare(u32 value);

/** @ghidraAddress USA: 0x802a6150 */
extern u32 __OSGlobalIntMask;

/** @ghidraAddress USA: 0x8037f480 */
extern u32 __osFinalrom;

/** @ghidraAddress USA: 0x802a62a4 */
extern OSThread *__osFaultedThread;

/** @ghidraAddress USA: 0x8026e7d0 */
void __osSetGlobalIntMask(u32 mask);

/** @ghidraAddress USA: 0x8026e6d0 */
void __osResetGlobalIntMask(u32 mask);

/** @ghidraAddress USA: 0x80266390 */
OSThread *__osGetCurrFaultedThread(void);

/** @ghidraAddress USA: 0x8026b9f0 */
void __osSetSR(u32 value);

/** @ghidraAddress USA: 0x8026ba00 */
u32 __osGetSR(void);

/** @ghidraAddress USA: 0x8026ba10 */
u32 __osSetFpcCsr(u32 value);

/** @ghidraAddress USA: 0x8026e600 */
u32 __osProbeTLB(void *vaddr);

/** @ghidraAddress USA: 0x8026acc0 */
void __osExceptionPreamble(void);

/** @ghidraAddress USA: 0x8026acd0 */
void __osException(void);

/** @ghidraAddress USA: 0x80268540 */
OSIntMask __osDisableInt(void);

/** @ghidraAddress USA: 0x80268560 */
void __osRestoreInt(OSIntMask im);

/** @ghidraAddress USA: 0x8026b2ec */
void __osEnqueueAndYield(OSThread **queue);

/** @ghidraAddress USA: 0x8026b3ec */
void __osEnqueueThread(OSThread **queue, OSThread *t);

/** @ghidraAddress USA: 0x8026b434 */
OSThread *__osPopThread(OSThread **queue);

/** @ghidraAddress USA: 0x8026ac80 */
void __osDequeueThread(OSThread **queue, OSThread *t);

/** @ghidraAddress USA: 0x8026b444 */
void __osDispatchThread(void);

/** @ghidraAddress USA: 0x8026b5c0 */
void __osCleanupThread(void);

#endif

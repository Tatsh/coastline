#ifndef LIBULTRA_OS_OSINT_H
#define LIBULTRA_OS_OSINT_H

/**
 * @file
 * Scheduler state and the routines of the exception handler that the thread and message routines
 * use. The exception handler itself is hand-written assembly.
 */

#include <ultra64.h>

/** One registration of osSetEventMesg(). */
typedef struct {
    OSMesgQueue *messageQueue;
    OSMesg message;
} __OSEventState;

/**
 * The head of the empty thread queue. Every queue ends with it. Its priority of -1 is lower than
 * that of every thread.
 *
 * @ghidraAddress USA: 0x802a6290
 */
extern struct __osThreadTail {
    OSThread *next;
    OSPri priority;
} __osThreadTail;

/** @ghidraAddress USA: 0x802a6298 */
extern OSThread *__osRunQueue;

/** @ghidraAddress USA: 0x802a629c */
extern OSThread *__osActiveQueue;

/** @ghidraAddress USA: 0x802a62a0 */
extern OSThread *__osRunningThread;

/**
 * The queue and message of each event osSetEventMesg() registered, by event number.
 *
 * @ghidraAddress USA: 0x8037f370
 */
extern __OSEventState __osEventStateTab[];

/**
 * The first timer of the list of pending timers, a sentinel that never expires.
 *
 * @ghidraAddress USA: 0x803818a0
 */
extern OSTimer __osBaseTimer;

/** @ghidraAddress USA: 0x802a62d0 */
extern OSTimer *__osTimerList;

/**
 * The time osGetTime() returns, as of the last retrace.
 *
 * @ghidraAddress USA: 0x803818c0
 */
extern OSTime __osCurrentTime;

/**
 * The CPU counter at the last retrace.
 *
 * @ghidraAddress USA: 0x803818c8
 */
extern u32 __osBaseCounter;

/** @ghidraAddress USA: 0x803818cc */
extern u32 __osViIntrCount;

/**
 * The CPU counter when the compare register was last set.
 *
 * @ghidraAddress USA: 0x803818d0
 */
extern u32 __osTimerCounter;

/**
 * Empty the timer list and clear the time.
 *
 * @ghidraAddress USA: 0x8026bc40
 */
void __osTimerServicesInit(void);

/**
 * Send the message of every timer that expired, restart the periodic ones, and set the compare
 * register for the next.
 *
 * @ghidraAddress USA: 0x8026bccc
 */
void __osTimerInterrupt(void);

/**
 * Make the compare register raise its interrupt value cycles from now.
 *
 * @ghidraAddress USA: 0x8026be44
 */
void __osSetTimerIntr(OSTime value);

/**
 * Insert a timer into the list, whose values are relative to the timer before.
 *
 * @return The value of the timer relative to the one before it.
 * @ghidraAddress USA: 0x8026beb8
 */
OSTime __osInsertTimer(OSTimer *t);

/**
 * Write the compare register of coprocessor 0.
 *
 * @ghidraAddress USA: 0x8026e6c0
 */
void __osSetCompare(u32 value);

/**
 * The interrupts the system allows. osSetIntMask() and the exception handler mask with it.
 *
 * @ghidraAddress USA: 0x802a6150
 */
extern u32 __OSGlobalIntMask;

/**
 * Set by osInitialize(). The development library uses it to tell a retail cartridge apart.
 *
 * @ghidraAddress USA: 0x8037f480
 */
extern u32 __osFinalrom;

/**
 * The thread that last raised a CPU exception.
 *
 * @ghidraAddress USA: 0x802a62a4
 */
extern OSThread *__osFaultedThread;

/**
 * Allow more interrupts in the global mask.
 *
 * @ghidraAddress USA: 0x8026e7d0
 */
void __osSetGlobalIntMask(u32 mask);

/**
 * Remove interrupts from the global mask. The RCP interrupt and the interrupt enable bit remain.
 *
 * @ghidraAddress USA: 0x8026e6d0
 */
void __osResetGlobalIntMask(u32 mask);

/**
 * Return the thread that last raised a CPU exception.
 *
 * @ghidraAddress USA: 0x80266390
 */
OSThread *__osGetCurrFaultedThread(void);

/**
 * Write the Status register of coprocessor 0.
 *
 * @ghidraAddress USA: 0x8026b9f0
 */
void __osSetSR(u32 value);

/**
 * Read the Status register of coprocessor 0.
 *
 * @ghidraAddress USA: 0x8026ba00
 */
u32 __osGetSR(void);

/**
 * Replace the control and status register of the FPU.
 *
 * @return The previous value.
 * @ghidraAddress USA: 0x8026ba10
 */
u32 __osSetFpcCsr(u32 value);

/**
 * Translate a virtual address through the TLB.
 *
 * @return The physical address, or -1 when no valid entry maps it.
 * @ghidraAddress USA: 0x8026e600
 */
u32 __osProbeTLB(void *vaddr);

/**
 * The code osInitialize() copies to the exception vectors. It jumps to __osException().
 *
 * @ghidraAddress USA: 0x8026acc0
 */
void __osExceptionPreamble(void);

/**
 * Handle a CPU exception or interrupt, and resume the thread of highest priority.
 *
 * @ghidraAddress USA: 0x8026acd0
 */
void __osException(void);

/**
 * Disable interrupts.
 *
 * @return The interrupt enable bit of the status register before the call.
 * @ghidraAddress USA: 0x80268540
 */
OSIntMask __osDisableInt(void);

/**
 * Restore the interrupt enable bit __osDisableInt() returned.
 *
 * @param im The value __osDisableInt() returned.
 * @ghidraAddress USA: 0x80268560
 */
void __osRestoreInt(OSIntMask im);

/**
 * Save the running thread, insert it into a queue, and dispatch the next runnable thread.
 *
 * @param queue Queue the running thread waits in.
 * @ghidraAddress USA: 0x8026b2ec
 */
void __osEnqueueAndYield(OSThread **queue);

/**
 * Insert a thread into a queue after every thread of the same or a higher priority.
 *
 * @ghidraAddress USA: 0x8026b3ec
 */
void __osEnqueueThread(OSThread **queue, OSThread *t);

/**
 * Remove and return the first thread of a queue.
 *
 * @ghidraAddress USA: 0x8026b434
 */
OSThread *__osPopThread(OSThread **queue);

/**
 * Remove a thread from a queue.
 *
 * @ghidraAddress USA: 0x8026ac80
 */
void __osDequeueThread(OSThread **queue, OSThread *t);

/**
 * Restore the context of the first runnable thread and resume it.
 *
 * @ghidraAddress USA: 0x8026b444
 */
void __osDispatchThread(void);

/**
 * Return address of every thread osCreateThread() creates. It destroys the thread that returned.
 *
 * @ghidraAddress USA: 0x8026b5c0
 */
void __osCleanupThread(void);

#endif

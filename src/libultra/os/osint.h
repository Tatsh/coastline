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

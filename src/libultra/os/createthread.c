#include <ultra64.h>

#include "osint.h"

void osCreateThread(OSThread *t, OSId id, void (*entry)(void *), void *arg, void *sp, OSPri pri) {
    register OSIntMask saveMask;
    OSIntMask mask;

    t->id = id;
    t->priority = pri;
    t->next = NULL;
    t->queue = NULL;
    t->context.pc = (u32)entry;
    t->context.a0 = (s64)(s32)arg;
    /* The first stack frame reserves room for the argument registers. */
    t->context.sp = (s64)(s32)sp - 16;
    t->context.ra = (s64)(s32)__osCleanupThread;
    mask = OS_IM_ALL;
    t->context.sr = SR_IMASK | SR_EXL | SR_IE;
    t->context.rcp = (mask & 0x3F0000) >> 16;
    t->context.fpcsr = FPCSR_FS | FPCSR_EV;
    t->fp = 0;
    t->state = OS_STATE_STOPPED;
    t->flags = 0;
    saveMask = __osDisableInt();
    t->tlnext = __osActiveQueue;
    __osActiveQueue = t;
    __osRestoreInt(saveMask);
}

#include <ultra64.h>

#include "common/libultra/os/osint.h"

void osDestroyThread(OSThread *t) {
    register OSIntMask saveMask = __osDisableInt();
    register OSThread *pred;
    register OSThread *succ;

    if (t == NULL) {
        t = __osRunningThread;
    } else if (t->state != OS_STATE_STOPPED) {
        __osDequeueThread(t->queue, t);
    }
    if (__osActiveQueue == t) {
        __osActiveQueue = __osActiveQueue->tlnext;
    } else {
        pred = __osActiveQueue;
        while ((succ = pred->tlnext) != NULL) {
            if (succ == t) {
                pred->tlnext = t->tlnext;
                break;
            }
            pred = succ;
        }
    }
    if (t == __osRunningThread) {
        __osDispatchThread();
    }
    __osRestoreInt(saveMask);
}

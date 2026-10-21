#include <ultra64.h>

#include "osint.h"

void osYieldThread(void) {
    register OSIntMask saveMask = __osDisableInt();

    __osRunningThread->state = OS_STATE_RUNNABLE;
    __osEnqueueAndYield(&__osRunQueue);
    __osRestoreInt(saveMask);
}

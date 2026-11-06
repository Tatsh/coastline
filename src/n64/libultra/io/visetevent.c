#include <ultra64.h>

#include "common/libultra/os/osint.h"
#include "viint.h"

void osViSetEvent(OSMesgQueue *mq, OSMesg m, u32 retraceCount) {
    register OSIntMask saveMask = __osDisableInt();

    __osViNext->msgq = mq;
    __osViNext->msg = m;
    __osViNext->retraceCount = retraceCount;
    __osRestoreInt(saveMask);
}

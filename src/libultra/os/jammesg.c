#include <ultra64.h>

#include "osint.h"

s32 osJamMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) {
    register OSIntMask saveMask = __osDisableInt();

    while (MQ_IS_FULL(mq)) {
        if (flag != OS_MESG_BLOCK) {
            __osRestoreInt(saveMask);
            return -1;
        }
        __osRunningThread->state = OS_STATE_WAITING;
        __osEnqueueAndYield(&mq->fullqueue);
    }
    mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
    mq->msg[mq->first] = msg;
    mq->validCount++;
    if (mq->mtqueue->next != NULL) {
        osStartThread(__osPopThread(&mq->mtqueue));
    }
    __osRestoreInt(saveMask);
    return 0;
}

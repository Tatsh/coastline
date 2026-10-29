#include <PR/os.h>

#if !(TGR_BOSS_RALLY)
#include <ultra64.h>

#include "common/libultra/os/osint.h"
#endif

#if TGR_BOSS_RALLY
s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag) {
    return 1;
}
#else

s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag) {
    register OSIntMask saveMask = __osDisableInt();

    while (MQ_IS_EMPTY(mq)) {
        if (flag == OS_MESG_NOBLOCK) {
            __osRestoreInt(saveMask);
            return -1;
        }
        __osRunningThread->state = OS_STATE_WAITING;
        __osEnqueueAndYield(&mq->mtqueue);
    }
    if (msg != NULL) {
        *msg = mq->msg[mq->first];
    }
    mq->first = (mq->first + 1) % mq->msgCount;
    mq->validCount--;
    if (mq->fullqueue->next != NULL) {
        osStartThread(__osPopThread(&mq->fullqueue));
    }
    __osRestoreInt(saveMask);
    return 0;
}
#endif /* TGR_BOSS_RALLY */

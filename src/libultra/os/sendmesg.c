#include <PR/os.h>

#if !(TGR_BOSS_RALLY)
#include <ultra64.h>

#include "osint.h"
#endif

#if TGR_BOSS_RALLY
s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) {
    return 1;
}
#else

s32 osSendMesg(OSMesgQueue *mq, OSMesg msg, s32 flag) {
    register OSIntMask saveMask = __osDisableInt();
    register s32 last;

    while (MQ_IS_FULL(mq)) {
        if (flag == OS_MESG_BLOCK) {
            __osRunningThread->state = OS_STATE_WAITING;
            __osEnqueueAndYield(&mq->fullqueue);
        } else {
            __osRestoreInt(saveMask);
            return -1;
        }
    }
    last = (mq->first + mq->validCount) % mq->msgCount;
    mq->msg[last] = msg;
    mq->validCount++;
    if (mq->mtqueue->next != NULL) {
        osStartThread(__osPopThread(&mq->mtqueue));
    }
    __osRestoreInt(saveMask);
    return 0;
}
#endif /* TGR_BOSS_RALLY */

#include <ultra64.h>

#include "../os/osint.h"
#include "viint.h"

void osCreateViManager(OSPri pri) {
    u32 savedMask;
    OSPri oldPri;
    OSPri myPri;

    if (__osViDevMgr.active != 0) {
        return;
    }
    __osTimerServicesInit();
    osCreateMesgQueue(&viEventQueue, viEventBuf, 5);
    viRetraceMsg.hdr.type = OS_MESG_TYPE_VRETRACE;
    viRetraceMsg.hdr.pri = OS_MESG_PRI_NORMAL;
    viRetraceMsg.hdr.retQueue = NULL;
    viCounterMsg.hdr.type = OS_MESG_TYPE_COUNTER;
    viCounterMsg.hdr.pri = OS_MESG_PRI_NORMAL;
    viCounterMsg.hdr.retQueue = NULL;
    osSetEventMesg(OS_EVENT_VI, &viEventQueue, &viRetraceMsg);
    osSetEventMesg(OS_EVENT_COUNTER, &viEventQueue, &viCounterMsg);
    oldPri = -1;
    myPri = osGetThreadPri(NULL);
    if (myPri < pri) {
        oldPri = myPri;
        osSetThreadPri(NULL, pri);
    }
    savedMask = __osDisableInt();
    __osViDevMgr.active = 1;
    __osViDevMgr.thread = &viThread;
    __osViDevMgr.cmdQueue = &viEventQueue;
    __osViDevMgr.evtQueue = &viEventQueue;
    __osViDevMgr.acsQueue = NULL;
    __osViDevMgr.dma = NULL;
    __osViDevMgr.edma = NULL;
    osCreateThread(&viThread,
                   0,
                   viMgrMain,
                   &__osViDevMgr,
                   &viThreadStack[sizeof(viThreadStack) / sizeof(viThreadStack[0])],
                   pri);
    __osViInit();
    osStartThread(&viThread);
    __osRestoreInt(savedMask);
    if (oldPri != -1) {
        osSetThreadPri(NULL, oldPri);
    }
}

void viMgrMain(void *arg) {
    __OSViContext *vc;
    OSDevMgr *dm = arg;
    OSIoMesg *mb = NULL;
    u32 count;

    vc = __osViGetCurrentContext();
    viMgrRetrace = vc->retraceCount;
    if (viMgrRetrace == 0) {
        viMgrRetrace = 1;
    }
    for (;;) {
        osRecvMesg(dm->evtQueue, (OSMesg *)&mb, OS_MESG_BLOCK);
        switch (mb->hdr.type) {
        case OS_MESG_TYPE_VRETRACE:
            __osViSwapContext();
            viMgrRetrace--;
            if (viMgrRetrace == 0) {
                vc = __osViGetCurrentContext();
                if (vc->msgq != NULL) {
                    osSendMesg(vc->msgq, vc->msg, OS_MESG_NOBLOCK);
                }
                viMgrRetrace = vc->retraceCount;
            }
            __osViIntrCount++;
            /* The SDK sets the time from the counter on the first retrace, behind a flag the
             * original never sets. The binary also skips the step. */
            count = __osBaseCounter;
            __osBaseCounter = osGetCount();
            count = __osBaseCounter - count;
            __osCurrentTime = __osCurrentTime + count;
            break;
        case OS_MESG_TYPE_COUNTER:
            __osTimerInterrupt();
            break;
        }
    }
}

#include <ultra64.h>

#include "common/libultra/os/osint.h"
#include "piint.h"

OSDevMgr __osPiDevMgr = {0};
OSPiHandle *__osPiTable = NULL;
OSPiHandle *__osCurrentHandle[2] = {&CartRomHandle, &LeoDiskHandle};
OSThread piThread;
u64 piThreadStack[0x1000 / sizeof(u64)];
OSMesgQueue piEventQueue;
OSMesg piEventBuf[1];

void osCreatePiManager(OSPri pri, OSMesgQueue *cmdQ, OSMesg *cmdBuf, s32 cmdMsgCnt) {
    u32 savedMask;
    OSPri oldPri;
    OSPri myPri;

    if (__osPiDevMgr.active) {
        return;
    }
    osCreateMesgQueue(cmdQ, cmdBuf, cmdMsgCnt);
    osCreateMesgQueue(&piEventQueue, piEventBuf, 1);
    if (!__osPiAccessQueueEnabled) {
        __osPiCreateAccessQueue();
    }
    osSetEventMesg(OS_EVENT_PI, &piEventQueue, (OSMesg)0x22222222);
    oldPri = -1;
    myPri = osGetThreadPri(NULL);
    if (myPri < pri) {
        oldPri = myPri;
        osSetThreadPri(NULL, pri);
    }
    savedMask = __osDisableInt();
    __osPiDevMgr.active = 1;
    __osPiDevMgr.thread = &piThread;
    __osPiDevMgr.cmdQueue = cmdQ;
    __osPiDevMgr.evtQueue = &piEventQueue;
    __osPiDevMgr.acsQueue = &__osPiAccessQueue;
    __osPiDevMgr.dma = osPiRawStartDma;
    __osPiDevMgr.edma = osEPiRawStartDma;
    osCreateThread(&piThread,
                   0,
                   __osDevMgrMain,
                   &__osPiDevMgr,
                   &piThreadStack[sizeof(piThreadStack) / sizeof(piThreadStack[0])],
                   pri);
    osStartThread(&piThread);
    __osRestoreInt(savedMask);
    if (oldPri != -1) {
        osSetThreadPri(NULL, oldPri);
    }
}

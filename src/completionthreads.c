#include "system.h"
#include "timing.h"

#define VBLANK_COUNTER_MASK 15

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* Windows redirects the console hooks to its empty compatibility entry. */
#define osCreateMesgQueue(...) NullStub()
#define osSetEventMesg(...) NullStub()
#define osViSetEvent(...) NullStub()
#define AddTimingMarker(...) NullStub()
#define RETRACE_INCREMENT(value) ((s32)((u32)(value) + 1u))
#else
#define RETRACE_INCREMENT(value) ((value) + 1)
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

void RspDoneThread(void *arg) {
    osCreateMesgQueue(&g_RspEventMsgQueue, g_aRspEventMsgBuf, 1);
    osSetEventMesg(OS_EVENT_SP, &g_RspEventMsgQueue, (OSMesg)g_dwDummyOsMesg);
    for (;;) {
        osRecvMesg(&g_RspEventMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(1, 0, 0, 0, 0xFF);
        osSendMesg(&g_RspDoneMsgQueue, (OSMesg)g_dwDummyOsMesg, OS_MESG_BLOCK);
    }
}

void RdpDoneThread(void *arg) {
    osCreateMesgQueue(&g_RdpEventMsgQueue, g_aRdpEventMsgBuf, 1);
    osSetEventMesg(OS_EVENT_DP, &g_RdpEventMsgQueue, (OSMesg)g_dwDummyOsMesg);
    for (;;) {
        osRecvMesg(&g_RdpEventMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(2, 0, 0, 0, 0xFF);
        osSendMesg(&g_RdpDoneMsgQueue, (OSMesg)g_dwDummyOsMesg, OS_MESG_BLOCK);
    }
}

void ViRetraceThread(void *arg) {
    osCreateMesgQueue(&g_ViRetracePollMesgQueue, g_ViRetracePollMesgBuf, 1);
    osViSetEvent(&g_ViRetracePollMesgQueue, (OSMesg)g_dwDummyOsMesg, 1);
    for (;;) {
        osRecvMesg(&g_ViRetracePollMesgQueue, NULL, OS_MESG_BLOCK);
        osSendMesg(&g_ViRetraceMesgQueue, (OSMesg)g_dwDummyOsMesg, OS_MESG_BLOCK);
        g_dwVblankCounter = RETRACE_INCREMENT(g_dwVblankCounter) & VBLANK_COUNTER_MASK;
    }
}

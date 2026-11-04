#include "common/audio.h"
#include "common/system.h"
#include "common/timing.h"

/* clang-format off */
#include "common/framebegin.c"
#include "common/emptycallback.c"
#include "common/screendraw.c"
#include "n64/framebuffer.c"
#include "common/texturestream.c"
#include "common/controllerread.c"
#include "common/frameflush.c"
#include "common/camerarender.c"
#include "common/displaylistops.c"
/* clang-format on */

#define VBLANK_COUNTER_MASK 15
#define DUMMY_OS_MESG ((OSMesg)g_dwDummyOsMesg)

void RspDoneThread(void *arg) {
    osCreateMesgQueue(&g_RspEventMsgQueue, g_aRspEventMsgBuf, 1);
    osSetEventMesg(OS_EVENT_SP, &g_RspEventMsgQueue, DUMMY_OS_MESG);
    for (;;) {
        osRecvMesg(&g_RspEventMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(1, 0, 0, 0, 0xFF);
        osSendMesg(&g_RspDoneMsgQueue, DUMMY_OS_MESG, OS_MESG_BLOCK);
    }
}

void RdpDoneThread(void *arg) {
    osCreateMesgQueue(&g_RdpEventMsgQueue, g_aRdpEventMsgBuf, 1);
    osSetEventMesg(OS_EVENT_DP, &g_RdpEventMsgQueue, DUMMY_OS_MESG);
    for (;;) {
        osRecvMesg(&g_RdpEventMsgQueue, NULL, OS_MESG_BLOCK);
        AddTimingMarker(2, 0, 0, 0, 0xFF);
        osSendMesg(&g_RdpDoneMsgQueue, DUMMY_OS_MESG, OS_MESG_BLOCK);
    }
}

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void PreNmiResetHandlerThread(void *arg) {
    osCreateMesgQueue(&g_PreNmiMesgQueue, g_aPreNmiMesgBuf, 1);
    osSetEventMesg(OS_EVENT_PRENMI, &g_PreNmiMesgQueue, (OSMesg)g_dwDummyOsMesg);
    for (;;) {
        osRecvMesg(&g_PreNmiMesgQueue, NULL, OS_MESG_BLOCK);
        g_bSfxMasterVolume = 0;
        g_bBgmVolume = 0;
        g_dwPreNmiResetPending = TRUE;
        BlankDisplay();
    }
}
#endif

void ViRetraceThread(void *arg) {
    static s32 g_dwVblankCounter = 0;

    osCreateMesgQueue(&g_ViRetracePollMesgQueue, g_ViRetracePollMesgBuf, 1);
    osViSetEvent(&g_ViRetracePollMesgQueue, (OSMesg)g_dwDummyOsMesg, 1);
    for (;;) {
        osRecvMesg(&g_ViRetracePollMesgQueue, NULL, OS_MESG_BLOCK);
        osSendMesg(&g_ViRetraceMesgQueue, DUMMY_OS_MESG, OS_MESG_BLOCK);
        g_dwVblankCounter = (g_dwVblankCounter + 1) & VBLANK_COUNTER_MASK;
    }
}

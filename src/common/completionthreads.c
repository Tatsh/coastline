#include "common/audio.h"
#include "common/driving.h"
#include "common/system.h"
#include "common/timing.h"

#define VBLANK_COUNTER_MASK 15
#define JOLT_MAX_ONCE 2.5f
#define JOLT_MAX 5.0f

/* The message each thread waits for, a word that is never read as an address. */
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#define DUMMY_OS_MESG ((OSMesg)(uintptr_t)g_dwDummyOsMesg)
#else
#define DUMMY_OS_MESG ((OSMesg)g_dwDummyOsMesg)
#endif

#if TGR_BOSS_RALLY
/* Windows redirects the console hooks to its empty compatibility entry. */
#define osCreateMesgQueue(...) NullStub()
#define osSetEventMesg(...) NullStub()
#define osViSetEvent(...) NullStub()
#define AddTimingMarker(...) NullStub()
#define RETRACE_INCREMENT(value) ((s32)((u32)(value) + 1u))
#else
#define RETRACE_INCREMENT(value) ((value) + 1)
#endif /* TGR_BOSS_RALLY */

#if TGR_PLATFORM == TGR_PLATFORM_N64
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
        g_dwVblankCounter = RETRACE_INCREMENT(g_dwVblankCounter) & VBLANK_COUNTER_MASK;
    }
}
#endif

void AddVehicleJolt(s32 index, f32 amount) {
    if (JOLT_MAX_ONCE < amount) {
        amount = JOLT_MAX_ONCE;
    }
    /* The index of a computer's vehicle may pass the end of the array. */
    g_afVehicleJolts[index] += amount;
    if (JOLT_MAX < g_afVehicleJolts[index]) {
        g_afVehicleJolts[index] = JOLT_MAX;
    }
}

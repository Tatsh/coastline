#include "common/rumble.h"
#include "common/system.h"

#if TGR_BOSS_RALLY
#include "common/screen.h"

#pragma push_macro("errno")
#undef errno

/* The Windows build disables these console queue and thread operations. */
#define osCreateMesgQueue(...) NullStub()
#define osSetEventMesg(...) NullStub()
#define osViSetEvent(...) NullStub()
#define osCreateThread(...) NullStub()
#define osStartThread(...) NullStub()
#define osPfsInit(...) ReturnZero()
#define osMotorInit(...) ReturnZero()
#define osMotorStop(...) ReturnZero()

DisplayListCommand *g_pDisplayListBase;
f32 g_fCountsPerMillisecond;
OSContStatus g_aContStatus[MAXCONTROLLERS];
u8 g_abRumblePakPresent[MAXCONTROLLERS];
#endif /* TGR_BOSS_RALLY */

#define IO_MESSAGES 32
#define COUNTS_PER_MILLISECOND 46875.0f
#define RSP_DONE_THREAD_ID 10
#define RDP_DONE_THREAD_ID 11
#define PRE_NMI_THREAD_ID 12
#define EVENT_THREAD_PRIORITY 60

static s32 IsPakController(OSContStatus *status, u8 pattern, s32 i) {
    return ((pattern >> i) & 1) && !(status->CONT_ERRNO & CONT_NO_RESPONSE_ERROR) &&
           (status->type & CONT_JOYPORT) && (status->status & CONT_CARD_ON);
}

void InitRenderingSubsystem(void) {
    OSMesgQueue queue;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    OSMesg message;
#endif
    u8 pattern;
    s32 result;
    s32 i;

    g_pDisplayListBase = g_aGfxDisplayListPool;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    g_apFramebuffers[0] = g_aFramebuffer0;
    g_apFramebuffers[1] = g_aFramebuffer1;
#endif
    osCreateMesgQueue(&g_DmaMessageQueue, g_aDmaMsgBuf, IO_MESSAGES);
    osCreateMesgQueue(&g_RspDoneMsgQueue, g_aRspDoneMsgBuf, 1);
    osSetEventMesg(OS_EVENT_SP, &g_RspDoneMsgQueue, (OSMesg)g_dwDummyOsMesg);
    osCreateMesgQueue(&g_RdpDoneMsgQueue, g_aRdpDoneMsgBuf, 1);
    osSetEventMesg(OS_EVENT_DP, &g_RdpDoneMsgQueue, (OSMesg)g_dwDummyOsMesg);
    osCreateMesgQueue(&g_ViRetraceMesgQueue, g_ViRetraceMesgBuf, 1);
    osViSetEvent(&g_ViRetraceMesgQueue, (OSMesg)g_dwDummyOsMesg, 1);
    osCreateThread(&g_RspDoneThreadObj,
                   RSP_DONE_THREAD_ID,
                   RspDoneThread,
                   g_ppszMainArgv,
                   &g_adwRspDoneThreadStack[THREAD_STACK_WORDS],
                   EVENT_THREAD_PRIORITY);
    osStartThread(&g_RspDoneThreadObj);
    osCreateThread(&g_RdpDoneThreadObj,
                   RDP_DONE_THREAD_ID,
                   RdpDoneThread,
                   g_ppszMainArgv,
                   &g_adwRdpDoneThreadStack[THREAD_STACK_WORDS],
                   EVENT_THREAD_PRIORITY);
    osStartThread(&g_RdpDoneThreadObj);
#if TGR_PLATFORM == TGR_PLATFORM_N64 && (TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP)
    osCreateThread(&g_PreNmiResetHandlerThreadObj,
                   PRE_NMI_THREAD_ID,
                   PreNmiResetHandlerThread,
                   g_ppszMainArgv,
                   &g_adwPreNmiResetThreadStack[THREAD_STACK_WORDS],
                   EVENT_THREAD_PRIORITY);
    osStartThread(&g_PreNmiResetHandlerThreadObj);
#endif
    g_fCountsPerMillisecond = COUNTS_PER_MILLISECOND;

    /* The serial event reaches the queue on the stack until it is pointed elsewhere below. */
    osCreateMesgQueue(&queue, &message, 1);
    osSetEventMesg(OS_EVENT_SI, &queue, (OSMesg)1);
    /* Windows osContInit() returns without initialising pattern. */
    osContInit(&queue, &pattern, g_aContStatus);
    osCreateMesgQueue(&g_SiMesgQueue, g_aSiEventMesgBuf, 1);
    osSetEventMesg(OS_EVENT_SI, &g_SiMesgQueue, (OSMesg)0);
#if TGR_PLATFORM == TGR_PLATFORM_N64
    PrepareRumblePaks();
#endif
    for (i = 0; i < MAXCONTROLLERS; i++) {
        g_abRumblePakPresent[i] = FALSE;
        if (!IsPakController(&g_aContStatus[i], pattern, i)) {
            continue;
        }
        /* A pak that is not a Controller Pak may be a Rumble Pak. */
        result = osPfsInit(&g_SiMesgQueue, &g_aContPfs[i], i);
        if (result != 0 && (result == PFS_ERR_ID_FATAL || result == PFS_ERR_DEVICE) &&
            osMotorInit(&g_SiMesgQueue, &g_aContPfs[i], i) == 0) {
            g_abRumblePakPresent[i] = TRUE;
            osMotorStop(&g_aContPfs[i]);
        }
    }
#if TGR_BOSS_RALLY
    CreateWindowsRenderMutex();
#endif
}

#if TGR_BOSS_RALLY
#pragma pop_macro("errno")
#endif

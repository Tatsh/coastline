#include "system.h"

#define IO_MESSAGES 32

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define IO_INCREMENT(value) ((s32)((u32)(value) + 1u))
#else
#define IO_INCREMENT(value) ((value) + 1)
#endif

OSIoMesg *AllocIoMessage(void) {
    if (g_dwIoMessagePendingCount == IO_MESSAGES) {
        /* With every message in flight, one finished DMA frees its message. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        osContInit(&g_DmaMessageQueue, NULL, NULL);
#else
        osRecvMesg(&g_DmaMessageQueue, NULL, OS_MESG_BLOCK);
#endif
    } else {
        g_dwIoMessagePendingCount = IO_INCREMENT(g_dwIoMessagePendingCount);
    }
    g_dwIoMessagePoolIndex = IO_INCREMENT(g_dwIoMessagePoolIndex) % IO_MESSAGES;
    return &g_aIoMessagePool[g_dwIoMessagePoolIndex];
}

void WaitForDmaMessages(void) {
    if (g_dwIoMessagePendingCount != 0) {
        do {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            osContInit(&g_DmaMessageQueue, NULL, NULL);
            g_dwIoMessagePendingCount = (s32)((u32)g_dwIoMessagePendingCount - 1u);
#else
            osRecvMesg(&g_DmaMessageQueue, NULL, OS_MESG_BLOCK);
            --g_dwIoMessagePendingCount;
#endif
        } while (g_dwIoMessagePendingCount != 0);
    }
}

void WaitForVblankMessage(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    osContInit(&g_ViRetraceMesgQueue, NULL, NULL);
#else
    osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
#endif
}

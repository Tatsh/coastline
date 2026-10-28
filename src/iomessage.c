#include "system.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The DMA started and not yet waited for.
 *
 * @ghidraAddress USA: 0x80272d44
 * @ghidraAddress EUR: 0x80274f44
 * @ghidraAddress JPN: 0x80274a14
 * @ghidraAddress BRD3D: 0x106c65dc
 */
static s32 g_dwIoMessagePendingCount;
/**
 * The message of g_aIoMessagePool last handed out.
 *
 * @ghidraAddress USA: 0x80272d40
 * @ghidraAddress EUR: 0x80274f40
 * @ghidraAddress JPN: 0x80274a10
 * @ghidraAddress BRD3D: 0x106c65d8
 */
static s32 g_dwIoMessagePoolIndex;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

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

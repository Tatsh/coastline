/* Test of src/system.c: the DMA in blocks, the pool of its messages, the clock of the frames and
 * the routines of the screen. */
#include <assert.h>
#include <stdio.h>

#include "system.c"

OSIoMesg g_aIoMessagePool[32];
s32 g_dwIoMessagePendingCount, g_dwIoMessagePoolIndex;
OSMesgQueue g_DmaMessageQueue;
u32 g_dwDmaAssetSizeBuffer, g_dwDmaReadWordBuffer, g_dwDmaChunkSize;
u64 g_qwTimestampBase;
u32 g_dwFrameTimestamp;
s32 g_dwElapsedTimeMs, g_dwPrevElapsedTimeMs, g_dwFrameDeltaMs, g_bRaceScreenActive;
f32 g_fFrameDeltaTime;
u64 osClockRate = 62500000;
ScreenCallback g_pfnCurrentScreen, g_pfnNextScreen, g_pfnSecondaryScreen;
const char *g_pszDebugMessage;

static u32 count;
static s32 dmaCount, received, dmaSizes[8];
static u32 dmaRoms[8];

u32 osGetCount(void) {
    return count;
}
s32 osRecvMesg(OSMesgQueue *q, OSMesg *m, s32 flag) {
    received++;
    return 0;
}
s32 osPiStartDma(
    OSIoMesg *mb, s32 pri, s32 direction, u32 devAddr, void *dramAddr, u32 size, OSMesgQueue *mq) {
    dmaRoms[dmaCount] = devAddr;
    dmaSizes[dmaCount++] = size;
    return 0;
}
void osInvalDCache(void *vaddr, s32 nbytes) {
}
void UpdateControllerState(void) {
}
void DebugFrameStubA(void) {
}
void DebugFrameStubB(void) {
}

static s32 calls;
static void Screen(void) {
    calls++;
}

int main(void) {
    static u8 buffer[0x2800];
    s32 i;

    /* A read of 10 KB goes as two blocks of 4 KB and the rest, then waits for all three. */
    DmaRead(buffer, 0x1000, 0x2800);
    assert(dmaCount == 3 && dmaRoms[1] == 0x2000 && dmaSizes[2] == 0x800);
    assert(received == 3 && g_dwIoMessagePendingCount == 0);

    /* With 32 messages in flight, the next waits for one and the count stays at 32. The index goes
     * round from the 3 the read took. */
    received = 0;
    for (i = 0; i < 33; i++) {
        AllocIoMessage();
    }
    assert(g_dwIoMessagePendingCount == 32 && received == 1 && g_dwIoMessagePoolIndex == 4);

    /* The clock counts the CPU counter, here 62500 counts a millisecond... */
    ResetFrameClock();
    count = 1000;
    UpdateFrameTiming();
    count = 1000 + 62500 * 3;
    UpdateFrameTiming();
    assert(g_dwElapsedTimeMs == 3 && g_dwFrameDeltaMs == 3 && g_fFrameDeltaTime == 0.003f);
    /* ...and a race steps 1/30 of a second a frame whatever the counter does. */
    g_bRaceScreenActive = TRUE;
    UpdateFrameTiming();
    assert(g_dwFrameDeltaMs == 33);

    SetCurrentScreenCallback(Screen);
    assert(IsCurrentScreen(Screen) && !IsCurrentScreen(NULL));
    CallCurrentGameMode();
    assert(calls == 1);
    SetDebugMessage("hello");
    assert(g_pszDebugMessage[0] == 'h');

    puts("system ok");
    return 0;
}

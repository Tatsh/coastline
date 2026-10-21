#include "system.h"

#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "debugtext.h"
#include "game.h"
#include "input.h"
#include "libultra/os/osint.h"
#include "platform/n64/boot.h"
#include "render.h"
#include "rumble.h"
#include "timing.h"
#include "zlib.h"

/* The messages of the DMA that may wait at once. */
#define IO_MESSAGES 32

/* A DMA copies at most 4 KB at once. */
#define DMA_BLOCK_SIZE 0x1000

/* Each half of a buffer of compressed data, and the room the decompression of one gets. */
#define CHUNK_BUFFER_SIZE 16000
#define CHUNK_OUTPUT_SIZE 16000

/* The heap lent to zlib by a decompression that runs over several calls. */
#define INFLATE_HEAP_SIZE 100000

/* The words of the header of a compressed asset: its compressed and decompressed sizes. */
#define ASSET_HEADER_WORD 4

/* The retraces the counter of the vertical blank counts before it wraps. */
#define VBLANK_COUNTER_MASK 15

/* The CPU counter counts 46875 times a millisecond. */
#define COUNTS_PER_MILLISECOND 46875.0f

/* The fixed step of a race, 1/30 of a second, in nanoseconds. */
#define RACE_STEP_NANOSECONDS 33333333

/* The priorities of the threads. */
#define RSP_DONE_THREAD_ID 10
#define RDP_DONE_THREAD_ID 11
#define EVENT_THREAD_PRIORITY 60
#define FAULT_THREAD_ID 5
#define FAULT_THREAD_PRIORITY 50
#define MAIN_THREAD_ID 3
#define MAIN_THREAD_PRIORITY 10
#define IDLE_THREAD_ID 1
#define IDLE_THREAD_PRIORITY 10

/* The message the fault event sends. */
#define FAULT_MESSAGE 0x10

/* The block of arguments a development kit leaves in the cartridge space, 64 bytes. */
#define BOOT_ARGUMENTS_ADDRESS 0xFFB000
#define BOOT_ARGUMENT_WORDS 16

/* The two words the stacks are filled with, to show how deep they were used. */
#define STACK_FILL_HI 0x5015A1DB
#define STACK_FILL_LO 0xFED15C00

/* The screen the boot sets up. */
#define BOOT_SCREEN_WIDTH 320
#define BOOT_SCREEN_HEIGHT 240

OSIoMesg *AllocIoMessage(void) {
    if (g_dwIoMessagePendingCount == IO_MESSAGES) {
        /* With every message in flight, one finished DMA frees its message. */
        osRecvMesg(&g_DmaMessageQueue, NULL, OS_MESG_BLOCK);
    } else {
        g_dwIoMessagePendingCount++;
    }
    g_dwIoMessagePoolIndex = (g_dwIoMessagePoolIndex + 1) % IO_MESSAGES;
    return &g_aIoMessagePool[g_dwIoMessagePoolIndex];
}

void WaitForDmaMessages(void) {
    if (g_dwIoMessagePendingCount != 0) {
        do {
            osRecvMesg(&g_DmaMessageQueue, NULL, OS_MESG_BLOCK);
        } while (--g_dwIoMessagePendingCount != 0);
    }
}

s32 UnusedVblankQueryStub(s32 unused0, s32 unused1) {
    return 0;
}

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
        g_dwVblankCounter = (g_dwVblankCounter + 1) & VBLANK_COUNTER_MASK;
    }
}

/* Whether a controller holds a pak that may be a Rumble Pak. */
static s32 IsPakController(OSContStatus *status, u8 pattern, s32 i) {
    return ((pattern >> i) & 1) && !(status->errno & CONT_NO_RESPONSE_ERROR) &&
           (status->type & CONT_JOYPORT) && (status->status & CONT_CARD_ON);
}

void InitRenderingSubsystem(void) {
    OSMesgQueue queue;
    OSMesg message;
    u8 pattern;
    s32 result;
    s32 i;

    g_pDisplayListBase = g_aGfxDisplayListPool;
    g_apFramebuffers[0] = g_aFramebuffer0;
    g_apFramebuffers[1] = g_aFramebuffer1;
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
    g_fCountsPerMillisecond = COUNTS_PER_MILLISECOND;

    /* The serial event reaches the queue on the stack until it is pointed elsewhere below. */
    osCreateMesgQueue(&queue, &message, 1);
    osSetEventMesg(OS_EVENT_SI, &queue, (OSMesg)1);
    osContInit(&queue, &pattern, g_aContStatus);
    osCreateMesgQueue(&g_SiMesgQueue, g_aSiEventMesgBuf, 1);
    osSetEventMesg(OS_EVENT_SI, &g_SiMesgQueue, (OSMesg)0);
    PrepareRumblePaks();
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
}

void ResetFrameClock(void) {
    g_qwTimestampBase = 0;
    g_dwFrameTimestamp = 0;
    g_dwElapsedTimeMs = 0;
    g_dwPrevElapsedTimeMs = 0;
    g_dwFrameDeltaMs = 0;
    g_fFrameDeltaTime = 0.0f;
}

void UpdateFrameTiming(void) {
    u32 now;
    u32 delta;

    if (g_bRaceScreenActive) {
        /* A race steps a fixed 1/30 of a second a frame. */
        g_qwTimestampBase += (u64)RACE_STEP_NANOSECONDS * osClockRate / 1000000000;
        g_dwFrameTimestamp = osGetCount();
    } else if (g_qwTimestampBase != 0) {
        now = osGetCount();
        g_qwTimestampBase += (u32)(now - g_dwFrameTimestamp);
        g_dwFrameTimestamp = now;
    } else {
        now = osGetCount();
        g_qwTimestampBase = now;
        g_dwFrameTimestamp = now;
    }
    g_dwPrevElapsedTimeMs = g_dwElapsedTimeMs;
    g_dwElapsedTimeMs = g_qwTimestampBase * 1000000 / osClockRate / 1000;
    UpdateControllerState();
    DebugFrameStubA();
    DebugFrameStubB();
    delta = g_dwElapsedTimeMs - g_dwPrevElapsedTimeMs;
    g_fFrameDeltaTime = (f32)delta / 1000.0f;
    g_dwFrameDeltaMs = delta;
}

void UnusedFrameTimingStub(void) {
}

void StubRenderCallback(void) {
}

void SetNextScreenCallback(ScreenCallback callback) {
    g_pfnNextScreen = callback;
}

void SetSecondaryScreenCallback(ScreenCallback callback) {
    g_pfnSecondaryScreen = callback;
}

s32 IsCurrentScreen(ScreenCallback callback) {
    return callback == g_pfnCurrentScreen;
}

void SetCurrentScreenCallback(ScreenCallback callback) {
    g_pfnCurrentScreen = callback;
}

void CallCurrentGameMode(void) {
    g_pfnCurrentScreen();
}

void GameModeMainLoop(void) {
    for (;;) {
        CallCurrentGameMode();
    }
}

void StubUpdateCallback(void) {
}

void DmaRead(void *dst, u32 romOffset, s32 size) {
    u8 *bytes = dst;

    osInvalDCache(dst, size);
    while (size > DMA_BLOCK_SIZE) {
        osPiStartDma(AllocIoMessage(),
                     OS_MESG_PRI_NORMAL,
                     OS_READ,
                     romOffset,
                     bytes,
                     DMA_BLOCK_SIZE,
                     &g_DmaMessageQueue);
        size -= DMA_BLOCK_SIZE;
        romOffset += DMA_BLOCK_SIZE;
        bytes += DMA_BLOCK_SIZE;
    }
    osPiStartDma(
        AllocIoMessage(), OS_MESG_PRI_NORMAL, OS_READ, romOffset, bytes, size, &g_DmaMessageQueue);
    WaitForDmaMessages();
}

u32 DmaReadAssetSize(u32 romOffset) {
    DmaRead(&g_dwDmaAssetSizeBuffer, romOffset + ASSET_HEADER_WORD, sizeof(g_dwDmaAssetSizeBuffer));
    return g_dwDmaAssetSizeBuffer;
}

u32 DmaReadWord(u32 romOffset) {
    DmaRead(&g_dwDmaReadWordBuffer, romOffset, sizeof(g_dwDmaReadWordBuffer));
    return g_dwDmaReadWordBuffer;
}

void InitDmaRequest(DecompressJob *job, u8 *buffer) {
    job->dst = NULL;
    job->buffer = buffer;
}

/* The size of a chunk on the cartridge: its length word, then its bytes, padded to an even count.
 */
#define CHUNK_SPAN(size) (((size) + 5) & ~1)

u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
    u8 *out = dst;
    u8 *buffer;
    u32 compressedSize;
    u32 decompressedSize;
    u32 remaining;
    u32 half;
    u32 chunkSize;
    uLong outputSize;
    u32 start;
    s32 first;

    if (job != NULL && job->dst != NULL) {
        romOffset = job->romOffset;
        out = job->dst;
        compressedSize = job->compressedSize;
        remaining = job->remaining;
        decompressedSize = job->decompressedSize;
        buffer = job->buffer;
        half = job->bufferHalf;
        first = FALSE;
        g_dwDmaChunkSize = job->pendingChunkSize;
    } else {
        DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
        compressedSize = g_dwDmaChunkSize;
        remaining = g_dwDmaChunkSize;
        romOffset += ASSET_HEADER_WORD;
        DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
        decompressedSize = g_dwDmaChunkSize;
        romOffset += ASSET_HEADER_WORD;
        remaining -= 2 * ASSET_HEADER_WORD;
        buffer = job != NULL ? job->buffer : g_aDmaWorkBuffers;
        half = 0;
        first = TRUE;
        g_dwDmaChunkSize = 0;
    }
    start = osGetCount();

    /* While one half of the buffer is read, the chunk in the other is decompressed. */
    for (;;) {
        chunkSize = g_dwDmaChunkSize;
        if (CHUNK_SPAN(g_dwDmaChunkSize) < remaining) {
            if (romOffset & 1) {
                romOffset++;
            }
            DmaRead(&g_dwDmaChunkSize, romOffset, sizeof(g_dwDmaChunkSize));
            romOffset += ASSET_HEADER_WORD;
            osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
            osPiStartDma(AllocIoMessage(),
                         OS_MESG_PRI_NORMAL,
                         OS_READ,
                         romOffset,
                         &buffer[half],
                         (g_dwDmaChunkSize + 1) & ~1,
                         &g_DmaMessageQueue);
            romOffset += (g_dwDmaChunkSize + 1) & ~1;
        } else {
            WaitForDmaMessages();
        }
        osInvalDCache(&buffer[half], CHUNK_BUFFER_SIZE);
        half ^= CHUNK_BUFFER_SIZE;
        if (first) {
            first = FALSE;
        } else {
            outputSize = CHUNK_OUTPUT_SIZE;
            if (job != NULL) {
                g_pInflateHeapBase = g_aInflateHeapBuffer;
                g_pInflateHeapEnd = &g_aInflateHeapBuffer[INFLATE_HEAP_SIZE];
            }
            uncompress(out, &outputSize, &buffer[half], chunkSize);
            if (job != NULL) {
                g_pInflateHeapEnd = NULL;
                g_pInflateHeapBase = NULL;
            }
            remaining -= CHUNK_SPAN(chunkSize);
            out += outputSize;
            if (remaining == 0) {
                break;
            }
        }
        /* A decompression over several calls takes one step a call. */
        if (job != NULL) {
            break;
        }
    }
    /* The time it took, in microseconds, is worked out and not kept. */
    (void)((u64)(osGetCount() - start) * 1000000 / osClockRate);

    if (job != NULL) {
        job->romOffset = romOffset;
        job->remaining = remaining;
        job->dst = out;
        job->compressedSize = compressedSize;
        job->bufferHalf = half;
        job->decompressedSize = decompressedSize;
        job->buffer = buffer;
        job->pendingChunkSize = g_dwDmaChunkSize;
    }
    return decompressedSize;
}

void WaitForVblankMessage(void) {
    osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
}

void StubFunction(s32 unused) {
}

void SetDebugMessage(const char *message) {
    g_pszDebugMessage = message;
    StubFunction(0);
}

void FaultThread(void *arg) {
    OSMesg message;

    osSetEventMesg(OS_EVENT_FAULT, &g_FaultMsgQueue, (OSMesg)FAULT_MESSAGE);
    g_dwFaultState = 0;
    for (;;) {
        osRecvMesg(&g_FaultMsgQueue, &message, OS_MESG_BLOCK);
        osSyncPrintf("\n=> faultproc - got a fault message...\n");
        g_pFaultedThread = __osGetCurrFaultedThread();
        if (g_pFaultedThread != NULL) {
            StubFunction((s32)g_pFaultedThread);
        }
    }
}

/* Split the arguments a development kit left, at the spaces, into g_apszBootArgv. */
static void ParseBootArguments(char *text) {
    char *p = text;
    char c;

    if (*p == '\0') {
        return;
    }
    c = *p;
    do {
        while (c != '\0' && c == ' ') {
            c = p[1];
            *p++ = '\0';
        }
        if (c != '\0') {
            /* There is no limit on the count. */
            g_apszBootArgv[g_dwBootArgc++] = p;
            c = *p;
        }
        while (c != '\0' && c != ' ') {
            c = p[1];
            p++;
        }
    } while (c != '\0');
}

/* Fill a stack with a pattern that shows how deep it was used. */
static void FillStack(u32 *stack, s32 words) {
    s32 i;

    for (i = 0; i < words; i += 2) {
        stack[i] = STACK_FILL_HI;
        stack[i + 1] = STACK_FILL_LO;
    }
}

void BootIdleThread(void *arg) {
    u32 arguments[BOOT_ARGUMENT_WORDS];
    s32 i;

    for (i = 0; i < BOOT_ARGUMENT_WORDS; i++) {
        osPiRawReadIo(BOOT_ARGUMENTS_ADDRESS + i * sizeof(u32), &arguments[i]);
    }
    /* The words are read as text. */
    ParseBootArguments((char *)arguments);
    g_ppszMainArgv = g_ppszBootArgv;
    g_pdwMainArgc = &g_dwBootArgc;

    osCreateViManager(OS_PRIORITY_VIMGR);
    g_dwScreenWidth = BOOT_SCREEN_WIDTH;
    g_dwScreenHeight = BOOT_SCREEN_HEIGHT;
    if (osTvType == OS_TV_MPAL) {
        osViSetMode(&g_ViModeMpalLan1);
    } else {
        osViSetMode(&g_ViModeNtscLan1);
    }
    osViBlack(TRUE);
    osCreatePiManager(OS_PRIORITY_PIMGR, &g_PiMgrCmdQueue, g_aPiMgrCmdMsgBuf, IO_MESSAGES);
    osCreateMesgQueue(&g_FaultMsgQueue, g_aFaultMsgBuf, 1);
    osCreateThread(&g_FaultThreadObj,
                   FAULT_THREAD_ID,
                   FaultThread,
                   g_ppszBootArgv,
                   &g_adwFaultThreadStack[FAULT_STACK_WORDS],
                   FAULT_THREAD_PRIORITY);
    osStartThread(&g_FaultThreadObj);

    FillStack(g_adwMainThreadStack, MAIN_STACK_WORDS);
    FillStack(g_adwRspDoneThreadStack, THREAD_STACK_WORDS);
    FillStack(g_adwRdpDoneThreadStack, THREAD_STACK_WORDS);
    osCreateThread(&g_MainGameThreadObj,
                   MAIN_THREAD_ID,
                   InitGameAndShowMenus,
                   arg,
                   &g_adwMainThreadStack[MAIN_STACK_WORDS],
                   MAIN_THREAD_PRIORITY);
    osStartThread(&g_MainGameThreadObj);
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);
    for (;;) {
    }
}

void OsBootEntry(void) {
    osInitialize();
    osCreateThread(&g_BootIdleThreadObj,
                   IDLE_THREAD_ID,
                   BootIdleThread,
                   NULL,
                   &g_adwIdleThreadStack[IDLE_STACK_WORDS],
                   IDLE_THREAD_PRIORITY);
    osStartThread(&g_BootIdleThreadObj);
}

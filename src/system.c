#include "system.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "camera.h"
#include "debugtext.h"
#include "fade.h"
#include "game.h"
#include "input.h"
#include "libultra/os/osint.h"
#include "platform/n64/boot.h"
#include "race.h"
#include "render.h"
#include "rumble.h"
#include "texrect.h"
#include "text.h"
#include "timing.h"
#include "track.h"
#include "trackrender.h"
#include "trackselect.h"
#include "util.h"
#include "vecmath.h"
#include "vehicle.h"
#include "weather.h"
#include "zlib.h"

/* The messages of the DMA that may wait at once. */
#define IO_MESSAGES 32

/* Each half of a buffer of compressed data, and the room the decompression of one gets. */
#define CHUNK_BUFFER_SIZE 16000
#define CHUNK_OUTPUT_SIZE 16000

/* The heap lent to zlib by a decompression that runs over several calls. */
#define INFLATE_HEAP_SIZE 100000

/* The words of the header of a compressed asset: its compressed and decompressed sizes. */
#define ASSET_HEADER_WORD 4

/* The priorities of the threads. */
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

/* The full-height video modes: the game's copies in the USA release, the SDK's in the others. */
#if TGR_REGION == TGR_REGION_US
#define VI_MODE_LAN1_MPAL g_ViModeMpalLan1
#define VI_MODE_LAN1 g_ViModeNtscLan1
#elif TGR_REGION == TGR_REGION_EU
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModePalLan1
#else
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModeNtscLan1
#endif

/* Whether the European and Japanese releases may show the screen: it is not held blank, no delay
 * before showing it runs, and no reset is pending. */
#define SCREEN_MAY_SHOW()                                                                          \
    (!g_dwKeepScreenBlanked && !g_dwScreenUnblankDelay && !g_dwPreNmiResetPending)

/* The European release adjusts its copies of the PAL modes: the vertical start of each field
 * (the bits outside the mask kept) and the vertical scale of the half-height and full-height
 * modes. */
#define PAL_VSTART_KEEP 0xFC00FC00
#define PAL_VSTART_FIELD0 0x0039025B
#define PAL_VSTART_FIELD1 0x003B025D
#define PAL_HAF1_YSCALE 0x6F0
#define PAL_LAN1_YSCALE 0x378

/* The memory the European and Japanese releases clear at boot, below the code of the game. */
#define BOOT_CLEAR_START 0x800D4000
#define BOOT_CLEAR_END 0x80200000

#define ARRAY_COUNT(array) (s32)(sizeof(array) / sizeof((array)[0]))

/* The address the RSP and the RDP read memory of KSEG0 at. */
#ifdef __GNUC__
#define TO_RSP_ADDRESS(p) ((u32)(p) - K0BASE)
#else
/* IDO folds the base into the relocation of a datum only through pointer arithmetic. */
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))
#endif

/* A fill of the depth buffer with the farthest depth, as two depths. */
#define FARTHEST_DEPTHS ((u32)GPACK_ZDZ(G_MAXFIXED, 0) << 16 | GPACK_ZDZ(G_MAXFIXED, 0))

void StubEmpty(void) {
}

void ClearDepthBuffer(void) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_aDepthBuffer));
    gDPSetFillColor(gDisplayListPtr++, FARTHEST_DEPTHS);
    gDPFillRectangle(gDisplayListPtr++,
                     0,
                     0,
                     (g_dwScreenWidth << g_dwScreenScaleShift) - 1,
                     (g_dwScreenHeight << g_dwScreenScaleShift) - 1);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));
}

void ClearZBufferRect(s32 x, s32 y, s32 width, s32 height) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_aDepthBuffer));
    gDPSetFillColor(gDisplayListPtr++, FARTHEST_DEPTHS);
    /* The bottom edge is taken from x, not y. */
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width - 1, x + height - 1);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorImage(gDisplayListPtr++,
                     G_IM_FMT_RGBA,
                     G_IM_SIZ_16b,
                     g_dwScreenWidth << g_dwScreenScaleShift,
                     TO_RSP_ADDRESS(g_apFramebuffers[g_dwFrameBufferIndex]));
}

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void BlankDisplay(void) {
#if TGR_REGION == TGR_REGION_EU
    if (!g_dwDisplayBlanked) {
        if (g_dwAppliedScreenScaleShift) {
            osViSetMode(&osViModePalHaf1);
        } else {
            osViSetMode(&osViModePalLan1);
        }
    }
#endif
    osViBlack(TRUE);
    g_dwDisplayBlanked = TRUE;
}

void UnblankDisplay(void) {
    osViBlack(FALSE);
#if TGR_REGION == TGR_REGION_EU
    if (g_dwDisplayBlanked) {
        if (g_dwAppliedScreenScaleShift) {
            osViSetMode(&g_ViModePalHaf1);
        } else {
            osViSetMode(&g_ViModePalLan1);
        }
    }
#endif
    g_dwDisplayBlanked = FALSE;
}
#endif

void ClearFramebuffersHighRes(void) {
#if TGR_REGION == TGR_REGION_US
    g_dwKeepScreenBlanked = TRUE;
    osViBlack(TRUE);
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    osViBlack(TRUE);
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#else
    u32 *word;
    u32 *end;

    g_dwKeepScreenBlanked = TRUE;
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    /* The high resolution frame buffers lie from the end of the interface heap to the program. */
    word = (u32 *)0x800D4000;
    end = (u32 *)0x80200000;
    for (; word < end; word++) {
        *word = 0;
    }
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#endif
}

void ClearFramebuffersLowRes(void) {
#if TGR_REGION == TGR_REGION_US
    g_dwKeepScreenBlanked = TRUE;
    osViBlack(TRUE);
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    osViBlack(TRUE);
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#else
    u32 *word;
    u32 *end;

    g_dwKeepScreenBlanked = TRUE;
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    StartHighResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    /* The two low resolution frame buffers end where the program starts. */
    word = (u32 *)0x801B5000;
    end = (u32 *)0x80200000;
    for (; word < end; word++) {
        *word = 0;
    }
    StartLowResFrame();
    ClearScreen(0, 0, 0);
    FlushDisplayList();
    g_dwKeepScreenBlanked = FALSE;
#endif
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

void UnusedFrameTimingStub(void) {
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
    start = osGetCount() - start;
    (void)((u64)start * 1000000 / osClockRate);

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

void BootIdleThread(void *arg) {
    u32 arguments[BOOT_ARGUMENT_WORDS];
    s32 i;
    char *p;
    char c;
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    u32 *clear;
#endif

    for (i = 0; i < BOOT_ARGUMENT_WORDS; i++) {
        osPiRawReadIo(BOOT_ARGUMENTS_ADDRESS + i * sizeof(u32), &arguments[i]);
    }

    /* The words are read as text, split at the spaces into g_apszBootArgv. */
    p = (char *)arguments;
    if (*p != '\0') {
        c = *p;
        do {
            while (c != '\0' && c == ' ') {
                c = p[1];
                *p++ = '\0';
            }
            if (c != '\0') {
                /* There is no limit on the count. The original reads the count again to raise
                 * it. */
                g_apszBootArgv[g_dwBootArgc] = p;
                g_dwBootArgc++;
                c = *p;
            }
            while (c != '\0' && c != ' ') {
                c = p[1];
                p++;
            }
        } while (c != '\0');
    }
    g_ppszMainArgv = g_ppszBootArgv;
    g_pdwMainArgc = &g_dwBootArgc;

    osCreateViManager(OS_PRIORITY_VIMGR);
    g_dwScreenWidth = BOOT_SCREEN_WIDTH;
    g_dwScreenHeight = BOOT_SCREEN_HEIGHT;
#if TGR_REGION == TGR_REGION_EU
    memcpy(&g_ViModePalHaf1, &osViModePalHaf1, sizeof(OSViMode));
    memcpy(&g_ViModePalLan1, &osViModePalLan1, sizeof(OSViMode));
    g_ViModePalHaf1.fldRegs[0].vStart =
        (osViModePalHaf1.fldRegs[0].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD0;
    g_ViModePalHaf1.fldRegs[1].vStart =
        (osViModePalHaf1.fldRegs[1].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD1;
    g_ViModePalHaf1.fldRegs[0].yScale = PAL_HAF1_YSCALE;
    g_ViModePalHaf1.fldRegs[1].yScale = PAL_HAF1_YSCALE;
    g_ViModePalLan1.fldRegs[0].vStart =
        (osViModePalLan1.fldRegs[0].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD0;
    g_ViModePalLan1.fldRegs[1].vStart =
        (osViModePalLan1.fldRegs[1].vStart & PAL_VSTART_KEEP) | PAL_VSTART_FIELD1;
    g_ViModePalLan1.fldRegs[0].yScale = PAL_LAN1_YSCALE;
    g_ViModePalLan1.fldRegs[1].yScale = PAL_LAN1_YSCALE;
#endif
    if (osTvType == OS_TV_MPAL) {
        osViSetMode(&VI_MODE_LAN1_MPAL);
    } else {
        osViSetMode(&VI_MODE_LAN1);
    }
    osViBlack(TRUE);
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    g_dwDisplayBlanked = TRUE;
    for (clear = (u32 *)BOOT_CLEAR_START; clear < (u32 *)BOOT_CLEAR_END; clear++) {
        *clear = 0;
    }
#endif
    osCreatePiManager(OS_PRIORITY_PIMGR, &g_PiMgrCmdQueue, g_aPiMgrCmdMsgBuf, IO_MESSAGES);
    osCreateMesgQueue(&g_FaultMsgQueue, g_aFaultMsgBuf, 1);
    osCreateThread(&g_FaultThreadObj,
                   FAULT_THREAD_ID,
                   FaultThread,
                   g_ppszBootArgv,
                   &g_adwFaultThreadStack[FAULT_STACK_WORDS],
                   FAULT_THREAD_PRIORITY);
    osStartThread(&g_FaultThreadObj);

    /* Fill the stacks with a pattern that shows how deep they were used. */
    for (i = 0; i < MAIN_STACK_WORDS / 2; i++) {
        g_adwMainThreadStack[i * 2] = STACK_FILL_HI;
        g_adwMainThreadStack[i * 2 + 1] = STACK_FILL_LO;
    }
    for (i = 0; i < THREAD_STACK_WORDS / 2; i++) {
        g_adwRspDoneThreadStack[i * 2] = STACK_FILL_HI;
        g_adwRspDoneThreadStack[i * 2 + 1] = STACK_FILL_LO;
    }
    for (i = 0; i < THREAD_STACK_WORDS / 2; i++) {
        g_adwRdpDoneThreadStack[i * 2] = STACK_FILL_HI;
        g_adwRdpDoneThreadStack[i * 2 + 1] = STACK_FILL_LO;
    }
    osCreateThread(&g_MainGameThreadObj,
                   MAIN_THREAD_ID,
                   (void (*)(void *))InitGameAndShowMenus,
                   arg,
                   &g_adwMainThreadStack[MAIN_STACK_WORDS],
                   MAIN_THREAD_PRIORITY);
    osStartThread(&g_MainGameThreadObj);
    osSetThreadPri(NULL, OS_PRIORITY_IDLE);
    for (;;) {
    }
}

void OsBootEntry(void) {
#if TGR_REGION == TGR_REGION_EU
    /* The PAL release forces the television standard before osInitialize() reads it. */
    osTvType = OS_TV_PAL;
    osViClock = VI_PAL_CLOCK;
#endif
    osInitialize();
    osCreateThread(&g_BootIdleThreadObj,
                   IDLE_THREAD_ID,
                   BootIdleThread,
                   NULL,
                   &g_adwIdleThreadStack[IDLE_STACK_WORDS],
                   IDLE_THREAD_PRIORITY);
    osStartThread(&g_BootIdleThreadObj);
}

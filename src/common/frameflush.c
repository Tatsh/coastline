#include "common/debugmessage.h"
#include "common/input.h"
#include "common/render.h"
#include "common/shadowbuffers.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/util.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "common/text.h"
#endif

#if TGR_BOSS_RALLY
#include "common/renderbackend.h"
#else
#include "common/libultra/os/osint.h"
#endif

#if TGR_BOSS_RALLY
/* The Windows task stubs retain these legacy RSP addresses without dereferencing them. */
/* @ghidraAddress BRD3D: 0x100aa728 */
static u64 *g_pRdpOutputBuffer = (u64 *)0x803b1000;
/* @ghidraAddress BRD3D: 0x100aa72c */
static u64 *g_pRdpOutputBufferEnd = (u64 *)0x803c8000;
/**
 * The commands of the display list of the last frame.
 *
 * @ghidraAddress USA: 0x8028ab70
 * @ghidraAddress EUR: 0x8028ce1c
 * @ghidraAddress JPN: 0x8028c84c
 * @ghidraAddress BRD3D: 0x106c1170
 * @ghidraAddress BRGlide: 0x106e8200
 */
[[maybe_unused, gnu::used]] static s32 g_dwDisplayListCommandCount;
/**
 * Whether a frame has been sent to the RSP.
 *
 * @ghidraAddress USA: 0x8028ab80
 * @ghidraAddress EUR: 0x8028ce2c
 * @ghidraAddress JPN: 0x8028c85c
 * @ghidraAddress BRD3D: 0x106c6664
 * @ghidraAddress BRGlide: 0x106ed6f4
 */
static s32 g_dwFirstFlushDone;
/**
 * The counts of the CPU counter the end of a frame took, between the ends of two frames, and at
 * the end of the last frame.
 *
 * @ghidraAddress USA: 0x8028aae8
 * @ghidraAddress EUR: 0x8028ccf0
 * @ghidraAddress JPN: 0x8028c7c0
 * @ghidraAddress BRD3D: 0x106c020c
 * @ghidraAddress BRGlide: 0x106e729c
 */
static u32 g_dwFlushDuration;
/**
 * @ghidraAddress USA: 0x8028aae0
 * @ghidraAddress EUR: 0x8028cce8
 * @ghidraAddress JPN: 0x8028c7b8
 * @ghidraAddress BRD3D: 0x106c1620
 * @ghidraAddress BRGlide: 0x106e86b0
 */
[[maybe_unused, gnu::used]] static u32 g_dwFlushInterval;
/**
 * @ghidraAddress USA: 0x8028aae4
 * @ghidraAddress EUR: 0x8028ccec
 * @ghidraAddress JPN: 0x8028c7bc
 * @ghidraAddress BRD3D: 0x106c0208
 * @ghidraAddress BRGlide: 0x106e7298
 */
static u32 g_dwFlushStartTime;
/**
 * @ghidraAddress USA: 0x8028aa24
 * @ghidraAddress EUR: 0x8028cc2c
 * @ghidraAddress JPN: 0x8028c6fc
 * @ghidraAddress BRD3D: 0x106c56e4
 * @ghidraAddress BRGlide: 0x106ec774
 */
[[maybe_unused, gnu::used]] static u32 g_dwLastFlushValue;
/**
 * @ghidraAddress USA: 0x8028ab7c
 * @ghidraAddress EUR: 0x8028ce28
 * @ghidraAddress JPN: 0x8028c858
 * @ghidraAddress BRD3D: 0x106c6660
 * @ghidraAddress BRGlide: 0x106ed6f0
 */
static s32 g_dwPeakDisplayListCommands;
/**
 * The most commands of a shadow, vertices of a shadow, and commands of a frame seen.
 *
 * @ghidraAddress USA: 0x8028ab74
 * @ghidraAddress EUR: 0x8028ce20
 * @ghidraAddress JPN: 0x8028c850
 * @ghidraAddress BRD3D: 0x106c6658
 * @ghidraAddress BRGlide: 0x106ed6e8
 */
static s32 g_dwPeakShadowCommandCount;
/**
 * @ghidraAddress USA: 0x8028ab78
 * @ghidraAddress EUR: 0x8028ce24
 * @ghidraAddress JPN: 0x8028c854
 * @ghidraAddress BRD3D: 0x106c665c
 * @ghidraAddress BRGlide: 0x106ed6ec
 */
static s32 g_dwPeakShadowVertexCount;
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x106c65e8 */
s32 g_dwAppliedScreenScaleShift;
/** @ghidraAddress BRD3D: 0x106c65e0 */
s32 g_dwDisplayModeChangeCountdown;
/** @ghidraAddress BRD3D: 0x106c6598 */
u32 g_dwFlushValueSource;
/**
 * @ghidraAddress BRD3D: 0x106c662c
 * @ghidraAddress BRGlide: 0x106ed6bc
 */
s32 g_dwHudDebugLevel;
/** @ghidraAddress BRD3D: 0x106c65f4 */
u32 g_dwKeepScreenBlanked;
/** @ghidraAddress BRD3D: 0x106c6668 */
s32 g_dwOutputBuffReserve;
/** @ghidraAddress BRD3D: 0x106c65f8 */
u32 g_dwScreenUnblankDelay;
/** @ghidraAddress BRD3D: 0x1067d570 */
s32 g_dwTimingFrame;
/** @ghidraAddress BRD3D: 0x1067d578 */
u32 g_dwTimingFrameStart;
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
#define FRAME_WRITEBACK() NullStub()
#define FRAME_TIMING_MARKER(bar, red, green, blue, alpha) NullStub()
/* Windows maps queue waits to the controller stub that returns 1. */
#define FRAME_WAIT_QUEUE(queue) osContInit(queue, NULL, NULL)
#define FRAME_DECREMENT(value) ((value) = (s32)((u32)(value) - 1u))
#else
#define FRAME_WRITEBACK() osWritebackDCacheAll()
#define FRAME_TIMING_MARKER(bar, red, green, blue, alpha)                                          \
    AddTimingMarker(bar, red, green, blue, alpha)
#define FRAME_WAIT_QUEUE(queue) osRecvMesg(queue, NULL, OS_MESG_BLOCK)
#define FRAME_DECREMENT(value) (--(value))
#endif /* TGR_BOSS_RALLY */

#if TGR_REGION == TGR_REGION_US
#define VI_MODE_LAN1_MPAL g_ViModeMpalLan1
#define VI_MODE_LAN1 g_ViModeNtscLan1
#elif TGR_REGION == TGR_REGION_EU
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModePalLan1
#else
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModeNtscLan1
#endif /* TGR_REGION == TGR_REGION_US */

#define SCREEN_MAY_SHOW()                                                                          \
    (!g_dwKeepScreenBlanked && !g_dwScreenUnblankDelay && !g_dwPreNmiResetPending)

#define DISPLAY_LIST_START()                                                                       \
    (g_pDisplayListBase + g_dwFrameBufferIndex * DISPLAY_LIST_COMMANDS + DISPLAY_LIST_RESERVED)

#if TGR_BOSS_RALLY
/* Windows records these addresses in task metadata without reading microcode. */
/* @ghidraAddress BRD3D: 0x118ab150 */
static u64 g_aGspUcodeText[1];
/* @ghidraAddress BRD3D: 0x118ab160 */
static u64 g_abRspUcodeData[1];
#endif

void FlushDisplayList(void) {
    OSTask *task;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    u16 *framebuffer;
    u32 *pairsOut, *pairsIn;
    s32 pairs, i;
#endif
    s32 commands, count;

    gDPFullSync(gDisplayListPtr++);
    gSPEndDisplayList(gDisplayListPtr++);
    task = &g_aGfxTask[g_dwFrameBufferIndex];
    task->t.type = M_GFXTASK;
    task->t.flags = OS_TASK_DP_WAIT | OS_TASK_LOADABLE;
    task->t.ucode = g_aGspUcodeText;
    task->t.ucode_data = g_abRspUcodeData;
    task->t.output_buff = g_pRdpOutputBuffer;
    task->t.ucode_size = SP_UCODE_SIZE;
    task->t.output_buff_size = g_pRdpOutputBufferEnd - g_dwOutputBuffReserve;
    task->t.ucode_data_size = SP_UCODE_DATA_SIZE;
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    task->t.dram_stack = (u64 *)(((uintptr_t)g_abRspDramStackBuffer + 15) & ~(uintptr_t)15);
#else
    task->t.dram_stack = (u64 *)(((u32)g_abRspDramStackBuffer + 15) & ~15);
#endif
    task->t.dram_stack_size = SP_DRAM_STACK_SIZE8;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    task->t.ucode_boot = g_aRspBootUcode;
    task->t.ucode_boot_size = (g_aGspUcodeText - g_aRspBootUcode) * sizeof(u64);
    /* The original works out the start of the display list again for each use. */
    task->t.data_ptr = (u64 *)DISPLAY_LIST_START();
#else
    task->t.data_ptr = DISPLAY_LIST_START();
#endif
    task->t.data_size = (gDisplayListPtr - DISPLAY_LIST_START()) * sizeof(Gfx);
    commands = gDisplayListPtr - DISPLAY_LIST_START();
    if (g_dwPeakDisplayListCommands < commands) {
        g_dwPeakDisplayListCommands = commands;
    }
    count = g_pShadowDisplayListWritePtr - g_pShadowDisplayListStart;
    if (g_dwPeakShadowCommandCount < count) {
        g_dwPeakShadowCommandCount = count;
    }
    count = g_pShadowVertexWritePtr - g_pShadowVertexStart;
    if (g_dwPeakShadowVertexCount < count) {
        g_dwPeakShadowVertexCount = count;
    }
    g_dwDisplayListCommandCount = commands;
    if (commands > DISPLAY_LIST_COMMANDS) {
        SetDebugMessage("HUGE GLIST ERROR");
    }
    FRAME_WRITEBACK();
    if (g_dwFirstFlushDone) {
        FRAME_TIMING_MARKER(0, 0, 0, 0, 0xFF);
        FRAME_WAIT_QUEUE(&g_RspDoneMsgQueue);
        FRAME_TIMING_MARKER(0, 0xFF, 0xFF, 0, 0xFF);
        if (g_pfnNextScreen != NULL) {
            g_pfnNextScreen();
            g_pfnNextScreen = NULL;
        }
        FRAME_TIMING_MARKER(0, 0, 0, 0, 0xFF);
        FRAME_WAIT_QUEUE(&g_RdpDoneMsgQueue);
#if TGR_PLATFORM == TGR_PLATFORM_N64
        if (g_bShowDepthBuffer) {
            /* The depth buffer is copied over the frame about to show, two depths at a time. */
            pairs = ((g_dwScreenScaleShift ? 4 : 1) * g_dwScreenWidth * g_dwScreenHeight) >> 1;
            pairsOut = (u32 *)g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
            pairsIn = (u32 *)g_aDepthBuffer;
            for (i = 0; i < pairs; i++) {
                pairsOut[i] = pairsIn[i];
            }
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
        /* The test is of the second routine, but the call is of the next. The next is NULL here
         * unless a routine has set it since it was cleared. */
        if (g_pfnSecondaryScreen != NULL) {
            g_pfnNextScreen();
            g_pfnNextScreen = NULL;
        }
#if TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_JP
        LoadCachedGlyphTextures();
#endif
        FRAME_WRITEBACK();
        g_dwFlushDuration = osGetCount();
        g_dwFlushInterval = g_dwFlushDuration - g_dwFlushStartTime;
        if (g_dwDisplayModeChangeCountdown != 0 &&
            FRAME_DECREMENT(g_dwDisplayModeChangeCountdown) == 0) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
#if TGR_REGION == TGR_REGION_EU
            /* The European release changes the mode only while the screen may show. */
            if (g_dwScreenScaleShift) {
                if (SCREEN_MAY_SHOW()) {
                    osViSetMode(&g_ViModePalHaf1);
                }
            } else if (SCREEN_MAY_SHOW()) {
                osViSetMode(&g_ViModePalLan1);
            }
#else
            if (g_dwScreenScaleShift) {
                osViSetMode(osTvType == OS_TV_MPAL ? &g_ViModeMpalHaf1 : &g_ViModeNtscHaf1);
            } else {
                osViSetMode(osTvType == OS_TV_MPAL ? &VI_MODE_LAN1_MPAL : &VI_MODE_LAN1);
            }
#endif /* TGR_REGION == TGR_REGION_EU */
#if TGR_REGION == TGR_REGION_US
            osViBlack(TRUE);
#endif
#else
            NullStub();
            NullStub();
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
            g_dwAppliedScreenScaleShift = g_dwScreenScaleShift;
        }
        FRAME_TIMING_MARKER(1, 0x20, 0x20, 0x20, 0xFF);
        FRAME_TIMING_MARKER(2, 0x20, 0x20, 0x20, 0xFF);
        FRAME_TIMING_MARKER(0, 0x20, 0x20, 0x20, 0xFF);
#if TGR_PLATFORM == TGR_PLATFORM_N64
        framebuffer = g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
        osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
        osViSwapBuffer(framebuffer);
        osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
        if (osViGetCurrentFramebuffer() != framebuffer) {
            osRecvMesg(&g_ViRetraceMesgQueue, NULL, OS_MESG_BLOCK);
        }
#endif
#if TGR_PLATFORM == TGR_PLATFORM_N64
#if TGR_REGION == TGR_REGION_US
        if (!g_dwKeepScreenBlanked && !g_dwScreenUnblankDelay) {
            osViBlack(FALSE);
        }
#else
        if (SCREEN_MAY_SHOW()) {
            UnblankDisplay();
        }
#endif
#else
        if (!g_dwKeepScreenBlanked && !g_dwScreenUnblankDelay) {
            NullStub();
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
        if (g_dwScreenUnblankDelay) {
            FRAME_DECREMENT(g_dwScreenUnblankDelay);
        }
        BeginTimingFrame();
        g_dwFlushStartTime = osGetCount();
        g_dwFlushDuration = osGetCount() - g_dwFlushDuration;
    } else {
        g_dwFirstFlushDone++;
    }
    FRAME_TIMING_MARKER(0, 0xC8, 0, 0xC8, 0xFF);
    g_dwLastFlushValue = g_dwFlushValueSource;
    FRAME_TIMING_MARKER(2, 0xC8, 0, 0, 0xFF);
    FRAME_TIMING_MARKER(1, 0xC8, 0x64, 0, 0xFF);
#if TGR_BOSS_RALLY
    g_pfnRenderBackendSubmitDisplayList(task->t.data_ptr);
#else
    osSpTaskLoad(task);
    osSpTaskStartGo(task);
#endif
    g_dwFrameBufferIndex ^= 1;
}

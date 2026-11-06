#include "common/debugmessage.h"
#include "common/input.h"
#include "common/libultra/os/osint.h"
#include "common/render.h"
#include "common/shadowbuffers.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/util.h"

#define DISPLAY_LIST_START()                                                                       \
    (((DisplayListCommand(*)[DISPLAY_LIST_COMMANDS])g_pDisplayListBase)[g_dwFrameBufferIndex] +    \
     DISPLAY_LIST_RESERVED)
#if TGR_REGION == TGR_REGION_EU
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModePalLan1
#elif TGR_REGION == TGR_REGION_JP
#define VI_MODE_LAN1_MPAL osViModeMpalLan1
#define VI_MODE_LAN1 osViModeNtscLan1
#else
#define VI_MODE_LAN1_MPAL g_ViModeMpalLan1
#define VI_MODE_LAN1 g_ViModeNtscLan1
#endif

void FlushDisplayList(void) {
    static int firstFlushDone = 0;
    OSTask *t;
    int n;
    int m;
    int v;
    int mul;
    int *src;
    int *dst;
    int i;
    void *fb;

    gDPFullSync(gDisplayListPtr++);
    gSPEndDisplayList(gDisplayListPtr++);
    t = &g_aGfxTask[g_dwFrameBufferIndex];
    t->t.type = 1;
    t->t.ucode = g_aGspUcodeText;
    t->t.ucode_data = g_abRspUcodeData;
    t->t.flags = 6;
    t->t.output_buff = g_pRdpOutputBuffer[0];
    t->t.output_buff_size = g_pRdpOutputBuffer[1] - g_dwOutputBuffReserve;
    t->t.ucode_size = 0x1000;
    t->t.ucode_data_size = 0x800;
    t->t.dram_stack = (u64 *)(((unsigned int)g_abRspDramStackBuffer + 0xf) & ~0xf);
    t->t.dram_stack_size = 0x400;
    t->t.ucode_boot = g_aRspBootUcode;
    t->t.ucode_boot_size = (unsigned int)g_aRspBootUcodeEnd - (unsigned int)g_aRspBootUcode;
    t->t.data_ptr = (u64 *)DISPLAY_LIST_START();
    t->t.data_size = (gDisplayListPtr - (DISPLAY_LIST_START())) * sizeof(Gfx);
    do {
    } while (0);
    m = gDisplayListPtr - (DISPLAY_LIST_START());
    if (g_dwPeakDisplayListCommands < m) {
        g_dwPeakDisplayListCommands = m;
    }
    m = ((int *)g_pShadowDisplayListWritePtr - (int *)g_pShadowDisplayListStart) >> 1;
    if (g_dwPeakShadowCommandCount < m) {
        g_dwPeakShadowCommandCount = m;
    }
    m = g_pShadowVertexWritePtr - g_pShadowVertexStart;
    if (g_dwPeakShadowVertexCount < m) {
        g_dwPeakShadowVertexCount = m;
    }
    g_dwDisplayListCommandCount = gDisplayListPtr - (DISPLAY_LIST_START());
    if (g_dwDisplayListCommandCount > 6000) {
        SetDebugMessage("HUGE GLIST ERROR");
    }
    osWritebackDCacheAll();
    if (firstFlushDone != 0) {
        AddTimingMarker(0, 0, 0, 0, 0xff);
        osRecvMesg(&g_RspDoneMsgQueue, 0, 1);
        AddTimingMarker(0, 0xff, 0xff, 0, 0xff);
        if (g_pfnNextScreen != 0) {
            g_pfnNextScreen();
            g_pfnNextScreen = 0;
        }
        AddTimingMarker(0, 0, 0, 0, 0xff);
        osRecvMesg(&g_RdpDoneMsgQueue, 0, 1);
        if (g_bShowDepthBuffer != 0) {
            /* clang-format off */
            mul = g_dwScreenScaleShift != 0 ? 4 : 1; \
            n = mul * g_dwScreenWidth * g_dwScreenHeight >> 1;
            /* clang-format on */
            src = (int *)g_aDepthBuffer;
            dst = (int *)g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
            for (i = 0; i < n; i++) {
                dst[i] = src[i];
            }
        }
        if (g_pfnSecondaryScreen != 0) {
            g_pfnNextScreen();
            g_pfnNextScreen = 0;
        }
#if TGR_REGION == TGR_REGION_JP
        LoadCachedGlyphTextures();
#endif
        osWritebackDCacheAll();
        /* clang-format off */
        g_dwFlushDuration = osGetCount(); \
        g_dwFlushInterval = g_dwFlushDuration - g_dwFlushStartTime;
        /* clang-format on */
        if (g_dwDisplayModeChangeCountdown != 0) {
            if (--g_dwDisplayModeChangeCountdown == 0) {
#if TGR_REGION == TGR_REGION_EU
                if (g_dwScreenScaleShift != 0) {
                    if (g_dwKeepScreenBlanked == 0 && g_dwScreenUnblankDelay == 0 &&
                        g_dwPreNmiResetPending == 0) {
                        osViSetMode(&g_ViModePalHaf1);
                    }
                } else {
                    if (g_dwKeepScreenBlanked == 0 && g_dwScreenUnblankDelay == 0 &&
                        g_dwPreNmiResetPending == 0) {
                        osViSetMode(&g_ViModePalLan1);
                    }
                }
#else
                if (g_dwScreenScaleShift != 0) {
                    if (osTvType == 2) {
                        osViSetMode(&g_ViModeMpalHaf1);
                    } else {
                        osViSetMode(&g_ViModeNtscHaf1);
                    }
                } else {
                    if (osTvType == 2) {
                        osViSetMode(&VI_MODE_LAN1_MPAL);
                    } else {
                        osViSetMode(&VI_MODE_LAN1);
                    }
                }
#if TGR_REGION != TGR_REGION_JP
                osViBlack(1);
#endif
#endif
                g_dwAppliedScreenScaleShift = g_dwScreenScaleShift;
            }
        }
        AddTimingMarker(1, 0x20, 0x20, 0x20, 0xff);
        AddTimingMarker(2, 0x20, 0x20, 0x20, 0xff);
        AddTimingMarker(0, 0x20, 0x20, 0x20, 0xff);
        fb = (void *)g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
        osRecvMesg(&g_ViRetraceMesgQueue, 0, 1);
        osViSwapBuffer(fb);
        osRecvMesg(&g_ViRetraceMesgQueue, 0, 1);
        if (osViGetCurrentFramebuffer() != fb) {
            osRecvMesg(&g_ViRetraceMesgQueue, 0, 1);
        }
#if TGR_REGION == TGR_REGION_EU
        if (g_dwKeepScreenBlanked == 0 && g_dwScreenUnblankDelay == 0 &&
            g_dwPreNmiResetPending == 0) {
            UnblankDisplay();
        } else {
            /* the empty arm is the ROM's block layout */
        }
#elif TGR_REGION == TGR_REGION_JP
        if (g_dwKeepScreenBlanked == 0 && g_dwScreenUnblankDelay == 0 &&
            g_dwPreNmiResetPending == 0) {
            UnblankDisplay();
        } else {
            /* the empty arm is the ROM's block layout */
        }
#else
        if (g_dwKeepScreenBlanked == 0 && g_dwScreenUnblankDelay == 0) {
            osViBlack(0);
        }
#endif
        if (g_dwScreenUnblankDelay != 0) {
            g_dwScreenUnblankDelay--;
        }
        BeginTimingFrame();
        g_dwFlushStartTime = osGetCount();
        g_dwFlushDuration = osGetCount() - g_dwFlushDuration;
    } else {
        firstFlushDone++;
    }
    AddTimingMarker(0, 200, 0, 200, 0xff);
    g_dwLastFlushValue = g_dwFlushValueSource;
    AddTimingMarker(2, 200, 0, 0, 0xff);
    AddTimingMarker(1, 200, 100, 0, 0xff);
    osSpTaskLoad(t);
    osSpTaskStartGo(t);
    g_dwFrameBufferIndex ^= 1;
}

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os.h>

#include "common/render.h"
#include "common/system.h"
#include "common/util.h"

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
    {
        u32 *command = (u32 *)gDisplayListPtr++;
        command[0] = 0xff100000 | (((g_dwScreenWidth << g_dwScreenScaleShift) - 1) & 0xfff);
        command[1] = TO_RSP_ADDRESS(g_aDepthBuffer);
    }
    {
        u32 *command = (u32 *)gDisplayListPtr++;
        command[0] = 0xf7000000;
        command[1] = FARTHEST_DEPTHS;
    }
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
    {
        u32 *command = (u32 *)gDisplayListPtr++;
        command[0] = 0xff100000 | (((g_dwScreenWidth << g_dwScreenScaleShift) - 1) & 0xfff);
        command[1] = TO_RSP_ADDRESS(g_aDepthBuffer);
    }
    {
        u32 *command = (u32 *)gDisplayListPtr++;
        command[0] = 0xf7000000;
        command[1] = FARTHEST_DEPTHS;
    }
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
#endif /* TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU */

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
#endif /* TGR_REGION == TGR_REGION_US */
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
#endif /* TGR_REGION == TGR_REGION_US */
}

void UnusedFrameTimingStub(void) {
}

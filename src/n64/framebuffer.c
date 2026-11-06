#include <PR/os.h>

#include "common/render.h"
#include "common/system.h"

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
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
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
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
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

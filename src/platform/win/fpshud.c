#include <stdio.h>

#include <hud.h>

#include "platform/win/framestatistics.h"
#include "platform/win/hudcanvas.h"
#include "platform/win/settings.h"
#include "text.h"

#undef DrawText

#ifdef ENABLE_PATCHES
#define g_dwConfigFramebufferWidth GetHudCanvasWidth()
#define g_dwConfigFramebufferHeight GetHudCanvasHeight()
#endif

/** @ghidraAddress BRD3D: 0x100a6cf0 */
/* The original updates the game rate but draws only the renderer rate. */
[[maybe_unused, gnu::used]] static float g_fGameFrameRate = 30.0f;

/** @ghidraAddress BRD3D: 0x100a6cf4 */
static float g_fRenderFrameRate = 30.0f;

s32 g_bShowFpsCounter;

void DrawFpsCounterOverlay(void) {
    if (g_bShowFpsCounter == 0) {
        return;
    }
    if (g_dwGameFpsFrameCountdown == 0) {
        long double total = 0.0L;
        for (int index = 0; index < g_dwGameFpsSampleFrames; ++index) {
            total += (s32)g_adwGameFpsFrameTimeSamples[index];
        }
        g_fGameFrameRate = (float)(((long double)g_dwGameFpsSampleFrames * 1000.0f) / total);
    }
    if (g_dwFpsFrameCountdown == 0) {
        long double total = 0.0L;
        for (int index = 0; index < g_dwFpsSampleFrames; ++index) {
            total += (s32)g_adwFpsFrameTimeSamples[index];
        }
        g_fRenderFrameRate = (float)(((long double)g_dwFpsSampleFrames * 1000.0f) / total);
    }
    char text[256];
    /* Preserve the x87 promotion before formatting. */
    volatile long double rate = g_fRenderFrameRate;
    sprintf(text, "%6.2f FPS", (double)rate);
    SetTextSize(15);
    SetTextAlignCenter();
    DrawText(text, g_dwConfigFramebufferWidth / 2, (s32)((u32)g_dwConfigFramebufferHeight - 10));
}

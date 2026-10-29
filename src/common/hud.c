#include "common/hud.h"

#include <common/fade.h>
#include <common/render.h>
#include <common/text.h>

#include "common/frametime.h"

const char *g_pszDebugOverlayMessage;
float g_fDebugOverlayMessageTimer;

void DrawDebugOverlayBanner(const ViewportRect *viewports) {
    if (IsFadingOut()) {
        return;
    }
    s32 lineHeight = g_dwNumViewports == 1 ? 30 : 20;
    s32 size = g_dwNumViewports == 1 ? 20 : 15;
    const ViewportRect *viewport = &viewports[g_dwCurrentSplitScreenIndex];
    s32 x = (s32)((u32)(viewport->width / 2) + (u32)viewport->x);
    s32 y = (s32)((u32)viewport->y + (u32)(viewport->height / 3) + 24);

    SetTextAlignCenter();
    if (g_pszDebugOverlayMessage != NULL) {
        SetTextSize(size);
        DrawText(g_pszDebugOverlayMessage, x, (s32)((u32)y + lineHeight * 3 / 16));
    }
}

void TickDebugOverlayMessageTimer(void) {
    if (g_fDebugOverlayMessageTimer == 0.0f) {
        return;
    }
    g_fDebugOverlayMessageTimer -= g_fFrameDeltaTime;
    if (g_fDebugOverlayMessageTimer <= 0.0f) {
        g_fDebugOverlayMessageTimer = 0.0f;
        g_pszDebugOverlayMessage = NULL;
    }
}

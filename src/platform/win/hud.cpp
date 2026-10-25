#include "platform/win/hud.h"

#include "frametime.h"

const char *g_pszDebugOverlayMessage;
float g_fDebugOverlayMessageTimer;

void TickDebugOverlayMessageTimer(void) {
    if (g_fDebugOverlayMessageTimer == 0.0f) {
        return;
    }
    g_fDebugOverlayMessageTimer -= g_fFrameDeltaTime;
    if (g_fDebugOverlayMessageTimer <= 0.0f) {
        g_fDebugOverlayMessageTimer = 0.0f;
        g_pszDebugOverlayMessage = nullptr;
    }
}

#include <hud.h>

#include "camera.h"
#include "platform/win/settings.h"
#include "render.h"
#include "text.h"
#include "vehicle.h"

s32 g_bHudShowLapTimes = 1;

void DrawLapTimeListHud(const ViewportRect *viewports) {
    if (g_bHudShowLapTimes != 0) {
        s32 x = (s32)((u32)g_dwConfigFramebufferWidth - 16);
        s32 below = g_dwNumViewports == 1 ? 30 : 0;
        s32 y = (s32)((u32)viewports[g_dwCurrentSplitScreenIndex].y + 20);

        SetTextStyleNormal();
        SetTextAlignRight();
        SetTextSize(15);
        y = (s32)((u32)y + below + 37);
        for (s32 index = 0; index < g_pActiveCameraVehicle->lap; ++index) {
            DrawFormattedLapTimeText(
                "%ww", index + 1, g_pActiveCameraVehicle->lapTimes[index], x, y);
            y = (s32)((u32)y + 15);
        }
    }
}

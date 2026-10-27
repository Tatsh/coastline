#include "camera.h"
#include "fade.h"
#include "hud.h"
#include "pause.h"
#include "text.h"
#include "vehicleaccess.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define HUD_MESSAGE_VIEWPORTS viewports
#define HUD_MESSAGE_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
void DrawHudMessage(const ViewportRect *viewports) {
#else
#define HUD_MESSAGE_VIEWPORTS g_aViewportRects
#define HUD_MESSAGE_ADD(a, b) ((a) + (b))
void DrawHudMessage(void) {
#endif
    s32 x, top, large, small;

    if (g_dwRacePauseState != 0) {
        return;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (IsFadingOut()) {
        return;
    }
#endif
    if (g_dwNumViewports == 1) {
        large = 30;
        small = 20;
    } else {
        large = 20;
        small = 15;
    }
    x = HUD_MESSAGE_ADD(HUD_MESSAGE_VIEWPORTS[g_dwCurrentSplitScreenIndex].x,
                        HUD_MESSAGE_VIEWPORTS[g_dwCurrentSplitScreenIndex].width / 2);
    top = HUD_MESSAGE_ADD(HUD_MESSAGE_VIEWPORTS[g_dwCurrentSplitScreenIndex].y,
                          HUD_MESSAGE_VIEWPORTS[g_dwCurrentSplitScreenIndex].height / 3);
    SetTextAlignCenter();
    if (VEHICLE_GET(g_pActiveCameraVehicle, Message) != NULL) {
        SetTextSize(large);
        DrawText(VEHICLE_GET(g_pActiveCameraVehicle, Message), x, HUD_MESSAGE_ADD(large / 4, top));
    } else if (VEHICLE_GET(g_pActiveCameraVehicle, SubMessage) != NULL) {
        SetTextSize(small);
        DrawText(VEHICLE_GET(g_pActiveCameraVehicle, SubMessage),
                 x,
                 HUD_MESSAGE_ADD(large * 3 / 16, top));
    }
}

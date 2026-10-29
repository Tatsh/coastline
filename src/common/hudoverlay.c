#include "common/camera.h"
#include "common/hud.h"
#include "common/render.h"
#include "common/screendraw.h"
#include "common/text.h"
#include "common/vehicleaccess.h"

#if TGR_PC
#include <stdio.h>

#include "win32/configfile.h"
#include "win32/directplay.h"
#include "win32/hudcanvas.h"
#include "win32/settings.h"
#include "win32/strings.h"
#else
#include <PR/os_libc.h>

#include "common/input.h"
#include "options.h"
#endif /* TGR_PC */

#if TGR_PC

#undef DrawText
#define HUD_OVERLAY_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define HUD_SPEED_MPH g_bSpeedUnitsMph
#define HUD_OVERLAY_DRAW(function) function(viewports)
#define HUD_MPH_LABEL GetStringResource(0xeb)
#define HUD_KPH_LABEL GetStringResource(0xec)
#define HUD_SPEED_Y y
#else

#define HUD_OVERLAY_ADD(a, b) ((a) + (b))
#define HUD_SPEED_MPH g_OptionSettings.speedInMph
#define HUD_OVERLAY_DRAW(function) function()
#define HUD_MPH_LABEL "%wwmph"
#define HUD_KPH_LABEL "%wwkph"
#define HUD_SPEED_Y (y - 3)
#endif /* TGR_PC */

/* The right edge of the times, the speed and the dashboard. */
#if TGR_PC && defined(ENABLE_PATCHES)
#define g_dwConfigFramebufferWidth GetHudCanvasWidth()
#endif

#define HUD_RIGHT 296

/* The left edge of the speed when the dashboard shows. */
#define SPEED_DASHBOARD_RIGHT 266

/* The scissor of the display. */
#define HUD_SCISSOR_LEFT 8
#define HUD_SCISSOR_RIGHT 312

/* The gap below the dashboard and the speed. */
#define HUD_BOTTOM_MARGIN 4

/* The text sizes of the display. */
#define SPEED_TEXT_SIZE 20
#define SPEED_TEXT_SIZE_SPLIT 15
#define SPEED_UNIT_TEXT_SIZE 15

/* The kilometres in a mile. */
#define KM_PER_MILE 1.609344f

#if TGR_PC
void DrawSpeedAndHudOverlay(const ViewportRect *viewports, const Vehicle *vehicles) {
#else
void DrawSpeedAndHudOverlay(void) {
#endif
    f32 speed = VEHICLE_GET(g_pActiveCameraVehicle, SpeedKph);
    s32 x, y;

#if TGR_PC
    SetScissorRect(0,
                   viewports[g_dwCurrentSplitScreenIndex].y,
                   g_dwConfigFramebufferWidth,
                   viewports[g_dwCurrentSplitScreenIndex].height);
    /* The Windows comparison also clamps unordered speeds. */
    if (!(speed >= 0.0f)) {
#else
    SetScissorRect(HUD_SCISSOR_LEFT,
                   g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                   HUD_SCISSOR_RIGHT,
                   g_aViewportRects[g_dwCurrentSplitScreenIndex].height);
    if ((f64)speed < 0.0) {
#endif /* TGR_PC */
        speed = 0.0f;
    }
#if TGR_PLATFORM == TGR_PLATFORM_N64
    if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
#endif
        HUD_OVERLAY_DRAW(RenderDashboardGauges);
        HUD_OVERLAY_DRAW(DrawRaceTimerHud);
#if TGR_PLATFORM == TGR_PLATFORM_N64
    }
#else
    DrawLapTimeListHud(viewports);
#endif
#if TGR_PC
    DrawLapCounterHud(viewports, vehicles);
#else
    DrawLapCounterHud();
#endif
    HUD_OVERLAY_DRAW(DrawHudMessage);
    SetTextStyleNormal();
    SetTextAlignRight();
    if (HUD_SPEED_MPH) {
#if TGR_PC
        sprintf(
            g_szHudSpeedTextBuf, "%%yw%.0f", (double)((long double)speed * 0.6213712096214294f));
#else
        sprintf(g_szHudSpeedTextBuf, "%%yw%.0f", (f64)(speed / KM_PER_MILE));
#endif
    } else {
        sprintf(g_szHudSpeedTextBuf, "%%yw%.0f", (f64)speed);
    }

    /* Speed and its unit appear above the dashboard. */
#if TGR_PC
    x = HUD_OVERLAY_ADD(g_dwConfigFramebufferWidth, -16);
    y = HUD_OVERLAY_ADD(HUD_OVERLAY_ADD(viewports[g_dwCurrentSplitScreenIndex].y,
                                        viewports[g_dwCurrentSplitScreenIndex].height),
                        -4);
    if (g_bNetSpectator || !g_bHudShowSpeedometer) {
        return;
    }
    x = HUD_OVERLAY_ADD(x, -30);
    y = HUD_OVERLAY_ADD(
        y, -g_aVehicleModels[viewports[g_dwCurrentSplitScreenIndex].vehicleIndex].dashboardHeight);
#else
    x = HUD_RIGHT;
    y = g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
        g_aViewportRects[g_dwCurrentSplitScreenIndex].height - HUD_BOTTOM_MARGIN;
    if (g_dwNumViewports == 2) {
        if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
            y = y - g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height * 3 / 4 - 1;
        }
        SetTextSize(SPEED_TEXT_SIZE_SPLIT);
        DrawText(g_szHudSpeedTextBuf, x, y);
        return;
    }
    if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
        x = SPEED_DASHBOARD_RIGHT;
        y -= g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height;
    }
#endif /* TGR_PC */
    SetTextSize(SPEED_TEXT_SIZE);
#if TGR_PC
    y = HUD_OVERLAY_ADD(y, -3);
#endif
#if TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_JP
    /* The longer units of the Japanese release move the speed and the unit left. */
    if (HUD_SPEED_MPH) {
        DrawText(g_szHudSpeedTextBuf, x - 20, y - 3);
    } else {
        DrawText(g_szHudSpeedTextBuf, x - 11, y - 3);
    }
#else
    if (HUD_SPEED_MPH) {
        DrawText(g_szHudSpeedTextBuf, x, HUD_SPEED_Y);
    } else {
        DrawText(g_szHudSpeedTextBuf, HUD_OVERLAY_ADD(x, -3), HUD_SPEED_Y);
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_JP */
#if TGR_PLATFORM == TGR_PLATFORM_N64
    if (g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
        return;
    }
#endif
    SetTextSize(SPEED_UNIT_TEXT_SIZE);
    SetTextAlignLeft();
#if TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_JP
    if (HUD_SPEED_MPH) {
        DrawText("%wwMile/h", x - 20, y - 3);
    } else {
        DrawText("%wwKm/h", x - 11, y - 3);
    }
#else
    if (HUD_SPEED_MPH) {
        DrawText(HUD_MPH_LABEL, x, HUD_SPEED_Y);
    } else {
        DrawText(HUD_KPH_LABEL, HUD_OVERLAY_ADD(x, -3), HUD_SPEED_Y);
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_JP */
}

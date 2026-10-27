#include "hud.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "game.h"
#include "input.h"
#include "options.h"
#include "race.h"
#include "render.h"
#include "system.h"
#include "text.h"
#include "util.h"
#include "vecmath.h"
#include "vehicle.h"
#include "weather.h"

/* The right edge of the times, the speed and the dashboard. */
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

/* The kilometers in a mile. */
#define KM_PER_MILE 1.609344f

void StubDashboard(void) {
}

void DrawSpeedAndHudOverlay(void) {
    f32 speed = g_pActiveCameraVehicle->speedKph;
    s32 x, y;

    SetScissorRect(HUD_SCISSOR_LEFT,
                   g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                   HUD_SCISSOR_RIGHT,
                   g_aViewportRects[g_dwCurrentSplitScreenIndex].height);
    if ((f64)speed < 0.0) {
        speed = 0.0f;
    }
    if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
        RenderDashboardGauges();
        DrawRaceTimerHud();
    }
    DrawLapCounterHud();
    DrawHudMessage();
    SetTextStyleNormal();
    SetTextAlignRight();
    if (g_OptionSettings.speedInMph) {
        sprintf(g_szHudSpeedTextBuf, "%%yw%.0f", (f64)(speed / KM_PER_MILE));
    } else {
        sprintf(g_szHudSpeedTextBuf, "%%yw%.0f", (f64)speed);
    }

    /* The speed sits above the dashboard when it shows, and its unit beside it. */
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
    SetTextSize(SPEED_TEXT_SIZE);
#if TGR_REGION == TGR_REGION_JP
    /* The longer units of the Japanese release move the speed and the unit left. */
    if (g_OptionSettings.speedInMph) {
        DrawText(g_szHudSpeedTextBuf, x - 20, y - 3);
    } else {
        DrawText(g_szHudSpeedTextBuf, x - 11, y - 3);
    }
#else
    if (g_OptionSettings.speedInMph) {
        DrawText(g_szHudSpeedTextBuf, x, y - 3);
    } else {
        DrawText(g_szHudSpeedTextBuf, x - 3, y - 3);
    }
#endif
    if (g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
        return;
    }
    SetTextSize(SPEED_UNIT_TEXT_SIZE);
    SetTextAlignLeft();
#if TGR_REGION == TGR_REGION_JP
    if (g_OptionSettings.speedInMph) {
        DrawText("%wwMile/h", x - 20, y - 3);
    } else {
        DrawText("%wwKm/h", x - 11, y - 3);
    }
#else
    if (g_OptionSettings.speedInMph) {
        DrawText("%wwmph", x, y - 3);
    } else {
        DrawText("%wwkph", x - 3, y - 3);
    }
#endif
}

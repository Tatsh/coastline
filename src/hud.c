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

/* The palettes come first in the dashboard on the cartridge, one for day and one for night. */
#define DASHBOARD_PALETTE_BYTES (DASHBOARD_PALETTE_COLORS * 2)
#define DASHBOARD_PALETTES_BYTES (DASHBOARD_PALETTE_BYTES * 2)

/* The revolutions at which the needle reaches its full angle and the bar its end. */
#define TACHOMETER_FULL_RPM 8000.0f
#define TACHOMETER_FULL_RPM_D 8000.0

/* The half-widths of the tip and the base of the needle, in radians. */
#define NEEDLE_TIP_HALF 0.05f
#define NEEDLE_BASE_HALF 0.3f

/* The lengths of the tip and the base of the needle for one view and for two. */
#define NEEDLE_TIP_LENGTH 20.0f
#define NEEDLE_BASE_LENGTH 7.0f
#define NEEDLE_TIP_LENGTH_SPLIT 15.0f
#define NEEDLE_BASE_LENGTH_SPLIT 5.0f

/* The needle shakes by up to this many revolutions, half of it while paused. */
#define NEEDLE_JITTER_MASK 0x7F
#define NEEDLE_JITTER_PAUSED 0x40

/* The height of the low-resolution screen, which the needle's vertices count up from. */
#define SCREEN_HEIGHT 240

/* A color of a palette dimmed to a quarter, and the bit that makes it opaque. */
#define PALETTE_QUARTER_MASK 0x18C6
#define PALETTE_OPAQUE 0x0001

/* The text sizes of the display. */
#define SPEED_TEXT_SIZE 20
#define SPEED_TEXT_SIZE_SPLIT 15
#define SPEED_UNIT_TEXT_SIZE 15

/* The kilometers in a mile. */
#define KM_PER_MILE 1.609344f

/* The dashboard of the current view and the car of the active camera, read afresh at every use. */
#define DASHBOARD_FACE g_aDashboardTextures[g_dwCurrentSplitScreenIndex]
#define DASHBOARD_GEAR g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex]
#define ACTIVE_CAR_MODEL g_aCarModels[g_pActiveCameraVehicle->carModel]

void RenderDashboardGauges(void) {
    ViewportRect *rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    CarModel *car;
    Vtx *verts;
    u16 *palette;
    s32 x, y, gear, key, reload, jitter, first, lit, i;
    f32 rpm, angle, tipLength, baseLength;

    x = HUD_RIGHT - DASHBOARD_FACE.width;
    y = rect->y + rect->height - DASHBOARD_FACE.height - HUD_BOTTOM_MARGIN;
    gear = 0;
    if (g_pActiveCameraVehicle->throttle >= 0.0f) {
        gear = g_pActiveCameraVehicle->gear + 1;
    }

    /* Load the face and the palette when the car or the time of day changes. */
    key = g_pActiveCameraVehicle->carModel | (g_dwNightMode << 8);
    reload = key != g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex];
    if (reload) {
        DASHBOARD_FACE.width = ACTIVE_CAR_MODEL.dashboardWidth;
        DASHBOARD_FACE.height = ACTIVE_CAR_MODEL.dashboardHeight;
        DmaRead(DASHBOARD_FACE.pixels,
                ACTIVE_CAR_MODEL.dashboardRom + DASHBOARD_PALETTES_BYTES,
                DASHBOARD_FACE.height * DASHBOARD_FACE.width);
        DmaRead(g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex],
                ACTIVE_CAR_MODEL.dashboardRom + g_dwNightMode * DASHBOARD_PALETTE_BYTES,
                DASHBOARD_PALETTE_BYTES);
        DASHBOARD_GEAR.width = ACTIVE_CAR_MODEL.gearWidth;
        DASHBOARD_GEAR.height = ACTIVE_CAR_MODEL.gearHeight;
        g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex] = key;
    }
    if (reload || gear != g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex]) {
        DmaRead(DASHBOARD_GEAR.pixels,
                ACTIVE_CAR_MODEL.dashboardRom + DASHBOARD_PALETTES_BYTES +
                    DASHBOARD_FACE.height * DASHBOARD_FACE.width +
                    DASHBOARD_GEAR.height * DASHBOARD_GEAR.width * gear,
                DASHBOARD_GEAR.height * DASHBOARD_GEAR.width);
        g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex] = gear;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPLoadTLUT_pal256(gDisplayListPtr++, g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex]);
    if (g_dwNumViewports == 1) {
        DrawTexturedRect(&DASHBOARD_FACE,
                         x,
                         y,
                         DASHBOARD_FACE.width,
                         DASHBOARD_FACE.height,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
        DrawTexturedRect(&DASHBOARD_GEAR,
                         ACTIVE_CAR_MODEL.gearX + x,
                         ACTIVE_CAR_MODEL.gearY + y,
                         DASHBOARD_GEAR.width,
                         DASHBOARD_GEAR.height,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
    } else {
        /* A smaller dashboard, three quarters of the size, with its bottom right in place. */
        x += DASHBOARD_FACE.width / 4;
        y += DASHBOARD_FACE.height / 4;
        DrawTexturedRect(&DASHBOARD_FACE,
                         x,
                         y,
                         DASHBOARD_FACE.width * 3 / 4,
                         DASHBOARD_FACE.height * 3 / 4,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
        DrawTexturedRect(&DASHBOARD_GEAR,
                         ACTIVE_CAR_MODEL.gearX * 3 / 4 + x,
                         ACTIVE_CAR_MODEL.gearY * 3 / 4 + y,
                         DASHBOARD_GEAR.width * 3 / 4,
                         DASHBOARD_GEAR.height * 3 / 4,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
    }

    jitter = g_dwRacePauseState ? NEEDLE_JITTER_PAUSED : AdvanceRng() & NEEDLE_JITTER_MASK;
    rpm = (f32)jitter + g_pActiveCameraVehicle->engineRpm;
    car = &g_aCarModels[g_pActiveCameraVehicle->carModel];
    if (car->tachometer == TACHOMETER_NEEDLE) {
        verts = g_aGaugeNeedleVerts[g_dwFrameBufferIndex][g_dwCurrentSplitScreenIndex];
        if (g_dwNumViewports == 2) {
            x += car->needleX * 3 / 4;
            y += car->needleY * 3 / 4;
            tipLength = NEEDLE_TIP_LENGTH_SPLIT;
            baseLength = NEEDLE_BASE_LENGTH_SPLIT;
        } else {
            x += car->needleX;
            y += car->needleY;
            tipLength = NEEDLE_TIP_LENGTH;
            baseLength = NEEDLE_BASE_LENGTH;
        }
        y = SCREEN_HEIGHT - y;
        angle = car->needleRestAngle;
        if (rpm > 0.0f) {
            angle = car->needleRestAngle -
                    rpm * (car->needleRestAngle - car->needleFullAngle) / TACHOMETER_FULL_RPM;
        }

        /* A narrow quad from the tip on one side of the center to the base on the other. */
        verts[0].v.ob[0] = (s32)(cosf(angle - NEEDLE_TIP_HALF) * tipLength + x);
        verts[0].v.ob[1] = (s32)(sinf(angle - NEEDLE_TIP_HALF) * tipLength + y);
        verts[0].v.ob[2] = 0;
        verts[1].v.ob[0] = (s32)(cosf(angle + NEEDLE_TIP_HALF) * tipLength + x);
        verts[1].v.ob[1] = (s32)(sinf(angle + NEEDLE_TIP_HALF) * tipLength + y);
        verts[1].v.ob[2] = 0;
        verts[2].v.ob[0] = (s32)(cosf(angle + NEEDLE_BASE_HALF) * baseLength + x);
        verts[2].v.ob[1] = (s32)(sinf(angle + NEEDLE_BASE_HALF) * baseLength + y);
        verts[2].v.ob[2] = 0;
        verts[3].v.ob[0] = (s32)(cosf(angle - NEEDLE_BASE_HALF) * baseLength + x);
        verts[3].v.ob[1] = (s32)(sinf(angle - NEEDLE_BASE_HALF) * baseLength + y);
        verts[3].v.ob[2] = 0;
        for (i = 0; i < 4; i++) {
            verts[i].v.cn[0] = 0;
            verts[i].v.cn[1] = 0xFF;
            verts[i].v.cn[2] = 0;
            verts[i].v.cn[3] = 0xFF;
        }
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE);
        gDPSetRenderMode(gDisplayListPtr++,
                         AA_EN | IM_RD | ALPHA_CVG_SEL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
        gSPClearGeometryMode(gDisplayListPtr++, G_CULL_FRONT | G_CULL_BACK | G_FOG | G_LIGHTING);
        gSPSetGeometryMode(gDisplayListPtr++, G_SHADE);
        gSPVertex(gDisplayListPtr++, verts, 4, 0);
        gSP2Triangles(gDisplayListPtr++, 0, 1, 2, 0, 0, 2, 3, 0);
        gDPPipeSync(gDisplayListPtr++);
        return;
    }
    if (car->tachometer != TACHOMETER_BAR_DIMMED && car->tachometer != TACHOMETER_BAR_HIDDEN) {
        return;
    }

    /*
     * The segments of a bar are the last colors of the palette. The ones the revolutions reach
     * keep their colors, and the rest dim or disappear.
     */
    first = (DASHBOARD_PALETTE_COLORS - car->barSegments) & ~3;
    DmaRead(&g_awDashboardBarPalette[first],
            car->dashboardRom + g_dwNightMode * DASHBOARD_PALETTE_BYTES + first * 2,
            (DASHBOARD_PALETTE_COLORS - first) * 2);
    car = &g_aCarModels[g_pActiveCameraVehicle->carModel];
    lit = (s32)((f64)rpm / TACHOMETER_FULL_RPM_D * (f64)(f32)(car->barSegments + 1) - 0.5);
    first = DASHBOARD_PALETTE_COLORS - car->barSegments;
    if (lit < 0) {
        lit = 0;
    }
    lit += first;
    if (lit > DASHBOARD_PALETTE_COLORS) {
        lit = DASHBOARD_PALETTE_COLORS;
    }
    palette = g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex];
    for (i = first; i < lit; i++) {
        palette[i] = g_awDashboardBarPalette[i];
    }
    if (ACTIVE_CAR_MODEL.tachometer == TACHOMETER_BAR_DIMMED) {
        for (i = lit; i < DASHBOARD_PALETTE_COLORS; i++) {
            palette[i] =
                ((g_awDashboardBarPalette[i] >> 3) & PALETTE_QUARTER_MASK) | PALETTE_OPAQUE;
        }
    } else {
        for (i = lit; i < DASHBOARD_PALETTE_COLORS; i++) {
            palette[i] = g_awDashboardBarPalette[i] & ~PALETTE_OPAQUE;
        }
    }
}

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

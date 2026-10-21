#include "hud.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "game.h"
#include "input.h"
#include "racehud.h"
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
#define TIME_TEXT_SIZE 15
#define LAP_TEXT_SIZE 15
#define PLACE_TEXT_SIZE 40
#define PLACE_SUFFIX_TEXT_SIZE 20
#define PLACE_TEXT_SIZE_SPLIT 26
#define PLACE_SUFFIX_TEXT_SIZE_SPLIT 13
#define SPEED_TEXT_SIZE 20
#define SPEED_TEXT_SIZE_SPLIT 15
#define SPEED_UNIT_TEXT_SIZE 15

/* The kilometers in a mile. */
#define KM_PER_MILE 1.609344f

void RenderDashboardGauges(void) {
    Texture *face = &g_aDashboardTextures[g_dwCurrentSplitScreenIndex];
    Texture *cell = &g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex];
    ViewportRect *rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    Vehicle *vehicle = g_pActiveCameraVehicle;
    CarModel *car;
    Vtx *verts;
    u16 *palette;
    s32 x, y, gear, key, reload, jitter, first, lit, i;
    f32 rpm, angle, tipLength, baseLength, centerX, centerY;

    x = HUD_RIGHT - face->width;
    y = rect->y + rect->height - face->height - HUD_BOTTOM_MARGIN;
    gear = 0;
    if (vehicle->throttle >= 0.0f) {
        gear = vehicle->gear + 1;
    }

    /* Load the face and the palette when the car or the time of day changes. */
    key = vehicle->carModel | (g_dwNightMode << 8);
    reload = key != g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex];
    if (reload) {
        car = &g_aCarModels[vehicle->carModel];
        face->width = car->dashboardWidth;
        face->height = car->dashboardHeight;
        DmaRead(
            face->pixels, car->dashboardRom + DASHBOARD_PALETTES_BYTES, face->height * face->width);
        DmaRead(g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex],
                car->dashboardRom + g_dwNightMode * DASHBOARD_PALETTE_BYTES,
                DASHBOARD_PALETTE_BYTES);
        cell->width = car->gearWidth;
        cell->height = car->gearHeight;
        g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex] = key;
    }
    if (reload || gear != g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex]) {
        car = &g_aCarModels[vehicle->carModel];
        DmaRead(cell->pixels,
                car->dashboardRom + DASHBOARD_PALETTES_BYTES + face->height * face->width +
                    cell->height * cell->width * gear,
                cell->height * cell->width);
        g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex] = gear;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPLoadTLUT_pal256(gDisplayListPtr++, g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex]);
    car = &g_aCarModels[g_pActiveCameraVehicle->carModel];
    if (g_dwNumViewports == 1) {
        DrawTexturedRect(face,
                         x,
                         y,
                         face->width,
                         face->height,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
        DrawTexturedRect(cell,
                         car->gearX + x,
                         car->gearY + y,
                         cell->width,
                         cell->height,
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
        x += face->width / 4;
        y += face->height / 4;
        DrawTexturedRect(face,
                         x,
                         y,
                         face->width * 3 / 4,
                         face->height * 3 / 4,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
        DrawTexturedRect(cell,
                         car->gearX * 3 / 4 + x,
                         car->gearY * 3 / 4 + y,
                         cell->width * 3 / 4,
                         cell->height * 3 / 4,
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
        centerX = x;
        centerY = y;
        verts[0].v.ob[0] = (s32)(cosf(angle - NEEDLE_TIP_HALF) * tipLength + centerX);
        verts[0].v.ob[1] = (s32)(sinf(angle - NEEDLE_TIP_HALF) * tipLength + centerY);
        verts[0].v.ob[2] = 0;
        verts[1].v.ob[0] = (s32)(cosf(angle + NEEDLE_TIP_HALF) * tipLength + centerX);
        verts[1].v.ob[1] = (s32)(sinf(angle + NEEDLE_TIP_HALF) * tipLength + centerY);
        verts[1].v.ob[2] = 0;
        verts[2].v.ob[0] = (s32)(cosf(angle + NEEDLE_BASE_HALF) * baseLength + centerX);
        verts[2].v.ob[1] = (s32)(sinf(angle + NEEDLE_BASE_HALF) * baseLength + centerY);
        verts[2].v.ob[2] = 0;
        verts[3].v.ob[0] = (s32)(cosf(angle - NEEDLE_BASE_HALF) * baseLength + centerX);
        verts[3].v.ob[1] = (s32)(sinf(angle - NEEDLE_BASE_HALF) * baseLength + centerY);
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
    if (car->tachometer == TACHOMETER_BAR_DIMMED) {
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

void DrawLabelWithTime(const char *label, const char *color, f32 seconds, s32 x, s32 y) {
    char text[44];
    s32 hundredths = (s32)(seconds * 100.0f);
    s32 whole = hundredths / 100;
    s32 minutes = whole / 60;

    sprintf(
        text, "%s%d'%02d\"%02d", color, minutes, whole - minutes * 60, hundredths - whole * 100);
    DrawText(text, x, y + TIME_TEXT_SIZE);
    DrawText(label, x, y);
}

void DrawRaceTimerHud(void) {
    s32 below = g_dwNumViewports == 1 ? 2 * TIME_TEXT_SIZE : 0;
    s32 y = g_aViewportRects[g_dwCurrentSplitScreenIndex].y + 20;

    SetTextStyleNormal();
    SetTextAlignRight();
    SetTextSize(TIME_TEXT_SIZE);
    switch (g_dwGameMode) {
    case GAME_MODE_CHAMPIONSHIP:
    case GAME_MODE_TIME_ATTACK:
        if (g_dwNumViewports == 1) {
            DrawLabelWithTime(
                "%15TOTAL TIME", "%ww", g_pActiveCameraVehicle->totalTime, HUD_RIGHT, y);
        }
        if (g_pActiveCameraVehicle->lap >= g_dwTotalLaps) {
            DrawLabelWithTime(
                "%15BEST LAP", "%ww", g_pActiveCameraVehicle->bestLapTime, HUD_RIGHT, y + below);
        } else {
            DrawLabelWithTime(
                "%15LAP TIME", "%ww", g_pActiveCameraVehicle->lapTime, HUD_RIGHT, y + below);
        }
        break;
    case GAME_MODE_ARCADE:
        if (g_dwNumViewports == 1) {
            DrawLabelWithTime(
                "%15TOTAL TIME", "%ww", g_pActiveCameraVehicle->totalTime, HUD_RIGHT, y);
        }
        if (g_pActiveCameraVehicle->lap >= g_dwTotalLaps) {
            DrawLabelWithTime(
                "%15BEST LAP", "%ww", g_pActiveCameraVehicle->bestLapTime, HUD_RIGHT, y + below);
        } else if (g_dwNumLocalPlayers == 1) {
            DrawLabelWithTime(
                "%15TIME LEFT", "%ww", g_pActiveCameraVehicle->timeLeft, HUD_RIGHT, y + below);
        } else {
            DrawLabelWithTime(
                "%15LAP TIME", "%ww", g_pActiveCameraVehicle->lapTime, HUD_RIGHT, y + below);
        }
        break;
    case GAME_MODE_PRACTICE:
        if (g_dwNumViewports == 1) {
            DrawLabelWithTime(
                "%15BEST LAP", "%ww", g_pActiveCameraVehicle->bestLapTime, HUD_RIGHT, y);
        }
        DrawLabelWithTime(
            "%15LAP TIME", "%ww", g_pActiveCameraVehicle->lapTime, HUD_RIGHT, y + below);
        break;
    }
}

void DrawLapCounterHud(void) {
    char text[20];
    const char *suffix;
    Vehicle *vehicle;
    ViewportRect *rect;
    s32 x, y, nudge, width;

    if (g_dwGameMode == GAME_MODE_PRACTICE) {
        return;
    }

    /* The lap, at the left of the first view, unless the camera is on the bumper. */
    vehicle = g_pActiveCameraVehicle;
    x = g_aViewportRects[0].x + 16;
    if (g_pActiveCamera != &vehicle->cameras[VEHICLE_CAMERA_BUMPER] &&
        (vehicle->lap < g_dwTotalLaps || g_dwNumViewports == 1)) {
        y = g_aViewportRects[g_dwCurrentSplitScreenIndex].y + 5;
        if (vehicle->lap < g_dwTotalLaps) {
            sprintf(text,
                    "%%y1%s%d/%d",
                    g_dwNumViewports == 2 ? "L" : "LAP ",
                    vehicle->lap + 1,
                    g_dwTotalLaps);
        } else {
            sprintf(text, "FINISHED");
        }
        y += LAP_TEXT_SIZE;
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(LAP_TEXT_SIZE);
        DrawText(text, x, y);
    }

    /* The place, at the bottom left, with its ordinal suffix raised beside it. */
    rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    x -= 2;
    y = rect->height + rect->y - 12;
    EnableTextShadow();
    SetTextAlignLeft();
    SetTextCustomColors(0xFF, 0xF0, 0x7D, 0xFF, 0x78, 0);
    sprintf(text, "%d", g_pActiveCameraVehicle->racePosition + 1);
    nudge = 0;
    switch (g_pActiveCameraVehicle->racePosition) {
    case 0:
        suffix = "st";
        nudge = -3;
        break;
    case 1:
        suffix = "nd";
        nudge = 1;
        break;
    case 2:
        suffix = "rd";
        break;
    default:
        suffix = "th";
        nudge = 1;
        break;
    }
    if (g_dwNumViewports == 1) {
        SetTextSize(PLACE_TEXT_SIZE);
        width = MeasureTextWidth(text, PLACE_TEXT_SIZE);
        DrawText(text, x - 1, y - 1);
        SetTextSize(PLACE_SUFFIX_TEXT_SIZE);
        DrawText(suffix, x + nudge + width + 3, y - 15);
    } else {
        SetTextSize(PLACE_TEXT_SIZE_SPLIT);
        width = MeasureTextWidth(text, PLACE_TEXT_SIZE_SPLIT);
        DrawText(text, x, y);
        SetTextSize(PLACE_SUFFIX_TEXT_SIZE_SPLIT);
        DrawText(suffix, x + nudge * 2 / 3 + width + 3, y - 10);
    }
    DisableTextShadow();
}

void DrawHudMessage(void) {
    ViewportRect *rect;
    s32 large, small, x, top;

    if (g_dwRacePauseState != 0) {
        return;
    }
    if (g_dwNumViewports == 1) {
        large = 30;
        small = 20;
    } else {
        large = 20;
        small = 15;
    }
    rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    x = rect->width / 2 + rect->x;
    top = rect->height / 3 + rect->y;
    SetTextAlignCenter();
    if (g_pActiveCameraVehicle->message != NULL) {
        SetTextSize(large);
        DrawText(g_pActiveCameraVehicle->message, x, large / 4 + top);
    } else if (g_pActiveCameraVehicle->subMessage != NULL) {
        SetTextSize(small);
        DrawText(g_pActiveCameraVehicle->subMessage, x, large * 3 / 16 + top);
    }
}

void DrawSpeedAndHudOverlay(void) {
    ViewportRect *rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    CameraFrame *bumper;
    f32 speed = g_pActiveCameraVehicle->speedKph;
    s32 x, y;

    SetScissorRect(HUD_SCISSOR_LEFT, rect->y, HUD_SCISSOR_RIGHT, rect->height);
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
    if (g_bSpeedUnitsMph) {
        sprintf(g_szHudSpeedTextBuf, "%%yw%.0f", (f64)(speed / KM_PER_MILE));
    } else {
        sprintf(g_szHudSpeedTextBuf, "%%yw%.0f", (f64)speed);
    }

    /* The speed sits above the dashboard when it shows, and its unit beside it. */
    rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    y = rect->height + rect->y - HUD_BOTTOM_MARGIN;
    bumper = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER];
    if (g_dwNumViewports == 2) {
        if (g_pActiveCamera != bumper) {
            y -= g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height * 3 / 4 + 1;
        }
        SetTextSize(SPEED_TEXT_SIZE_SPLIT);
        DrawText(g_szHudSpeedTextBuf, HUD_RIGHT, y);
        return;
    }
    x = HUD_RIGHT;
    if (g_pActiveCamera != bumper) {
        x = SPEED_DASHBOARD_RIGHT;
        y -= g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height;
    }
    SetTextSize(SPEED_TEXT_SIZE);
    if (g_bSpeedUnitsMph) {
        DrawText(g_szHudSpeedTextBuf, x, y - 3);
    } else {
        DrawText(g_szHudSpeedTextBuf, x - 3, y - 3);
    }
    if (g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
        return;
    }
    SetTextSize(SPEED_UNIT_TEXT_SIZE);
    SetTextAlignLeft();
    if (g_bSpeedUnitsMph) {
        DrawText("%wwmph", x, y - 3);
    } else {
        DrawText("%wwkph", x - 3, y - 3);
    }
}

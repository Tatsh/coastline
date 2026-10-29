#include "common/camera.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/laps.h"
#include "common/text.h"
#include "common/vehicleaccess.h"

#if TGR_BOSS_RALLY
#include <stdio.h>

#include "common/settings.h"
#include "common/strings.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/directplay.h"
#endif
#include "win32/hudcanvas.h"
#else
#include <PR/os_libc.h>
#endif

#define TIME_TEXT_SIZE 15
#if TGR_BOSS_RALLY && defined(ENABLE_PATCHES)
#define g_dwConfigFramebufferWidth GetHudCanvasWidth()
#endif
#define LAP_TEXT_SIZE 15
#define PLACE_TEXT_SIZE 40
#define PLACE_SUFFIX_TEXT_SIZE 20
#define PLACE_TEXT_SIZE_SPLIT 26
#define PLACE_SUFFIX_TEXT_SIZE_SPLIT 13

#if TGR_BOSS_RALLY

#undef DrawText

s32 g_bHudShowLapCounter = 1;
s32 g_bHudShowRaceTimer = 1;
s32 g_bHudShowPosition = 1;

#define HUD_VIEWPORTS viewports
#define HUD_RACE_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define HUD_RACE_RIGHT right
#define HUD_TOTAL_TIME 0xe7u
#define HUD_BEST_LAP 0xe8u
#define HUD_LAP_TIME 0xe9u
#define HUD_LAP_LABEL GetStringResource(0xe5)
#define HUD_FINISHED GetStringResource(0xe6)
#define HUD_SUFFIX_ST GetStringResource(0xb3)
#define HUD_SUFFIX_ND GetStringResource(0xb4)
#define HUD_SUFFIX_RD GetStringResource(0xb5)
#define HUD_SUFFIX_TH GetStringResource(0xb6)
static inline void DrawHudTimeLabel(unsigned label, const char *color, f32 seconds, s32 x, s32 y) {
    DrawLabelWithTime(GetStringResource(label), color, seconds, x, y);
}
#define DRAW_HUD_TIME DrawHudTimeLabel
#else
#define HUD_VIEWPORTS g_aViewportRects
#define HUD_RACE_ADD(a, b) ((a) + (b))
#define HUD_RACE_RIGHT 296
#define HUD_TOTAL_TIME "%15TOTAL TIME"
#define HUD_BEST_LAP "%15BEST LAP"
#define HUD_LAP_TIME "%15LAP TIME"
#define HUD_LAP_LABEL "LAP "
#define HUD_FINISHED "FINISHED"
#define HUD_SUFFIX_ST "st"
#define HUD_SUFFIX_ND "nd"
#define HUD_SUFFIX_RD "rd"
#define HUD_SUFFIX_TH "th"
#define DRAW_HUD_TIME DrawLabelWithTime
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
void DrawRaceTimerHud(const ViewportRect *viewports) {
    if (!g_bHudShowRaceTimer) {
        return;
    }
    s32 right = HUD_RACE_ADD(g_dwConfigFramebufferWidth, -16);
#else
void DrawRaceTimerHud(void) {
#endif
    s32 below = g_dwNumViewports == 1 ? 2 * TIME_TEXT_SIZE : 0;
    s32 y = HUD_RACE_ADD(HUD_VIEWPORTS[g_dwCurrentSplitScreenIndex].y, 20);

    SetTextStyleNormal();
    SetTextAlignRight();
    SetTextSize(TIME_TEXT_SIZE);
    switch (g_dwGameMode) {
    case GAME_MODE_CHAMPIONSHIP:
    case GAME_MODE_TIME_ATTACK:
#if TGR_BOSS_RALLY
    case GAME_MODE_ARCADE:
    case GAME_MODE_NETWORK:
#endif
        if (g_dwNumViewports == 1) {
            DRAW_HUD_TIME(HUD_TOTAL_TIME,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, TotalTime),
                          HUD_RACE_RIGHT,
                          y);
        }
        if (VEHICLE_GET(g_pActiveCameraVehicle, Lap) >= g_dwTotalLaps) {
            DRAW_HUD_TIME(HUD_BEST_LAP,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, BestLapTime),
                          HUD_RACE_RIGHT,
                          HUD_RACE_ADD(y, below));
        } else {
            DRAW_HUD_TIME(HUD_LAP_TIME,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, LapTime),
                          HUD_RACE_RIGHT,
                          HUD_RACE_ADD(y, below));
        }
        break;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    case GAME_MODE_ARCADE:
        if (g_dwNumViewports == 1) {
            DRAW_HUD_TIME(HUD_TOTAL_TIME,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, TotalTime),
                          HUD_RACE_RIGHT,
                          y);
        }
        if (VEHICLE_GET(g_pActiveCameraVehicle, Lap) >= g_dwTotalLaps) {
            DRAW_HUD_TIME(HUD_BEST_LAP,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, BestLapTime),
                          HUD_RACE_RIGHT,
                          HUD_RACE_ADD(y, below));
        } else if (g_dwNumLocalPlayers == 1) {
            DRAW_HUD_TIME("%15TIME LEFT",
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, TimeLeft),
                          HUD_RACE_RIGHT,
                          HUD_RACE_ADD(y, below));
        } else {
            DRAW_HUD_TIME(HUD_LAP_TIME,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, LapTime),
                          HUD_RACE_RIGHT,
                          HUD_RACE_ADD(y, below));
        }
        break;
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
    case GAME_MODE_PRACTICE:
        if (g_dwNumViewports == 1) {
            DRAW_HUD_TIME(HUD_BEST_LAP,
                          "%ww",
                          VEHICLE_GET(g_pActiveCameraVehicle, BestLapTime),
                          HUD_RACE_RIGHT,
                          y);
        }
        DRAW_HUD_TIME(HUD_LAP_TIME,
                      "%ww",
                      VEHICLE_GET(g_pActiveCameraVehicle, LapTime),
                      HUD_RACE_RIGHT,
                      HUD_RACE_ADD(y, below));
        break;
    }
}

#if TGR_BOSS_RALLY
void DrawLapCounterHud(const ViewportRect *viewports, const Vehicle *) {
    char text[256];
#else
void DrawLapCounterHud(void) {
#ifdef ENABLE_PATCHES
    /* The lap text needs 31 bytes for two numbers of any size. */
    char text[32];
#else
    char text[20];
#endif /* ENABLE_PATCHES */
#endif
    const char *suffix;
    s32 x, y, nudge, width;

    if (g_dwGameMode == GAME_MODE_PRACTICE) {
        return;
    }

    /* The lap, at the left of the first view, unless the camera is on the bumper. */
    x = HUD_RACE_ADD(HUD_VIEWPORTS[0].x, 16);
#if TGR_BOSS_RALLY
    if (g_bHudShowLapCounter &&
#else
    if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER] &&
#endif
        (VEHICLE_GET(g_pActiveCameraVehicle, Lap) < g_dwTotalLaps || g_dwNumViewports == 1)) {
        y = HUD_RACE_ADD(HUD_VIEWPORTS[g_dwCurrentSplitScreenIndex].y, 5);
        if (VEHICLE_GET(g_pActiveCameraVehicle, Lap) < g_dwTotalLaps) {
#if TGR_BOSS_RALLY
            const char *lapLabel = g_dwNumViewports == 2 ? "L" : HUD_LAP_LABEL;
#endif
            sprintf(text,
                    "%%y1%s%d/%d",
#if TGR_BOSS_RALLY
                    lapLabel,
#else
                    g_dwNumViewports == 2 ? "L" : HUD_LAP_LABEL,
#endif
                    HUD_RACE_ADD(VEHICLE_GET(g_pActiveCameraVehicle, Lap), 1),
                    g_dwTotalLaps);
        } else {
#ifdef ENABLE_PATCHES
            sprintf(text, "%s", HUD_FINISHED);
#else
            /* The original passes the text directly as a format string. */
            sprintf(text, HUD_FINISHED);
#endif /* ENABLE_PATCHES */
        }
        y = HUD_RACE_ADD(y, LAP_TEXT_SIZE);
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(LAP_TEXT_SIZE);
        DrawText(text, x, y);
    }

#if TGR_BOSS_RALLY
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* A spectator of a network game has no car of its own. */
    if (!g_bHudShowPosition || g_bNetSpectator) {
#else
    if (!g_bHudShowPosition) {
#endif
        return;
    }
#endif
    /* The place, at the bottom left, with its ordinal suffix raised beside it. */
    y = HUD_RACE_ADD(HUD_RACE_ADD(HUD_VIEWPORTS[g_dwCurrentSplitScreenIndex].y,
                                  HUD_VIEWPORTS[g_dwCurrentSplitScreenIndex].height),
                     -12);
    x = HUD_RACE_ADD(x, -2);
    EnableTextShadow();
    SetTextAlignLeft();
    SetTextCustomColors(0xFF, 0xF0, 0x7D, 0xFF, 0x78, 0);
    sprintf(text, "%d", HUD_RACE_ADD(VEHICLE_GET(g_pActiveCameraVehicle, RacePosition), 1));
    nudge = 0;
    switch (VEHICLE_GET(g_pActiveCameraVehicle, RacePosition)) {
    case 0:
        suffix = HUD_SUFFIX_ST;
        nudge = -3;
        break;
    case 1:
        suffix = HUD_SUFFIX_ND;
        nudge = 1;
        break;
    case 2:
        suffix = HUD_SUFFIX_RD;
        break;
    default:
        suffix = HUD_SUFFIX_TH;
        nudge = 1;
        break;
    }
    if (g_dwNumViewports == 1) {
        SetTextSize(PLACE_TEXT_SIZE);
        width = MeasureTextWidth(text, PLACE_TEXT_SIZE);
        DrawText(text, HUD_RACE_ADD(x, -1), HUD_RACE_ADD(y, -1));
        SetTextSize(PLACE_SUFFIX_TEXT_SIZE);
        DrawText(suffix,
                 HUD_RACE_ADD(HUD_RACE_ADD(x, nudge), HUD_RACE_ADD(width, 3)),
                 HUD_RACE_ADD(y, -15));
    } else {
        SetTextSize(PLACE_TEXT_SIZE_SPLIT);
        width = MeasureTextWidth(text, PLACE_TEXT_SIZE_SPLIT);
        DrawText(text, x, y);
        SetTextSize(PLACE_SUFFIX_TEXT_SIZE_SPLIT);
        DrawText(suffix,
                 HUD_RACE_ADD(HUD_RACE_ADD(x, 2 * nudge / 3), HUD_RACE_ADD(width, 3)),
                 HUD_RACE_ADD(y, -10));
    }
    DisableTextShadow();
}

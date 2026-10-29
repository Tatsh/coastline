#include "common/racerender.h"

#include <PR/os.h>

#if TGR_BOSS_RALLY
#include <stdio.h>

#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/strings.h"
#include "win32/buildinfo.h"
#include "win32/hud.h"
#include "win32/hudcanvas.h"
#include "win32/render.h"
#else
#include <PR/os_libc.h>

#include "common/optionsettings.h"
#endif /* TGR_BOSS_RALLY */

#include "common/camera.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/race.h"
#include "common/racerslotaccess.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/sky.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackrender.h"
#include "common/trackselect.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY && defined(ENABLE_PATCHES)
#define g_dwScreenWidth GetHudCanvasWidth()
#define g_dwScreenHeight GetHudCanvasHeight()
#endif

#if TGR_BOSS_RALLY
namespace {
    /**
     * @ghidraAddress USA: 0x8028aafc
     * @ghidraAddress EUR: 0x8028cd04
     * @ghidraAddress JPN: 0x8028c7d4
     * @ghidraAddress BRD3D: 0x106c56f0
     * @ghidraAddress BRGlide: 0x106ec780
     */
    s32 g_dwCameraRayGroundSurface;
    /**
     * @ghidraAddress USA: 0x8028ab04
     * @ghidraAddress EUR: 0x8028cd0c
     * @ghidraAddress JPN: 0x8028c7dc
     * @ghidraAddress BRD3D: 0x10690a34
     * @ghidraAddress BRGlide: 0x106b7ac4
     */
    s32 g_dwCameraRayNearbyCount;
    /**
     * What CastRayAgainstTrack() found below the camera of the view being drawn.
     *
     * @ghidraAddress USA: 0x8028aaf8
     * @ghidraAddress EUR: 0x8028cd00
     * @ghidraAddress JPN: 0x8028c7d0
     * @ghidraAddress BRD3D: 0x106c01fc
     * @ghidraAddress BRGlide: 0x106e728c
     */
    f32 g_fCameraRayGroundDistance;
} // namespace
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
namespace {
    /**
     * @ghidraAddress BRD3D: 0x1039b740
     * @ghidraAddress BRGlide: 0x10396f28
     */
    char g_szRaceHudTextBuffer[32];
    /**
     * @ghidraAddress BRD3D: 0x100a73a4
     * @ghidraAddress BRGlide: 0x100a6b64
     */
    const char g_szTextCodeHighlight[] = "%y1";
    /**
     * @ghidraAddress BRD3D: 0x100a73a0
     * @ghidraAddress BRGlide: 0x100a6b60
     */
    const char g_szTextCodeNormal[] = "%ry";
}

#define RACE_VIEW_CONTEXT ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles
#define RACE_VIEW_PARAMETERS RACE_VIEW_CONTEXT,
#define RACE_VIEW_ARGUMENTS views, slots, vehicles,
#define RACE_VIEW_VEHICLES vehicles
#define RACE_VIEW_SLOTS slots
#define RACE_VIEW_RECTS views
#define RACE_VIEW_TIMING(bar, red, green, blue, alpha) NullStub()
#define RACE_VIEW_WEATHER_EXCLUDED 8
#define RACE_VIEW_SCREEN_WIDTH ((s32)g_dwScreenWidth)
#define RACE_VIEW_SCREEN_HEIGHT ((s32)g_dwScreenHeight)
#define RACE_VIEW_SKY() RenderSkybox(views)
#define RACE_VIEW_SMOKE()                                                                          \
    do {                                                                                           \
        NullStub2(views);                                                                          \
        RenderTireSmoke(views);                                                                    \
    } while (0)
#define RACE_VIEW_GEOMETRY(pass) RenderTrackGeometry(views, pass, slots, vehicles)
#define RACE_VIEW_EXHAUST() RenderExhaustAndTireSpray(views, slots)
#define RACE_VIEW_GLARE()                                                                          \
    RenderHeadlightGlare(                                                                          \
        views, (f32)((long double)g_fHeadlightGlare - (long double)g_fScreenFogPulse * -0.5f))
#define RACE_VIEW_MIRROR_GLARE() RenderHeadlightGlare(views, g_fScreenFogPulse + g_fHeadlightGlare)
#else
#define RACE_VIEW_CONTEXT void
#define RACE_VIEW_PARAMETERS
#define RACE_VIEW_ARGUMENTS
#define RACE_VIEW_VEHICLES g_aVehicles
#define RACE_VIEW_SLOTS g_aRacerSlots
#define RACE_VIEW_RECTS g_aViewportRects
#define RACE_VIEW_TIMING(bar, red, green, blue, alpha) AddTimingMarker(bar, red, green, blue, alpha)
#define RACE_VIEW_WEATHER_EXCLUDED TRACK_MIRROR_COASTLINE
#define RACE_VIEW_SCREEN_WIDTH g_dwScreenWidth
#define RACE_VIEW_SCREEN_HEIGHT g_dwScreenHeight
#define RACE_VIEW_SKY() RenderSkybox()
#define RACE_VIEW_SMOKE() RenderTireSmoke()
#define RACE_VIEW_GEOMETRY(pass) RenderTrackGeometry(pass)
#define RACE_VIEW_EXHAUST() RenderExhaustAndTireSpray()
#define RACE_VIEW_GLARE() RenderHeadlightGlare()
#define RACE_VIEW_MIRROR_GLARE() RenderHeadlightGlare()
#endif /* TGR_BOSS_RALLY */

#define SCREEN_CENTER_X 160

/* Draw the vehicles of the racers that are, or are not, ghosts. */
static void RenderRacerVehicles(RACE_VIEW_PARAMETERS s32 ghosts) {
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        if (RACER_SLOT_GET(&RACE_VIEW_SLOTS[i], Vehicle) != NULL &&
            (VEHICLE_GET(RACER_SLOT_GET(&RACE_VIEW_SLOTS[i], Vehicle), GhostMode) ==
             VEHICLE_GHOST) == ghosts) {
            RenderVehicleModel(RACER_SLOT_GET(&RACE_VIEW_SLOTS[i], Vehicle), 0);
        }
    }
}

/* Draw the track and the vehicles the camera sees, for the view or for its rear-view mirror, which
 * leaves out the start lights and the smoke of the tires after the track in the snow. */
static void RenderRaceScene(RACE_VIEW_PARAMETERS s32 mainView) {
    s32 i;

    RACE_VIEW_TIMING(0, 0, 0x82, 0, 0xFF);
    RACE_VIEW_SKY();
    ResetHeadlightGlare();
    BuildVisibleGrid();
    if (!mainView) {
        RACE_VIEW_TIMING(0, 0, 0x82, 0, 0xFF);
    } else {
        g_dwVehicleViewModeIndex = g_dwNightMode ? 2 : 1;
    }
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        CheckVehicleVisibility(&RACE_VIEW_VEHICLES[i]);
    }
    RACE_VIEW_GEOMETRY(0);
    /* In the snow, other than on the coastline, the vehicles are drawn after the track. */
    if (!g_dwSnowMode || g_dwSelectedTrackId == TRACK_COASTLINE ||
        g_dwSelectedTrackId == RACE_VIEW_WEATHER_EXCLUDED) {
        SetupFog();
        SetupWeatherLights();
        RACE_VIEW_TIMING(0, 0x80, 0x80, 0x80, 0xFF);
        RenderRacerVehicles(RACE_VIEW_ARGUMENTS FALSE);
        RACE_VIEW_TIMING(0, 0x80, 0x80, 0, 0xFF);
        RACE_VIEW_SMOKE();
    }
    if (mainView) {
        RACE_VIEW_TIMING(0, 0x40, 0x40, 0x40, 0xFF);
        RenderStartLights();
    }
    RACE_VIEW_GEOMETRY(1);
    if (g_dwSnowMode && g_dwSelectedTrackId != TRACK_COASTLINE &&
        g_dwSelectedTrackId != RACE_VIEW_WEATHER_EXCLUDED) {
        SetupFog();
        SetupWeatherLights();
        RACE_VIEW_TIMING(0, 0x80, 0x80, 0x80, 0xFF);
        RenderRacerVehicles(RACE_VIEW_ARGUMENTS FALSE);
        if (mainView) {
            RACE_VIEW_TIMING(0, 0x80, 0x80, 0, 0xFF);
            RACE_VIEW_SMOKE();
        }
    }
}

/* Draw the rear-view mirror over a view whose camera is the bumper camera. */
static void RenderRearViewMirror(RACE_VIEW_PARAMETERS s32 index) {
    CameraFrame *saved;
    s32 x, y, width, height;

    if (g_dwRearViewMirrorMode == 1) {
        width = RACE_VIEW_RECTS[index].width / 4;
    } else if (g_dwRearViewMirrorMode == 2) {
        width = RACE_VIEW_RECTS[index].width * 5 / 16;
    } else {
        width = RACE_VIEW_RECTS[index].width * 3 / 8 + 2;
    }
    height = width >> 2;
    x = ((RACE_VIEW_RECTS[index].width - width) >> 1) + RACE_VIEW_RECTS[index].x;
    y = RACE_VIEW_RECTS[index].height / 16 + RACE_VIEW_RECTS[index].y;
    saved = g_pActiveCamera;
    VEHICLE_SET(g_pActiveCameraVehicle,
                ActiveCamera,
                &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_MIRROR]);
    g_pActiveCamera = VEHICLE_GET(g_pActiveCameraVehicle, ActiveCamera);
    /* The negative width mirrors the view left to right. */
    SetViewport(x, y, -width, height, TRUE);
    ClearZBufferRect(x, y, width, height);
    SetupFrustumCorners(g_pActiveCamera,
                        g_pActiveCamera->fov,
                        g_fCameraProjectionScale * 0.2f,
                        (f32)width,
                        (f32)height);
    SetupCameraMatrix(g_pActiveCamera,
                      g_pActiveCamera->fov,
                      g_fCameraProjectionScale * 0.3f,
                      (f32)width,
                      (f32)height);
    SetupFog();
    SetupWeatherLights();
    RenderRaceScene(RACE_VIEW_ARGUMENTS FALSE);
#if TGR_BOSS_RALLY
    {
        s32 i;

        RACE_VIEW_TIMING(0, 0x80, 0x80, 0x80, 0xFF);
        for (i = 0; i < g_dwNumVehicles; i++) {
            if (RACER_SLOT_GET(&slots[i], Vehicle) != NULL) {
                RenderVehicleHeadlightBeams(RACER_SLOT_GET(&slots[i], Vehicle));
            }
        }
    }
#endif /* TGR_BOSS_RALLY */
    RACE_VIEW_TIMING(0, 0x80, 0x80, 0x80, 0xFF);
    RenderRacerVehicles(RACE_VIEW_ARGUMENTS TRUE);
    RACE_VIEW_MIRROR_GLARE();
    RACE_VIEW_TIMING(0, 0, 0x82, 0, 0xFF);
    VEHICLE_SET(g_pActiveCameraVehicle, ActiveCamera, saved);
#if TGR_BOSS_RALLY
    g_pActiveCamera = saved;
#else
    g_pActiveCamera = VEHICLE_GET(g_pActiveCameraVehicle, ActiveCamera);
#endif
    SetViewport(RACE_VIEW_RECTS[index].x,
                RACE_VIEW_RECTS[index].y,
                RACE_VIEW_RECTS[index].width,
                RACE_VIEW_RECTS[index].height,
                TRUE);
    DrawRdpBorderRect(x, y, width, height);
}

/* Draw the HUD of a view, or the text of the races the computer drives over it. */
#if TGR_BOSS_RALLY
static void DrawViewOverlay(RACE_VIEW_PARAMETERS s32 index) {
#else
static void DrawViewOverlay(void) {
#endif
#if TGR_BOSS_RALLY
    const ViewportRect *view;

    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextSize(40);
        DrawText(GetStringResource(vehicles[0].GetProfile()->season ? 240 : 241),
                 RACE_VIEW_SCREEN_WIDTH / 2,
                 RACE_VIEW_SCREEN_HEIGHT - 30);
    } else if (g_bShowHudHelpText) {
        if (g_dwGameMode == GAME_MODE_DEMO) {
            if (g_dwRaceSessionType == 0) {
                SetTextStyleNormal();
                SetTextSize(15);
                SetTextAlignRight();
                DrawText(GetStringResource(244),
                         RACE_VIEW_SCREEN_WIDTH - 28,
                         RACE_VIEW_SCREEN_HEIGHT - 24);
            }
        } else if (gInstantReplayActive) {
            SetTextStyleNormal();
            SetTextAlignLeft();
            SetTextSize(15);
            DrawText(GetStringResource(242), 28, 32);
            if (g_dwGameMode == GAME_MODE_NETWORK) {
                DrawNetworkRaceStandingsHud(views, vehicles);
            }
            SetTextAlignRight();
            DrawText(
                GetStringResource(244), RACE_VIEW_SCREEN_WIDTH - 28, RACE_VIEW_SCREEN_HEIGHT - 24);
        } else if (g_pActiveCamera ==
                   &g_pActiveCameraVehicle->GetCameras()[VEHICLE_CAMERA_TRACKSIDE]) {
            if (g_pActiveCameraVehicle->GetRacer()->GetStatus() & RACER_FINISHED) {
                DrawHudMessage(views);
            }
        } else {
            NullStub();
            view = &views[index];
            SetupViewportAndScissor(view->x, view->y, view->width, view->height, TRUE);
            DrawSpeedAndHudOverlay(views, vehicles);
            DrawNetworkRaceStandingsHud(views, vehicles);
            NullStub();
            if (g_pActiveCamera != &g_pActiveCameraVehicle->GetCameras()[VEHICLE_CAMERA_BUMPER]) {
                NullStub();
            }
        }
    }
#else
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextSize(40);
        if (VEHICLE_GET(&RACE_VIEW_VEHICLES[0], Profile)->season) {
            DrawText("%rySEASON WINNER!", SCREEN_CENTER_X, 210);
        } else {
            DrawText("%ryCHAMPION!", SCREEN_CENTER_X, 210);
        }
        return;
    }
    if (!g_bShowHudHelpText || g_dwGameMode == GAME_MODE_DEMO) {
        return;
    }
    if (gInstantReplayActive) {
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(15);
#if TGR_REGION != TGR_REGION_JP
        DrawText("%wwINSTANT REPLAY!", 28, 32);
#endif
        DrawText("%wwA to restart", 28, g_dwScreenHeight - 24);
        SetTextAlignRight();
        DrawText("%wwSTART to exit", g_dwScreenWidth - 28, g_dwScreenHeight - 24);
    } else if (g_pActiveCamera !=
               &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE]) {
        RACE_VIEW_TIMING(0, 0xFF, 0, 0, 0xFF);
        DrawSpeedAndHudOverlay();
        RACE_VIEW_TIMING(0, 0, 0x82, 0, 0xFF);
        if (g_pActiveCamera !=
            &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_BUMPER]) {
            StubDashboard();
        }
    } else if (RACER_SLOT_GET(VEHICLE_GET(g_pActiveCameraVehicle, Racer), Status) &
               RACER_FINISHED) {
        DrawHudMessage();
    }
#endif /* TGR_BOSS_RALLY */
}

/* Draw a view of the race. */
void RenderRaceView(RACE_VIEW_PARAMETERS s32 index) {
    RACE_VIEW_TIMING(0, 0, 0, 0, 0xFF);
    g_pActiveCameraVehicle = &RACE_VIEW_VEHICLES[RACE_VIEW_RECTS[index].vehicleIndex];
    g_pActiveCamera = VEHICLE_GET(g_pActiveCameraVehicle, ActiveCamera);
    g_dwCurrentSplitScreenIndex = index;
    CastRayAgainstTrack(NULL,
                        NULL,
                        &g_pActiveCamera->position,
                        g_awCameraRayHitInstances,
                        &g_dwCameraRayHitCount,
                        g_awCameraRayNearbyInstances,
                        &g_dwCameraRayNearbyCount,
                        &g_fCameraRayGroundDistance,
                        &g_dwCameraRayGroundSurface);
    SetViewport(RACE_VIEW_RECTS[index].x,
                RACE_VIEW_RECTS[index].y,
                RACE_VIEW_RECTS[index].width,
                RACE_VIEW_RECTS[index].height,
                TRUE);
    if (g_dwNumViewports >= 2) {
        /* Split views look narrower and not as far. */
        SetupFrustumCorners(g_pActiveCamera,
                            g_pActiveCamera->fov * 0.7f,
                            g_fCameraProjectionScale * 0.65f,
                            (f32)RACE_VIEW_RECTS[index].width,
                            (f32)RACE_VIEW_RECTS[index].height);
        SetupCameraMatrix(g_pActiveCamera,
                          g_pActiveCamera->fov * 0.7f,
                          g_fCameraProjectionScale * 0.65f,
                          (f32)RACE_VIEW_RECTS[index].width,
                          (f32)RACE_VIEW_RECTS[index].height);
    } else {
        SetupFrustumCorners(g_pActiveCamera,
                            g_pActiveCamera->fov,
                            g_fCameraProjectionScale,
                            (f32)RACE_VIEW_RECTS[index].width,
                            (f32)RACE_VIEW_RECTS[index].height);
        SetupCameraMatrix(g_pActiveCamera,
                          g_pActiveCamera->fov,
                          g_fCameraProjectionScale,
                          (f32)RACE_VIEW_RECTS[index].width,
                          (f32)RACE_VIEW_RECTS[index].height);
    }
    SetupFog();
    SetupWeatherLights();
    RenderRaceScene(RACE_VIEW_ARGUMENTS TRUE);
    RACE_VIEW_TIMING(0, 0x80, 0x80, 0x80, 0xFF);
    {
        s32 i;

        for (i = 0; i < g_dwNumVehicles; i++) {
            if (RACER_SLOT_GET(&RACE_VIEW_SLOTS[i], Vehicle) != NULL) {
                RenderVehicleHeadlightBeams(RACER_SLOT_GET(&RACE_VIEW_SLOTS[i], Vehicle));
            }
        }
    }
    RACE_VIEW_TIMING(0, 0x80, 0x80, 0x80, 0xFF);
    RenderRacerVehicles(RACE_VIEW_ARGUMENTS TRUE);
    RACE_VIEW_TIMING(0, 0x80, 0x80, 0, 0xFF);
    RACE_VIEW_EXHAUST();
    RACE_VIEW_TIMING(0, 0, 0xFF, 0xFF, 0xFF);
    RenderPrecipitationEffect();
    RACE_VIEW_TIMING(0, 0, 0x82, 0, 0xFF);
    RACE_VIEW_GLARE();
    SetupMenuProjection((f32)RACE_VIEW_RECTS[index].width, (f32)RACE_VIEW_RECTS[index].height);
    SetupViewportAndScissor(RACE_VIEW_RECTS[index].x,
                            RACE_VIEW_RECTS[index].y,
                            RACE_VIEW_RECTS[index].width,
                            RACE_VIEW_RECTS[index].height,
                            TRUE);
    if (g_pActiveCamera == &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_BUMPER] &&
        g_dwNumViewports == 1 && g_dwRearViewMirrorMode) {
        RenderRearViewMirror(RACE_VIEW_ARGUMENTS index);
    }
#if TGR_BOSS_RALLY
#ifdef ENABLE_PATCHES
    ViewportRect *hudViews = BeginRaceHudCanvas(views);
    SetupViewportAndScissor(0, 0, GetHudCanvasWidth(), GetHudCanvasHeight(), TRUE);
    DrawFpsCounterOverlay();
    DrawViewOverlay(hudViews, slots, vehicles, index);
    DrawRaceBuildIdentification();
    DrawDebugOverlayBanner(hudViews);
    EndRaceHudCanvas();
#else
    DrawFpsCounterOverlay();
    DrawViewOverlay(RACE_VIEW_ARGUMENTS index);
    DrawRaceBuildIdentification();
#endif /* ENABLE_PATCHES */
#else
    DrawViewOverlay();
#endif
    DrawRdpOutlineRect(RACE_VIEW_RECTS[index].x,
                       RACE_VIEW_RECTS[index].y,
                       RACE_VIEW_RECTS[index].width,
                       RACE_VIEW_RECTS[index].height);
#if TGR_BOSS_RALLY
#ifndef ENABLE_PATCHES
    DrawDebugOverlayBanner(views);
#endif
#endif
}

/* Draw the pause menu over the race. */
void DrawPauseMenu(RACE_VIEW_CONTEXT) {
    s32 x, y;

#if TGR_BOSS_RALLY && defined(ENABLE_PATCHES)
    views = BeginRaceHudCanvas(views);
    SetupViewportAndScissor(0, 0, GetHudCanvasWidth(), GetHudCanvasHeight(), TRUE);
#endif

    x = (RACE_VIEW_RECTS[0].width >> 1) + RACE_VIEW_RECTS[0].x;
#if TGR_BOSS_RALLY
    NullStub();
#else
    DrawScreenFadeOverlay(0.33f);
#endif
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(40);
#if TGR_BOSS_RALLY
    DrawText(GetStringResource(245), x, RACE_VIEW_SCREEN_HEIGHT * 5 / 16);
#else
    DrawText("%ryPAUSED", x, g_dwScreenHeight * 5 / 16);
#endif
    y = RACE_VIEW_SCREEN_HEIGHT * 5 / 11;
    SetTextSize(20);
#if TGR_BOSS_RALLY
    if (g_dwRacePauseState == 2) {
        DrawText(GetStringResource(246), x, y);
        DrawText(GetStringResource(g_dwPauseMenuSelection == 0 ? 247 : 248), x, y + 40);
        DrawText(GetStringResource(g_dwPauseMenuSelection == 1 ? 249 : 250), x, y + 60);
    } else {
        DrawText(GetStringResource(g_dwPauseMenuSelection == 0 ? 251 : 252), x, y);
        if (!g_dwNetworkPlayMode) {
            DrawText(GetStringResource(g_dwPauseMenuSelection == 1 ? 255 : 256), x, y + 20);
        }
        sprintf(g_szRaceHudTextBuffer,
                GetStringResource(257),
                g_dwPauseMenuSelection == 2 ? g_szTextCodeHighlight : g_szTextCodeNormal,
                g_GlobalRallyConfig.dwMusicVolume);
        DrawText(g_szRaceHudTextBuffer, x, y + 40);
        sprintf(g_szRaceHudTextBuffer,
                GetStringResource(258),
                g_dwPauseMenuSelection == 3 ? g_szTextCodeHighlight : g_szTextCodeNormal,
                g_GlobalRallyConfig.dwSfxVolume);
        DrawText(g_szRaceHudTextBuffer, x, y + 60);
        DrawText(GetStringResource(g_dwGameMode == 0 ? (g_dwPauseMenuSelection == 4 ? 259 : 260) :
                                                       (g_dwPauseMenuSelection == 4 ? 261 : 262)),
                 x,
                 y + 80);
        DrawText(GetStringResource(g_dwPauseMenuSelection == 5 ? 263 : 264), x, y + 100);
    }
#else
    DrawText(g_dwPauseMenuSelection == PAUSE_CONTINUE ? "%y1Continue" : "%ryContinue", x, y);
    y += 20;
    DrawText(g_dwPauseMenuSelection == PAUSE_RESTART ? "%y1Restart Race" : "%ryRestart Race", x, y);
    y += 20;
    sprintf(g_szFormatBuffer,
            "%sBGM Volume: %d",
            g_dwPauseMenuSelection == PAUSE_MUSIC ? "%y1" : "%ry",
            g_OptionSettings.musicVolume);
    DrawText(g_szFormatBuffer, x, y);
    y += 20;
    sprintf(g_szFormatBuffer,
            "%sSFX Volume: %d",
            g_dwPauseMenuSelection == PAUSE_SOUND ? "%y1" : "%ry",
            g_OptionSettings.sfxVolume);
    DrawText(g_szFormatBuffer, x, y);
    y += 20;
    DrawText(g_dwPauseMenuSelection == PAUSE_EXIT ? "%y1Exit to Main Menu" : "%ryExit to Main Menu",
             x,
             y);
    /* The original steps past the last item and does not use the result. */
    y += 20;
#endif /* TGR_BOSS_RALLY */
#if TGR_BOSS_RALLY && defined(ENABLE_PATCHES)
    EndRaceHudCanvas();
#endif
}

#undef RACE_VIEW_CONTEXT
#undef RACE_VIEW_PARAMETERS
#undef RACE_VIEW_ARGUMENTS
#undef RACE_VIEW_VEHICLES
#undef RACE_VIEW_SLOTS
#undef RACE_VIEW_RECTS
#undef RACE_VIEW_TIMING
#undef RACE_VIEW_WEATHER_EXCLUDED
#undef RACE_VIEW_SCREEN_WIDTH
#undef RACE_VIEW_SCREEN_HEIGHT
#undef RACE_VIEW_SKY
#undef RACE_VIEW_SMOKE
#undef RACE_VIEW_GEOMETRY
#undef RACE_VIEW_EXHAUST
#undef RACE_VIEW_GLARE
#undef RACE_VIEW_MIRROR_GLARE

/* Show the captions of the credits over the race. */
void DrawCreditCaption(void) {
    s32 lines, top;

    if (0.5f < g_fRaceCaptionTimer) {
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
        for (lines = 0; lines < CAPTION_LINES; lines++) {
            if (g_aCreditCaptions[g_dwRaceCaptionIndex].lines[lines] == NULL) {
                break;
            }
        }
#if TGR_BOSS_RALLY
        top = (s32)g_dwScreenHeight / 2 - lines * 40 / 4 + 10;
#else
        top = 132 - lines * 40 / 4;
#endif
        if (g_aCreditCaptions[g_dwRaceCaptionIndex].title[0] == '\0') {
            top -= 5;
        }
        for (lines--; lines >= 0; lines--) {
#if TGR_BOSS_RALLY
            const char *line = g_aCreditCaptions[g_dwRaceCaptionIndex].lines[lines];

            if (line[0] == '`') {
                SetTextSize(15);
                line++;
            } else {
                SetTextSize(20);
            }
            DrawText(line, (s32)g_dwScreenWidth / 2, lines * 40 / 2 + top);
#else
            SetTextSize(20);
            DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].lines[lines],
                     SCREEN_CENTER_X,
                     lines * 40 / 2 + top);
#endif /* TGR_BOSS_RALLY */
        }
        SetTextSize(15);
#if TGR_BOSS_RALLY
        DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].title, (s32)g_dwScreenWidth / 2, top - 20);
#else
        DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].title, SCREEN_CENTER_X, top - 20);
#endif
    }
#if !TGR_BOSS_RALLY
    g_fRaceCaptionTimer += g_fFrameDeltaTime;
    if (g_aCreditCaptions[g_dwRaceCaptionIndex].duration < g_fRaceCaptionTimer) {
        g_fRaceCaptionTimer = 0.0f;
        g_dwRaceCaptionIndex++;
    }
#endif
}

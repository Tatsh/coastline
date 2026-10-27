#include "raceload.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "audiomix.h"
#include "camera.h"
#include "carselect.h"
#include "driving.h"
#include "effects.h"
#include "fade.h"
#include "game.h"
#include "hud.h"
#include "input.h"
#include "mainmenu.h"
#include "options.h"
#include "physics.h"
#include "race.h"
#include "raceinit.h"
#include "racemenu.h"
#include "racephases.h"
#include "racerender.h"
#include "racespecials.h"
#include "render.h"
#include "replay.h"
#include "rumble.h"
#include "sky.h"
#include "splash.h"
#include "system.h"
#include "text.h"
#include "timing.h"
#include "track.h"
#include "trackrender.h"
#include "trackselect.h"
#include "vecmath.h"
#include "vehicle.h"
#include "vehiclerender.h"
#include "visgrid.h"
#include "weather.h"

#define FADE_SECONDS 0.2f
#define ROM_START_LIGHTS ((u32)_startLightsSegmentRomStart)
#define ROM_START_LIGHTS_END ((u32)_startLightsSegmentRomEnd)
#define AFTER_SEASON_TRACK 10

/* Set the race up, the first frame of its screen. */
void InitRace(void) {
    s32 i;

    g_dwKeepScreenBlanked = TRUE;
#if TGR_REGION == TGR_REGION_US
    osViBlack(TRUE);
    StartLowResFrame();
    FlushDisplayList();
    osViBlack(TRUE);
#else
    BlankDisplay();
    StartLowResFrame();
    FlushDisplayList();
    BlankDisplay();
#endif
    StartLowResFrame();
    FlushDisplayList();
    SetRngSeed(123);
    g_bReplayAfterRace = FALSE;
    if (g_dwGameMode != GAME_MODE_ARCADE) {
        g_dwTotalLaps = 3;
    }
    g_bTrackCollisionFrameToggle = 0;
    SetupRaceVehicles();

    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        g_dwTrackMirrored = FALSE;
    } else {
        g_dwTrackMirrored =
            (g_apTrackMenuItems[g_dwSelectedTrackId]->flags & MENU_ITEM_MIRRORED) != 0;
    }
    if (g_dwGameMode != GAME_MODE_DEMO && !gInstantReplayActive) {
        InitAllRumblePaks();
        LoadAndPlayTrackBgm();
    }
    LoadAllSoundEffects();
    LoadTrackGeometryFromRom(g_dwGameMode == GAME_MODE_AFTER_SEASON ? AFTER_SEASON_TRACK :
                                                                      (s32)g_dwSelectedTrackId);
    if (!gInstantReplayActive) {
        FindTrackSpecials();
    }

    g_dwNumViewports = gInstantReplayActive ? 1 : (s32)g_dwNumLocalPlayers;
    if (gInstantReplayActive) {
        g_aViewportRects[0].vehicleIndex = g_dwBestReplayPlayer;
    } else {
        g_aViewportRects[0].vehicleIndex = 0;
    }
    g_aViewportRects[1].vehicleIndex = g_aViewportRects[0].vehicleIndex == 0;

    if (g_dwGameMode == GAME_MODE_DEMO) {
        g_aVehicles[0].activeCamera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_CHASE];
#if TGR_REGION == TGR_REGION_US
        g_dwReplayCameraSwitchTimer = 180;
#else
        /* The first demonstration race switches its camera at once. */
        if (!g_dwRaceSessionType) {
            g_dwReplayCameraSwitchTimer = 0;
        } else {
            g_dwReplayCameraSwitchTimer = 180;
        }
#endif
    } else {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            g_aVehicles[i].chosenTransmission = g_aVehicles[i].profile->transmission;
            g_aVehicles[i].suspensionStiffness = g_aVehicles[i].profile->suspension;
            g_aVehicles[i].tireType = g_aVehicles[i].profile->tires;
            g_aVehicles[i].steeringMode = g_aVehicles[i].profile->handling;
            g_aVehicles[i].input->controllerLayout = g_aVehicles[i].profile->controllerLayout;
        }
        /* The ghost of a time attack drives with the setup it was recorded with, from the
         * vehicle after the first player whatever the number of players. */
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK && g_aVehicles[1].input->playbackBuffer != NULL) {
            SetupFromRecording(&g_aVehicles[1]);
        } else {
            for (; i < g_dwNumActiveVehicles; i++) {
                g_aVehicles[i].chosenTransmission = 1;
                g_aVehicles[i].suspensionStiffness = 1;
                g_aVehicles[i].tireType = 2;
                g_aVehicles[i].steeringMode = 0;
                g_aVehicles[i].input->controllerLayout = 0;
            }
        }
    }

    if (!gInstantReplayActive) {
        g_dwSavedResultsValid = FALSE;
        for (i = 0; i < g_dwNumActiveVehicles; i++) {
            g_aVehicles[i].opacity = 1.0f;
            if (i < (s32)g_dwNumLocalPlayers) {
                g_aVehicles[i].driver = UpdatePhysicsStep;
                g_aVehicles[i].ghostMode = 0;
            } else if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
                g_aVehicles[i].driver = UpdatePhysicsStep;
                g_aVehicles[i].ghostMode = VEHICLE_GHOST;
                g_aVehicles[i].opacity = 0.375f;
            } else {
                ChooseVehicleCar(&g_aVehicles[i]);
                g_aVehicles[i].driver = DriveAiVehicle;
                g_aVehicles[i].ghostMode = 0;
            }
#if TGR_REGION == TGR_REGION_JP
            /* The players' cars, and the ghost or rival of arcade and time attack, are painted. */
            if (!g_aVehicles[i].customDecals) {
                if (i < (s32)g_dwNumLocalPlayers ||
                    (i == 1 &&
                     (g_dwGameMode == GAME_MODE_ARCADE || g_dwGameMode == GAME_MODE_TIME_ATTACK))) {
                    LoadAndActivateTrackWithPaintColor(&g_aVehicles[i], i, g_aVehicles[i].carModel);
                } else {
                    InstallVehicleModel(&g_aVehicles[i], i, g_aVehicles[i].carModel);
                }
            } else {
                osSyncPrintf("not loading %d\n", i);
            }
#else
            if (!g_aVehicles[i].customDecals) {
                InstallVehicleModel(&g_aVehicles[i], i, g_aVehicles[i].carModel);
            }
#endif
            SetupVehicleForRaceStart(&g_aVehicles[i]);
            g_aVehicles[i].flyMode = FALSE;
            g_aVehicles[i].message = NULL;
            g_aVehicles[i].subMessage = NULL;
        }
    } else {
        for (i = g_dwNumLocalPlayers; i < g_dwNumActiveVehicles; i++) {
            SetMaterialColor(g_aVehicles[i].model,
                             g_aVehicles[i].color[0] >> 3,
                             g_aVehicles[i].color[1] >> 3,
                             g_aVehicles[i].color[2] >> 3);
        }
    }

    /* The replay and the race after a season start racing at once. */
    if (gInstantReplayActive) {
        g_dwRacePhaseIndex = 4;
    } else if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        g_dwRacePhaseIndex = 4;
#if TGR_REGION != TGR_REGION_US
    } else if (g_dwGameMode == GAME_MODE_DEMO && !g_dwRaceSessionType) {
        /* The first demonstration race starts racing at once as well. */
        g_dwRacePhaseIndex = 4;
#endif
    } else {
        g_dwRacePhaseIndex = 0;
    }
    g_dwRacePhase = g_aRacePhaseTable[g_dwRacePhaseIndex].phase;
    g_fRacePhaseTimer = g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
    if (!gInstantReplayActive) {
        g_dwRaceFinishOrderCounter = 0;
        for (i = 0; i < g_dwNumVehicles; i++) {
            InitRacerSlot(&g_aRacerSlots[i]);
        }
    }

    ApplyWeather(g_dwCurrentWeather);
    /* By day on the desert, the airplane stays on the ground. */
    if (!g_dwNightMode &&
        (g_dwSelectedTrackId == TRACK_DESERT || g_dwSelectedTrackId == TRACK_MIRROR_DESERT)) {
        g_dwAirplaneInstance = 0;
    }
    SetFadeTarget(1.0f, FADE_SECONDS);
    FadeSfxTo(1.0f, FADE_SECONDS);
    FadeMusicTo(1.0f, FADE_SECONDS);
    g_dwScreenClearFrames = 2;
    g_dwRaceInitialized = TRUE;
    /* The original takes the buffer as the model and discards the return of the read. */
    g_pStartLightsModel = (Model *)g_aDmaWorkBuffers;
    DmaReadAndFixupTrackModel(g_aDmaWorkBuffers, ROM_START_LIGHTS, ROM_START_LIGHTS_END);
    PlayModelAnimationsOnce(g_pStartLightsModel);
    InitParticleSystem();
    ResetCollisionCaches();
    g_dwRacePauseState = FALSE;
    g_dwRaceExitState = FALSE;
    g_dwPrevViewportCount = -1;
    g_fRaceCaptionTimer = 0.0f;
    g_dwRaceCaptionIndex = 0;
    g_bRaceScreenActive = TRUE;
    if (!gInstantReplayActive) {
        ResetFrameClock();
        ResetAllSoundState();
    }
#if TGR_REGION != TGR_REGION_US
    g_bDemoInterrupted = FALSE;
#endif
    g_dwKeepScreenBlanked = FALSE;
#if TGR_REGION != TGR_REGION_US
    g_dwReplayKeyframeFrameCount = -1;
#endif
}

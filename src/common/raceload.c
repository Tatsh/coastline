#include "common/raceload.h"

#include <PR/gu.h>
#include <PR/os.h>

#include "common/audio.h"
#include "common/audiomix.h"
#include "common/camera.h"
#include "common/driving.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/input.h"
#include "common/physics.h"
#include "common/race.h"
#include "common/raceinit.h"
#include "common/racephases.h"
#include "common/racerender.h"
#include "common/racespecials.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/rumble.h"
#include "common/sky.h"
#include "common/splash.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackmenuitem.h"
#include "common/trackrender.h"
#include "common/trackselect.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"

#if !TGR_BOSS_RALLY
#include <PR/os_libc.h>

#include "carselect.h"
#include "mainmenu.h"
#include "options.h"
#include "racemenu.h"
#endif

#if TGR_BOSS_RALLY
#include <string.h>

#include "common/ghost.h"
#include "common/snapshot.h"
#include "common/trackrelocation.h"
#include "win32/dashboardtextures.h"
#include "win32/frametimer.h"
#include "win32/rallyconfig.h"
#include "win32/render.h"
#include "win32/sound.h"

namespace {
    union StartLightsStorage {
        Model model;
        u8 bytes[0x10000];
    };
    /**
     * @ghidraAddress BRD3D: 0x10680950
     * @ghidraAddress BRGlide: 0x105bcaf8
     */
    StartLightsStorage g_abModelLightsBlobBuffer;
    /**
     * @ghidraAddress BRD3D: 0x100b5d78
     * @ghidraAddress BRGlide: 0x100b5580
     */
    const char g_szMiscModelLightsBlob[] = "misc\\modelLights.blob";
    /**
     * @ghidraAddress BRD3D: 0x106806e0
     * @ghidraAddress BRGlide: 0x105bc888
     */
    [[maybe_unused, gnu::used]] s32 g_dwRaceCaptionLast;
}

void Vehicle::ConfigureRaceControls(bool localPlayer) {
    if (localPlayer) {
        chosenTransmission = profile->transmission;
        suspensionStiffness = profile->suspension;
        tireType = profile->tires;
        steeringMode = profile->handling;
        if (g_GlobalRallyConfig.dwInputDeviceType == 1 ||
            g_GlobalRallyConfig.dwInputDeviceType == 2 ||
            g_GlobalRallyConfig.dwInputDeviceType == 3) {
            input->controllerLayout = 5;
        } else {
            input->controllerLayout = 2;
        }
    } else {
        chosenTransmission = 1;
        suspensionStiffness = 1;
        tireType = 2;
        steeringMode = 0;
        input->controllerLayout = 0;
    }
}

void Vehicle::PrepareRaceVehicle(s32 vehicleIndex) {
    opacity = 1.0f;
    if (vehicleIndex < (s32)g_dwNumLocalPlayers) {
        driver = PlayerVehicleControllerCallback;
        ghostMode = 0;
    } else if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
        driver = PlayerVehicleControllerCallback;
        ghostMode = VEHICLE_GHOST;
        opacity = 0.375f;
    } else {
        if (g_dwGameMode == GAME_MODE_NETWORK) {
            driver = UpdateVehicleSpeedAndPhysics;
        } else {
            ChooseVehicleCar();
            driver = AiVehicleControllerCallback;
        }
        ghostMode = 0;
    }
    if (!customDecals) {
        InstallVehicleModel(vehicleIndex, carModel);
    }
    SetupVehicleForRaceStart();
    flyMode = FALSE;
    message = NULL;
    subMessage = NULL;
}

void Vehicle::PrepareEmptyRaceView(void) {
    SetVehicleModelSlot(0);
    SetupVehicleForRaceStart();
    UpdateCameraSystem();
    cameras[0].position.z += 1.0f;
}

#define AFTER_SEASON_TRACK 12
#else
#define AFTER_SEASON_TRACK 10
#endif /* TGR_BOSS_RALLY */

#define FADE_SECONDS 0.2f
#define ROM_START_LIGHTS ((u32)_startLightsSegmentRomStart)
#define ROM_START_LIGHTS_END ((u32)_startLightsSegmentRomEnd)

/* Set the race up, the first frame of its screen. */
void InitRace(void) {
    s32 i;

    g_dwKeepScreenBlanked = TRUE;
#if TGR_BOSS_RALLY
    NullStub();
    ClearSplitScreenRenderState();
    if (!gInstantReplayActive) {
        NullStub();
        NullStub();
        g_pfnRenderBackendShutdown();
        g_pfnShutdownTextureSubsystem();
        g_pfnInitializeTextureSubsystem();
        LoadTrackTextureMap(g_dwGameMode == GAME_MODE_AFTER_SEASON ? AFTER_SEASON_TRACK :
                                                                     (s32)g_dwSelectedTrackId);
        CreateGlobalEffectTextures();
        g_pfnRenderBackendInit();
        ResetModelDataPool();
    }
    StartLowResFrame(g_aViewportRects);
    NullStub();
    StartLowResFrame(g_aViewportRects);
#else
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
#endif /* TGR_REGION == TGR_REGION_US */
    StartLowResFrame();
    FlushDisplayList();
#endif /* TGR_BOSS_RALLY */
    SetRngSeed(123);
    g_bReplayAfterRace = FALSE;
    if (g_dwGameMode != GAME_MODE_ARCADE
#if TGR_BOSS_RALLY
        && g_dwGameMode != GAME_MODE_TIME_ATTACK && g_dwGameMode != GAME_MODE_NETWORK
#endif
    ) {
        g_dwTotalLaps = 3;
    }
    g_bTrackCollisionFrameToggle = 0;
    SetupRaceVehicles();

    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        g_dwTrackMirrored = FALSE;
    } else {
#if TGR_BOSS_RALLY
        g_dwTrackMirrored =
            (g_apTrackMenuItems[g_dwSelectedTrackId]->item.flags & MENU_ITEM_MIRRORED) != 0;
#else
        g_dwTrackMirrored =
            (g_apTrackMenuItems[g_dwSelectedTrackId]->flags & MENU_ITEM_MIRRORED) != 0;
#endif
    }
#if TGR_BOSS_RALLY
    ApplyWeather(g_dwCurrentWeather);
#endif
    if (g_dwGameMode != GAME_MODE_DEMO && !gInstantReplayActive) {
#if TGR_BOSS_RALLY
        NullStub();
        NullStub();
#else
        InitAllRumblePaks();
        LoadAndPlayTrackBgm();
#endif
    }
    LoadAllSoundEffects();
#if TGR_BOSS_RALLY
    if (!gInstantReplayActive) {
#endif
        LoadTrackGeometryFromRom(g_dwGameMode == GAME_MODE_AFTER_SEASON ? AFTER_SEASON_TRACK :
                                                                          (s32)g_dwSelectedTrackId);
        if (!gInstantReplayActive) {
            FindTrackSpecials();
        }
#if TGR_BOSS_RALLY
    }
#endif

    g_dwNumViewports = gInstantReplayActive ? 1 : (s32)g_dwNumLocalPlayers;
    if (gInstantReplayActive) {
        g_aViewportRects[0].vehicleIndex = g_dwBestReplayPlayer;
    } else {
        g_aViewportRects[0].vehicleIndex = 0;
    }
    g_aViewportRects[1].vehicleIndex = g_aViewportRects[0].vehicleIndex == 0;

    if (g_dwGameMode == GAME_MODE_DEMO) {
        VEHICLE_SET(&g_aVehicles[0],
                    ActiveCamera,
                    &VEHICLE_GET(&g_aVehicles[0], Cameras)[VEHICLE_CAMERA_CHASE]);
#if TGR_REGION == TGR_REGION_US
        g_dwReplayCameraSwitchTimer = 180;
#else
        /* The first demonstration race switches its camera at once. */
        if (!g_dwRaceSessionType) {
            g_dwReplayCameraSwitchTimer = 0;
        } else {
            g_dwReplayCameraSwitchTimer = 180;
        }
#endif /* TGR_REGION == TGR_REGION_US */
    } else {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
#if TGR_BOSS_RALLY
            g_aVehicles[i].ConfigureRaceControls(true);
#else
            g_aVehicles[i].chosenTransmission = g_aVehicles[i].profile->transmission;
            g_aVehicles[i].suspensionStiffness = g_aVehicles[i].profile->suspension;
            g_aVehicles[i].tireType = g_aVehicles[i].profile->tires;
            g_aVehicles[i].steeringMode = g_aVehicles[i].profile->handling;
            g_aVehicles[i].input->controllerLayout = g_aVehicles[i].profile->controllerLayout;
#endif
        }
        /* The ghost of a time attack drives with the setup it was recorded with, from the
         * vehicle after the first player whatever the number of players. */
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK &&
            VEHICLE_GET(&g_aVehicles[1], Input)->playbackBuffer != NULL) {
#if TGR_BOSS_RALLY
            g_aVehicles[1].SetupFromRecording();
#else
            SetupFromRecording(&g_aVehicles[1]);
#endif
        } else {
            for (; i < g_dwNumActiveVehicles; i++) {
#if TGR_BOSS_RALLY
                g_aVehicles[i].ConfigureRaceControls(false);
#else
                g_aVehicles[i].chosenTransmission = 1;
                g_aVehicles[i].suspensionStiffness = 1;
                g_aVehicles[i].tireType = 2;
                g_aVehicles[i].steeringMode = 0;
                g_aVehicles[i].input->controllerLayout = 0;
#endif
            }
        }
    }

    if (!gInstantReplayActive) {
        g_dwSavedResultsValid = FALSE;
        for (i = 0; i < g_dwNumActiveVehicles; i++) {
#if TGR_BOSS_RALLY
            g_aVehicles[i].PrepareRaceVehicle(i);
#else
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
#endif /* TGR_REGION == TGR_REGION_JP */
            SetupVehicleForRaceStart(&g_aVehicles[i]);
            g_aVehicles[i].flyMode = FALSE;
            g_aVehicles[i].message = NULL;
            g_aVehicles[i].subMessage = NULL;
#endif /* TGR_BOSS_RALLY */
        }
#if TGR_BOSS_RALLY
        if (!g_dwNumActiveVehicles) {
            memset(&g_aVehicleModels[0], 0, sizeof(VehicleModel));
            g_aVehicles[0].PrepareEmptyRaceView();
        }
        InitializeDashboardTextures();
#endif
    } else {
#if !TGR_BOSS_RALLY
        for (i = g_dwNumLocalPlayers; i < g_dwNumActiveVehicles; i++) {
            SetMaterialColor(g_aVehicles[i].model,
                             g_aVehicles[i].color[0] >> 3,
                             g_aVehicles[i].color[1] >> 3,
                             g_aVehicles[i].color[2] >> 3);
        }
#endif
    }

    /* The replay and the race after a season start racing at once. */
    if (gInstantReplayActive) {
        g_dwRacePhaseIndex = 4;
    } else if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        g_dwRacePhaseIndex = 4;
#if !TGR_BOSS_RALLY && TGR_REGION != TGR_REGION_US
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
#if TGR_BOSS_RALLY
            g_aRacerSlots[i].InitRacerSlot();
#else
            InitRacerSlot(&g_aRacerSlots[i]);
#endif
        }
    }

#if TGR_BOSS_RALLY
    SetWeatherAndRefreshVehicleMaterials(g_dwCurrentWeather);
#else
    ApplyWeather(g_dwCurrentWeather);
#endif
    /* By day on the desert, the airplane stays on the ground. */
    if (!g_dwNightMode &&
        (g_dwSelectedTrackId == TRACK_DESERT || g_dwSelectedTrackId == TRACK_MIRROR_DESERT)) {
        g_dwAirplaneInstance = 0;
    }
    SetFadeTarget(1.0f, FADE_SECONDS);
    FadeSfxTo(1.0f, FADE_SECONDS);
    FadeMusicTo(1.0f, FADE_SECONDS);
#if !TGR_BOSS_RALLY
    g_dwScreenClearFrames = 2;
#endif
    g_dwRaceInitialized = TRUE;
#if TGR_BOSS_RALLY
    if (!gInstantReplayActive) {
        g_pStartLightsModel = &g_abModelLightsBlobBuffer.model;
        DecompressModel(&g_abModelLightsBlobBuffer, g_szMiscModelLightsBlob);
        PlayModelAnimationsOnce(g_pStartLightsModel);
    }
#else
    /* The original takes the buffer as the model and discards the return of the read. */
    g_pStartLightsModel = (Model *)g_aDmaWorkBuffers;
    DmaReadAndFixupTrackModel(g_aDmaWorkBuffers, ROM_START_LIGHTS, ROM_START_LIGHTS_END);
    PlayModelAnimationsOnce(g_pStartLightsModel);
#endif /* TGR_BOSS_RALLY */
    InitParticleSystem();
    ResetCollisionCaches();
    g_dwRacePauseState = FALSE;
    g_dwRaceExitState = FALSE;
#if TGR_BOSS_RALLY
    g_dwRaceCaptionLast = -1;
#else
    g_dwPrevViewportCount = -1;
#endif
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
#if TGR_BOSS_RALLY
    if (!gInstantReplayActive) {
        ResetGhostRecording();
        if (g_dwGameMode == GAME_MODE_DEMO) {
            StartGhostRecording();
        }
        if (g_bBgmVolume) {
            s32 track;

            if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == 2) {
                track = 12;
            } else if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == 1) {
                track = 13;
            } else {
                track = RandomizeMusicTrack();
            }
            SetCurrentMusicTrack(track);
            DispatchMusicVolumeChange(g_bBgmVolume);
        }
    } else {
        ClearGhostPlaybackState();
    }
    InitFrameSnapshotSlots();
    NullStub();
    ResetFrameTimerState();
#endif /* TGR_BOSS_RALLY */
#if TGR_REGION != TGR_REGION_US
    g_dwReplayKeyframeFrameCount = -1;
#endif
}

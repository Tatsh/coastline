#include "race.h"

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

/* The time the sounds and the screen take to fade, in seconds. */
#define FADE_SECONDS 0.2f

/* The cartridge range of the model of the start lights. */
#define ROM_START_LIGHTS ((u32)_startLightsSegmentRomStart)
#define ROM_START_LIGHTS_END ((u32)_startLightsSegmentRomEnd)

/* The cartridge offsets of the recordings the credits replay. */
#define ROM_CREDITS_RECORDING ((u32)_creditsRecordingSegmentRomStart)
#define ROM_ENDING_RECORDING ((u32)_endingRecordingSegmentRomStart)

/* The track the race after a season is driven on. */
#define AFTER_SEASON_TRACK 10

/* The vehicles of the championship, of which the first three are driven in full. */
#define CHAMPIONSHIP_VEHICLES 20
#define CHAMPIONSHIP_ACTIVE_VEHICLES 3

/* The time the races the computer drives last, in seconds. */
#if TGR_REGION == TGR_REGION_US
#define ATTRACT_RACE_SECONDS 39.6f
#define ENDING_RACE_SECONDS 140.0f
#else
/* The attract race of the later releases lasts as long as its recording. */
#define CREDITS_RACE_SECONDS 42.0f
#define ENDING_RACE_SECONDS 170.0f
#endif
#define AFTER_SEASON_RACE_SECONDS 16.0f

/* The sound effect channels a race uses. */
#define RACE_SOUND_CHANNELS 6

/* The buttons that close the pause menu or end a race the computer drives. */
#define CONFIRM_BUTTONS (INPUT_A | INPUT_START | INPUT_Z)

/* The x of the middle of the screen. */
#define SCREEN_CENTER_X 160

/* The bytes of the end of the recording of the first demonstration race it fades out over. */
#define DEMO_FADE_BYTES 30

/* Fade the screen and the sounds out to leave the race, once. */
static void LeaveRace(void) {
    if (!g_dwRaceExitState) {
        g_dwRaceExitState = TRUE;
        g_bReplayAfterRace = FALSE;
        FadeSfxTo(0.0f, FADE_SECONDS);
        SetFadeTarget(0.0f, FADE_SECONDS);
    }
}

void LoadAndPlayTrackBgm(void) {
    ResetMusicPlaybackState();
    DecompressFromRom(g_abMenuMusicModule, g_adwTrackMusicRom[g_dwSelectedTrackId], NULL);
    InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
}

void StartAttractDemo(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    SetCurrentScreenCallback(ProcessRacingScreen);
}

void StartCreditsRace(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    if (g_dwMultiplayerMode) {
        g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    } else {
        g_dwRaceSessionType = RACE_SESSION_CREDIT_CAPTIONS;
    }
    SetCurrentScreenCallback(ProcessRacingScreen);
}

/* Read the car and its setup from the header of the recording a vehicle replays. */
static void SetupFromRecording(Vehicle *vehicle) {
    vehicle->steeringMode = vehicle->input->playbackBuffer[2];
    vehicle->chosenTransmission = vehicle->input->playbackBuffer[3];
    vehicle->tireType = vehicle->input->playbackBuffer[4];
    vehicle->suspensionStiffness = vehicle->input->playbackBuffer[5];
    vehicle->input->controllerLayout = vehicle->input->playbackBuffer[6];
}

/* Set up the vehicles, the recordings and the replays of the mode of play. */
static void SetupRaceVehicles(void) {
    Vehicle *vehicle;
    PlayerInputState *ghost;
    u8 *recording;
    s32 i;

    switch (g_dwGameMode) {
    case GAME_MODE_CHAMPIONSHIP:
        g_dwNumVehicles = CHAMPIONSHIP_VEHICLES;
        g_dwNumActiveVehicles = CHAMPIONSHIP_ACTIVE_VEHICLES;
        goto replays;
    case GAME_MODE_ARCADE:
        g_dwNumVehicles = 2;
        g_dwNumActiveVehicles = 2;
    replays:
        if (gInstantReplayActive) {
            /* The replay starts from the snapshot of the last lap of the best player. */
            RestoreRaceSnapshot(&g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - g_dwBestReplayPlayer]);
            for (i = 0; i < RACE_SOUND_CHANNELS; i++) {
                g_aSoundEffectChannels[i].volume = 0;
                g_aSoundEffectChannels[i].mixVolume = 0;
            }
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                g_aVehicles[i].input->playbackBuffer = g_apReplayGhostPtrs[i];
                g_aVehicles[i].input->playbackLimit = g_adwReplayGhostSizes[i];
                g_aVehicles[i].input->playbackPosition = 0;
            }
        } else {
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                g_aVehicles[i].input->playbackBuffer = NULL;
                g_aVehicles[i].input->recordBuffers[0] = NULL;
                g_aVehicles[i].input->recordBuffers[1] = NULL;
            }
        }
        g_bReplayAfterRace = gInstantReplayActive == 0;
        break;

    case GAME_MODE_AFTER_SEASON:
        g_dwNumLocalPlayers = 1;
        g_dwNumVehicles = 1;
        g_dwNumActiveVehicles = 1;
        g_bReplayAfterRace = FALSE;
        gInstantReplayActive = FALSE;
        g_aVehicles[0].customDecals = TRUE;
        g_aVehicles[0].activeCamera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_CHASE];
        g_dwReplayCameraSwitchTimer = 0;
        break;

    case GAME_MODE_DEMO:
        g_dwCurrentWeather = 1;
        g_dwNumLocalPlayers = 1;
        g_dwNumVehicles = 1;
        g_dwNumActiveVehicles = 1;
        g_bReplayAfterRace = FALSE;
        g_aVehicles[0].customDecals = FALSE;
        g_aVehicles[0].input->playbackBuffer = g_abPlaybackRecording;
        switch (g_dwRaceSessionType) {
        case RACE_SESSION_ATTRACT:
            g_aVehicles[0].input->playbackLimit = DmaReadAssetSize(ROM_GHOST_RECORDING);
            DecompressFromRom(g_aVehicles[0].input->playbackBuffer, ROM_GHOST_RECORDING, NULL);
            break;
        case RACE_SESSION_CREDIT_CAPTIONS:
            g_aVehicles[0].input->playbackLimit = DmaReadAssetSize(ROM_CREDITS_RECORDING);
            DecompressFromRom(g_aVehicles[0].input->playbackBuffer, ROM_CREDITS_RECORDING, NULL);
            break;
        case RACE_SESSION_SCROLLING_CREDITS:
            CalculateScrollPositions();
            g_aVehicles[0].input->playbackLimit = DmaReadAssetSize(ROM_ENDING_RECORDING);
            DecompressFromRom(g_aVehicles[0].input->playbackBuffer, ROM_ENDING_RECORDING, NULL);
            break;
        }
        g_aVehicles[0].input->playbackPosition = RECORDING_HEADER_SIZE;
        g_aVehicles[0].input->recordBuffers[0] = NULL;
        g_aVehicles[0].input->recordBuffers[1] = NULL;
        vehicle = &g_aVehicles[0];
        vehicle->color[0] = 0xFF;
        vehicle->color[1] = 0xFF;
        vehicle->color[2] = 0xFF;
        g_dwSelectedTrackId = g_aVehicles[0].input->playbackBuffer[0];
        g_aVehicles[0].carModel = g_aVehicles[0].selectedCar =
            g_aVehicles[0].input->playbackBuffer[1];
        SetupFromRecording(&g_aVehicles[0]);
        break;

    case GAME_MODE_TIME_ATTACK:
        /* The strip mine and its mirror are raced in the fog. */
        if (g_dwSelectedTrackId == TRACK_STRIP_MINE ||
            g_dwSelectedTrackId == TRACK_MIRROR_STRIP_MINE) {
            g_dwCurrentWeather = 1;
        } else {
            g_dwCurrentWeather = 0;
        }
        g_dwNumVehicles = g_dwNumActiveVehicles = g_dwNumLocalPlayers + 1;
        ghost = g_aVehicles[g_dwNumLocalPlayers].input;
        g_bReplayAfterRace = FALSE;
        g_dwSaveAvailable = FALSE;
        ghost->playbackBuffer = g_abPlaybackRecording;
        osSyncPrintf("savedRecordingLength = %d\n", g_dwGhostRecordingSize);
        memcpy(g_aVehicles[g_dwNumLocalPlayers].input->playbackBuffer,
               g_abGhostRecording,
               g_dwGhostRecordingSize);
        recording = g_aVehicles[g_dwNumLocalPlayers].input->playbackBuffer;
        g_aVehicles[g_dwNumLocalPlayers].carModel = g_aVehicles[g_dwNumLocalPlayers].selectedCar =
            recording[1];
        if (g_aVehicles[g_dwNumLocalPlayers].input->playbackBuffer[0] != g_dwSelectedTrackId) {
            osSyncPrintf("WRONG TRACK (%d!=%d)\n",
                         g_aVehicles[g_dwNumLocalPlayers].input->playbackBuffer[0],
                         g_dwSelectedTrackId);
            g_aVehicles[g_dwNumLocalPlayers].input->playbackBuffer = NULL;
        } else {
            osSyncPrintf("TRACK=%d\n", g_aVehicles[g_dwNumLocalPlayers].input->playbackBuffer[0]);
            g_aVehicles[g_dwNumLocalPlayers].input->playbackPosition = RECORDING_HEADER_SIZE;
            osSyncPrintf("PLAYLIMIT=%d\n", g_dwGhostRecordingSize);
            g_aVehicles[g_dwNumLocalPlayers].input->playbackLimit = g_dwGhostRecordingSize;
        }
        /* Each player records into a buffer of their own, under the same index. */
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            g_aVehicles[i].input->recordBuffers[i] = g_aabInputRecordings[i];
            g_aVehicles[i].input->recordBuffers[i][0] = g_dwSelectedTrackId;
            g_aVehicles[i].input->recordBuffers[i][1] = g_aVehicles[i].carModel;
            g_aVehicles[i].input->recordBuffers[i][2] = g_aVehicles[i].profile->handling;
            g_aVehicles[i].input->recordBuffers[i][3] = g_aVehicles[i].profile->transmission;
            g_aVehicles[i].input->recordBuffers[i][4] = g_aVehicles[i].profile->tires;
            g_aVehicles[i].input->recordBuffers[i][5] = g_aVehicles[i].profile->suspension;
            g_aVehicles[i].input->recordBuffers[i][6] = g_aVehicles[i].profile->controllerLayout;
            g_aVehicles[i].input->recordPositions[i] = RECORDING_HEADER_SIZE;
            g_aVehicles[i].input->recordLimits[i] = RECORDING_CAPACITY;
        }
        break;

    default:
        g_dwNumVehicles = g_dwNumLocalPlayers;
        g_dwNumActiveVehicles = g_dwNumLocalPlayers;
        g_bReplayAfterRace = FALSE;
        break;
    }
}

/* Set the race up, the first frame of its screen. */
static void InitRace(void) {
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

/* End the instant replay once the recordings it plays run out. */
static void CheckInstantReplayEnd(void) {
    PlayerInputState *input;
    s32 other;

    if (g_dwGameMode != GAME_MODE_ARCADE) {
        input = g_aVehicles[0].input;
        if (input->playbackLimit == input->playbackPosition) {
            LeaveRace();
        }
        return;
    }
    if (g_aViewportRects[0].vehicleIndex == g_dwBestReplayPlayer) {
        input = g_aVehicles[g_dwBestReplayPlayer].input;
        if (input->playbackLimit != input->playbackPosition) {
            return;
        }
        if (g_dwNumLocalPlayers == 1) {
            LeaveRace();
            return;
        }
        /* Once the best recording ends, the view turns to the other player's. */
        if (g_aVehicles[g_dwBestReplayPlayer].input->playbackLimit + 0xF0 !=
            g_aVehicles[g_dwBestReplayPlayer == 0].input->playbackPosition) {
            return;
        }
        g_aViewportRects[1].vehicleIndex = g_dwBestReplayPlayer;
        g_aViewportRects[0].vehicleIndex = g_aViewportRects[1].vehicleIndex == 0;
    }
    other = g_dwBestReplayPlayer == 0;
    if (g_aVehicles[other].input->playbackLimit == g_aVehicles[other].input->playbackPosition) {
        LeaveRace();
    }
}


/* Take the input of the pause menu, and tell whether the race stays paused. */
static s32 ProcessPauseMenu(s32 paused) {
    s32 i;

    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        UpdateStickRepeat(g_aVehicles[i].input);
        if (g_aVehicles[i].input->buttons & INPUT_B) {
            TransferFlags(g_aVehicles[i].input, INPUT_B);
            FadeSfxTo(1.0f, FADE_SECONDS);
            FadeMusicTo(1.0f, FADE_SECONDS);
            g_dwScreenClearFrames = 2;
            paused = FALSE;
        }
        if (g_aVehicles[i].input->buttons & CONFIRM_BUTTONS) {
            /* A that continues the race is left for the race to take. */
            if (g_dwPauseMenuSelection != PAUSE_CONTINUE ||
                !(g_aVehicles[i].input->buttons & INPUT_A)) {
                TransferFlags(g_aVehicles[i].input, CONFIRM_BUTTONS);
            }
            switch (g_dwPauseMenuSelection) {
            case PAUSE_CONTINUE:
                FadeSfxTo(1.0f, FADE_SECONDS);
                FadeMusicTo(1.0f, FADE_SECONDS);
                g_dwScreenClearFrames = 2;
                paused = FALSE;
                break;
            case PAUSE_RESTART:
                SetFadeTarget(0.0f, FADE_SECONDS);
                g_dwRaceExitState = FALSE;
                g_bReplayAfterRace = FALSE;
                g_dwRaceInitialized = FALSE;
                break;
            case PAUSE_EXIT:
                SetFadeTarget(0.0f, FADE_SECONDS);
                FadeSfxTo(0.0f, FADE_SECONDS);
                FadeMusicTo(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = FALSE;
                g_dwRaceExitState = TRUE;
                break;
            }
        }
        if (g_aVehicles[i].input->buttons & INPUT_UP) {
            TransferFlags(g_aVehicles[i].input, INPUT_RIGHT | INPUT_LEFT);
            g_aVehicles[i].input->buttons &= ~(INPUT_RIGHT | INPUT_LEFT);
            TransferFlags(g_aVehicles[i].input, INPUT_UP);
            g_dwPauseMenuSelection = (g_dwPauseMenuSelection + PAUSE_ITEMS - 1) % PAUSE_ITEMS;
        }
        if (g_aVehicles[i].input->buttons & INPUT_DOWN) {
            TransferFlags(g_aVehicles[i].input, INPUT_RIGHT | INPUT_LEFT);
            g_aVehicles[i].input->buttons &= ~(INPUT_RIGHT | INPUT_LEFT);
            TransferFlags(g_aVehicles[i].input, INPUT_DOWN);
            g_dwPauseMenuSelection = (g_dwPauseMenuSelection + 1) % PAUSE_ITEMS;
        }
        if (g_aVehicles[i].input->buttons & INPUT_LEFT) {
            if (g_dwPauseMenuSelection != PAUSE_CONTINUE) {
                TransferFlags(g_aVehicles[i].input, INPUT_LEFT);
            }
            if (g_dwPauseMenuSelection == PAUSE_MUSIC) {
                DecreaseBgmVolume();
            } else if (g_dwPauseMenuSelection == PAUSE_SOUND) {
                DecreaseSfxVolume();
            }
        }
        if (g_aVehicles[i].input->buttons & INPUT_RIGHT) {
            if (g_dwPauseMenuSelection != PAUSE_CONTINUE) {
                TransferFlags(g_aVehicles[i].input, INPUT_RIGHT);
            }
            if (g_dwPauseMenuSelection == PAUSE_MUSIC) {
                IncreaseBgmVolume();
            } else if (g_dwPauseMenuSelection == PAUSE_SOUND) {
                IncreaseSfxVolume();
            }
        }
    }
    DrawPauseMenu();
    return paused;
}

/* Take the buttons of the players while the race runs, and tell whether it is paused. */
static s32 ProcessRaceButtons(s32 paused) {
    s32 i;

    if (g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        if (IsFadingOut()) {
            return paused;
        }
        /* Either controller ends a race the computer drives. */
        for (i = 0; i < 2; i++) {
            if (g_aPlayerInputState[i].buttons & CONFIRM_BUTTONS) {
                TransferFlags(&g_aPlayerInputState[i], CONFIRM_BUTTONS);
                g_bDemoInterrupted = TRUE;
                g_dwRaceExitState = TRUE;
                g_bReplayAfterRace = FALSE;
                FadeSfxTo(0.0f, FADE_SECONDS);
                SetFadeTarget(0.0f, FADE_SECONDS);
            }
        }
        return paused;
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        if (g_aVehicles[i].input->buttons & INPUT_START) {
            if (gInstantReplayActive) {
                SetFadeTarget(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = FALSE;
                g_dwRaceExitState = TRUE;
            } else {
                paused = TRUE;
                g_dwPauseMenuSelection = PAUSE_CONTINUE;
            }
            FadeSfxTo(0.0f, FADE_SECONDS);
            FadeMusicTo(0.0f, FADE_SECONDS);
            TransferFlags(g_aVehicles[0].input, INPUT_START);
            TransferFlags(g_aVehicles[1].input, INPUT_START);
            return paused;
        }
        if (g_aVehicles[i].input->buttons & INPUT_A) {
            /* A restarts the instant replay. */
            if (gInstantReplayActive) {
                SetFadeTarget(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = TRUE;
                g_dwRaceExitState = TRUE;
            }
            TransferFlags(g_aVehicles[0].input, INPUT_A);
            TransferFlags(g_aVehicles[1].input, INPUT_A);
            return paused;
        }
    }
    return paused;
}

/* Choose the instant replay of the best recording of the players. */
static void ChooseInstantReplay(void) {
    s32 i, j;

    g_dwBestReplayLength = g_aVehicles[0].input->recordPositions[0];
    g_dwBestReplayPlayer = 0;
    for (i = 1; i < (s32)g_dwNumLocalPlayers; i++) {
        if (g_aVehicles[i].input->recordPositions[i] < g_dwBestReplayLength) {
            g_dwBestReplayLength = g_aVehicles[i].input->recordPositions[i];
            g_dwBestReplayPlayer = i;
        }
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        g_apReplayGhostPtrs[i] = g_aVehicles[i].input->recordBuffers[g_dwBestReplayPlayer];
        g_adwReplayGhostSizes[i] = g_aVehicles[i].input->recordPositions[g_dwBestReplayPlayer];
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        for (j = 0; j < (s32)g_dwNumLocalPlayers; j++) {
            /* The address of the positions is printed rather than one of them. */
            osSyncPrintf(
                "veh[%d]->recIndex[%d] = %d\n", i, j, g_aVehicles[i].input->recordPositions);
            g_aVehicles[i].input->recordBuffers[j] = NULL;
        }
    }
    osSyncPrintf("bestNumber = %d, bestIndex = %d\n", g_dwBestReplayPlayer, g_dwBestReplayLength);
}

/* Go on from the race once the screen faded out. */
static void FinishRace(void) {
    s32 i, j;

    for (i = 0; i < MAX_VEHICLES; i++) {
        for (j = 0; j < (s32)g_dwNumLocalPlayers; j++) {
            g_aVehicles[i].input->recordLimits[j] = g_aVehicles[i].input->recordPositions[j];
        }
    }
    g_dwSoundEffectEchoOffset = 0;
    for (i = 0; i < RACE_SOUND_CHANNELS; i++) {
        g_aSoundEffectChannels[i].volume = 0;
        g_aSoundEffectChannels[i].mixVolume = 0;
        g_aSoundEffectChannels[i].step = 0;
        g_aSoundEffectChannels[i].sample = g_SilenceSampleData;
    }
    g_bRaceScreenActive = FALSE;
    if (g_dwRaceExitState) {
        if (g_bReplayAfterRace) {
            if (!gInstantReplayActive) {
                ChooseInstantReplay();
            }
            gInstantReplayActive = TRUE;
        } else {
            ClearFramebuffersHighRes();
            g_dwTrackMirrored = FALSE;
            if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
                if (g_aVehicles[0].profile->season) {
                season:
                    /* After the race of a season or the credits, the championship goes on. */
                    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
                    SetTrackFromSeason();
                    g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_SEASON;
                    SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
                } else {
                    /* The last season ends with the credits. */
                    StartEndingCredits();
                }
            } else if (g_dwGameMode == GAME_MODE_DEMO &&
                       g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS) {
                /* The credits share the return to the championship of the race after a season. */
                goto season;
            } else if (g_dwSavedResultsValid &&
                       (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_ARCADE ||
                        g_dwGameMode == GAME_MODE_CHAMPIONSHIP)) {
                RestoreRaceResultsFromSlots();
                SetCurrentScreenCallback(ProcessRaceResultsScreen);
            } else if (g_dwGameMode == GAME_MODE_DEMO &&
                       g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS) {
                SetCurrentScreenCallback(ProcessOptionsMenu);
            } else if (g_dwGameMode == GAME_MODE_DEMO &&
                       g_dwRaceSessionType == RACE_SESSION_ATTRACT && !g_bDemoInterrupted) {
                SetCurrentScreenCallback(ProcessSplashScreens);
            } else {
                SetCurrentScreenCallback(ProcessMainMenu);
            }
            gInstantReplayActive = FALSE;
        }
        g_dwRaceInitialized = FALSE;
    }
    g_wRaceScreenLoopExited = TRUE;
    for (i = 0; i < MAX_VEHICLES; i++) {
        g_aVehicles[i].input->playbackBuffer = NULL;
    }
}

void ProcessRacingScreen(void) {
    s32 paused, i;

    if (!g_dwRaceInitialized) {
        InitRace();
    }
    g_wRaceScreenLoopExited = FALSE;
#if TGR_REGION != TGR_REGION_US
    RestoreReplayKeyframeState();
#endif
    if (g_dwRacePhase == RACE_PHASE_RACING) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            SaveLastLapSnapshot(&g_aVehicles[i]);
        }
    }
    if (g_dwGameMode != GAME_MODE_DEMO) {
        g_bTrackCollisionFrameToggle ^= 1;
    }
    if (gInstantReplayActive && g_dwRacePhase == RACE_PHASE_RACING) {
        CheckInstantReplayEnd();
    }

    UpdateFrameTiming();
    g_dwStartLightsAnimation = 0;
    paused = g_dwRacePauseState;
    g_bStartLightsVisible = FALSE;
    StepRacePhases();

    AddTimingMarker(0, 0, 0, 0xC8, 0xFF);
    ClearPerFrameVariables();
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        ClearCameraChanged(&g_aVehicles[i]);
    }
    for (i = 0; i < g_dwNumVehicles; i++) {
        UpdateRacerSlotMotion(&g_aRacerSlots[i]);
    }
    for (i = 0; i < g_dwNumVehicles; i++) {
        UpdateRacerSlotRaceState(&g_aRacerSlots[i]);
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            ManageAiRacerSlots(&g_aVehicles[i]);
        }
    }
    SortRacersByPosition();
    if (!g_dwRacePauseState) {
        AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
        UpdateAllVehicleEffects();
        AddTimingMarker(0, 0, 0xFF, 0xFF, 0xFF);
        UpdatePrecipitationParticles();
        UpdateTrackSpecials();
    }

    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        UpdateVehicleSounds(&g_aVehicles[i]);
    }
    ResetSoundSourceTracking();
    for (i = 0; i < g_dwNumViewports; i++) {
        g_pActiveCameraVehicle = &g_aVehicles[g_aViewportRects[i].vehicleIndex];
        g_pActiveCamera = g_pActiveCameraVehicle->activeCamera;
        if (g_dwAirplaneInstance && g_bAirplaneFlying) {
            UpdateAirplaneSound(g_dwAirplaneInstance,
                                (Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[3],
                                g_pActiveCamera);
        }
        if (g_pActiveCamera != NULL) {
            s32 j;

            for (j = 0; j < g_dwWaterfallCount; j++) {
                /* The waterfalls are heard under the source of the airplane. */
                UpdateWaterfallSound(
                    g_dwAirplaneInstance,
                    (Vec3f *)g_LoadedTrack.instances[g_adwWaterfallInstances[j]].matrix[3],
                    g_pActiveCamera);
                if (g_pActiveCamera == NULL) {
                    break;
                }
            }
        }
        StubSoundUpdate(g_pActiveCamera);
    }
    UpdateAmbientSoundChannel();

    UpdateFadeTransitions();
    StartLowResFrame();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    ResetShadowBuffers();
    ClearDepthBuffer();
    if (g_dwScreenClearFrames || g_dwSkyboxDisabled) {
        ClearScreen(0, 0, 0);
        if (g_dwScreenClearFrames) {
            g_dwScreenClearFrames--;
        }
    } else if (g_aViewportRects[0].width < 0x130) {
        /* Black out the screen right of a narrow view. */
        DrawFilledRect(g_aViewportRects[0].width + g_aViewportRects[0].x,
                       8,
                       0x130 - g_aViewportRects[0].width,
                       0xE0,
                       0,
                       0,
                       0);
    }
    if (g_dwNumViewports != g_dwPrevViewportCount) {
        g_dwScreenClearFrames = 2;
        g_dwPrevViewportCount = g_dwNumViewports;
    }
    for (i = 0; i < g_dwNumViewports; i++) {
        RenderRaceView(i);
    }
    StubEmpty();
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
#if TGR_REGION != TGR_REGION_US
    /* The cheat finishes the race, the first player first and the second second. */
    if (g_dwCheatFinishRacePending) {
        g_aVehicles[0].racePosition = 0;
        g_aVehicles[0].lap = g_dwTotalLaps;
        g_aRacerSlots[0].status &= RACER_HELD;
        g_aRacerSlots[0].status |= RACER_FINISHED;
        g_aVehicles[1].racePosition = 1;
        g_aVehicles[1].lap = g_dwTotalLaps;
        g_aRacerSlots[1].status &= RACER_HELD;
        g_aRacerSlots[1].status |= RACER_FINISHED;
        g_bReplayAfterRace = FALSE;
        if (g_dwRacePhase < RACE_PHASE_RACING) {
            g_dwRacePhase = RACE_PHASE_RACING;
        }
        g_dwCheatFinishRacePending = FALSE;
    }
#endif

    if (g_dwRacePauseState) {
        paused = ProcessPauseMenu(paused);
    } else {
        paused = ProcessRaceButtons(paused);
    }
    if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS) {
        ScrollTextUp();
        DrawScrollingText();
    } else if (g_dwGameMode == GAME_MODE_DEMO &&
               g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS &&
               g_aCreditCaptions[g_dwRaceCaptionIndex].title != NULL) {
        DrawCreditCaption();
    }
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        DrawTimingBars();
    }
    AddTimingMarker(0, 0xFF, 0xE0, 0, 0xFF);
    StreamAnimatedTrackTextures();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    SetNextScreenCallback(UpdateRaceAnimations);
#if TGR_REGION != TGR_REGION_US
    /* The first demonstration race fades to black over the end of its recording. */
    if (g_dwGameMode == GAME_MODE_DEMO && !g_dwRaceSessionType &&
        g_aVehicles[0].input->playbackPosition >=
            g_aVehicles[0].input->playbackLimit - DEMO_FADE_BYTES) {
#if TGR_REGION == TGR_REGION_JP
        /* The Japanese release clears to black at once. */
        ClearScreen(0, 0, 0);
#else
        DrawScreenFadeOverlay(
            (f32)(g_aVehicles[0].input->playbackLimit - g_aVehicles[0].input->playbackPosition) /
            DEMO_FADE_BYTES);
#endif
    }
#endif
    FlushDisplayList();
    if (IsFadingOut() && IsFadeComplete()) {
        FinishRace();
    }

    if (g_dwRacePauseState != (u32)paused) {
        if (paused) {
            SaveClockForPause();
        } else {
            RestoreClockAfterPause();
        }
        g_dwRacePauseState = paused;
    }
}

/* Add a line of the bonus of a finished championship, after a blank line when others precede it. */
#define ADD_BONUS_LINE(line)                                                                       \
    {                                                                                              \
        if (count) {                                                                               \
            g_apszSeasonResultMsgLines[count++] = "";                                              \
        }                                                                                          \
        g_apszSeasonResultMsgLines[count++] = line;                                                \
    }

void ProcessSeasonResults(void) {
    s32 count, i, race, base;

    count = 0;
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && g_dwSavedResultsValid) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            g_aVehicles[i].profile->round++;
            if (g_aVehicles[i].profile->round ==
                g_SeasonTable[g_aVehicles[i].profile->season].raceCount) {
                g_aVehicles[i].profile->round = 0;
                if (g_aVehicles[i].profile->seasonPoints[g_aVehicles[i].profile->season] >=
                    g_SeasonTable[g_aVehicles[i].profile->season].pointsNeeded) {
                    g_aVehicles[i].profile->unlocks |=
                        g_SeasonTable[g_aVehicles[i].profile->season].unlocks;
                    g_aVehicles[i].profile->season++;
                    if (g_aVehicles[i].profile->season == SEASONS) {
                        g_apszSeasonResultMsgLines[count++] = "ALL SEASONS COMPLETED!";
                        /* Winning every race of the last season adds the strip mine. */
                        for (race = 0; race < g_SeasonTable[SEASONS - 1].raceCount; race++) {
                            if (g_aVehicles[i].profile->seasonPlaces[SEASONS - 1][race] != 0) {
                                goto cars;
                            }
                        }
                        if (g_aVehicles[i].profile->championshipFlags & 1) {
                            if (!(g_aVehicles[i].profile->unlockedTracks &
                                  (1 << TRACK_MIRROR_STRIP_MINE))) {
                                ADD_BONUS_LINE("You Won all races this season!");
                                g_apszSeasonResultMsgLines[count++] =
                                    "Bonus: Mirror Mine Track Added!";
                                g_aVehicles[i].profile->unlockedTracks |=
                                    1 << TRACK_MIRROR_STRIP_MINE;
                            }
                        } else if (!(g_aVehicles[i].profile->unlockedTracks &
                                     (1 << TRACK_STRIP_MINE))) {
                            ADD_BONUS_LINE("You Won all races this season!");
                            g_apszSeasonResultMsgLines[count++] = "Bonus: Mine Track Added!";
                            g_aVehicles[i].profile->unlockedTracks |= 1 << TRACK_STRIP_MINE;
                        }
                    cars:
                        /* Each championship of the years unlocks a car. */
                        if (g_aVehicles[i].profile->championshipFlags == 0) {
                            ADD_BONUS_LINE("Bonus: Milk Truck Added!");
                            g_aVehicles[i].profile->unlocks |= 0x400;
                        } else if (g_aVehicles[i].profile->championshipFlags == 1) {
#if TGR_REGION == TGR_REGION_JP
                            ADD_BONUS_LINE("Bonus: Type-MN Car Added!");
#else
                            ADD_BONUS_LINE("Bonus: Helmet Car Added!");
#endif
                            g_aVehicles[i].profile->unlocks |= 0x200;
                        } else if (g_aVehicles[i].profile->championshipFlags == 2) {
                            ADD_BONUS_LINE("Bonus: Cupra Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x800;
                        } else if (g_aVehicles[i].profile->championshipFlags == 3) {
                            ADD_BONUS_LINE("Bonus: Beach Ball Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x1000;
                        } else if (g_aVehicles[i].profile->championshipFlags == 4) {
                            ADD_BONUS_LINE("Bonus: Mirrored Cars Added!");
                            g_aVehicles[i].profile->unlocks |= 0x8000;
                        }
                        g_aVehicles[i].profile->season = 0;
                        g_aVehicles[i].profile->championshipFlags++;
                    } else {
#if TGR_REGION == TGR_REGION_JP
                        g_apszSeasonResultMsgLines[count++] = "SEASON COMPLETE";
#else
                        g_apszSeasonResultMsgLines[count++] = "SEASON COMPLETED!";
                        g_apszSeasonResultMsgLines[count++] = "";
                        g_apszSeasonResultMsgLines[count++] = "PROGRESS TO THE NEXT SEASON";
#endif
                    }
                } else {
#if TGR_REGION == TGR_REGION_JP
                    g_apszSeasonResultMsgLines[count++] = "SEASON TRY AGAIN";
#else
                    g_apszSeasonResultMsgLines[count++] = "Sorry, you don't have enough points to";
                    g_apszSeasonResultMsgLines[count++] =
                        "pass this season.  You'll have to try again.";
#endif
                }
                g_aVehicles[i].profile->seasonPoints[g_aVehicles[i].profile->season] = 0;
            }
            /* The track and the weather of the next race become available. */
            base = g_aVehicles[i].profile->championshipFlags & 1 ? TRACK_MIRRORED : 0;
            g_aVehicles[i].profile->unlockedTracks |=
                1 << (g_SeasonTable[g_aVehicles[i].profile->season]
                          .races[g_aVehicles[i].profile->round][0] +
                      base);
            g_aVehicles[i].profile->unlockedWeathers |=
                1 << g_SeasonTable[g_aVehicles[i].profile->season]
                         .races[g_aVehicles[i].profile->round][1];
        }
    }
    g_dwSeasonResultMsgLineCount = count;
}

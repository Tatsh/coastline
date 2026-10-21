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
#define ATTRACT_RACE_SECONDS 39.6f
#define ENDING_RACE_SECONDS 140.0f
#define AFTER_SEASON_RACE_SECONDS 16.0f

/* The sound effect channels a race uses. */
#define RACE_SOUND_CHANNELS 6

/* The buttons that close the pause menu or end a race the computer drives. */
#define CONFIRM_BUTTONS (INPUT_A | INPUT_START | INPUT_Z)

/* The x of the middle of the screen. */
#define SCREEN_CENTER_X 160

/* Fade the screen and the sounds out to leave the race, once. */
static void LeaveRace(void) {
    if (!g_dwRaceExitState) {
        g_dwRaceExitState = TRUE;
        g_bReplayAfterRace = FALSE;
        FadeSfxTo(0.0f, FADE_SECONDS);
        SetFadeTarget(0.0f, FADE_SECONDS);
    }
}

/* Go on to the next phase of the race. */
static void AdvanceRacePhase(void) {
    g_dwRacePhaseIndex++;
    g_dwRacePhase = g_aRacePhaseTable[g_dwRacePhaseIndex].phase;
    g_fRacePhaseTimer = g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
}

void LoadAndPlayTrackBgm(void) {
    ResetMusicPlaybackState();
    DecompressFromRom(g_abMenuMusicModule, g_adwTrackMusicRom[g_dwSelectedTrackId], NULL);
    InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
}

/* The original saves the frame time only when the lights are held. A step that holds the lights
 * restores an uninitialised value. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
void AnimateStartLights(void) {
    f32 delta;

    if (g_dwStartLightsAnimation == 0) {
        return;
    }
    if (g_dwStartLightsAnimation == -1) {
        /* Held, the lights take their first pose without moving on. */
        delta = g_fFrameDeltaTime;
        g_fFrameDeltaTime = 0.0f;
    }
    AdvanceModelAnimations(g_pStartLightsModel);
    if (g_dwStartLightsAnimation == -1) {
        g_fFrameDeltaTime = delta;
    }
}
#pragma GCC diagnostic pop

void UpdateRaceAnimations(void) {
    if (g_dwRacePauseState) {
        return;
    }
    AddTimingMarker(0, 0x80, 0x80, 0xF0, 0xFF);
    AnimateStartLights();
    AddTimingMarker(0, 0, 0, 0xC0, 0xFF);
    MoveTireSpray();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
}

void RenderStartLights(void) {
    TrackSection *start;
    f32 m[4][4];

    if (!g_bStartLightsVisible || (start = g_LoadedTrack.rootSection) == NULL) {
        return;
    }
    /* Face the lights across the track from the right edge of its first point. */
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[3][3] = 1.0f;
    m[2][2] = 1.0f;
    Vec3fDirection((Vec3f *)m[1], &start->points[0].position, &start->points[0].right);
    Vec3fCross((Vec3f *)m[0], (Vec3f *)m[1], (Vec3f *)m[2]);
    Vec3fCross((Vec3f *)m[1], (Vec3f *)m[2], (Vec3f *)m[0]);
    start = g_LoadedTrack.rootSection;
    m[3][0] = start->points[0].position.x;
    m[3][1] = start->points[0].position.y;
    m[3][2] = start->points[0].position.z + 2.0f + g_fStartLightsDrop;
    Vec3fMulAdd((Vec3f *)m[3], (Vec3f *)m[3], (Vec3f *)m[0], -0.3f);
    Vec3fMulAdd((Vec3f *)m[3], (Vec3f *)m[3], (Vec3f *)m[1], -0.6f);
    gSPMatrix(gDisplayListPtr++, &g_mtxIdentity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    guScaleF(g_afMvpMatrix, 1.0f / 1024.0f, 1.0f / 1024.0f, 1.0f / 1024.0f);
    guMtxCatF(g_afMvpMatrix, m, m);
    DrawModelList(g_pStartLightsModel, m);
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

void StartEndingCredits(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    SetCurrentScreenCallback(ProcessRacingScreen);
}

void CalculateScrollPositions(void) {
    CreditLine *line;
    s32 y, above;

    y = 240;
    for (line = g_aCreditScrollLines; line->text != NULL; line++) {
        /* A line sits on its baseline, three quarters of its size down. */
        above = line->size * 3 / 4;
        y += above;
        line->y = y;
        y = y + line->size - above + line->gap;
    }
}

void ScrollTextUp(void) {
    CreditLine *line;

    for (line = g_aCreditScrollLines; line->text != NULL; line++) {
        line->y--;
    }
}

void DrawScrollingText(void) {
    CreditLine *line;

    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetScissorRect(g_aViewportRects[0].x,
                   g_aViewportRects[0].y,
                   g_aViewportRects[0].width,
                   g_aViewportRects[0].height);
    for (line = g_aCreditScrollLines; line->text != NULL; line++) {
        if (line->y >= -0x4F && line->y < 0x118) {
            SetTextSize(line->size);
            DrawText(line->text, SCREEN_CENTER_X, line->y);
        }
    }
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

/* Find the special objects of the track: the airplane, its path and trigger, and the waterfalls. */
static void FindTrackSpecials(void) {
    Vec3f axis;
    s32 i;

    g_bAirplaneFlying = FALSE;
    g_dwAirplaneTrigger = 0;
    g_dwAirplaneInstance = 0;
    g_pAirplanePathRight = NULL;
    g_pAirplanePathLeft = NULL;
    g_fAirplaneDistance = 0.0f;
    g_fAirplaneSegmentLength = 0.0f;
    g_dwAirplaneNode = 0;
    g_dwAirplaneNodeCount = 0;
    osSyncPrintf("specials: %d\n", g_LoadedTrack.specialCount);
    g_dwWaterfallCount = 0;
    for (i = 0; i < g_LoadedTrack.specialCount; i++) {
        switch (g_LoadedTrack.specials[i].type) {
        case TRACK_SPECIAL_AIRPLANE:
            g_dwAirplaneInstance = g_LoadedTrack.specials[i].target.instance;
            osSyncPrintf("airplane = %d\n", g_dwAirplaneInstance);
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_LEFT:
            g_dwAirplaneNodeCount = g_LoadedTrack.specials[i].param.pathLength;
            g_pAirplanePathLeft = g_LoadedTrack.specials[i].target.path;
            osSyncPrintf("airplanePathLeft = %08x\n", g_pAirplanePathLeft);
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_RIGHT:
            g_pAirplanePathRight = g_LoadedTrack.specials[i].target.path;
            osSyncPrintf("airplanePathRight = %08x\n", g_pAirplanePathRight);
            break;
        case TRACK_SPECIAL_AIRPLANE_TRIGGER:
            g_dwAirplaneTrigger = g_LoadedTrack.specials[i].target.instance;
            osSyncPrintf("airplanetrigger = %d\n", g_dwAirplaneTrigger);
            break;
        case TRACK_SPECIAL_WATERFALL:
            g_adwWaterfallInstances[g_dwWaterfallCount] = g_LoadedTrack.specials[i].target.instance;
            g_dwWaterfallCount++;
            osSyncPrintf("waterfall = %d\n", g_LoadedTrack.specials[i].target.instance);
            break;
        }
    }
    if (!g_dwAirplaneTrigger || !g_pAirplanePathLeft || !g_pAirplanePathRight) {
        g_dwAirplaneInstance = 0;
    }
    if (g_dwAirplaneInstance) {
        /* The scale of the model is the length its matrix gives a unit vector. */
        axis.x = 1.0f;
        axis.y = 0.0f;
        axis.z = 0.0f;
        Mtx4x4ProjectPoint(&axis, &axis, g_LoadedTrack.instances[g_dwAirplaneInstance].matrix);
        g_fAirplaneScale = Vec3fLength(&axis);
    }
    g_dwAirplaneTriggerLap = g_dwTotalLaps - 1;
}

/* Set the race up, the first frame of its screen. */
static void InitRace(void) {
    s32 i;

    g_dwKeepScreenBlanked = TRUE;
    osViBlack(TRUE);
    StartLowResFrame();
    FlushDisplayList();
    osViBlack(TRUE);
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
        g_dwReplayCameraSwitchTimer = 180;
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
            if (!g_aVehicles[i].customDecals) {
                InstallVehicleModel(&g_aVehicles[i], i, g_aVehicles[i].carModel);
            }
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
    g_dwKeepScreenBlanked = FALSE;
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

/* Hold the vehicles and run the start lights before the race starts. */
static void HoldBeforeStart(void) {
    TrackMenuItem *track;
    Vehicle *vehicle;
    s16 weather;
    s32 i;

    g_bStartLightsVisible = TRUE;
    for (i = 0; i < g_dwNumVehicles; i++) {
        g_aRacerSlots[i].status |= RACER_HELD;
        vehicle = g_aRacerSlots[i].vehicle;
        if (vehicle == NULL || vehicle->index >= (s32)g_dwNumLocalPlayers) {
            continue;
        }
        if (g_dwGameMode == GAME_MODE_ARCADE) {
            weather = g_dwCurrentWeather - 1;
            if (weather >= ARCADE_WEATHERS || weather < 0) {
                weather = 0;
            }
            /* The descriptor of a track starts with its menu item. */
            track = (TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId];
            vehicle->timeLeft = track->arcadeTimes[vehicle->tableIndex][weather].startTime;
        }
        vehicle->messageTime = 1.0f;
        /* The countdown runs once for each local player, so two players step it twice a frame. */
        switch (g_dwRacePhase) {
        case RACE_PHASE_HOLD:
            g_dwStartLightsAnimation = -1;
            g_dwStartLightsStep = 0;
            break;
        case RACE_PHASE_WAIT:
            /* The original tests for the phase and does nothing in it. */
            break;
        case RACE_PHASE_COUNTDOWN:
            g_dwStartLightsAnimation = 1;
            if (g_fRacePhaseTimer < g_afStartLightThresholds[g_dwStartLightsStep]) {
                g_dwStartLightsStep++;
                if (g_dwStartLightsStep == 4) {
                    PlayGoSound();
                } else {
                    PlayBeepSound();
                }
            }
            break;
        }
    }
    g_fStartLightsDrop = 0.0f;
}

/* Keep a time attack's recording of the first player when it beats the ghost, or when there is
 * none worth keeping. */
static void KeepTimeAttackRecording(s32 player) {
    s32 j;

    osSyncPrintf("# Format SB BitSize 24 Rows %d Columns 1\n",
                 g_aVehicles[player].input->recordPositions[player] >> 1);
    for (j = 0; j < g_aVehicles[player].input->recordPositions[player]; j += 2) {
        osSyncPrintf("%.02x %.02x\n",
                     g_aVehicles[player].input->recordBuffers[player][j],
                     g_aVehicles[player].input->recordBuffers[player][j + 1]);
    }
    if (player == 0) {
        if (g_aVehicles[player].input->recordPositions[player] < (s32)g_dwGhostRecordingSize ||
            g_abGhostRecording[0] != g_dwSelectedTrackId ||
            (s32)g_dwGhostRecordingSize <= RECORDING_HEADER_SIZE) {
            osSyncPrintf("%d: %d<%d=%d || %d!=%d=%d || %d<=%d=%d\n",
                         player,
                         g_aVehicles[player].input->recordPositions[player],
                         g_dwGhostRecordingSize,
                         g_aVehicles[player].input->recordPositions[player] <
                             (s32)g_dwGhostRecordingSize,
                         g_abGhostRecording[0],
                         g_dwSelectedTrackId,
                         g_abGhostRecording[0] != g_dwSelectedTrackId,
                         g_dwGhostRecordingSize,
                         RECORDING_HEADER_SIZE,
                         (s32)g_dwGhostRecordingSize <= RECORDING_HEADER_SIZE);
            g_dwGhostRecordingSize = g_aVehicles[player].input->recordPositions[player];
            memcpy(g_abGhostRecording,
                   g_aVehicles[player].input->recordBuffers[player],
                   g_dwGhostRecordingSize);
            g_aVehicles[player].message = "NEW RECORD!";
            g_aVehicles[player].messageTime = 1.0f;
            FormatLapTime(g_aVehicles[player].messageText, g_aVehicles[player].totalTime);
            g_aVehicles[player].subMessage = g_aVehicles[player].messageText;
            g_aVehicles[player].subMessageTime = 1.0f;
            g_dwSaveAvailable = TRUE;
        }
    }
    g_aVehicles[player].input->recordBuffers[player] = NULL;
}

/* Run the race, and tell whether to go on to the next phase. */
static s32 RunRacingPhase(void) {
    s32 allFinished, i, j;

    allFinished = TRUE;
    if ((g_dwGameMode == GAME_MODE_DEMO &&
         ((g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS &&
           ENDING_RACE_SECONDS < g_aVehicles[0].totalTime) ||
          (g_dwRaceSessionType != RACE_SESSION_SCROLLING_CREDITS &&
           ATTRACT_RACE_SECONDS < g_aVehicles[0].totalTime))) ||
        (g_dwGameMode == GAME_MODE_AFTER_SEASON &&
         AFTER_SEASON_RACE_SECONDS < g_aVehicles[0].totalTime)) {
        if (!IsFadingOut()) {
            SetFadeTarget(0.0f, FADE_SECONDS);
            FadeSfxTo(0.0f, FADE_SECONDS);
            g_dwRaceExitState = TRUE;
            gInstantReplayActive = FALSE;
            g_bReplayAfterRace = FALSE;
            allFinished = FALSE;
        }
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        if (!(g_aRacerSlots[i].status & RACER_FINISHED)) {
            allFinished = FALSE;
            continue;
        }
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK &&
            g_aVehicles[i].input->recordBuffers[i] != NULL) {
            KeepTimeAttackRecording(i);
        } else if (g_dwGameMode == GAME_MODE_ARCADE) {
            /* A finished player records no further. */
            for (j = 0; j < (s32)g_dwNumLocalPlayers; j++) {
                g_aVehicles[i].input->recordLimits[j] = g_aVehicles[i].input->recordPositions[j];
            }
        }
    }
    if (allFinished) {
        return TRUE;
    }
    if (g_dwGameMode == GAME_MODE_ARCADE && g_dwNumLocalPlayers == 1 &&
        (f64)g_aVehicles[0].timeLeft == 0.0) {
        g_bReplayAfterRace = FALSE;
        g_aVehicles[1].message = g_aVehicles[0].message = "%ryTIME UP!";
        g_aVehicles[1].messageTime = 2.0f;
        g_aVehicles[0].messageTime = g_aVehicles[1].messageTime;
        return TRUE;
    }
    return FALSE;
}

/* Step the phases of the race. */
static void StepRacePhases(void) {
    s32 i;

    /* The phases share the countdown of the timer and the step to the next phase. */
    if (g_dwRacePhase < RACE_PHASE_GO) {
        HoldBeforeStart();
    tick:
        if (!g_dwRacePauseState) {
            g_fRacePhaseTimer -= g_fFrameDeltaTime;
            if (g_fRacePhaseTimer < 0.0f) {
            advance:
                AdvanceRacePhase();
            }
        }
    } else if (g_dwRacePhase == RACE_PHASE_GO) {
        g_dwStartLightsAnimation = 1;
        g_bStartLightsVisible = TRUE;
        for (i = 0; i < g_dwNumVehicles; i++) {
            g_aRacerSlots[i].status &= ~RACER_HELD;
        }
        /* The lights rise away faster and faster through the phase. */
        g_fStartLightsDrop = (g_aRacePhaseTable[g_dwRacePhaseIndex].seconds - g_fRacePhaseTimer) /
                             g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
        g_fStartLightsDrop = g_fStartLightsDrop * (15.0f * g_fStartLightsDrop);
        goto tick;
    } else if (g_dwRacePhase == RACE_PHASE_RACING) {
        if (RunRacingPhase()) {
            goto advance;
        }
    } else if (g_dwRacePhase == RACE_PHASE_FINISHED) {
        if ((g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_ARCADE ||
             g_dwGameMode == GAME_MODE_CHAMPIONSHIP) &&
            !gInstantReplayActive) {
            SaveRaceResultsToSlots();
        }
        goto advance;
    } else if (g_dwRacePhase == RACE_PHASE_AFTER) {
        goto tick;
    } else if (g_dwRacePhase == RACE_PHASE_LEAVE) {
        if (!g_dwRaceExitState) {
            FadeSfxTo(0.0f, FADE_SECONDS);
            if (!g_bReplayAfterRace) {
                FadeMusicTo(0.0f, FADE_SECONDS);
            }
            SetFadeTarget(0.0f, FADE_SECONDS);
            g_dwRaceExitState = TRUE;
        }
    }
}

/* Fly the airplane along its path and orient it along the legs of the path. */
static void FlyAirplane(void) {
    Vec3f a, b, c;
    f32 t;

    if (g_dwSelectedTrackId == TRACK_MOUNTAIN || g_dwSelectedTrackId == TRACK_MIRROR_MOUNTAIN) {
        g_fAirplaneDistance += g_fFrameDeltaTime * 18.0f;
    } else {
        g_fAirplaneDistance += g_fFrameDeltaTime * 50.0f;
    }
    while (g_fAirplaneSegmentLength < g_fAirplaneDistance) {
        g_fAirplaneDistance -= g_fAirplaneSegmentLength;
        g_dwAirplaneNode++;
        if (g_dwAirplaneNode >= g_dwAirplaneNodeCount) {
            g_dwAirplaneInstance = 0;
            break;
        }
        Vec3fMidpoint(&a,
                      &g_pAirplanePathLeft[g_dwAirplaneNode - 1],
                      &g_pAirplanePathRight[g_dwAirplaneNode - 1]);
        Vec3fMidpoint(
            &b, &g_pAirplanePathLeft[g_dwAirplaneNode], &g_pAirplanePathRight[g_dwAirplaneNode]);
        g_fAirplaneSegmentLength = Vec3fDistance(&a, &b);
        Vec3fSubtract(&g_vAirplaneDir, &b, &a);
        Vec3fNormalize(&g_vAirplaneDir);
        if (g_dwAirplaneNode < 2) {
            g_vAirplaneDirPrev = g_vAirplaneDir;
        } else {
            Vec3fMidpoint(&c,
                          &g_pAirplanePathLeft[g_dwAirplaneNode - 2],
                          &g_pAirplanePathRight[g_dwAirplaneNode - 2]);
            Vec3fSubtract(&g_vAirplaneDirPrev, &a, &c);
            Vec3fNormalize(&g_vAirplaneDirPrev);
        }
        if (g_dwAirplaneNode + 1 == g_dwAirplaneNodeCount) {
            g_vAirplaneDirNext = g_vAirplaneDir;
        } else {
            Vec3fMidpoint(&c,
                          &g_pAirplanePathLeft[g_dwAirplaneNode + 1],
                          &g_pAirplanePathRight[g_dwAirplaneNode + 1]);
            Vec3fSubtract(&g_vAirplaneDirNext, &c, &b);
            Vec3fNormalize(&g_vAirplaneDirNext);
        }
    }
    if (!g_dwAirplaneInstance) {
        return;
    }
    t = g_fAirplaneDistance / g_fAirplaneSegmentLength;
    Vec3fLerp(
        &a, &g_pAirplanePathLeft[g_dwAirplaneNode], &g_pAirplanePathLeft[g_dwAirplaneNode - 1], t);
    Vec3fLerp(&b,
              &g_pAirplanePathRight[g_dwAirplaneNode],
              &g_pAirplanePathRight[g_dwAirplaneNode - 1],
              t);
    Vec3fMidpoint((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[3], &a, &b);
    /* The nose turns from the leg before to the leg after across the middle of each leg. */
    if (0.5f < t) {
        Vec3fLerp((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[0],
                  &g_vAirplaneDirNext,
                  &g_vAirplaneDir,
                  t - 0.5f);
    } else {
        Vec3fLerp((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[0],
                  &g_vAirplaneDir,
                  &g_vAirplaneDirPrev,
                  t + 0.5f);
    }
    Vec3fNormalize((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[0]);
    Vec3fSubtract((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[2], &a, &b);
    Vec3fCross((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[1],
               (Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[0],
               (Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[2]);
    Vec3fNormalize((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[1]);
    Vec3fCross((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[2],
               (Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[1],
               (Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[0]);
    Vec3fNormalize((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[2]);
    Vec3fScaleInPlace((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[0],
                      g_fAirplaneScale);
    Vec3fScaleInPlace((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[2],
                      -g_fAirplaneScale);
    Vec3fScaleInPlace((Vec3f *)g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[1],
                      g_fAirplaneScale);
}

/* Move the special objects of the track. */
static void UpdateTrackSpecials(void) {
    s32 i, j;

    if (g_dwAirplaneInstance && !g_bAirplaneFlying &&
        (!(g_dwRainMode || g_dwSnowMode || g_dwNightMode) ||
         (g_dwSelectedTrackId != TRACK_COASTLINE &&
          g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE))) {
        /* A player over the trigger on the last lap sends the airplane off. */
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            for (j = 0; j < g_aVehicles[i].groundInstanceCount; j++) {
                if (g_aVehicles[i].lap == g_dwAirplaneTriggerLap &&
                    g_aVehicles[i].groundInstances[j] == g_dwAirplaneTrigger) {
                    g_bAirplaneFlying = TRUE;
                    break;
                }
            }
        }
    }
    for (i = 0; i < g_LoadedTrack.specialCount; i++) {
        switch (g_LoadedTrack.specials[i].type) {
        case TRACK_SPECIAL_SPIN_Z:
            guRotateF(g_afMvpMatrix, g_LoadedTrack.specials[i].param.spinAngle, 0.0f, 0.0f, 1.0f);
            goto spin;
        case TRACK_SPECIAL_SPIN_X:
            guRotateF(g_afMvpMatrix, g_LoadedTrack.specials[i].param.spinAngle, 1.0f, 0.0f, 0.0f);
            goto spin;
        case TRACK_SPECIAL_SPIN_Y:
            guRotateF(g_afMvpMatrix, g_LoadedTrack.specials[i].param.spinAngle, 0.0f, 1.0f, 0.0f);
        spin:
            /* The three spins share the turn of the instance. */
            guMtxCatF(g_afMvpMatrix,
                      g_LoadedTrack.instances[g_LoadedTrack.specials[i].target.instance].matrix,
                      g_LoadedTrack.instances[g_LoadedTrack.specials[i].target.instance].matrix);
            g_LoadedTrack.instances[g_LoadedTrack.specials[i].target.instance].flags &=
                ~TRACK_INSTANCE_UNROTATED;
            break;
        case TRACK_SPECIAL_AIRPLANE:
            if (g_dwAirplaneInstance && g_bAirplaneFlying) {
                FlyAirplane();
            }
            break;
        }
    }
}

/* Draw the vehicles of the racers that are, or are not, ghosts. */
static void RenderRacerVehicles(s32 ghosts) {
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        if (g_aRacerSlots[i].vehicle != NULL &&
            (g_aRacerSlots[i].vehicle->ghostMode == VEHICLE_GHOST) == ghosts) {
            RenderVehicleModel(g_aRacerSlots[i].vehicle, 0);
        }
    }
}

/* Draw the track and the vehicles the camera sees, for the view or for its rear-view mirror, which
 * leaves out the start lights and the smoke of the tires after the track in the snow. */
static void RenderRaceScene(s32 mainView) {
    s32 i;

    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    RenderSkybox();
    ResetHeadlightGlare();
    BuildVisibleGrid();
    if (!mainView) {
        AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    } else {
        g_dwVehicleViewModeIndex = g_dwNightMode ? 2 : 1;
    }
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        CheckVehicleVisibility(&g_aVehicles[i]);
    }
    RenderTrackGeometry(0);
    /* In the snow, other than on the coastline, the vehicles are drawn after the track. */
    if (!g_dwSnowMode || g_dwSelectedTrackId == TRACK_COASTLINE ||
        g_dwSelectedTrackId == TRACK_MIRROR_COASTLINE) {
        SetupFog();
        SetupWeatherLights();
        AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
        RenderRacerVehicles(FALSE);
        AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
        RenderTireSmoke();
    }
    if (mainView) {
        AddTimingMarker(0, 0x40, 0x40, 0x40, 0xFF);
        RenderStartLights();
    }
    RenderTrackGeometry(1);
    if (g_dwSnowMode && g_dwSelectedTrackId != TRACK_COASTLINE &&
        g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE) {
        SetupFog();
        SetupWeatherLights();
        AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
        RenderRacerVehicles(FALSE);
        if (mainView) {
            AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
            RenderTireSmoke();
        }
    }
}

/* Draw the rear-view mirror over a view whose camera is the bumper camera. */
static void RenderRearViewMirror(s32 index) {
    CameraFrame *saved;
    s32 x, y, width, height;

    if (g_dwRearViewMirrorMode == 1) {
        width = g_aViewportRects[index].width / 4;
    } else if (g_dwRearViewMirrorMode == 2) {
        width = g_aViewportRects[index].width * 5 / 16;
    } else {
        width = g_aViewportRects[index].width * 3 / 8 + 2;
    }
    height = width >> 2;
    x = ((g_aViewportRects[index].width - width) >> 1) + g_aViewportRects[index].x;
    y = g_aViewportRects[index].height / 16 + g_aViewportRects[index].y;
    saved = g_pActiveCamera;
    g_pActiveCameraVehicle->activeCamera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_MIRROR];
    g_pActiveCamera = g_pActiveCameraVehicle->activeCamera;
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
    RenderRaceScene(FALSE);
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    RenderRacerVehicles(TRUE);
    RenderHeadlightGlare();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    g_pActiveCameraVehicle->activeCamera = saved;
    g_pActiveCamera = g_pActiveCameraVehicle->activeCamera;
    SetViewport(g_aViewportRects[index].x,
                g_aViewportRects[index].y,
                g_aViewportRects[index].width,
                g_aViewportRects[index].height,
                TRUE);
    DrawRdpBorderRect(x, y, width, height);
}

/* Draw the HUD of a view, or the text of the races the computer drives over it. */
static void DrawViewOverlay(void) {
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextSize(40);
        if (g_aVehicles[0].profile->season) {
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
        DrawText("%wwINSTANT REPLAY!", 28, 32);
        DrawText("%wwA to restart", 28, g_dwScreenHeight - 24);
        SetTextAlignRight();
        DrawText("%wwSTART to exit", g_dwScreenWidth - 28, g_dwScreenHeight - 24);
    } else if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
        AddTimingMarker(0, 0xFF, 0, 0, 0xFF);
        DrawSpeedAndHudOverlay();
        AddTimingMarker(0, 0, 0x82, 0, 0xFF);
        if (g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
            StubDashboard();
        }
    } else if (g_pActiveCameraVehicle->racer->status & RACER_FINISHED) {
        DrawHudMessage();
    }
}

/* Draw a view of the race. */
static void RenderRaceView(s32 index) {
    AddTimingMarker(0, 0, 0, 0, 0xFF);
    g_pActiveCameraVehicle = &g_aVehicles[g_aViewportRects[index].vehicleIndex];
    g_pActiveCamera = g_pActiveCameraVehicle->activeCamera;
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
    SetViewport(g_aViewportRects[index].x,
                g_aViewportRects[index].y,
                g_aViewportRects[index].width,
                g_aViewportRects[index].height,
                TRUE);
    if (g_dwNumViewports >= 2) {
        /* Split views look narrower and not as far. */
        SetupFrustumCorners(g_pActiveCamera,
                            g_pActiveCamera->fov * 0.7f,
                            g_fCameraProjectionScale * 0.65f,
                            (f32)g_aViewportRects[index].width,
                            (f32)g_aViewportRects[index].height);
        SetupCameraMatrix(g_pActiveCamera,
                          g_pActiveCamera->fov * 0.7f,
                          g_fCameraProjectionScale * 0.65f,
                          (f32)g_aViewportRects[index].width,
                          (f32)g_aViewportRects[index].height);
    } else {
        SetupFrustumCorners(g_pActiveCamera,
                            g_pActiveCamera->fov,
                            g_fCameraProjectionScale,
                            (f32)g_aViewportRects[index].width,
                            (f32)g_aViewportRects[index].height);
        SetupCameraMatrix(g_pActiveCamera,
                          g_pActiveCamera->fov,
                          g_fCameraProjectionScale,
                          (f32)g_aViewportRects[index].width,
                          (f32)g_aViewportRects[index].height);
    }
    SetupFog();
    SetupWeatherLights();
    RenderRaceScene(TRUE);
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    {
        s32 i;

        for (i = 0; i < g_dwNumVehicles; i++) {
            if (g_aRacerSlots[i].vehicle != NULL) {
                RenderVehicleHeadlightBeams(g_aRacerSlots[i].vehicle);
            }
        }
    }
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    RenderRacerVehicles(TRUE);
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    RenderExhaustAndTireSpray();
    AddTimingMarker(0, 0, 0xFF, 0xFF, 0xFF);
    RenderPrecipitationEffect();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    RenderHeadlightGlare();
    SetupMenuProjection((f32)g_aViewportRects[index].width, (f32)g_aViewportRects[index].height);
    SetupViewportAndScissor(g_aViewportRects[index].x,
                            g_aViewportRects[index].y,
                            g_aViewportRects[index].width,
                            g_aViewportRects[index].height,
                            TRUE);
    if (g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER] &&
        g_dwNumViewports == 1 && g_dwRearViewMirrorMode) {
        RenderRearViewMirror(index);
    }
    DrawViewOverlay();
    DrawRdpOutlineRect(g_aViewportRects[index].x,
                       g_aViewportRects[index].y,
                       g_aViewportRects[index].width,
                       g_aViewportRects[index].height);
}

/* Draw the pause menu over the race. */
static void DrawPauseMenu(void) {
    s32 x, y;

    x = (g_aViewportRects[0].width >> 1) + g_aViewportRects[0].x;
    DrawScreenFadeOverlay(0.33f);
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(40);
    DrawText("%ryPAUSED", x, g_dwScreenHeight * 5 / 16);
    y = g_dwScreenHeight * 5 / 11;
    SetTextSize(20);
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

/* Show the captions of the credits over the race. */
static void DrawCreditCaption(void) {
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
        top = 132 - lines * 40 / 4;
        if (g_aCreditCaptions[g_dwRaceCaptionIndex].title[0] == '\0') {
            top -= 5;
        }
        for (lines--; lines >= 0; lines--) {
            SetTextSize(20);
            DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].lines[lines],
                     SCREEN_CENTER_X,
                     lines * 40 / 2 + top);
        }
        SetTextSize(15);
        DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].title, SCREEN_CENTER_X, top - 20);
    }
    g_fRaceCaptionTimer += g_fFrameDeltaTime;
    if (g_aCreditCaptions[g_dwRaceCaptionIndex].duration < g_fRaceCaptionTimer) {
        g_fRaceCaptionTimer = 0.0f;
        g_dwRaceCaptionIndex++;
    }
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

void SaveRaceResultsToSlots(void) {
    Vehicle *vehicle;
    s32 i, j;

    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        vehicle = &g_aVehicles[i];
        g_adwSavedResultPositions[i] = vehicle->racePosition;
        g_afSavedResultTimes[i] = vehicle->totalTime;
        g_afSavedResultBestLapTimes[i] = vehicle->bestLapTime;
        g_adwSavedResultBestLaps[i] = vehicle->bestLap;
        g_adwSavedResultLaps[i] = vehicle->lap;
        for (j = 0; j < g_dwTotalLaps; j++) {
            g_aafSavedResultLapTimes[i][j] = vehicle->lapTimes[j];
        }
    }
    g_dwSavedResultsValid = TRUE;
}

void RestoreRaceResultsFromSlots(void) {
    PlayerProfile *profile;
    Vehicle *vehicle;
    s32 i, j;

    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && g_dwSavedResultsValid) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            profile = g_aVehicles[i].profile;
            profile->seasonPoints[profile->season] +=
                g_abRacePositionPoints[g_adwSavedResultPositions[i]];
            profile = g_aVehicles[i].profile;
            profile->seasonPlaces[profile->season][profile->round] = g_adwSavedResultPositions[i];
            profile = g_aVehicles[i].profile;
            profile->seasonTimes[profile->season][profile->round] = g_afSavedResultTimes[i];
            profile = g_aVehicles[i].profile;
            osSyncPrintf("points = %d\n", profile->seasonPoints[profile->season]);
        }
    }
    /* The best lap time is kept but not given back. */
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        vehicle = &g_aVehicles[i];
        for (j = 0; j < g_dwTotalLaps; j++) {
            vehicle->lapTimes[j] = g_aafSavedResultLapTimes[i][j];
        }
        vehicle->lap = g_adwSavedResultLaps[i];
        vehicle->bestLap = g_adwSavedResultBestLaps[i];
        vehicle->totalTime = g_afSavedResultTimes[i];
        vehicle->racePosition = g_adwSavedResultPositions[i];
    }
}

void SetTrackFromSeason(void) {
    PlayerProfile *profile = g_pActiveProfile;

    g_dwSelectedTrackId = g_SeasonTable[profile->season].races[profile->round][0];
    if (profile->championshipFlags & 1) {
        if ((s32)g_dwSelectedTrackId < TRACK_MIRRORED) {
            g_dwSelectedTrackId += TRACK_MIRRORED;
        } else {
            g_dwSelectedTrackId -= TRACK_MIRRORED;
        }
    }
    g_dwCurrentWeather = g_SeasonTable[profile->season].races[profile->round][1];
}

/* Add a line of the bonus of a finished championship, after a blank line when others precede it. */
static s32 AddBonusLine(s32 count, const char *line) {
    if (count) {
        g_apszSeasonResultMsgLines[count++] = "";
    }
    g_apszSeasonResultMsgLines[count++] = line;
    return count;
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
                                count = AddBonusLine(count, "You Won all races this season!");
                                g_apszSeasonResultMsgLines[count++] =
                                    "Bonus: Mirror Mine Track Added!";
                                g_aVehicles[i].profile->unlockedTracks |=
                                    1 << TRACK_MIRROR_STRIP_MINE;
                            }
                        } else if (!(g_aVehicles[i].profile->unlockedTracks &
                                     (1 << TRACK_STRIP_MINE))) {
                            count = AddBonusLine(count, "You Won all races this season!");
                            g_apszSeasonResultMsgLines[count++] = "Bonus: Mine Track Added!";
                            g_aVehicles[i].profile->unlockedTracks |= 1 << TRACK_STRIP_MINE;
                        }
                    cars:
                        /* Each championship of the years unlocks a car. */
                        switch (g_aVehicles[i].profile->championshipFlags) {
                        case 0:
                            count = AddBonusLine(count, "Bonus: Milk Truck Added!");
                            g_aVehicles[i].profile->unlocks |= 0x400;
                            break;
                        case 1:
                            count = AddBonusLine(count, "Bonus: Helmet Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x200;
                            break;
                        case 2:
                            count = AddBonusLine(count, "Bonus: Cupra Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x800;
                            break;
                        case 3:
                            count = AddBonusLine(count, "Bonus: Beach Ball Car Added!");
                            g_aVehicles[i].profile->unlocks |= 0x1000;
                            break;
                        case 4:
                            count = AddBonusLine(count, "Bonus: Mirrored Cars Added!");
                            g_aVehicles[i].profile->unlocks |= 0x8000;
                            break;
                        }
                        g_aVehicles[i].profile->season = 0;
                        g_aVehicles[i].profile->championshipFlags++;
                    } else {
                        g_apszSeasonResultMsgLines[count] = "SEASON COMPLETED!";
                        g_apszSeasonResultMsgLines[count + 1] = "";
                        g_apszSeasonResultMsgLines[count + 2] = "PROGRESS TO THE NEXT SEASON";
                        count += 3;
                    }
                } else {
                    g_apszSeasonResultMsgLines[count] = "Sorry, you don't have enough points to";
                    g_apszSeasonResultMsgLines[count + 1] =
                        "pass this season.  You'll have to try again.";
                    count += 2;
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

#include <PR/gu.h>

#include "camera.h"
#include "game.h"
#include "input.h"
#include "racemenu.h"
#include "track.h"
#include "trackselect.h"
#include "vecmath.h"
#include "vehicle.h"
#include "vehiclechoice.h"
#include "weather.h"

/* The profiles a controller pak keeps, one for each of the first two players. */
#define SAVED_PROFILES 2

/* A vehicle falls back to the route when it has been upside down too long or drops this far below
 * the track, and comes back this high, nudged along x by its index in all but time attack. */
#define RESPAWN_FALL_DEPTH 10.0f
#define RESPAWN_HEIGHT 2.0
#define RESPAWN_SPREAD 0.01f
#define RESPAWN_OPACITY 0.1f

/* The grid: its columns 3 apart, its rows 8 apart behind the start, just above the track. */
#define GRID_COLUMN_SPACING 3.0f
#define GRID_ROW_SPACING 8.0f
#define GRID_HEIGHT 0.1f
#define QUARTER_TURN 1.570796371f

/* A computer waits this many frames more before it first checks whether it is stuck. */
#define AI_START_GRACE -180

void ChooseVehicleCar(Vehicle *vehicle) {
    s32 car;

    if (ChooseVehicleCarId(vehicle, &car) != CAR_CHOICE_UNCHANGED) {
        vehicle->carModel = car;
    }
}

void InitVehicleColor(Vehicle *vehicle) {
    u8 red, green, blue;

    ResetVehicleState(vehicle);
    red = g_abPlayerColorsRgb[vehicle->index][0];
    green = g_abPlayerColorsRgb[vehicle->index][1];
    blue = g_abPlayerColorsRgb[vehicle->index][2];
    vehicle->ghostMode = 0;
    vehicle->otherLook = 0;
    vehicle->driver = NULL;
    vehicle->color[0] = red;
    vehicle->color[1] = green;
    vehicle->color[2] = blue;
}

void InitPlayerVehicleSlot(Vehicle *vehicle) {
    PlayerProfile *profile;

    BindPlayerInputToVehicle(vehicle);
    vehicle->index = vehicle - g_aVehicles;
    if (vehicle->index < SAVED_PROFILES) {
        profile = &g_aSeasonSaveBuffers[vehicle->index];
        vehicle->profile = profile;
        InitPlayerProfileDefaults(profile, g_dwNumTracks);
    } else {
        vehicle->profile = NULL;
    }
    ChooseVehicleCar(vehicle);
    vehicle->customDecals = 0;
}

void RespawnVehicle(Vehicle *vehicle) {
    if (vehicle->body.uprightFrames >= 0 &&
        !(vehicle->frame.position.z < g_LoadedTrack.minZ - RESPAWN_FALL_DEPTH)) {
        return;
    }
    InitVehiclePhysics(vehicle);
#if TGR_REGION == TGR_REGION_US
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
#else
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO) {
#endif
        SetVehiclePosition(vehicle,
                           vehicle->section->points[vehicle->pointIndex].position.x,
                           vehicle->section->points[vehicle->pointIndex].position.y,
                           vehicle->section->points[vehicle->pointIndex].position.z +
                               RESPAWN_HEIGHT);
    } else {
        SetVehiclePosition(vehicle,
                           vehicle->section->points[vehicle->pointIndex].position.x +
                               (f32)vehicle->index * RESPAWN_SPREAD,
                           vehicle->section->points[vehicle->pointIndex].position.y,
                           vehicle->section->points[vehicle->pointIndex].position.z +
                               RESPAWN_HEIGHT);
    }
    InitVehicleOrientationFromAngle(vehicle,
                                    Atan2f(vehicle->trackDirection.x, vehicle->trackDirection.y));
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    SetVehicleTurnRate(vehicle, 0.0f, 0.0f, 0.0f);
    vehicle->body.wheels[0]->ground = NULL;
    vehicle->body.wheels[0]->contactFrames = 0;
    vehicle->body.wheels[0]->surface = TRACK_SURFACE_START;
    vehicle->body.wheels[1]->ground = NULL;
    vehicle->body.wheels[1]->contactFrames = 0;
    vehicle->body.wheels[1]->surface = TRACK_SURFACE_START;
    vehicle->body.wheels[3]->ground = NULL;
    vehicle->body.wheels[3]->contactFrames = 0;
    vehicle->body.wheels[3]->surface = TRACK_SURFACE_START;
    vehicle->body.wheels[2]->ground = NULL;
    vehicle->body.wheels[2]->contactFrames = 0;
    vehicle->body.wheels[2]->surface = TRACK_SURFACE_START;
    vehicle->forcedForwardFrames = 0;
    vehicle->forcedReverseFrames = 0;
    vehicle->reverseFrames = 0;
    vehicle->forwardFrames = AI_START_GRACE;
    vehicle->body.uprightFrames = 0;
    vehicle->ghostMode = VEHICLE_GHOST;
    vehicle->opacity = RESPAWN_OPACITY;
    UpdateCameraChase(vehicle, &vehicle->cameras[VEHICLE_CAMERA_CHASE], 1.0f);
    vehicle->cameraChanged = 1;
    vehicle->chasePosition.x = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x;
    vehicle->chasePosition.y = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y;
    vehicle->chasePosition.z = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z;
}

void ClearVehicleDamageState(Vehicle *vehicle) {
    s32 i;

    for (i = 0; i < VEHICLE_DENT_SIDES; i++) {
        vehicle->dents[i] = 0;
    }
    vehicle->dentJitter = 0;
}

void SetupVehicleForRaceStart(Vehicle *vehicle) {
    TrackSection *root;
    f32 row;
    f32 column;
    f32 forwardX;
    f32 forwardY;
    f32 sideX;
    f32 sideY;
    s16 weather;

    ResetVehicleState(vehicle);
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO ||
        (g_dwGameMode == GAME_MODE_PRACTICE && g_dwNumLocalPlayers == 1) ||
        g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        /* Alone on the grid, in the middle of the front row. */
        SetVehicleModelSlot(vehicle, vehicle->index);
        row = 0.0f;
        column = 0.5f;
    } else {
        SetVehicleModelSlot(vehicle, vehicle->index);
        row = (f32)(vehicle->index >> 1);
        column = (f32)((vehicle->index & 1) ^ 1);
    }
    forwardX = cosf(g_LoadedTrack.startHeading);
    forwardY = sinf(g_LoadedTrack.startHeading);
    sideX = cosf(g_LoadedTrack.startHeading + QUARTER_TURN);
    sideY = sinf(g_LoadedTrack.startHeading + QUARTER_TURN);
    SetVehiclePosition(
        vehicle,
        g_LoadedTrack.startPosition.x - sideX * GRID_COLUMN_SPACING * (column - 0.5f) -
            forwardX * GRID_ROW_SPACING * (row + 1.0f),
        g_LoadedTrack.startPosition.y - sideY * GRID_COLUMN_SPACING * (column - 0.5f) -
            forwardY * GRID_ROW_SPACING * (row + 1.0f),
        g_LoadedTrack.startPosition.z + GRID_HEIGHT);
    vehicle->distance = (row + 0.5f) * -GRID_ROW_SPACING;
    InitVehicleOrientationFromAngle(vehicle, g_LoadedTrack.startHeading);
    vehicle->previousPosition = vehicle->frame.position;
    vehicle->previousListenerPosition = vehicle->frame.position;
    InitCameraForVehicle(vehicle);
    vehicle->forcedForwardFrames = 0;
    vehicle->forcedReverseFrames = 0;
    vehicle->reverseFrames = 0;
    vehicle->forwardFrames = AI_START_GRACE;
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);

    root = g_LoadedTrack.rootSection;
    if (root != NULL) {
        vehicle->lapTime = 0.0f;
        vehicle->totalTime = 0.0f;
        vehicle->bestLapTime = 0.0f;
        weather = g_dwCurrentWeather - 1;
        if (weather >= ARCADE_WEATHERS || weather < 0) {
            weather = 0;
        }
        /* The descriptor of a track starts with its menu item. */
        vehicle->timeLeft = ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])
                                ->arcadeTimes[vehicle->tableIndex][weather]
                                .startTime;
        vehicle->pointIndex = 0;
        vehicle->section = g_LoadedTrack.rootSection;
        Vec3fDirection(&vehicle->trackDirection,
                       &g_LoadedTrack.rootSection->points[0].position,
                       &g_LoadedTrack.rootSection->points[1].position);
    } else {
        vehicle->trackDirection.y = 0.0f;
        vehicle->trackDirection.z = 0.0f;
        vehicle->trackDirection.x = 1.0f;
    }

    vehicle->lap = 0;
    vehicle->bestLap = 0;
    vehicle->racePosition = g_dwNumVehicles - vehicle->index - 1;
    vehicle->maxGate = 0;
    /* The players and the vehicles of these modes start behind the first gate. */
    if (vehicle->index < (s32)g_dwNumLocalPlayers || g_dwGameMode == GAME_MODE_ARCADE ||
        g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO) {
        vehicle->distanceLaps = -1;
        vehicle->gate = -1;
    } else {
        vehicle->distanceLaps = 0;
        vehicle->gate = 0;
    }
    vehicle->groundInstanceCount = 0;
    vehicle->nearbyInstanceCount = 0;
    vehicle->cameraHeading = 0.0f;
    vehicle->odometer = 0.0f;
    vehicle->speedKph = 0.0f;
    vehicle->joltPosition.x = 0.0f;
    vehicle->joltPosition.y = 0.0f;
    vehicle->joltPosition.z = 0.0f;
    vehicle->joltVelocity.x = 0.0f;
    vehicle->joltVelocity.y = 0.0f;
    vehicle->joltVelocity.z = 0.0f;
    vehicle->joltOffset.x = 0.0f;
    vehicle->joltOffset.y = 0.0f;
    vehicle->joltOffset.z = 0.0f;
    vehicle->groundHits = 0;
    vehicle->slotCooldown = 0;
    ResetVehicleEffects(vehicle);
    ClearVehicleDamageState(vehicle);
    vehicle->cameraChanged = 0;
    vehicle->effectSound = 0;
    vehicle->effectVolume = 0;
    vehicle->effectPriority = 0;
    vehicle->dopplerFactor = 1.0f;
}

void UnusedRaceStartStub(s32 unused) {
}

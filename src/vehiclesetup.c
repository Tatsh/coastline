#include "camera.h"
#include "game.h"
#include "input.h"
#include "racemenu.h"
#include "track.h"
#include "trackselect.h"
#include "vecmath.h"
#include "vehicle.h"

/* The seasons whose unlocks choose the car of a computer. */
#define CAR_CHOICE_SEASONS 4

/* The car of a computer when its season unlocks none of the first sixteen. */
#define FALLBACK_CAR 5
#define UNLOCK_BITS 16

/* The unlocks, tracks and weathers of a new profile, and its car options. */
#define NEW_PROFILE_UNLOCKS 0x102
#define NEW_PROFILE_TRACKS 4
#define NEW_PROFILE_WEATHERS 1
#define NEW_PROFILE_TRANSMISSION 1
#define NEW_PROFILE_HANDLING 0
#define NEW_PROFILE_SUSPENSION 1
#define NEW_PROFILE_TIRES 2

/* The profiles a controller pak keeps, one for each of the first two players. */
#define SAVED_PROFILES 2

/* A vehicle falls back to the route when it has been upside down too long or drops this far below
 * the track, and comes back this high, nudged along x by its index in all but time attack. */
#define RESPAWN_FALL_DEPTH 10.0f
#define RESPAWN_HEIGHT 2.0
#define RESPAWN_SPREAD 0.01f
#define RESPAWN_OPACITY 0.1f

/* A computer waits this many frames more before it first checks whether it is stuck. */
#define AI_START_GRACE -180

/* The spray of the wheels: where each starts its phase, and how far apart. */
#define SPRAY_FRAMES_STEP 0.137f
#define SPRAY_FRAMES_SPREAD 0.034f
#define SMOKE_TIMER_STEP 0.15f
#define SPRAY_VERTEX_ALPHA 0xFF

void ChooseVehicleCar(Vehicle *vehicle) {
    s32 season = g_aVehicles[0].profile->season;
    s32 unlockSeason = season < CAR_CHOICE_SEASONS ? season : CAR_CHOICE_SEASONS - 1;
    s16 unlocks;
    s32 car;

    if (vehicle->index < (s32)g_dwNumLocalPlayers) {
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
            vehicle->carModel = g_SeasonTable[season].cars[0];
        }
        return;
    }
    if (g_dwGameMode == GAME_MODE_ARCADE) {
        vehicle->carModel = g_aVehicles[0].carModel;
        return;
    }
    /* The lowest car the season unlocks; the clamped season chooses it, the season itself the
     * first car. */
    unlocks = g_SeasonTable[unlockSeason].unlocks;
    for (car = 0; car < UNLOCK_BITS; car++) {
        if (unlocks & 1) {
            break;
        }
        unlocks >>= 1;
    }
    if (car == UNLOCK_BITS) {
        car = FALLBACK_CAR;
    }
    if ((s32)g_dwNumLocalPlayers < vehicle->index) {
        vehicle->carModel = car;
    } else {
        vehicle->carModel = g_SeasonTable[season].cars[vehicle->index - g_dwNumLocalPlayers];
    }
}

void InitVehicleColor(Vehicle *vehicle) {
    u8 *color;

    ResetVehicleState(vehicle);
    color = g_abPlayerColorsRgb[vehicle->index];
    vehicle->ghostMode = 0;
    vehicle->otherLook = 0;
    vehicle->driver = NULL;
    vehicle->color[0] = color[0];
    vehicle->color[1] = color[1];
    vehicle->color[2] = color[2];
}

void InitPlayerVehicleSlot(Vehicle *vehicle) {
    PlayerProfile *profile;
    s32 i;

    BindPlayerInputToVehicle(vehicle);
    vehicle->index = vehicle - g_aVehicles;
    if (vehicle->index < SAVED_PROFILES) {
        profile = &g_aSeasonSaveBuffers[vehicle->index];
        vehicle->profile = profile;
        profile->championshipFlags = 0;
        vehicle->profile->season = 0;
        vehicle->profile->seasonPoints[0] = 0;
        vehicle->profile->round = 0;
        vehicle->profile->unlocks = NEW_PROFILE_UNLOCKS;
        vehicle->profile->unlockedTracks = NEW_PROFILE_TRACKS;
        vehicle->profile->unlockedWeathers = NEW_PROFILE_WEATHERS;
        vehicle->profile->transmission = NEW_PROFILE_TRANSMISSION;
        vehicle->profile->handling = NEW_PROFILE_HANDLING;
        vehicle->profile->suspension = NEW_PROFILE_SUSPENSION;
        vehicle->profile->tires = NEW_PROFILE_TIRES;
        vehicle->profile->controllerLayout = 0;
        for (i = 0; i < g_dwNumTracks; i++) {
            vehicle->profile->lapRecords[i] = 0.0f;
            vehicle->profile->raceRecords[i] = 0.0f;
        }
    } else {
        vehicle->profile = NULL;
    }
    ChooseVehicleCar(vehicle);
    vehicle->customDecals = 0;
}

void RespawnVehicle(Vehicle *vehicle) {
    TrackPoint *point;

    if (vehicle->body.uprightFrames >= 0 &&
        !(vehicle->frame.position.z < g_LoadedTrack.minZ - RESPAWN_FALL_DEPTH)) {
        return;
    }
    InitVehiclePhysics(vehicle);
    point = &vehicle->section->points[vehicle->pointIndex];
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
        SetVehiclePosition(
            vehicle, point->position.x, point->position.y, point->position.z + RESPAWN_HEIGHT);
    } else {
        SetVehiclePosition(vehicle,
                           point->position.x + (f32)vehicle->index * RESPAWN_SPREAD,
                           point->position.y,
                           point->position.z + RESPAWN_HEIGHT);
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
    vehicle->chasePosition = vehicle->cameras[VEHICLE_CAMERA_CHASE].position;
}

void ResetVehicleEffects(Vehicle *vehicle) {
    Vtx *vertex;
    s16 *velocity;
    s16 *surface;
    f32 start;
    s32 i;

    /* The staggered starts are written and at once cleared. */
    start = (f32)vehicle->index * SPRAY_FRAMES_STEP;
    vehicle->sprayFramesLeft[0] = start;
    vehicle->sprayFramesLeft[1] = start + SPRAY_FRAMES_SPREAD;
    vehicle->sprayFramesLeft[2] = vehicle->sprayFramesLeft[1] + SPRAY_FRAMES_SPREAD;
    vehicle->sprayFramesLeft[3] = vehicle->sprayFramesLeft[2] + SPRAY_FRAMES_SPREAD;
    for (i = VEHICLE_WHEELS - 1; i >= 0; i--) {
        vehicle->sprayAdvanced[i] = 0;
        vehicle->spraySurfaces[i] = TRACK_SURFACE_START;
    }
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        vehicle->smokeTimers[i] = (f32)i * SMOKE_TIMER_STEP;
    }
    for (i = VEHICLE_WHEELS - 1; i >= 0; i--) {
        vehicle->sprayPhases[i] = 0.0f;
        vehicle->sprayFramesLeft[i] = 0.0f;
    }

    vertex = &vehicle->sprayVertices[0][0][0];
    velocity = &vehicle->sprayVelocities[0][0][0][0];
    for (i = 0; i < VEHICLE_WHEELS * SPRAY_SEGMENTS * SPRAY_SEGMENT_VERTICES; i++) {
        velocity[2] = 0;
        velocity[1] = velocity[2];
        velocity[0] = velocity[2];
        velocity += 3;
        vertex->v.ob[0] = 0;
        vertex->v.ob[1] = 0;
        vertex->v.ob[2] = 0;
        vertex->v.cn[0] = 0;
        /* Both kinds of vertex are cleared alike. */
        if (i & 1) {
            vertex->v.cn[1] = 0;
        } else {
            vertex->v.cn[1] = 0;
        }
        vertex->v.cn[2] = 0;
        vertex->v.cn[3] = SPRAY_VERTEX_ALPHA;
        vertex++;
    }

    surface = &vehicle->spraySegmentSurfaces[0][0];
    for (i = 0; i < VEHICLE_WHEELS * SPRAY_SEGMENTS; i++) {
        surface[i] = TRACK_SURFACE_START;
    }
}

void ClearVehicleDamageState(Vehicle *vehicle) {
    s32 i;

    for (i = 0; i < VEHICLE_DENT_SIDES; i++) {
        vehicle->dents[i] = 0;
    }
    vehicle->dentJitter = 0;
}

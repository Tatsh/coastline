#include "racersharing.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>

#include "util.h"
#else
#include <stdlib.h>

#include "screen.h"
#define osSyncPrintf(...) NullStub()
#define QuickSort qsort
#endif

#include "game.h"
#include "input.h"
#include "vehiclecount.h"
#include "vehicleeffects.h"
#include "vehiclereset.h"

#define RACER_KINDS 2

#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef long double SharingReal;
#define SHARING_INTERMEDIATE(value) ((long double)(value))
#define SHARING_ZERO(value) (!((value) < 0.0f) && !((value) > 0.0f))
#else
typedef f32 SharingReal;
#define SHARING_INTERMEDIATE(value) (value)
#define SHARING_ZERO(value) ((value) == 0.0f)
#endif

/* The maximum number of free vehicles during reassignment. */
#define FREE_VEHICLES 12

/* Other players and finished championship racers retain their positions during reassignment. */
#define KEY_OTHER_PLAYER 1e10f
#define KEY_FINISHED 1e9f

/* A computer's vehicle departs from a racer farther than this, by its key, and waits this many
 * frames before it takes another. */
#define RELEASE_KEY 80000.0
#define SLOT_COOLDOWN_FRAMES 60

/* A vehicle taken by a racer starts just above the route, at this speed, and at this many frames
 * upright. */
#define ACQUIRE_HEIGHT 0.1f
#define ACQUIRE_SPEED 50.0f
#define ACQUIRE_UPRIGHT_FRAMES 40
#define AI_START_GRACE -180

s32 CompareRacerProgressAsc(const void *a, const void *b) {
    const RacerProgress *first = a;
    const RacerProgress *second = b;

    if (second->progress < first->progress) {
        return 1;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!(first->progress >= second->progress)) {
#else
    if (first->progress < second->progress) {
#endif
        return -1;
    }
    return 0;
}

/* Bring a distance along the route within half a lap. */
static f32 WrapHalfLap(SharingReal rawDistance, f32 lapLength) {
    f32 distance = rawDistance;

    if (SHARING_INTERMEDIATE(lapLength) * 0.5f < rawDistance) {
        distance = SHARING_INTERMEDIATE(distance) - lapLength;
    } else if (distance < SHARING_INTERMEDIATE(lapLength) * -0.5f) {
        distance = SHARING_INTERMEDIATE(distance) + lapLength;
    }
    return distance;
}

/* The key a racer sorts by when vehicles are shared out: near the player first. */
static f32 RacerSharingKey(RacerSlot *slot, Vehicle *player, f32 lapLength) {
    Vehicle *vehicle = slot->vehicle;
    f32 gap;
    SharingReal rawGap;

    if (vehicle != NULL) {
        if (vehicle != player && vehicle->index < (s32)g_dwNumLocalPlayers) {
            return KEY_OTHER_PLAYER;
        }
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && vehicle->index >= (s32)g_dwNumLocalPlayers &&
            (vehicle->racer->status & RACER_FINISHED) && vehicle->ghostMode == VEHICLE_GHOST &&
            SHARING_ZERO(vehicle->opacity)) {
            return KEY_FINISHED;
        }
        rawGap = (SHARING_INTERMEDIATE(player->distance) - vehicle->distance) +
                 SHARING_INTERMEDIATE(vehicle->distanceLaps - player->distanceLaps) * lapLength;
        gap = WrapHalfLap(rawGap, lapLength);
        return Vec3fDistance(&player->frame.position, &vehicle->frame.position) +
               SHARING_INTERMEDIATE(gap) * gap;
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && slot->index >= (s32)g_dwNumLocalPlayers &&
        (slot->status & RACER_FINISHED)) {
        return KEY_FINISHED;
    }
    rawGap = SHARING_INTERMEDIATE(slot->distanceLaps - player->distanceLaps) * lapLength +
             (SHARING_INTERMEDIATE(player->distance) - slot->distance);
    gap = WrapHalfLap(rawGap, lapLength);
    return Vec3fDistance(&player->frame.position, &slot->position) +
           SHARING_INTERMEDIATE(gap) * gap;
}

/* Transfer the vehicle's progress to its racer before detaching the vehicle. */
static void ReleaseRacerVehicle(RacerSlot *slot, Vehicle *vehicle, f32 key) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    osSyncPrintf("enemy %d loses slot %d (dist=%f)\n", slot->index, vehicle->index, (double)key);
#endif
    slot->position = vehicle->frame.position;
    slot->velocity = vehicle->placedVelocity;
    slot->speedKph = vehicle->speedKph;
    slot->section = vehicle->section;
    slot->pointIndex = vehicle->pointIndex;
    slot->lapTime = vehicle->lapTime;
    slot->bestLapTime = vehicle->bestLapTime;
    slot->totalTime = vehicle->totalTime;
    slot->timeLeft = vehicle->timeLeft;
    slot->lap = vehicle->lap;
    slot->distanceLaps = vehicle->distanceLaps;
    slot->maxGate = vehicle->maxGate;
    slot->gate = vehicle->gate;
    osSyncPrintf("saving lap (%d/%d) and gate (%d/%d)\n",
                 slot->lap,
                 slot->distanceLaps,
                 slot->maxGate,
                 slot->gate);
    slot->distance = vehicle->distance;
    slot->racePosition = vehicle->racePosition;
}

/* Give a racer a vehicle, placed on the route where the racer is, facing along it. */
static void AcquireRacerVehicle(RacerSlot *slot, Vehicle *vehicle) {
    Vec3f left;
    Vec3f nextLeft;

    slot->vehicle = vehicle;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    osSyncPrintf("enemy %d acquires slot %d\n", slot->index, vehicle->index);
#endif
    vehicle->speedKph = slot->speedKph;
    vehicle->section = slot->section;
    vehicle->pointIndex = slot->pointIndex;
    vehicle->lapTime = slot->lapTime;
    vehicle->bestLapTime = slot->bestLapTime;
    vehicle->totalTime = slot->totalTime;
    vehicle->timeLeft = slot->timeLeft;
    vehicle->lap = slot->lap;
    vehicle->distanceLaps = slot->distanceLaps;
    vehicle->maxGate = slot->maxGate;
    vehicle->gate = slot->gate;
    osSyncPrintf("restoring lap (%d/%d) and gate (%d/%d)\n",
                 vehicle->lap,
                 vehicle->distanceLaps,
                 vehicle->maxGate,
                 vehicle->gate);
    vehicle->distance = slot->distance;
    vehicle->racer = slot;
    vehicle->ghostMode = 0;
    vehicle->opacity = 1.0f;
    vehicle->cameraChanged = 1;
    vehicle->racePosition = slot->racePosition;
    InitVehiclePhysics(vehicle);
    ResetVehicleEffects(vehicle);
    vehicle->previousPosition = slot->previousPosition;
    SetVehiclePosition(
        vehicle, slot->position.x, slot->position.y, slot->position.z + ACQUIRE_HEIGHT);
    /* Stored a second time. */
    vehicle->ghostMode = 0;
    vehicle->opacity = 1.0f;
    vehicle->color[0] = slot->color[0];
    vehicle->color[1] = slot->color[1];
    vehicle->color[2] = slot->color[2];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    ApplyEnemyCarColorPanels(vehicle, slot);
#else
    SetMaterialColor(vehicle->model, slot->color[0] >> 3, slot->color[1] >> 3, slot->color[2] >> 3);
#endif

    /* The point is found again from the vehicle for each call. */
    Vec3fSubtract(&left,
                  &vehicle->section->points[vehicle->pointIndex].left,
                  &vehicle->section->points[vehicle->pointIndex].position);
    Vec3fSubtract(&nextLeft,
                  &vehicle->section->points[vehicle->pointIndex + 1].left,
                  &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fDirection(&vehicle->frame.forward,
                   &vehicle->section->points[vehicle->pointIndex].position,
                   &vehicle->section->points[vehicle->pointIndex + 1].position);
    Vec3fMidpoint(&vehicle->frame.side, &left, &nextLeft);
    Vec3fCross(&vehicle->frame.up, &vehicle->frame.forward, &vehicle->frame.side);
    Vec3fNormalize(&vehicle->frame.up);
    Vec3fCross(&vehicle->frame.side, &vehicle->frame.up, &vehicle->frame.forward);
    Vec3fNormalize(&vehicle->frame.side);
    vehicle->trackDirection = vehicle->frame.forward;
    /* The frame is set from itself, to set the orientation of the body. */
    SetVehicleFrameMatrix(vehicle, vehicle->matrix);
    if (vehicle->racer->status & RACER_HELD) {
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    } else {
        SetVehicleVelocity(vehicle,
                           vehicle->frame.forward.x * ACQUIRE_SPEED,
                           vehicle->frame.forward.y * ACQUIRE_SPEED,
                           vehicle->frame.forward.z * ACQUIRE_SPEED);
    }
    /* Each wheel pointer is read again for each store, the third wheel after the fourth. */
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
    vehicle->body.uprightFrames = ACQUIRE_UPRIGHT_FRAMES;
    vehicle->steerAngle = 0.0f;
}

void ManageAiRacerSlots(Vehicle *player) {
    RacerProgress entries[RACER_SLOTS];
    Vehicle *freeVehicles[FREE_VEHICLES];
    RacerSlot *slot;
    Vehicle *vehicle;
    f32 lapLength = g_LoadedTrack.rootSection->points[0].distance;
    f32 progress;
    s32 kind;
    s32 noVehicle;
    s32 freeCount;
    s32 seen;
    s32 i;

    for (i = 0; i < g_dwNumVehicles; i++) {
        entries[i].progress = RacerSharingKey(&g_aRacerSlots[i], player, lapLength);
        entries[i].slot = i;
    }
    if (g_dwNumVehicles >= 2) {
        /* The player's entry goes first, and the others sort behind it. */
        progress = entries[player->index].progress;
        entries[player->index].progress = entries[0].progress;
        entries[0].progress = progress;
        entries[0].slot = player->index;
        entries[player->index].slot = 0;
        QuickSort(&entries[1], g_dwNumVehicles - 1, sizeof(RacerProgress), CompareRacerProgressAsc);
    }

    for (kind = 0; kind != RACER_KINDS; kind++) {
        noVehicle = TRUE;
        freeCount = 0;
        seen = 0;
        for (i = 0; i < g_dwNumVehicles; i++) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (slot->straggler != kind || slot->index < (s32)g_dwNumLocalPlayers) {
                continue;
            }
            vehicle = slot->vehicle;
            if (seen <= 0) {
                /* The nearest racer retains its vehicle until it finished and faded away. */
                seen++;
                if (vehicle == NULL) {
                    continue;
                }
                noVehicle = FALSE;
                if (!(slot->status & RACER_FINISHED) || vehicle->ghostMode != VEHICLE_GHOST ||
                    !SHARING_ZERO(vehicle->opacity)) {
                    continue;
                }
            } else {
                if (vehicle == NULL) {
                    continue;
                }
                noVehicle = FALSE;
                if (vehicle->slotCooldown) {
                    continue;
                }
                if (!(RELEASE_KEY < (double)entries[i].progress)) {
                    continue;
                }
            }
            ReleaseRacerVehicle(slot, vehicle, entries[i].progress);
            freeVehicles[freeCount++] = slot->vehicle;
            slot->vehicle = NULL;
            vehicle->racer = NULL;
            vehicle->slotCooldown = SLOT_COOLDOWN_FRAMES;
        }
        if (noVehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
            osSyncPrintf("init slot %d to being open\n", g_dwNumLocalPlayers + kind);
#endif
            freeVehicles[freeCount++] = &g_aVehicles[g_dwNumLocalPlayers + kind];
        }
        if (freeCount == 0 || g_dwNumVehicles <= 0) {
            continue;
        }
        for (i = 0;;) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (slot->straggler == kind && slot->index >= (s32)g_dwNumLocalPlayers &&
                !(slot->status & RACER_FINISHED) && slot->vehicle == NULL) {
                AcquireRacerVehicle(slot, freeVehicles[--freeCount]);
            }
            player->nearestRacers[i] = &g_aRacerSlots[entries[i].slot];
            i++;
            if (freeCount == 0 || i >= g_dwNumVehicles) {
                break;
            }
        }
    }
}

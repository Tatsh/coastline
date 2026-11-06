#include "common/racersharing.h"

#include <PR/os_libc.h>

#include "common/game.h"
#include "common/input.h"
#include "common/racerslotaccess.h"
#include "common/util.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/vehicleeffects.h"
#include "common/vehiclereset.h"

void ManageAiRacerSlots(Vehicle *player) {
    RacerProgress entries[RACER_SLOTS];
    s32 framePaddingA[4];
    f32 distance;
    Vehicle *freeVehicles[7];
    s32 framePaddingB[2];
    s32 kind;
    RacerSlot *slot;
    Vehicle *vehicle;
    s32 framePaddingC;
    f32 lapLength;
    s32 framePaddingD;
    s32 i;
    Vec3f left;
    Vec3f nextLeft;
    s32 seen;
    s32 freeCount;
    s32 noVehicle;

    lapLength = g_LoadedTrack.rootSection->points[0].distance;
    for (i = 0; i < g_dwNumVehicles; i++) {
        vehicle = g_aRacerSlots[i].vehicle;
        if (vehicle != NULL) {
            if (vehicle != player && vehicle->index < (s32)g_dwNumLocalPlayers) {
                entries[i].progress = 1e10f;
            } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP &&
                       vehicle->index >= (s32)g_dwNumLocalPlayers &&
                       (vehicle->racer->status & RACER_FINISHED) &&
                       vehicle->ghostMode == VEHICLE_GHOST && vehicle->opacity == 0.0f) {
                entries[i].progress = 1e9f;
            } else {
                distance = (player->distance - vehicle->distance) +
                           (f32)(vehicle->distanceLaps - player->distanceLaps) * lapLength;
                if (lapLength * 0.5f < distance) {
                    distance -= lapLength;
                } else if (distance < lapLength * -0.5f) {
                    distance += lapLength;
                }
                entries[i].progress =
                    Vec3fDistance(&player->frame.position, &vehicle->frame.position) +
                    distance * distance;
            }
        } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP &&
                   g_aRacerSlots[i].index >= (s32)g_dwNumLocalPlayers &&
                   (g_aRacerSlots[i].status & RACER_FINISHED)) {
            entries[i].progress = 1e9f;
        } else {
            distance = (player->distance - g_aRacerSlots[i].distance) +
                       (f32)(g_aRacerSlots[i].distanceLaps - player->distanceLaps) * lapLength;
            if (lapLength * 0.5f < distance) {
                distance -= lapLength;
            } else if (distance < lapLength * -0.5f) {
                distance += lapLength;
            }
            entries[i].progress =
                Vec3fDistance(&player->frame.position, &g_aRacerSlots[i].position) +
                distance * distance;
        }
        entries[i].slot = i;
    }
    if (g_dwNumVehicles > 1) {
        distance = entries[player->index].progress;
        entries[player->index].progress = entries[0].progress;
        entries[0].progress = distance;
        entries[0].slot = player->index;
        entries[player->index].slot = 0;
        QuickSort(&entries[1], g_dwNumVehicles - 1, 8, CompareRacerProgressAsc);
    }
    kind = 0;
    do {
        noVehicle = 1;
        for (i = 0, freeCount = 0, seen = 0; i < g_dwNumVehicles; i++) {
            slot = &g_aRacerSlots[entries[i].slot];
            if (slot->straggler == kind) {
                if (slot->index < (s32)g_dwNumLocalPlayers) {
                } else if (seen < 1) {
                    seen++;
                    if (slot->vehicle != NULL) {
                        noVehicle = 0;
                        if ((slot->status & RACER_FINISHED) &&
                            slot->vehicle->ghostMode == VEHICLE_GHOST &&
                            slot->vehicle->opacity == 0.0f) {
                            vehicle = slot->vehicle;
                            goto release;
                        }
                    }
                } else {
                    vehicle = slot->vehicle;
                    if (slot->vehicle != NULL) {
                        noVehicle = 0;
                        if (slot->vehicle->slotCooldown == 0 && 80000.0 < entries[i].progress) {
                        release:
                            osSyncPrintf("enemy %d loses slot %d (dist=%f)\n",
                                         slot->index,
                                         slot->vehicle->index,
                                         entries[i].progress);
                            slot->position.x = vehicle->frame.position.x;
                            slot->position.y = vehicle->frame.position.y;
                            slot->position.z = vehicle->frame.position.z;
                            slot->velocity.x = vehicle->placedVelocity.x;
                            slot->velocity.y = vehicle->placedVelocity.y;
                            slot->velocity.z = vehicle->placedVelocity.z;
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
                            freeVehicles[freeCount++] = slot->vehicle;
                            slot->vehicle = NULL;
                            vehicle->racer = NULL;
                            vehicle->slotCooldown = 60;
                        }
                    }
                }
            }
        }
        i = 0;
        if (noVehicle) {
            osSyncPrintf("init slot %d to being open\n", (s32)g_dwNumLocalPlayers + kind);
            freeVehicles[freeCount++] = &g_aVehicles[(s32)g_dwNumLocalPlayers + kind];
        }
        for (; freeCount != 0 && i < g_dwNumVehicles; i++) {
            slot = g_aRacerSlots + entries[i].slot;
            if (slot->straggler == kind && slot->index >= (s32)g_dwNumLocalPlayers &&
                !(slot->status & RACER_FINISHED) && slot->vehicle == NULL) {
                vehicle = freeVehicles[--freeCount];
                slot->vehicle = vehicle;
                osSyncPrintf("enemy %d acquires slot %d\n", slot->index, vehicle->index);
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
                vehicle->racePosition = slot->racePosition;
                vehicle->racer = slot;
                vehicle->ghostMode = 0;
                vehicle->opacity = 1.0f;
                vehicle->cameraChanged = 1;
                InitVehiclePhysics(vehicle);
                ResetVehicleEffects(vehicle);
                vehicle->previousPosition.x = slot->previousPosition.x;
                vehicle->previousPosition.y = slot->previousPosition.y;
                vehicle->previousPosition.z = slot->previousPosition.z;
                SetVehiclePosition(
                    vehicle, slot->position.x, slot->position.y, 0.1f + slot->position.z);
                vehicle->ghostMode = 0;
                vehicle->opacity = 1.0;
                vehicle->color[0] = slot->color[0];
                vehicle->color[1] = slot->color[1];
                vehicle->color[2] = slot->color[2];
                SetMaterialColor(
                    vehicle->model, slot->color[0] >> 3, slot->color[1] >> 3, slot->color[2] >> 3);
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
                vehicle->trackDirection.x = vehicle->frame.forward.x;
                vehicle->trackDirection.y = vehicle->frame.forward.y;
                vehicle->trackDirection.z = vehicle->frame.forward.z;
                SetVehicleFrameMatrix(vehicle, vehicle->matrix);
                if (vehicle->racer->status & RACER_HELD) {
                    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
                } else {
                    SetVehicleVelocity(vehicle,
                                       vehicle->frame.forward.x * 50.0f,
                                       vehicle->frame.forward.y * 50.0f,
                                       50.0f * vehicle->frame.forward.z);
                }
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
                vehicle->forwardFrames = -180;
                vehicle->body.uprightFrames = 40;
                vehicle->steerAngle = 0.0f;
            }
            player->nearestRacers[i] = &g_aRacerSlots[entries[i].slot];
        }
    } while (++kind != 2);
}

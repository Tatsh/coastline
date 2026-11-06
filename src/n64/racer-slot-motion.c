#include "common/audiomix.h"
#include "common/driving.h"
#include "common/frametime.h"
#include "common/game.h"
#include "common/input.h"
#include "common/pause.h"
#include "common/racermotion.h"
#include "common/racers.h"
#include "common/vehicle.h"
#include "common/vehiclecount.h"

#define VEC3_COPY(d, s)                                                                            \
    {                                                                                              \
        f32 *_d = (f32 *)(d);                                                                      \
        f32 *_s = (f32 *)(s);                                                                      \
        _d[0] = _s[0];                                                                             \
        _d[1] = _s[1];                                                                             \
        _d[2] = _s[2];                                                                             \
    }

void UpdateRacerSlotMotion(RacerSlot *slot) {
    Vehicle *vehicle;
    u32 flags;
    void (*driver)(Vehicle *);

    if (slot->vehicle != 0 && slot->vehicle->driver == DriveAiVehicle &&
        g_dwGameMode != GAME_MODE_AFTER_SEASON) {
        slot->vehicle->input->buttons &= INPUT_VIEWS;
        slot->vehicle->input->steering = 0;
    }
    if (g_dwRacePauseState != 0) {
        g_aSoundEffectChannels[0].step = 0;
        g_aSoundEffectChannels[0].volume = 0;
        g_aSoundEffectChannels[1].step = 0;
        g_aSoundEffectChannels[1].volume = 0;
        g_aSoundEffectChannels[2].step = 0;
        g_aSoundEffectChannels[2].volume = 0;
        g_aSoundEffectChannels[3].step = 0;
        g_aSoundEffectChannels[3].volume = 0;
        g_aSoundEffectChannels[4].step = 0;
        g_aSoundEffectChannels[4].volume = 0;
        return;
    }
    flags = slot->status;
    if (flags & RACER_HELD) {
        if (slot->vehicle != 0) {
            Vehicle *vehicle;
            void (*driver)(Vehicle *);

            slot->vehicle->input->buttons |= INPUT_BRAKE;
            slot->vehicle->gear = 0;
            VEC3_COPY(&slot->vehicle->previousPosition, &slot->vehicle->frame.position);
            vehicle = slot->vehicle;
            driver = (void (*)(Vehicle *))vehicle->driver;
            if (driver != 0) {
                driver(vehicle);
            }
            slot->vehicle->gear = 0;
        }
    } else if (flags & RACER_FINISHED) {
        if (slot->vehicle != 0) {
            if (g_dwGameMode != GAME_MODE_CHAMPIONSHIP || slot->index < (s32)g_dwNumLocalPlayers) {
                slot->vehicle->input->buttons = INPUT_BRAKE | INPUT_EBRAKE;
                slot->vehicle->input->steeringInput = -0x7F;
                slot->vehicle->input->steering = -1.0f;
            }
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && slot->index >= (s32)g_dwNumLocalPlayers) {
                slot->vehicle->ghostMode = VEHICLE_GHOST;
                slot->vehicle->opacity -= g_fFrameDeltaTime;
                if (slot->vehicle->opacity < 0.0f) {
                    slot->vehicle->opacity = 0.0f;
                }
            }
            VEC3_COPY(&slot->vehicle->previousPosition, &slot->vehicle->frame.position);
            vehicle = slot->vehicle;
            driver = (void (*)(Vehicle *))vehicle->driver;
            if (driver != 0) {
                driver(vehicle);
            }
        }
    } else if (slot->vehicle != 0) {
        if (slot->vehicle->ghostMode == VEHICLE_GHOST) {
            slot->vehicle->opacity += g_fFrameDeltaTime * 1.6f;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK && slot->index != 0) {
                if (slot->vehicle->opacity > 0.375f) {
                    slot->vehicle->opacity = 0.375f;
                }
            } else if (slot->vehicle->opacity >= 1.0f) {
                slot->vehicle->opacity = 1.0f;
                slot->vehicle->ghostMode = 0;
            }
        }
        VEC3_COPY(&slot->vehicle->previousPosition, &slot->vehicle->frame.position);
        vehicle = slot->vehicle;
        driver = (void (*)(Vehicle *))vehicle->driver;
        if (driver != 0) {
            driver(vehicle);
        }
        slot->vehicle->odometer += slot->vehicle->speedKph * g_fFrameDeltaTime;
    } else {
        f32 t;

        slot->previousPosition.x = slot->position.x;
        slot->previousPosition.y = slot->position.y;
        slot->previousPosition.z = slot->position.z;
        t = g_LoadedTrack.rootSection->points[0].distance * (slot->distanceLaps + 1) -
            slot->distance;
        t -= slot->section->points[slot->pointIndex + 1].distance;
        t /= slot->section->points[slot->pointIndex].distance -
             slot->section->points[slot->pointIndex + 1].distance;
        if (g_aVehicles[g_dwNumLocalPlayers + slot->straggler].racer != 0) {
            AdvanceAlongTrackRoute(
                slot->section,
                slot->pointIndex,
                t,
                Vec3fLength(&g_aVehicles[g_dwNumLocalPlayers + slot->straggler].placedVelocity) *
                    g_fFrameDeltaTime);
            slot->distance +=
                Vec3fLength(&g_aVehicles[g_dwNumLocalPlayers + slot->straggler].placedVelocity) *
                g_fFrameDeltaTime;
        } else {
            AdvanceAlongTrackRoute(slot->section, slot->pointIndex, t, 2.22f);
            slot->distance = slot->distance + 2.22f;
        }
        slot->section = g_pTrackRouteSection;
        slot->pointIndex = g_dwTrackRoutePointIndex;
        VEC3_COPY(&slot->position, &g_vTrackRoutePoint);
        Vec3fSubtract(&slot->velocity, &slot->position, &slot->previousPosition);
        Vec3fScaleInPlace(&slot->velocity, 1.0f / g_fFrameDeltaTime);
        UpdateRaceProgress(slot);
    }
}

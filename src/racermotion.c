#include "racermotion.h"

#include "driving.h"
#include "frametime.h"
#include "game.h"
#include "input.h"
#include "pause.h"
#include "racers.h"
#include "vehiclecount.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include "audiomix.h"
#define RACER_AI_DRIVER DriveAiVehicle
#define MOTION_INTERMEDIATE(value) (value)
#define MOTION_LESS(a, b) ((a) < (b))
#else
#define RACER_AI_DRIVER AiVehicleControllerCallback
#define MOTION_INTERMEDIATE(value) ((long double)(value))
#define MOTION_LESS(a, b) (!((a) >= (b)))
#endif

/* A ghost fades in this fast, up to this in time attack; a racer without a vehicle moves this far
 * a frame when no vehicle of its kind sets the pace. */
#define GHOST_FADE_RATE 1.6f
#define TIME_ATTACK_GHOST_OPACITY 0.375f
#define IDLE_RACER_STEP 2.22f

/* The steering of a finished player's vehicle. */
#define FINISHED_STEERING_INPUT -127

/* Step the vehicle of a racer, retaining its last position and letting its driver drive it. The
 * vehicle is read from the racer at each use. */
#define DRIVE_RACER_VEHICLE(slot)                                                                  \
    {                                                                                              \
        Vehicle *vehicle = (slot)->vehicle;                                                        \
        Vec3f *from = &vehicle->frame.position;                                                    \
        Vec3f *to = &vehicle->previousPosition;                                                    \
        to->x = from->x;                                                                           \
        to->y = from->y;                                                                           \
        to->z = from->z;                                                                           \
    }                                                                                              \
    if ((slot)->vehicle->driver != NULL) {                                                         \
        (slot)->vehicle->driver((slot)->vehicle);                                                  \
    }

void UpdateRacerSlotMotion(RacerSlot *slot) {
    Vehicle *pacer;
    TrackPoint *point;
    f32 lapLength;
    f32 fraction;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    s32 i;
#endif

    /* The vehicle is read from the racer at each use. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (IsNetworkRacerMotionDisabled(slot)) {
        return;
    }
#endif
    if (slot->vehicle != NULL && slot->vehicle->driver == RACER_AI_DRIVER &&
        g_dwGameMode != GAME_MODE_AFTER_SEASON) {
        slot->vehicle->input->buttons &= INPUT_VIEWS;
        slot->vehicle->input->steering = 0.0f;
    }
    if (g_dwRacePauseState) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        ClearRacerSoundChannels();
#else
        for (i = 0; i != 5; i++) {
            g_aSoundEffectChannels[i].step = 0;
            g_aSoundEffectChannels[i].volume = 0;
        }
#endif
        return;
    }
    if (slot->status & RACER_HELD) {
        if (slot->vehicle == NULL) {
            return;
        }
        slot->vehicle->input->buttons |= INPUT_BRAKE;
        slot->vehicle->gear = 0;
        DRIVE_RACER_VEHICLE(slot);
        slot->vehicle->gear = 0;
        return;
    }
    if (slot->status & RACER_FINISHED) {
        if (slot->vehicle == NULL) {
            return;
        }
        if (g_dwGameMode != GAME_MODE_CHAMPIONSHIP || slot->index < (s32)g_dwNumLocalPlayers) {
            slot->vehicle->input->buttons = INPUT_BRAKE | INPUT_EBRAKE;
            slot->vehicle->input->steeringInput = FINISHED_STEERING_INPUT;
            slot->vehicle->input->steering = -1.0f;
        }
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && slot->index >= (s32)g_dwNumLocalPlayers) {
            slot->vehicle->ghostMode = VEHICLE_GHOST;
            slot->vehicle->opacity -= g_fFrameDeltaTime;
            if (MOTION_LESS(slot->vehicle->opacity, 0.0f)) {
                slot->vehicle->opacity = 0.0f;
            }
        }
        DRIVE_RACER_VEHICLE(slot);
        return;
    }
    if (slot->vehicle != NULL) {
        if (slot->vehicle->ghostMode == VEHICLE_GHOST) {
            slot->vehicle->opacity += MOTION_INTERMEDIATE(g_fFrameDeltaTime) * GHOST_FADE_RATE;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK && slot->index != 0) {
                /* The ghost of a time attack stays faint. */
                if (TIME_ATTACK_GHOST_OPACITY < slot->vehicle->opacity) {
                    slot->vehicle->opacity = TIME_ATTACK_GHOST_OPACITY;
                }
            } else if (1.0f <= slot->vehicle->opacity) {
                slot->vehicle->opacity = 1.0f;
                slot->vehicle->ghostMode = 0;
            }
        }
        DRIVE_RACER_VEHICLE(slot);
        slot->vehicle->odometer += MOTION_INTERMEDIATE(slot->vehicle->speedKph) * g_fFrameDeltaTime;
        return;
    }

    /* A racer without a vehicle follows the pace of a vehicle of its kind. */
    slot->previousPosition = slot->position;
    lapLength = g_LoadedTrack.rootSection->points[0].distance;
    point = &slot->section->points[slot->pointIndex];
    fraction = ((lapLength * MOTION_INTERMEDIATE(slot->distanceLaps + 1) - slot->distance) -
                point[1].distance) /
               (MOTION_INTERMEDIATE(point[0].distance) - point[1].distance);
    pacer = &g_aVehicles[g_dwNumLocalPlayers + slot->straggler];
    if (pacer->racer != NULL) {
        AdvanceAlongTrackRoute(slot->section,
                               slot->pointIndex,
                               fraction,
                               Vec3fLength(&pacer->placedVelocity) * g_fFrameDeltaTime);
        slot->distance +=
            Vec3fLength(&g_aVehicles[g_dwNumLocalPlayers + slot->straggler].placedVelocity) *
            g_fFrameDeltaTime;
    } else {
        AdvanceAlongTrackRoute(slot->section, slot->pointIndex, fraction, IDLE_RACER_STEP);
        slot->distance += IDLE_RACER_STEP;
    }
    slot->section = g_pTrackRouteSection;
    slot->pointIndex = g_dwTrackRoutePointIndex;
    slot->position = g_vTrackRoutePoint;
    Vec3fSubtract(&slot->velocity, &slot->position, &slot->previousPosition);
    Vec3fScaleInPlace(&slot->velocity, MOTION_INTERMEDIATE(1.0f) / g_fFrameDeltaTime);
    UpdateRaceProgress(slot);
}

#include "racermotion.h"

#include "driving.h"
#include "frametime.h"
#include "game.h"
#include "input.h"
#include "pause.h"
#include "racers.h"
#include "racerslotaccess.h"
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
        Vehicle *vehicle = RACER_SLOT_GET(slot, Vehicle);                                          \
        Vec3f *from = &vehicle->frame.position;                                                    \
        Vec3f *to = &vehicle->previousPosition;                                                    \
        to->x = from->x;                                                                           \
        to->y = from->y;                                                                           \
        to->z = from->z;                                                                           \
    }                                                                                              \
    if (RACER_SLOT_GET(slot, Vehicle)->driver != NULL) {                                           \
        RACER_SLOT_GET(slot, Vehicle)->driver(RACER_SLOT_GET(slot, Vehicle));                      \
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
    if (RACER_SLOT_GET(slot, Vehicle) != NULL &&
        RACER_SLOT_GET(slot, Vehicle)->driver == RACER_AI_DRIVER &&
        g_dwGameMode != GAME_MODE_AFTER_SEASON) {
        RACER_SLOT_GET(slot, Vehicle)->input->buttons &= INPUT_VIEWS;
        RACER_SLOT_GET(slot, Vehicle)->input->steering = 0.0f;
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
    if (RACER_SLOT_GET(slot, Status) & RACER_HELD) {
        if (RACER_SLOT_GET(slot, Vehicle) == NULL) {
            return;
        }
        RACER_SLOT_GET(slot, Vehicle)->input->buttons |= INPUT_BRAKE;
        RACER_SLOT_GET(slot, Vehicle)->gear = 0;
        DRIVE_RACER_VEHICLE(slot);
        RACER_SLOT_GET(slot, Vehicle)->gear = 0;
        return;
    }
    if (RACER_SLOT_GET(slot, Status) & RACER_FINISHED) {
        if (RACER_SLOT_GET(slot, Vehicle) == NULL) {
            return;
        }
        if (g_dwGameMode != GAME_MODE_CHAMPIONSHIP ||
            RACER_SLOT_GET(slot, Index) < (s32)g_dwNumLocalPlayers) {
            RACER_SLOT_GET(slot, Vehicle)->input->buttons = INPUT_BRAKE | INPUT_EBRAKE;
            RACER_SLOT_GET(slot, Vehicle)->input->steeringInput = FINISHED_STEERING_INPUT;
            RACER_SLOT_GET(slot, Vehicle)->input->steering = -1.0f;
        }
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP &&
            RACER_SLOT_GET(slot, Index) >= (s32)g_dwNumLocalPlayers) {
            RACER_SLOT_GET(slot, Vehicle)->ghostMode = VEHICLE_GHOST;
            RACER_SLOT_GET(slot, Vehicle)->opacity -= g_fFrameDeltaTime;
            if (MOTION_LESS(RACER_SLOT_GET(slot, Vehicle)->opacity, 0.0f)) {
                RACER_SLOT_GET(slot, Vehicle)->opacity = 0.0f;
            }
        }
        DRIVE_RACER_VEHICLE(slot);
        return;
    }
    if (RACER_SLOT_GET(slot, Vehicle) != NULL) {
        if (RACER_SLOT_GET(slot, Vehicle)->ghostMode == VEHICLE_GHOST) {
            RACER_SLOT_GET(slot, Vehicle)->opacity +=
                MOTION_INTERMEDIATE(g_fFrameDeltaTime) * GHOST_FADE_RATE;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK && RACER_SLOT_GET(slot, Index) != 0) {
                /* The ghost of a time attack stays faint. */
                if (TIME_ATTACK_GHOST_OPACITY < RACER_SLOT_GET(slot, Vehicle)->opacity) {
                    RACER_SLOT_GET(slot, Vehicle)->opacity = TIME_ATTACK_GHOST_OPACITY;
                }
            } else if (1.0f <= RACER_SLOT_GET(slot, Vehicle)->opacity) {
                RACER_SLOT_GET(slot, Vehicle)->opacity = 1.0f;
                RACER_SLOT_GET(slot, Vehicle)->ghostMode = 0;
            }
        }
        DRIVE_RACER_VEHICLE(slot);
        RACER_SLOT_GET(slot, Vehicle)->odometer +=
            MOTION_INTERMEDIATE(RACER_SLOT_GET(slot, Vehicle)->speedKph) * g_fFrameDeltaTime;
        return;
    }

    /* A racer without a vehicle follows the pace of a vehicle of its kind. */
    RACER_SLOT_GET(slot, PreviousPosition) = RACER_SLOT_GET(slot, Position);
    lapLength = g_LoadedTrack.rootSection->points[0].distance;
    point = &RACER_SLOT_GET(slot, Section)->points[RACER_SLOT_GET(slot, PointIndex)];
    fraction = ((lapLength * MOTION_INTERMEDIATE(RACER_SLOT_GET(slot, DistanceLaps) + 1) -
                 RACER_SLOT_GET(slot, Distance)) -
                point[1].distance) /
               (MOTION_INTERMEDIATE(point[0].distance) - point[1].distance);
    pacer = &g_aVehicles[g_dwNumLocalPlayers + RACER_SLOT_GET(slot, Straggler)];
    if (pacer->racer != NULL) {
        AdvanceAlongTrackRoute(RACER_SLOT_GET(slot, Section),
                               RACER_SLOT_GET(slot, PointIndex),
                               fraction,
                               Vec3fLength(&pacer->placedVelocity) * g_fFrameDeltaTime);
        RACER_SLOT_SET(
            slot,
            Distance,
            RACER_SLOT_GET(slot, Distance) +
                (Vec3fLength(&g_aVehicles[g_dwNumLocalPlayers + RACER_SLOT_GET(slot, Straggler)]
                                  .placedVelocity) *
                 g_fFrameDeltaTime));
    } else {
        AdvanceAlongTrackRoute(RACER_SLOT_GET(slot, Section),
                               RACER_SLOT_GET(slot, PointIndex),
                               fraction,
                               IDLE_RACER_STEP);
        RACER_SLOT_SET(slot, Distance, RACER_SLOT_GET(slot, Distance) + (IDLE_RACER_STEP));
    }
    RACER_SLOT_SET(slot, Section, g_pTrackRouteSection);
    RACER_SLOT_SET(slot, PointIndex, g_dwTrackRoutePointIndex);
    RACER_SLOT_GET(slot, Position) = g_vTrackRoutePoint;
    Vec3fSubtract(&RACER_SLOT_GET(slot, Velocity),
                  &RACER_SLOT_GET(slot, Position),
                  &RACER_SLOT_GET(slot, PreviousPosition));
    Vec3fScaleInPlace(&RACER_SLOT_GET(slot, Velocity),
                      MOTION_INTERMEDIATE(1.0f) / g_fFrameDeltaTime);
    UpdateRaceProgress(slot);
}

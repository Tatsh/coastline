#include "common/camera.h"
#include "common/driving.h"
#include "common/game.h"
#include "common/input.h"
#include "common/racerslotaccess.h"
#include "common/vehicleaccess.h"
#include "common/vehiclephysics.h"
#include "common/vehiclereset.h"

#if !TGR_BOSS_RALLY
#include <PR/gu.h>
#endif

#if TGR_BOSS_RALLY
#include <math.h>
#endif

#if TGR_BOSS_RALLY
#define CONTROL_REAL long double
#define CONTROL_LESS(a, b) (!((a) >= (b)))
#define CONTROL_NONZERO(x) ((x) < 0.0L || (x) > 0.0L)
#define CONTROL_EQUAL(a, b) (!((a) < (b)) && !((a) > (b)))

s32 g_bReplayCockpitNext;
f32 g_afVehicleJolts[4];

static f32 ReadVehicleControlAxis(f32 value, f32 offset, f32 scale, s32 negative) {
    long double amount = ((long double)value + offset) * scale;
    amount *= amount;
    return negative ? -amount : amount;
}
#else
#define CONTROL_REAL f32
#define CONTROL_LESS(a, b) ((a) < (b))
#define CONTROL_NONZERO(x) ((x) != 0.0f)
#define CONTROL_EQUAL(a, b) ((a) == (b))
#endif /* TGR_BOSS_RALLY */

/* The dead zone of the steering and of the stick that drives, and the share of the stick past
 * each. */
#define STEER_DEAD_ZONE 0.1f
#define STEER_RANGE 0.9f
#define DRIVE_DEAD_ZONE 0.25f
#define DRIVE_RANGE 0.75f

/* The trackside camera of a replay stands at the nearest of the points of the track no more than
 * 10 below the vehicle, within 150 of it; the views stay this many frames before the next choice
 * and after a player chooses one. */
#define CAMERA_POINT_MAX_DROP 10.0f
#define CAMERA_POINT_FAR 16777216.0f
#define CAMERA_POINT_RANGE_SQUARED 22500.0f
#define REPLAY_VIEW_FRAMES 60
#define REPLAY_CHOSEN_VIEW_FRAMES 450

/* The jolts of the weight on the spring of a vehicle: the gravity on it each frame, and the least
 * that jolts the vehicle. */
#define JOLT_GRAVITY 0.326666683f
#define JOLT_THRESHOLD 0.025f
#define JOLT_MAX_ONCE 2.5f
#define JOLT_MAX 5.0f

/* The drive after a season: how far ahead the route is read, the speed until the vehicle nears
 * the end, and where it brakes and stops. */
#define VICTORY_LOOK_AHEAD 15.0f
#define VICTORY_SPEED 27.0f
#define VICTORY_SLOW_DISTANCE 114.5f
#define VICTORY_END_DISTANCE 168.5f
#define VICTORY_BRAKE_DISTANCE 167.5f
#define VICTORY_STOP_DISTANCE 168.4f
#define VICTORY_STEER_SCALE 3.0f
#define VICTORY_CENTERING 0.2f

void AddVehicleJolt(s32 index, f32 amount) {
    if (JOLT_MAX_ONCE < amount) {
        amount = JOLT_MAX_ONCE;
    }
    /* The index of a computer's vehicle may pass the end of the array. */
    g_afVehicleJolts[index] += amount;
    if (JOLT_MAX < g_afVehicleJolts[index]) {
        g_afVehicleJolts[index] = JOLT_MAX;
    }
}

/* Whether a viewport follows the vehicle. */
static s32 IsVehicleViewed(Vehicle *vehicle) {
    return VEHICLE_GET(vehicle, Index) == g_aViewportRects[0].vehicleIndex ||
           (g_dwNumViewports >= 2 &&
            VEHICLE_GET(vehicle, Index) == g_aViewportRects[1].vehicleIndex);
}

#if TGR_BOSS_RALLY || TGR_REGION == TGR_REGION_US
/* Stand the trackside camera at the nearest point of the track, or switch away from it when no
 * point is close enough. The USA release compiles the routine into its caller. */
static void ChooseReplayCamera(Vehicle *vehicle);
#endif

/* The range test implies that a camera point was selected. */
void ChooseReplayCamera(Vehicle *vehicle) {
    CameraFrame *trackside = &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE];
    Vec3f *point = g_LoadedTrack.cameraPoints;
#ifdef ENABLE_PATCHES
    Vec3f chosen = {0.0f, 0.0f, 0.0f};
#else
    Vec3f chosen;
#endif /* ENABLE_PATCHES */
    f32 best = CAMERA_POINT_FAR;
#if TGR_BOSS_RALLY
    long double dx, dy, dz, distance;
    f32 storedX, storedY;
#else
    f32 dx, dy, dz, distance;
#endif
    s32 i;

    for (i = 0; i < g_LoadedTrack.cameraPointCount; i++, point++) {
        dx = (CONTROL_REAL)VEHICLE_GET(vehicle, Frame).position.x - point->x;
        dy = (CONTROL_REAL)VEHICLE_GET(vehicle, Frame).position.y - point->y;
        dz = (CONTROL_REAL)VEHICLE_GET(vehicle, Frame).position.z - point->z;
        if (!(CAMERA_POINT_MAX_DROP < dz)) {
#if TGR_BOSS_RALLY
            storedX = dx;
            storedY = dy;
            distance = (dz * dz + dy * storedY) + dx * storedX;
#else
            distance = dx * dx + dy * dy + dz * dz;
#endif
            if (CONTROL_LESS(distance, best)) {
                best = distance;
                chosen.x = point->x;
                chosen.y = point->y;
                chosen.z = point->z;
            }
        }
    }
    if (CONTROL_LESS(best, CAMERA_POINT_RANGE_SQUARED)) {
        if (VEHICLE_GET(vehicle, ActiveCamera) != trackside ||
            !CONTROL_EQUAL(trackside->position.x, chosen.x) ||
            !CONTROL_EQUAL(trackside->position.y, chosen.y) ||
            !CONTROL_EQUAL(trackside->position.z, chosen.z)) {
            VEHICLE_SET(vehicle, CameraChanged, 1);
        }
        trackside->position.x = chosen.x;
        trackside->position.y = chosen.y;
        trackside->position.z = chosen.z;
        VEHICLE_SET(vehicle, ActiveCamera, trackside);
    } else if (VEHICLE_GET(vehicle, ActiveCamera) == trackside) {
        VEHICLE_SET(vehicle, CameraChanged, 1);
        if (g_bReplayCockpitNext || g_dwReplayCameraHoldTimer) {
            VEHICLE_SET(
                vehicle, ActiveCamera, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT]);
        } else {
            VEHICLE_SET(
                vehicle, ActiveCamera, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE]);
        }
        g_dwReplayCameraSwitchTimer = REPLAY_VIEW_FRAMES;
        g_bReplayCockpitNext = !g_bReplayCockpitNext;
    }
}

/* Choose the view a replay, a race of the computer, or a finished racer shows, and take the view a
 * player selects. */
static void ChooseVehicleView(Vehicle *vehicle) {
    s32 choose = FALSE;
    s32 hold = FALSE;

    if ((RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & RACER_FINISHED) &&
        IsVehicleViewed(vehicle)) {
        if (g_dwReplayCameraSwitchTimer == 0) {
            choose = TRUE;
        }
    } else if ((g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
                gInstantReplayActive) &&
               IsVehicleViewed(vehicle)) {
        if (g_dwReplayCameraSwitchTimer != 0) {
            g_dwReplayCameraSwitchTimer--;
            hold = TRUE;
        } else {
            choose = TRUE;
        }
    } else if (IsVehicleViewed(vehicle)) {
        g_dwReplayCameraSwitchTimer = 0;
    }
    if (choose) {
        ChooseReplayCamera(vehicle);
        hold = TRUE;
    }
    if (hold && (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEWS)) {
        g_dwReplayCameraSwitchTimer = REPLAY_CHOSEN_VIEW_FRAMES;
    }

    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEW_COCKPIT) {
        VEHICLE_SET(vehicle, ActiveCamera, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT]);
        VEHICLE_SET(vehicle, CameraChanged, 1);
        TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_VIEW_COCKPIT);
    }
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEW_TRACKSIDE) {
        PlaceTrackCamera(vehicle);
        VEHICLE_SET(vehicle, CameraChanged, 1);
        TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_VIEW_TRACKSIDE);
    }
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEW_CHASE) {
        VEHICLE_SET(vehicle, ActiveCamera, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE]);
        VEHICLE_SET(vehicle, CameraChanged, 1);
        TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_VIEW_CHASE);
    }
    if (g_dwNumViewports == 1 && (VEHICLE_GET(vehicle, Input)->buttons & INPUT_VIEW_BUMPER)) {
        VEHICLE_SET(vehicle, ActiveCamera, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_BUMPER]);
        VEHICLE_SET(vehicle, CameraChanged, 1);
        TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_VIEW_BUMPER);
    }
}

/* Read the steering, the stick and the buttons into the controls of a vehicle. */
static void ReadVehicleControls(Vehicle *vehicle) {
    f32 steering = VEHICLE_GET(vehicle, Input)->steering;
    f32 stickY;
    u32 buttons;

    if (STEER_DEAD_ZONE < steering) {
#if TGR_BOSS_RALLY
        VEHICLE_SET(
            vehicle,
            Steering,
            ReadVehicleControlAxis(steering, -STEER_DEAD_ZONE, 1.111111164093017578125f, 1));
#else
        vehicle->steering = (steering - STEER_DEAD_ZONE) / STEER_RANGE;
        vehicle->steering = vehicle->steering * -vehicle->steering;
#endif
    } else if (CONTROL_LESS(steering, -STEER_DEAD_ZONE)) {
#if TGR_BOSS_RALLY
        VEHICLE_SET(
            vehicle,
            Steering,
            ReadVehicleControlAxis(steering, +STEER_DEAD_ZONE, 1.111111164093017578125f, 0));
#else
        vehicle->steering = (steering + STEER_DEAD_ZONE) / STEER_RANGE;
        vehicle->steering = vehicle->steering * vehicle->steering;
#endif
    } else {
        VEHICLE_SET(vehicle, Steering, 0.0f);
    }

    ChooseVehicleView(vehicle);

    stickY = VEHICLE_GET(vehicle, Input)->stickY;
    if (DRIVE_DEAD_ZONE < stickY) {
#if TGR_BOSS_RALLY
        VEHICLE_SET(
            vehicle,
            Accelerator,
            ReadVehicleControlAxis(stickY, -DRIVE_DEAD_ZONE, 1.33333337306976318359375f, 0));
#else
        vehicle->accelerator = (stickY - DRIVE_DEAD_ZONE) / DRIVE_RANGE;
        vehicle->accelerator = vehicle->accelerator * vehicle->accelerator;
#endif
    } else if (CONTROL_LESS(stickY, -DRIVE_DEAD_ZONE)) {
#if TGR_BOSS_RALLY
        VEHICLE_SET(
            vehicle,
            Accelerator,
            ReadVehicleControlAxis(stickY, +DRIVE_DEAD_ZONE, 1.33333337306976318359375f, 1));
#else
        vehicle->accelerator = (stickY + DRIVE_DEAD_ZONE) / DRIVE_RANGE;
        vehicle->accelerator = vehicle->accelerator * -vehicle->accelerator;
#endif
    } else {
        VEHICLE_SET(vehicle, Accelerator, 0.0f);
    }
    if (VEHICLE_GET(vehicle, FlyMode)) {
        VEHICLE_SET(vehicle, RollInput, 0.0f);
    }

    buttons = VEHICLE_GET(vehicle, Input)->buttons;
    if (buttons & INPUT_Z) {
        if (!VEHICLE_GET(vehicle, FlyMode)) {
            VEHICLE_SET(vehicle, Accelerator, 1.0f);
        }
    } else if (VEHICLE_GET(vehicle, FlyMode)) {
        /* Without Z, the stick pitches a flying vehicle. */
        VEHICLE_SET(vehicle, PitchInput, VEHICLE_GET(vehicle, Accelerator));
        VEHICLE_SET(vehicle, Accelerator, 0.0f);
    }
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_UP) {
        VEHICLE_SET(vehicle, Accelerator, 1.0f);
    }
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_DOWN) {
        VEHICLE_SET(vehicle, Accelerator, -1.0f);
    }
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_RIGHT) {
        if (VEHICLE_GET(vehicle, FlyMode)) {
            VEHICLE_SET(vehicle, RollInput, 1.0f);
        } else {
            VEHICLE_SET(vehicle, Steering, -1.0f);
        }
    }
    if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_LEFT) {
        if (VEHICLE_GET(vehicle, FlyMode)) {
            VEHICLE_SET(vehicle, RollInput, -1.0f);
        } else {
            VEHICLE_SET(vehicle, Steering, 1.0f);
        }
    }
}

/* Drive the vehicle of the race after a season along the middle of the route to a stop. */
static void DriveAfterSeason(Vehicle *vehicle) {
    Vec3f ahead;
    Vec3f velocity;

    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, Frame).position,
                       &VEHICLE_GET(vehicle, Frame).forward,
                       VICTORY_LOOK_AHEAD);
    ComputeTrackPosition(vehicle);
    ahead.x = VEHICLE_GET(vehicle, TrackForward).x;
    ahead.y = VEHICLE_GET(vehicle, TrackForward).y;
    ahead.z = VEHICLE_GET(vehicle, TrackForward).z;
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, Frame).position,
                       &VEHICLE_GET(vehicle, Frame).forward,
                       -VICTORY_LOOK_AHEAD);
    ComputeTrackPosition(vehicle);

    VEHICLE_GET(vehicle, Input)->buttons &= ~(INPUT_VIEWS | INPUT_RECORDED_ACTIONS);
    if (CONTROL_LESS(VEHICLE_GET(vehicle, Distance), VICTORY_SLOW_DISTANCE)) {
        Vec3fScale(&velocity, &VEHICLE_GET(vehicle, TrackForward), VICTORY_SPEED);
    } else {
        Vec3fScale(&velocity,
                   &VEHICLE_GET(vehicle, TrackForward),
                   (VICTORY_END_DISTANCE - (CONTROL_REAL)VEHICLE_GET(vehicle, Distance)) * 0.5f);
        if (VICTORY_BRAKE_DISTANCE < VEHICLE_GET(vehicle, Distance)) {
            VEHICLE_GET(vehicle, Input)->buttons |= INPUT_BRAKE;
            if (VICTORY_STOP_DISTANCE < VEHICLE_GET(vehicle, Distance)) {
                velocity.x = velocity.y = velocity.z = 0.0f;
            }
        } else {
            VEHICLE_GET(vehicle, Input)->buttons |= INPUT_EBRAKE;
        }
    }
    VEHICLE_SET(vehicle,
                SteerAngle,
                Vec3fDot(&ahead, &VEHICLE_GET(vehicle, Frame).side) * VICTORY_STEER_SCALE);
    velocity.x -= (CONTROL_REAL)VEHICLE_GET(vehicle, TrackOffset).x * VICTORY_CENTERING;
    velocity.y -= (CONTROL_REAL)VEHICLE_GET(vehicle, TrackOffset).y * VICTORY_CENTERING;
    SetVehicleVelocity(vehicle, velocity.x, velocity.y, velocity.z);
}

/* Swing the weight on the spring at the front left corner of a vehicle, and jolt the vehicle when
 * it swings hard. */
static void SwingJoltWeight(Vehicle *vehicle) {
    Vec3f corner;
    Vec3f predicted;
    Vec3f previousOffset;
#if TGR_BOSS_RALLY
    long double length, jolt;
    f32 distance;
#else
    f32 length;
    f32 jolt;
#endif

    Vec3fAdd(&corner, &VEHICLE_GET(vehicle, Frame).position, &VEHICLE_GET(vehicle, Frame).forward);
    Vec3fAddInPlace(&corner, &VEHICLE_GET(vehicle, Frame).side);
    Vec3fAdd(&predicted, &VEHICLE_GET(vehicle, JoltPosition), &VEHICLE_GET(vehicle, JoltVelocity));
    previousOffset.x = VEHICLE_GET(vehicle, JoltOffset).x;
    previousOffset.y = VEHICLE_GET(vehicle, JoltOffset).y;
    previousOffset.z = VEHICLE_GET(vehicle, JoltOffset).z;
    Vec3fSubtract(&VEHICLE_GET(vehicle, JoltOffset), &VEHICLE_GET(vehicle, JoltPosition), &corner);
    length = Vec3fLength(&VEHICLE_GET(vehicle, JoltOffset));
    if (CONTROL_NONZERO(length)) {
        Vec3fScaleInPlace(&VEHICLE_GET(vehicle, JoltOffset), length / (1.0f + length) / length);
    }
    /* The root of a distance, not of its square. */
#if TGR_BOSS_RALLY
    distance = Vec3fDistance(&VEHICLE_GET(vehicle, JoltOffset), &previousOffset);
    distance = sqrtf(distance);
    jolt = distance / (Vec3fLength(&VEHICLE_GET(vehicle, JoltVelocity)) + 1.0f);
#else
    jolt = sqrtf(Vec3fDistance(&VEHICLE_GET(vehicle, JoltOffset), &previousOffset)) /
           (Vec3fLength(&VEHICLE_GET(vehicle, JoltVelocity)) + 1.0f);
#endif
    if (JOLT_THRESHOLD < jolt) {
        AddVehicleJolt(VEHICLE_GET(vehicle, Index), jolt + jolt);
    }
    VEHICLE_GET(vehicle, JoltPosition).x = corner.x;
    VEHICLE_GET(vehicle, JoltPosition).y = corner.y;
    VEHICLE_GET(vehicle, JoltPosition).z = corner.z;
    /* The new position less the predicted one, not the reverse. */
    Vec3fSubtract(
        &VEHICLE_GET(vehicle, JoltVelocity), &VEHICLE_GET(vehicle, JoltPosition), &predicted);
    VEHICLE_GET(vehicle, JoltVelocity).z -= JOLT_GRAVITY;
}

void ProcessVehicleInputAndPhysics(Vehicle *vehicle) {
    if (g_dwTrackMirrored) {
        VEHICLE_GET(vehicle, Input)->steering = -VEHICLE_GET(vehicle, Input)->steering;
    }
    ReadVehicleControls(vehicle);
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        DriveAfterSeason(vehicle);
    }
#if TGR_BOSS_RALLY
    ApplyPlayerVehicleEffects(vehicle);
#endif
    UpdateVehiclePhysicsFrame(vehicle);
    if (!VEHICLE_GET(vehicle, FlyMode)) {
        SwingJoltWeight(vehicle);
    }
#if TGR_BOSS_RALLY
    ApplyPlayerVehicleRespawnRequest(vehicle);
#endif
    RespawnVehicle(vehicle);
}

#if !TGR_BOSS_RALLY
void UpdatePhysicsStep(Vehicle *vehicle) {
    ProcessVehicleInputAndPhysics(vehicle);
}
#endif

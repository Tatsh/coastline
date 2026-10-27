#include "camera.h"
#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#endif

#include "driving.h"
#include "game.h"
#include "input.h"
#include "vehiclephysics.h"
#include "vehiclereset.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>
#define RACER_FINISHED 2
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
#endif

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
    return vehicle->index == g_aViewportRects[0].vehicleIndex ||
           (g_dwNumViewports >= 2 && vehicle->index == g_aViewportRects[1].vehicleIndex);
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION == TGR_REGION_US
/* Stand the trackside camera at the nearest point of the track, or switch away from it when no
 * point is close enough. The USA release compiles the routine into its caller. */
static void ChooseReplayCamera(Vehicle *vehicle);
#endif

/* The range test implies that a camera point was selected. */
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
void ChooseReplayCamera(Vehicle *vehicle) {
    CameraFrame *trackside = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    Vec3f *point = g_LoadedTrack.cameraPoints;
    Vec3f chosen;
    f32 best = CAMERA_POINT_FAR;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double dx, dy, dz, distance;
    f32 storedX, storedY;
#else
    f32 dx, dy, dz, distance;
#endif
    s32 i;

    for (i = 0; i < g_LoadedTrack.cameraPointCount; i++, point++) {
        dx = (CONTROL_REAL)vehicle->frame.position.x - point->x;
        dy = (CONTROL_REAL)vehicle->frame.position.y - point->y;
        dz = (CONTROL_REAL)vehicle->frame.position.z - point->z;
        if (!(CAMERA_POINT_MAX_DROP < dz)) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
        if (vehicle->activeCamera != trackside || !CONTROL_EQUAL(trackside->position.x, chosen.x) ||
            !CONTROL_EQUAL(trackside->position.y, chosen.y) ||
            !CONTROL_EQUAL(trackside->position.z, chosen.z)) {
            vehicle->cameraChanged = 1;
        }
        trackside->position.x = chosen.x;
        trackside->position.y = chosen.y;
        trackside->position.z = chosen.z;
        vehicle->activeCamera = trackside;
    } else if (vehicle->activeCamera == trackside) {
        vehicle->cameraChanged = 1;
        if (g_bReplayCockpitNext || g_dwReplayCameraHoldTimer) {
            vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
        } else {
            vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
        }
        g_dwReplayCameraSwitchTimer = REPLAY_VIEW_FRAMES;
        g_bReplayCockpitNext = !g_bReplayCockpitNext;
    }
}
#pragma GCC diagnostic pop

/* Choose the view a replay, a race of the computer, or a finished racer shows, and take the view a
 * player selects. */
static void ChooseVehicleView(Vehicle *vehicle) {
    s32 choose = FALSE;
    s32 hold = FALSE;

    if ((vehicle->racer->status & RACER_FINISHED) && IsVehicleViewed(vehicle)) {
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
    if (hold && (vehicle->input->buttons & INPUT_VIEWS)) {
        g_dwReplayCameraSwitchTimer = REPLAY_CHOSEN_VIEW_FRAMES;
    }

    if (vehicle->input->buttons & INPUT_VIEW_COCKPIT) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_COCKPIT);
    }
    if (vehicle->input->buttons & INPUT_VIEW_TRACKSIDE) {
        PlaceTrackCamera(vehicle);
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_TRACKSIDE);
    }
    if (vehicle->input->buttons & INPUT_VIEW_CHASE) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_CHASE);
    }
    if (g_dwNumViewports == 1 && (vehicle->input->buttons & INPUT_VIEW_BUMPER)) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_BUMPER];
        vehicle->cameraChanged = 1;
        TransferFlags(vehicle->input, INPUT_VIEW_BUMPER);
    }
}

/* Read the steering, the stick and the buttons into the controls of a vehicle. */
static void ReadVehicleControls(Vehicle *vehicle) {
    f32 steering = vehicle->input->steering;
    f32 stickY;
    u32 buttons;

    if (STEER_DEAD_ZONE < steering) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->steering =
            ReadVehicleControlAxis(steering, -STEER_DEAD_ZONE, 1.111111164093017578125f, 1);
#else
        vehicle->steering = (steering - STEER_DEAD_ZONE) / STEER_RANGE;
        vehicle->steering = vehicle->steering * -vehicle->steering;
#endif
    } else if (CONTROL_LESS(steering, -STEER_DEAD_ZONE)) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->steering =
            ReadVehicleControlAxis(steering, +STEER_DEAD_ZONE, 1.111111164093017578125f, 0);
#else
        vehicle->steering = (steering + STEER_DEAD_ZONE) / STEER_RANGE;
        vehicle->steering = vehicle->steering * vehicle->steering;
#endif
    } else {
        vehicle->steering = 0.0f;
    }

    ChooseVehicleView(vehicle);

    stickY = vehicle->input->stickY;
    if (DRIVE_DEAD_ZONE < stickY) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->accelerator =
            ReadVehicleControlAxis(stickY, -DRIVE_DEAD_ZONE, 1.33333337306976318359375f, 0);
#else
        vehicle->accelerator = (stickY - DRIVE_DEAD_ZONE) / DRIVE_RANGE;
        vehicle->accelerator = vehicle->accelerator * vehicle->accelerator;
#endif
    } else if (CONTROL_LESS(stickY, -DRIVE_DEAD_ZONE)) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->accelerator =
            ReadVehicleControlAxis(stickY, +DRIVE_DEAD_ZONE, 1.33333337306976318359375f, 1);
#else
        vehicle->accelerator = (stickY + DRIVE_DEAD_ZONE) / DRIVE_RANGE;
        vehicle->accelerator = vehicle->accelerator * -vehicle->accelerator;
#endif
    } else {
        vehicle->accelerator = 0.0f;
    }
    if (vehicle->flyMode) {
        vehicle->rollInput = 0.0f;
    }

    buttons = vehicle->input->buttons;
    if (buttons & INPUT_Z) {
        if (!vehicle->flyMode) {
            vehicle->accelerator = 1.0f;
        }
    } else if (vehicle->flyMode) {
        /* Without Z, the stick pitches a flying vehicle. */
        vehicle->pitchInput = vehicle->accelerator;
        vehicle->accelerator = 0.0f;
    }
    if (vehicle->input->buttons & INPUT_UP) {
        vehicle->accelerator = 1.0f;
    }
    if (vehicle->input->buttons & INPUT_DOWN) {
        vehicle->accelerator = -1.0f;
    }
    if (vehicle->input->buttons & INPUT_RIGHT) {
        if (vehicle->flyMode) {
            vehicle->rollInput = 1.0f;
        } else {
            vehicle->steering = -1.0f;
        }
    }
    if (vehicle->input->buttons & INPUT_LEFT) {
        if (vehicle->flyMode) {
            vehicle->rollInput = -1.0f;
        } else {
            vehicle->steering = 1.0f;
        }
    }
}

/* Drive the vehicle of the race after a season along the middle of the route to a stop. */
static void DriveAfterSeason(Vehicle *vehicle) {
    Vec3f ahead;
    Vec3f velocity;

    Vec3fMulAddInPlace(&vehicle->frame.position, &vehicle->frame.forward, VICTORY_LOOK_AHEAD);
    ComputeTrackPosition(vehicle);
    ahead.x = vehicle->trackForward.x;
    ahead.y = vehicle->trackForward.y;
    ahead.z = vehicle->trackForward.z;
    Vec3fMulAddInPlace(&vehicle->frame.position, &vehicle->frame.forward, -VICTORY_LOOK_AHEAD);
    ComputeTrackPosition(vehicle);

    vehicle->input->buttons &= ~(INPUT_VIEWS | INPUT_RECORDED_ACTIONS);
    if (CONTROL_LESS(vehicle->distance, VICTORY_SLOW_DISTANCE)) {
        Vec3fScale(&velocity, &vehicle->trackForward, VICTORY_SPEED);
    } else {
        Vec3fScale(&velocity,
                   &vehicle->trackForward,
                   (VICTORY_END_DISTANCE - (CONTROL_REAL)vehicle->distance) * 0.5f);
        if (VICTORY_BRAKE_DISTANCE < vehicle->distance) {
            vehicle->input->buttons |= INPUT_BRAKE;
            if (VICTORY_STOP_DISTANCE < vehicle->distance) {
                velocity.x = velocity.y = velocity.z = 0.0f;
            }
        } else {
            vehicle->input->buttons |= INPUT_EBRAKE;
        }
    }
    vehicle->steerAngle = Vec3fDot(&ahead, &vehicle->frame.side) * VICTORY_STEER_SCALE;
    velocity.x -= (CONTROL_REAL)vehicle->trackOffset.x * VICTORY_CENTERING;
    velocity.y -= (CONTROL_REAL)vehicle->trackOffset.y * VICTORY_CENTERING;
    SetVehicleVelocity(vehicle, velocity.x, velocity.y, velocity.z);
}

/* Swing the weight on the spring at the front left corner of a vehicle, and jolt the vehicle when
 * it swings hard. */
static void SwingJoltWeight(Vehicle *vehicle) {
    Vec3f corner;
    Vec3f predicted;
    Vec3f previousOffset;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double length, jolt;
    f32 distance;
#else
    f32 length;
    f32 jolt;
#endif

    Vec3fAdd(&corner, &vehicle->frame.position, &vehicle->frame.forward);
    Vec3fAddInPlace(&corner, &vehicle->frame.side);
    Vec3fAdd(&predicted, &vehicle->joltPosition, &vehicle->joltVelocity);
    previousOffset.x = vehicle->joltOffset.x;
    previousOffset.y = vehicle->joltOffset.y;
    previousOffset.z = vehicle->joltOffset.z;
    Vec3fSubtract(&vehicle->joltOffset, &vehicle->joltPosition, &corner);
    length = Vec3fLength(&vehicle->joltOffset);
    if (CONTROL_NONZERO(length)) {
        Vec3fScaleInPlace(&vehicle->joltOffset, length / (1.0f + length) / length);
    }
    /* The root of a distance, not of its square. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    distance = Vec3fDistance(&vehicle->joltOffset, &previousOffset);
    distance = sqrtf(distance);
    jolt = distance / (Vec3fLength(&vehicle->joltVelocity) + 1.0f);
#else
    jolt = sqrtf(Vec3fDistance(&vehicle->joltOffset, &previousOffset)) /
           (Vec3fLength(&vehicle->joltVelocity) + 1.0f);
#endif
    if (JOLT_THRESHOLD < jolt) {
        AddVehicleJolt(vehicle->index, jolt + jolt);
    }
    vehicle->joltPosition.x = corner.x;
    vehicle->joltPosition.y = corner.y;
    vehicle->joltPosition.z = corner.z;
    /* The new position less the predicted one, not the reverse. */
    Vec3fSubtract(&vehicle->joltVelocity, &vehicle->joltPosition, &predicted);
    vehicle->joltVelocity.z -= JOLT_GRAVITY;
}

void ProcessVehicleInputAndPhysics(Vehicle *vehicle) {
    if (g_dwTrackMirrored) {
        vehicle->input->steering = -vehicle->input->steering;
    }
    ReadVehicleControls(vehicle);
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        DriveAfterSeason(vehicle);
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    ApplyPlayerVehicleEffects(vehicle);
#endif
    UpdateVehiclePhysicsFrame(vehicle);
    if (!vehicle->flyMode) {
        SwingJoltWeight(vehicle);
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    ApplyPlayerVehicleRespawnRequest(vehicle);
#endif
    RespawnVehicle(vehicle);
}

void UpdatePhysicsStep(Vehicle *vehicle) {
    ProcessVehicleInputAndPhysics(vehicle);
}

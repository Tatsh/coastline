#include "camera.h"
#include "game.h"

/* The distances of the chase camera behind and above the vehicle. */
#define CHASE_DISTANCE 11.0f
#define CHASE_DISTANCE_SPLIT_SCREEN 19.8f
#define CHASE_HEIGHT 2.4f
#define CHASE_DISTANCE_AFTER_SEASON 13.0f

void UpdateCameraChase(Vehicle *vehicle, CameraFrame *camera, f32 share) {
    Vec3f offset;
    f32 previous[3];
    /* The original reserves stack for eight bytes of locals it does not use. */
    f32 unused[2];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double length;
#else
    f32 length;
#endif

    if (vehicle->flyMode) {
        Vec3fMulAdd(&camera->position, &vehicle->frame.position, &vehicle->frame.up, CHASE_HEIGHT);
        Vec3fMulAdd(&camera->position,
                    &camera->position,
                    &vehicle->frame.forward,
                    g_dwNumViewports == 1 ? -CHASE_DISTANCE : -CHASE_DISTANCE_SPLIT_SCREEN);
        return;
    }

    /* The original keeps a copy of the old position that it does not use. */
    previous[0] = camera->position.x;
    previous[1] = camera->position.y;
    previous[2] = camera->position.z;
    (void)previous;
    (void)unused;

    /* The direction the camera was in from the vehicle, at the chase distance. */
    camera->position.z -= CHASE_HEIGHT;
    Vec3fSubtract(&offset, &camera->position, &vehicle->frame.position);
    length = Vec3fLength(&offset);
    if (length != 0.0f) {
        if (g_dwNumViewports == 1) {
            Vec3fScaleInPlace(&offset, CHASE_DISTANCE / length);
        } else {
            Vec3fScaleInPlace(&offset, CHASE_DISTANCE_SPLIT_SCREEN / length);
        }
    }

    /* The direction behind the vehicle, or ahead of it in an instant replay. */
    if (gInstantReplayActive) {
        Vec3fScale(&camera->position, &vehicle->frame.forward, CHASE_DISTANCE);
    } else if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        Vec3fScale(&camera->position, &vehicle->frame.side, -CHASE_DISTANCE);
        Vec3fMulAddInPlace(
            &camera->position, &vehicle->frame.forward, -CHASE_DISTANCE_AFTER_SEASON);
    } else {
        Vec3fScale(&camera->position, &vehicle->frame.forward, -CHASE_DISTANCE);
    }
    length = Vec3fLength(&camera->position);
    if (length != 0.0f) {
        Vec3fScaleInPlace(&camera->position, CHASE_DISTANCE / length);
    }

    Vec3fLerp(&camera->position, &camera->position, &offset, share);
    Vec3fAddInPlace(&camera->position, &vehicle->frame.position);
    camera->position.z += CHASE_HEIGHT;
}

#define LOOK_AT_HEIGHT 0.66f
#define LOCKED_LOOK_AT_HEIGHT 1.1f
#define LOOK_AHEAD_SLOW 2.0f
#define LOOK_AHEAD_SLOW_SPEED 3.5f
#define LOOK_AHEAD_FAST_SPEED 7.0f
#define LOOK_AHEAD_SLOPE 0.571428597f
#define LOOK_AHEAD_STEP 0.1f
#define TRACKSIDE_FORWARD 6.0f
#define TRACKSIDE_SIDE 2.0f

void UpdateCameraLookAtTarget(Vehicle *vehicle) {
    f32 speed;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double target, adjusted;
#else
    f32 target;
#endif

    if (vehicle->flyMode) {
        Vec3fMulAdd(
            &vehicle->lookAt, &vehicle->frame.position, &vehicle->frame.up, LOCKED_LOOK_AT_HEIGHT);
        return;
    }
    vehicle->lookAt.x = vehicle->frame.position.x;
    vehicle->lookAt.y = vehicle->frame.position.y;
    vehicle->lookAt.z = vehicle->frame.position.z + LOOK_AT_HEIGHT;
    speed = Vec3fLength(&vehicle->body.state.angularVelocity);
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        return;
    }

    /* The lead falls from 2 at a walk to nothing at speed. */
    target = 0.0f;
    if (speed < LOOK_AHEAD_SLOW_SPEED) {
        target = LOOK_AHEAD_SLOW;
    } else if (speed < LOOK_AHEAD_FAST_SPEED) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        target = 4.0f - (long double)speed * LOOK_AHEAD_SLOPE;
#else
        target = 4.0f - speed * LOOK_AHEAD_SLOPE;
#endif
    }
    if (vehicle->lookAhead < target) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        adjusted = (long double)vehicle->lookAhead + LOOK_AHEAD_STEP;
        vehicle->lookAhead = (f32)adjusted;
        if (target < adjusted) {
#else
        vehicle->lookAhead += LOOK_AHEAD_STEP;
        if (target < vehicle->lookAhead) {
#endif
            vehicle->lookAhead = target;
        }
    } else if (target < vehicle->lookAhead) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        adjusted = (long double)vehicle->lookAhead - LOOK_AHEAD_STEP;
        vehicle->lookAhead = (f32)adjusted;
        if (adjusted < target) {
#else
        vehicle->lookAhead -= LOOK_AHEAD_STEP;
        if (vehicle->lookAhead < target) {
#endif
            vehicle->lookAhead = target;
        }
    }
    Vec3fMulAddInPlace(&vehicle->lookAt, &vehicle->frame.forward, vehicle->lookAhead);
}

void ComputeCameraBasis(Vehicle *vehicle, CameraFrame *camera) {
    Vec3f direction;
    f32 length;
    /* The original reserves stack for eight bytes of locals it does not use. */
    f32 unused[2];

    (void)unused;
    Vec3fSubtract(&direction, &vehicle->lookAt, &camera->position);
    length = Vec3fLength(&direction);
    if (length != 0.0f) {
        Vec3fScaleByInverse(&camera->forward, &direction, length);
    } else if (Vec3fLength(&camera->forward) == 0.0f) {
        camera->forward.x = vehicle->frame.forward.x;
        camera->forward.y = vehicle->frame.forward.y;
        camera->forward.z = vehicle->frame.forward.z;
    }
    if (vehicle->flyMode) {
        Vec3fCross(&camera->side, &vehicle->frame.up, &camera->forward);
    } else {
        /* The direction is reused for the world up. */
        direction.x = 0.0f;
        direction.y = 0.0f;
        direction.z = 1.0f;
        Vec3fCross(&camera->side, &direction, &camera->forward);
    }
    Vec3fCross(&camera->up, &camera->forward, &camera->side);
}

void PlaceTrackCamera(Vehicle *vehicle) {
    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    Vec3fMulAdd(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position,
                &vehicle->frame.position,
                &vehicle->frame.forward,
                TRACKSIDE_FORWARD);
    Vec3fMulAddInPlace(
        &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position, &vehicle->frame.side, TRACKSIDE_SIDE);
    Vec3fAddInPlace(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position, &vehicle->frame.up);
    vehicle->cameraChanged = 2;
}

#define START_CHASE_HEIGHT 4.0f
#define START_CHASE_REPLAY_FORWARD 10.0f
#define START_LOOK_AHEAD 2.0f

void InitCameraForVehicle(Vehicle *vehicle) {
    Vec3f position;
    /* The original reserves stack for sixteen bytes of locals it does not use. */
    f32 unused[4];

    (void)unused;
    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
        vehicle->previousCamera = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
    } else {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
        vehicle->previousCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
    }
    Vec3fMulAdd(&vehicle->cameras[VEHICLE_CAMERA_CHASE].position,
                &vehicle->frame.position,
                &vehicle->frame.up,
                START_CHASE_HEIGHT);
    if (gInstantReplayActive) {
        Vec3fMulAddInPlace(&vehicle->cameras[VEHICLE_CAMERA_CHASE].position,
                           &vehicle->frame.forward,
                           START_CHASE_REPLAY_FORWARD);
    }
    Vec3fSubtractInPlace(&vehicle->cameras[VEHICLE_CAMERA_CHASE].position, &vehicle->frame.forward);
    position.x = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x;
    position.y = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y;
    position.z = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z;
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.x = position.x;
    vehicle->chasePosition.x = position.x;
    vehicle->initialChasePosition.x = position.x;
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.y = position.y;
    vehicle->chasePosition.y = position.y;
    vehicle->initialChasePosition.y = position.y;
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position.z = position.z;
    vehicle->chasePosition.z = position.z;
    vehicle->initialChasePosition.z = position.z;
    vehicle->collisionPull = 0.0f;
    vehicle->lookAhead = START_LOOK_AHEAD;
}

#include "camera.h"
#include "game.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>
#include <string.h>
#else
#include <PR/gu.h>
#include <PR/os_libc.h>
#endif

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

/* The smoothing of the chase camera. */
#define CHASE_SPEED_FLOOR 2.5f
#define CHASE_SPEED_MAX 31.4159279f
#define CHASE_LAG_SCALE 0.3f
#define CHASE_KEEP 0.95f
#define CHASE_NEW 0.05f
#define CHASE_PULL_SCALE 18.1818180f
#define CHASE_LAG_NUMERATOR 0.15f
#define CHASE_LAG_NUMERATOR_REPLAY 0.5f
#define CHASE_THRESHOLD 0.07f
#define CHASE_SPEED_COEFF 0.00954929646f

/* How the pull of the track on the chase camera grows and decays. */
#define PULL_MIN 0.02f
#define PULL_MAX 0.055f
#define PULL_STEP 0.01f
#define PULL_DEFAULT 0.05f
#define PULL_DECAY 0.005f

/* The frames of the timers a collision in a replay starts. */
#define REPLAY_HOLD_FRAMES 30
#define REPLAY_SWITCH_FRAMES 60

/* The cockpit camera looks along a point this far behind the vehicle, reversed. */
#define COCKPIT_AIM_DISTANCE -20.0f
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define BUMPER_FORWARD 0.6f
#else
#define BUMPER_FORWARD 0.5f
#endif

/* The zoom of the trackside camera with its distance from the vehicle. */
#define TRACKSIDE_NEAR 1.0f
#define TRACKSIDE_FAR 101.0f
#define TRACKSIDE_FAR_DISTANCE 100.0f
#define TRACKSIDE_COS_SCALE 0.0418879017f
#define TRACKSIDE_COS_WEIGHT 0.1f
#define TRACKSIDE_BIAS 0.7f
#define TRACKSIDE_MID 51.0f
#define TRACKSIDE_DISTANCE_SCALE 0.004f

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define COCKPIT_POSITION(axis)                                                                     \
    (((long double)model->cockpitUp * vehicle->frame.up.axis +                                     \
      (long double)model->cockpitForward * vehicle->frame.forward.axis) +                          \
     vehicle->frame.position.axis)
#define MIRROR_POSITION(axis)                                                                      \
    (((long double)model->cockpitUp * vehicle->frame.up.axis -                                     \
      ((long double)vehicle->frame.forward.axis * model->mirrorForward) * -2.0f) +                 \
     vehicle->frame.position.axis)
#else
#define COCKPIT_POSITION(axis)                                                                     \
    (model->cockpitUp * vehicle->frame.up.axis +                                                   \
     (vehicle->frame.position.axis + vehicle->frame.forward.axis * model->cockpitForward))
#define MIRROR_POSITION(axis)                                                                      \
    (model->cockpitUp * vehicle->frame.up.axis +                                                   \
     (vehicle->frame.position.axis + (vehicle->frame.forward.axis * model->mirrorForward +         \
                                      vehicle->frame.forward.axis * model->mirrorForward)))
#endif

#if TGR_PLATFORM == TGR_PLATFORM_N64 && (TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU)
void UpdateCameraViewBasisAndFov(Vehicle *vehicle, CameraFrame *camera) {
    f32 distance;
    Vec3f *up, *side;
    /* The original reserves stack for eight bytes of locals it does not use. */
    char unused[8];

    (void)unused;
    camera->up.y = 0.0f;
    camera->up.x = 0.0f;
    camera->up.z = 1.0f;
    Vec3fAdd(&camera->forward, &vehicle->frame.position, &vehicle->frame.up);
    Vec3fSubtractInPlace(&camera->forward, &camera->position);
    distance = Vec3fLength(&camera->forward);
    Vec3fDivideInPlace(&camera->forward, distance);
    up = &camera->up;
    side = &camera->side;
    Vec3fCross(side, up, &camera->forward);
    Vec3fCross(up, &camera->forward, side);
    if (distance <= TRACKSIDE_NEAR) {
        distance = 0.0f;
    } else if (TRACKSIDE_FAR <= distance) {
        distance = TRACKSIDE_FAR_DISTANCE;
    } else {
        distance -= TRACKSIDE_NEAR;
    }
    camera->fov =
        ((cosf((distance - 1.0f) * TRACKSIDE_COS_SCALE) * TRACKSIDE_COS_WEIGHT + TRACKSIDE_BIAS) +
         (TRACKSIDE_MID - distance) * TRACKSIDE_DISTANCE_SCALE) *
        g_fCameraFovRadians;
}
#endif

void UpdateCameraSystem(Vehicle *vehicle) {
    f32 previousCockpit[3];
    Vec3f target;
    VehicleModel *model;
    f32 lagSource, lag, lagNumerator, pull;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double speed, chaseTerm, lagTerm, smoothedLag, adjusted, zoom;
    f32 distance;
#else
    f32 speed, chaseTerm, lagTerm;
#if TGR_REGION == TGR_REGION_US
    f32 distance, zoom;
#endif
#endif

    /* The smoothed speed and lag of the chase camera. */
    target.x = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x;
    target.y = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y;
    target.z = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z;
    speed = (f32)Vec3fLength(&vehicle->body.state.angularVelocity);
    lagSource = Vec3fLength(&vehicle->body.state.velocity) * CHASE_LAG_SCALE;
    lag = lagSource;
    speed = speed > CHASE_SPEED_FLOOR ? speed - CHASE_SPEED_FLOOR : 0.0f;
    if (CHASE_SPEED_MAX < speed) {
        speed = CHASE_SPEED_MAX;
    }
    if (CHASE_SPEED_MAX < lagSource) {
        lag = CHASE_SPEED_MAX;
    }
    if (vehicle->chaseSpeed < speed) {
        vehicle->chaseSpeed = speed;
    } else {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->chaseSpeed = (long double)vehicle->chaseSpeed * CHASE_KEEP + speed * CHASE_NEW;
#else
        vehicle->chaseSpeed = vehicle->chaseSpeed * CHASE_KEEP + speed * CHASE_NEW;
#endif
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    smoothedLag = (long double)vehicle->chaseLag * CHASE_KEEP + (long double)lag * CHASE_NEW;
    vehicle->chaseLag = (f32)smoothedLag;
#else
    vehicle->chaseLag = vehicle->chaseLag * CHASE_KEEP + lag * CHASE_NEW;
#endif
    pull = vehicle->collisionPull;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x = vehicle->chasePosition.x;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y = vehicle->chasePosition.y;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z = vehicle->chasePosition.z;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    vehicle->chaseLag = smoothedLag * (1.0f - (long double)pull * CHASE_PULL_SCALE);
#else
    vehicle->chaseLag = vehicle->chaseLag * (1.0f - pull * CHASE_PULL_SCALE);
#endif

    lagNumerator = gInstantReplayActive ? CHASE_LAG_NUMERATOR_REPLAY : CHASE_LAG_NUMERATOR;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    chaseTerm = (long double)vehicle->chaseSpeed * CHASE_SPEED_COEFF;
    lagTerm = ((long double)lagNumerator * 0.0318309888f) * vehicle->chaseLag;
#else
    chaseTerm = vehicle->chaseSpeed * CHASE_SPEED_COEFF;
    lagTerm = vehicle->chaseLag * (lagNumerator / CHASE_SPEED_MAX);
#endif
    if (CHASE_THRESHOLD < pull + (chaseTerm - lagTerm)) {
        UpdateCameraChase(vehicle, &vehicle->cameras[VEHICLE_CAMERA_CHASE], 0.0f);
    } else {
        UpdateCameraChase(vehicle,
                          &vehicle->cameras[VEHICLE_CAMERA_CHASE],
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                          lagTerm - (((long double)pull + chaseTerm) - CHASE_THRESHOLD));
#else
                          ((CHASE_THRESHOLD - pull) - chaseTerm) + lagTerm);
#endif
    }
    UpdateCameraLookAtTarget(vehicle);
    vehicle->chasePosition.x = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x;
    vehicle->chasePosition.y = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y;
    vehicle->chasePosition.z = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z;

    /* Keep the chase camera out of the track, and pull it in harder while it hits. */
    if (!vehicle->flyMode) {
        ResolveVehicleTrackCollision(vehicle, &vehicle->cameras[VEHICLE_CAMERA_CHASE], &target);
        if (g_bVehicleTrackCollisionCorrected) {
            if (gInstantReplayActive) {
                g_dwReplayCameraHoldTimer = REPLAY_HOLD_FRAMES;
                if (vehicle->activeCamera == &vehicle->cameras[VEHICLE_CAMERA_CHASE]) {
                    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
                    vehicle->cameraChanged = 2;
                    g_dwReplayCameraSwitchTimer = REPLAY_SWITCH_FRAMES;
                }
            }
            if (vehicle->collisionPull < PULL_MIN) {
                vehicle->collisionPull = PULL_MIN;
            } else {
                vehicle->collisionPull += PULL_STEP;
                if (PULL_MAX < vehicle->collisionPull) {
                    vehicle->collisionPull = PULL_MAX;
                }
            }
            /* Whichever way it went, the original then sets the default pull. */
            vehicle->collisionPull = PULL_DEFAULT;
        } else {
            if (g_dwReplayCameraHoldTimer != 0) {
                g_dwReplayCameraHoldTimer--;
            }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            adjusted = (long double)vehicle->collisionPull - PULL_DECAY;
            vehicle->collisionPull = (f32)adjusted;
            if (adjusted < 0.0f) {
#else
            vehicle->collisionPull -= PULL_DECAY;
            if (vehicle->collisionPull < 0.0f) {
#endif
                vehicle->collisionPull = 0.0f;
            }
        }
    }

    ComputeCameraBasis(vehicle, &vehicle->cameras[VEHICLE_CAMERA_CHASE]);

    /* The cockpit camera sits at the eye of the driver and looks along the vehicle. */
    previousCockpit[0] = vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.x;
    previousCockpit[1] = vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.y;
    previousCockpit[2] = vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.z;
    (void)previousCockpit;
    if (vehicle->flyMode) {
        memcpy(&vehicle->cameras[VEHICLE_CAMERA_COCKPIT], &vehicle->frame, sizeof(CameraFrame));
    } else {
        model = vehicle->model;
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.x = COCKPIT_POSITION(x);
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.y = COCKPIT_POSITION(y);
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.z = COCKPIT_POSITION(z);
        Vec3fMulAddInPlace(&vehicle->cameras[VEHICLE_CAMERA_COCKPIT].forward,
                           &vehicle->frame.forward,
                           COCKPIT_AIM_DISTANCE);
        Vec3fNegate(&vehicle->cameras[VEHICLE_CAMERA_COCKPIT].forward,
                    &vehicle->cameras[VEHICLE_CAMERA_COCKPIT].forward);
        Vec3fNormalize(&vehicle->cameras[VEHICLE_CAMERA_COCKPIT].forward);
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].side = vehicle->frame.side;
        Vec3fCross(&vehicle->cameras[VEHICLE_CAMERA_COCKPIT].up,
                   &vehicle->cameras[VEHICLE_CAMERA_COCKPIT].forward,
                   &vehicle->cameras[VEHICLE_CAMERA_COCKPIT].side);
    }

    /* The bumper camera is the cockpit camera moved forward. */
    memcpy(&vehicle->cameras[VEHICLE_CAMERA_BUMPER],
           &vehicle->cameras[VEHICLE_CAMERA_COCKPIT],
           sizeof(CameraFrame));
    Vec3fMulAddInPlace(
        &vehicle->cameras[VEHICLE_CAMERA_BUMPER].position, &vehicle->frame.forward, BUMPER_FORWARD);

    /* The trackside camera looks at the vehicle from where it stands, and zooms with distance. The
     * European and Japanese releases call the routine that does the same. */
#if TGR_PLATFORM == TGR_PLATFORM_N64 && (TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP)
    UpdateCameraViewBasisAndFov(vehicle, &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]);
#else
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].up.x = 0.0f;
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].up.y = 0.0f;
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].up.z = 1.0f;
    Vec3fAdd(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
             &vehicle->frame.position,
             &vehicle->frame.up);
    Vec3fSubtractInPlace(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
                         &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position);
    distance = Vec3fLength(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
    Vec3fDivideInPlace(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].forward, distance);
    Vec3fCross(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].side,
               &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].up,
               &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
    Vec3fCross(&vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].up,
               &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
               &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].side);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!(TRACKSIDE_NEAR < distance)) {
#else
    if (distance <= TRACKSIDE_NEAR) {
#endif
        distance = 0.0f;
    } else if (TRACKSIDE_FAR <= distance) {
        distance = TRACKSIDE_FAR_DISTANCE;
    } else {
        distance -= TRACKSIDE_NEAR;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    zoom =
        cosl((f32)(((long double)distance - 1.0f) * TRACKSIDE_COS_SCALE)) * TRACKSIDE_COS_WEIGHT -
        ((long double)TRACKSIDE_MID - distance) * -TRACKSIDE_DISTANCE_SCALE;
    zoom -= -TRACKSIDE_BIAS;
#else
    zoom = (cosf((distance - 1.0f) * TRACKSIDE_COS_SCALE) * TRACKSIDE_COS_WEIGHT + TRACKSIDE_BIAS) +
           (TRACKSIDE_MID - distance) * TRACKSIDE_DISTANCE_SCALE;
#endif
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].fov = zoom * g_fCameraFovRadians;
#endif
    vehicle->cameras[VEHICLE_CAMERA_BUMPER].fov = g_fCameraFovRadians;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].fov = g_fCameraFovRadians;
    vehicle->cameras[VEHICLE_CAMERA_COCKPIT].fov = g_fCameraFovRadians;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].fov = g_fCameraFovRadians;

    /* The mirror looks back from above and ahead of the driver. */
    model = vehicle->model;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].position.x = MIRROR_POSITION(x);
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].position.y = MIRROR_POSITION(y);
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].position.z = MIRROR_POSITION(z);
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].forward.x = -vehicle->frame.forward.x;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].forward.y = -vehicle->frame.forward.y;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].forward.z = -vehicle->frame.forward.z;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].side.x = -vehicle->frame.side.x;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].side.z = -vehicle->frame.side.z;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].side.y = -vehicle->frame.side.y;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].up = vehicle->frame.up;
    vehicle->previousCamera = vehicle->activeCamera;
}

#include "camera.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "game.h"
#include "system.h"
#include "tricache.h"
#include "util.h"
#include "visgrid.h"

/* How far in front of a triangle the track puts a camera. */
#define TRACK_CLEARANCE 0.1f

/* The look-at point and its lead ahead of the vehicle. */
#define LOOK_AT_HEIGHT 0.66f
#define LOCKED_LOOK_AT_HEIGHT 1.1f
#define LOOK_AHEAD_SLOW 2.0f
#define LOOK_AHEAD_SLOW_SPEED 3.5f
#define LOOK_AHEAD_FAST_SPEED 7.0f
#define LOOK_AHEAD_SLOPE 0.571428597f
#define LOOK_AHEAD_STEP 0.1f

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
#define BUMPER_FORWARD 0.5f

/* The zoom of the trackside camera with its distance from the vehicle. */
#define TRACKSIDE_NEAR 1.0f
#define TRACKSIDE_FAR 101.0f
#define TRACKSIDE_FAR_DISTANCE 100.0f
#define TRACKSIDE_COS_SCALE 0.0418879017f
#define TRACKSIDE_COS_WEIGHT 0.1f
#define TRACKSIDE_BIAS 0.7f
#define TRACKSIDE_MID 51.0f
#define TRACKSIDE_DISTANCE_SCALE 0.004f

/* Where the cameras start, and where PlaceTrackCamera() puts the trackside camera. */
#define START_CHASE_HEIGHT 4.0f
#define START_CHASE_REPLAY_FORWARD 10.0f
#define START_LOOK_AHEAD 2.0f
#define TRACKSIDE_FORWARD 6.0f
#define TRACKSIDE_SIDE 2.0f

/* The share of the way from the camera to each corner of the far rectangle the frustum keeps. */
#define FRUSTUM_CORNER_SHARE 0.75f

/* The perspective of a camera: its near plane, the shape of the screen, and degrees a radian. */
#define CAMERA_NEAR 2.4f
#define ASPECT_4_3 1.3333334f
#define DEGREES_PER_RADIAN 57.295776f

/* The point SetupProjectionMatrix() looks at, its distance, and the perspective, in degrees. */
#define SCREEN_CENTER_X 512.0f
#define SCREEN_CENTER_Y 384.0f
#define SCREEN_DISTANCE 1000.0f
#define SCREEN_FOV 45.0f
#define SCREEN_NEAR 10.0f
#define SCREEN_FAR 2000.0f

/* The point SetupMenuProjection() looks at, its distance, and its default clip planes. */
#define MENU_CENTER_X 160.0f
#define MENU_CENTER_Y 120.0f
#define MENU_DISTANCE 290.0f
#define MENU_NEAR 1.0
#define MENU_FAR 500.0

/* The RSP takes physical addresses, which are the KSEG0 addresses less its base. */
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))

/*
 * Find the nearest triangle of the cached slots that the line from start along direction crosses
 * between 0 and limit, and the point where it does, in hit and hitPoint.
 */
#define FIND_TRACK_HIT(start)                                                                      \
    {                                                                                              \
        hit = NULL;                                                                                \
        for (i = 0; i < slotCount; i++) {                                                          \
            triangle = g_aTriCacheTriangles[slots[i]];                                             \
            end = triangle + g_awTriCacheTriCount[slots[i]];                                       \
            for (; triangle != end; triangle++) {                                                  \
                facing = Vec3fDot(&direction, &triangle->normal);                                  \
                if (!(facing < 0.0f)) {                                                            \
                    continue;                                                                      \
                }                                                                                  \
                Vec3fSubtract(&toVertex, triangle->vertices[0], start);                            \
                t = Vec3fDot(&toVertex, &triangle->normal) / facing;                               \
                if (!(0.0f < t) || !(t < limit)) {                                                 \
                    continue;                                                                      \
                }                                                                                  \
                Vec3fMulAdd(&point, start, &direction, t);                                         \
                if (PointInTriangle3D(&point,                                                      \
                                      triangle->vertices[0],                                       \
                                      triangle->vertices[1],                                       \
                                      triangle->vertices[2],                                       \
                                      &triangle->normal)) {                                        \
                    limit = t;                                                                     \
                    hit = triangle;                                                                \
                    hitPoint = point;                                                              \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    }

void ResolveVehicleTrackCollision(Vehicle *vehicle, CameraFrame *camera, Vec3f *target) {
    Vec3f direction, point, toVertex;
    s32 i;
    s32 slots[2];
    s32 slotCount;
    Vec3f hitPoint, offset;
    TrackTriangle *hit, *triangle, *end;
    f32 length, limit, distance, newLength, facing, t;

    g_bVehicleTrackCollisionCorrected = FALSE;
    slots[0] = BuildTrackTriangleCache(vehicle->lookAt.x, vehicle->lookAt.y);
    slots[1] = BuildTrackTriangleCache(camera->position.x, camera->position.y);
    slotCount = slots[0] == slots[1] ? 1 : 2;

    /* The line from the look-at point to just beyond the camera. */
    Vec3fSubtract(&direction, &camera->position, &vehicle->lookAt);
    length = Vec3fLength(&direction);
    limit = length != 0.0f ? (length + TRACK_CLEARANCE) / length : 1.0f;
    FIND_TRACK_HIT(&vehicle->lookAt);
    if (hit == NULL) {
        return;
    }

    /* The line from the target to the camera. */
    Vec3fSubtract(&direction, &camera->position, target);
    limit = 1.0f;
    FIND_TRACK_HIT(target);
    if (hit == NULL) {
        return;
    }
    distance = Vec3fDistance(&camera->position, &vehicle->lookAt);
    Vec3fMulAdd(&camera->position, &hitPoint, &hit->normal, TRACK_CLEARANCE);
    Vec3fSubtract(&offset, &camera->position, &vehicle->lookAt);
    newLength = Vec3fLength(&offset);
    if (distance < newLength && newLength != 0.0f) {
        Vec3fScaleInPlace(&offset, distance / newLength);
        Vec3fAdd(&camera->position, &vehicle->lookAt, &offset);
    }
    g_bVehicleTrackCollisionCorrected = TRUE;
}

void UpdateCameraLookAtTarget(Vehicle *vehicle) {
    f32 speed, target;

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
        target = 4.0f - speed * LOOK_AHEAD_SLOPE;
    }
    if (vehicle->lookAhead < target) {
        vehicle->lookAhead += LOOK_AHEAD_STEP;
        if (target < vehicle->lookAhead) {
            vehicle->lookAhead = target;
        }
    } else if (target < vehicle->lookAhead) {
        vehicle->lookAhead -= LOOK_AHEAD_STEP;
        if (vehicle->lookAhead < target) {
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

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
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
    f32 speed, lagSource, lag, lagNumerator, chaseTerm, lagTerm, pull;
#if TGR_REGION == TGR_REGION_US
    f32 distance, zoom;
#endif

    /* The smoothed speed and lag of the chase camera. */
    target.x = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x;
    target.y = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y;
    target.z = vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z;
    speed = Vec3fLength(&vehicle->body.state.angularVelocity);
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
        vehicle->chaseSpeed = vehicle->chaseSpeed * CHASE_KEEP + speed * CHASE_NEW;
    }
    vehicle->chaseLag = vehicle->chaseLag * CHASE_KEEP + lag * CHASE_NEW;
    pull = vehicle->collisionPull;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].position.x = vehicle->chasePosition.x;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].position.y = vehicle->chasePosition.y;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].position.z = vehicle->chasePosition.z;
    vehicle->chaseLag = vehicle->chaseLag * (1.0f - pull * CHASE_PULL_SCALE);

    lagNumerator = gInstantReplayActive ? CHASE_LAG_NUMERATOR_REPLAY : CHASE_LAG_NUMERATOR;
    chaseTerm = vehicle->chaseSpeed * CHASE_SPEED_COEFF;
    lagTerm = vehicle->chaseLag * (lagNumerator / CHASE_SPEED_MAX);
    if (CHASE_THRESHOLD < pull + (chaseTerm - lagTerm)) {
        UpdateCameraChase(vehicle, &vehicle->cameras[VEHICLE_CAMERA_CHASE], 0.0f);
    } else {
        UpdateCameraChase(vehicle,
                          &vehicle->cameras[VEHICLE_CAMERA_CHASE],
                          ((CHASE_THRESHOLD - pull) - chaseTerm) + lagTerm);
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
            vehicle->collisionPull -= PULL_DECAY;
            if (vehicle->collisionPull < 0.0f) {
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
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.x =
            model->cockpitUp * vehicle->frame.up.x +
            (vehicle->frame.position.x + vehicle->frame.forward.x * model->cockpitForward);
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.y =
            model->cockpitUp * vehicle->frame.up.y +
            (vehicle->frame.position.y + vehicle->frame.forward.y * model->cockpitForward);
        vehicle->cameras[VEHICLE_CAMERA_COCKPIT].position.z =
            model->cockpitUp * vehicle->frame.up.z +
            (vehicle->frame.position.z + vehicle->frame.forward.z * model->cockpitForward);
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
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
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
    if (distance <= TRACKSIDE_NEAR) {
        distance = 0.0f;
    } else if (TRACKSIDE_FAR <= distance) {
        distance = TRACKSIDE_FAR_DISTANCE;
    } else {
        distance -= TRACKSIDE_NEAR;
    }
    zoom = (cosf((distance - 1.0f) * TRACKSIDE_COS_SCALE) * TRACKSIDE_COS_WEIGHT + TRACKSIDE_BIAS) +
           (TRACKSIDE_MID - distance) * TRACKSIDE_DISTANCE_SCALE;
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].fov = zoom * g_fCameraFovRadians;
#endif
    vehicle->cameras[VEHICLE_CAMERA_BUMPER].fov = g_fCameraFovRadians;
    vehicle->cameras[VEHICLE_CAMERA_CHASE].fov = g_fCameraFovRadians;
    vehicle->cameras[VEHICLE_CAMERA_COCKPIT].fov = g_fCameraFovRadians;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].fov = g_fCameraFovRadians;

    /* The mirror looks back from above and ahead of the driver. */
    model = vehicle->model;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].position.x =
        model->cockpitUp * vehicle->frame.up.x +
        (vehicle->frame.position.x + (vehicle->frame.forward.x * model->mirrorForward +
                                      vehicle->frame.forward.x * model->mirrorForward));
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].position.y =
        model->cockpitUp * vehicle->frame.up.y +
        (vehicle->frame.position.y + (vehicle->frame.forward.y * model->mirrorForward +
                                      vehicle->frame.forward.y * model->mirrorForward));
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].position.z =
        model->cockpitUp * vehicle->frame.up.z +
        (vehicle->frame.position.z + (vehicle->frame.forward.z * model->mirrorForward +
                                      vehicle->frame.forward.z * model->mirrorForward));
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].forward.x = -vehicle->frame.forward.x;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].forward.y = -vehicle->frame.forward.y;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].forward.z = -vehicle->frame.forward.z;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].side.x = -vehicle->frame.side.x;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].side.z = -vehicle->frame.side.z;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].side.y = -vehicle->frame.side.y;
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].up = vehicle->frame.up;
    vehicle->previousCamera = vehicle->activeCamera;
}

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

void ClearCameraChanged(Vehicle *vehicle) {
    vehicle->cameraChanged = 0;
}

void SetupFrustumCorners(CameraFrame *camera, f32 fov, f32 farDistance, f32 width, f32 height) {
    f32 halfWidth = sinf(fov) * farDistance;
    f32 halfHeight = halfWidth * height / width;

    if (g_dwNumViewports == 2) {
        halfHeight *= 0.5f;
    }
    g_avFrustumPoints[FRUSTUM_CAMERA].x = camera->position.x;
    g_avFrustumPoints[FRUSTUM_CAMERA].y = camera->position.y;
    g_avFrustumPoints[FRUSTUM_CAMERA].z = camera->position.z;
    Vec3fMulAdd(
        &g_afFrustumCenter, &g_avFrustumPoints[FRUSTUM_CAMERA], &camera->forward, farDistance);
    Vec3fScale(&g_vFrustumHalfRight, &camera->side, halfWidth);
    Vec3fScale(&g_vFrustumHalfUp, &camera->up, halfHeight);
    g_avFrustumPoints[FRUSTUM_FAR].x = g_afFrustumCenter.x;
    g_avFrustumPoints[FRUSTUM_FAR].y = g_afFrustumCenter.y;
    g_avFrustumPoints[FRUSTUM_FAR].z = g_afFrustumCenter.z;

    Vec3fAdd(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fAddInPlace(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP], &g_vFrustumHalfUp);
    Vec3fAdd(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fSubtractInPlace(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM], &g_vFrustumHalfUp);
    Vec3fSubtract(&g_avFrustumPoints[FRUSTUM_LEFT_TOP], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fAddInPlace(&g_avFrustumPoints[FRUSTUM_LEFT_TOP], &g_vFrustumHalfUp);
    Vec3fSubtract(
        &g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fSubtractInPlace(&g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM], &g_vFrustumHalfUp);

    /* The corners are pulled a quarter of the way back towards the camera; the far point is not. */
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_LEFT_TOP],
              &g_avFrustumPoints[FRUSTUM_LEFT_TOP],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP],
              &g_avFrustumPoints[FRUSTUM_RIGHT_TOP],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    gFrustumFar = farDistance;
    g_fFrustumFovAngle = fov;
}

void SetupCameraMatrix(CameraFrame *camera, f32 fov, f32 farDistance, f32 width, f32 height) {
    guLookAtF(g_ViewLookAtMtx,
              camera->position.x,
              camera->position.y,
              camera->position.z,
              camera->forward.x + camera->position.x,
              camera->forward.y + camera->position.y,
              camera->forward.z + camera->position.z,
              camera->up.x,
              camera->up.y,
              camera->up.z);
    g_fCameraFarDistance = farDistance;
    g_fCameraNearScale = CAMERA_NEAR;
    guPerspectiveF(g_afPerspectiveMatrix,
                   &g_wPerspNorm,
                   fov * ASPECT_4_3 * (height / width) * DEGREES_PER_RADIAN,
                   width / height,
                   g_fCameraNearScale,
                   g_fCameraFarDistance,
                   1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afPerspectiveMatrix, g_afViewProjMatrix);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
}

/* Look at a point of a flat screen from in front of it, with y up. */
#define LOOK_AT_SCREEN(x, y, distance)                                                             \
    guLookAtF(g_ViewLookAtMtx, x, y, distance, x, y, 0.0f, 0.0f, 1.0f, 0.0f)

void SetupProjectionMatrix(f32 width, f32 height) {
    LOOK_AT_SCREEN(SCREEN_CENTER_X, SCREEN_CENTER_Y, SCREEN_DISTANCE);
    guPerspectiveF(
        g_afScratchMatrix, &g_wPerspNorm, SCREEN_FOV, ASPECT_4_3, SCREEN_NEAR, SCREEN_FAR, 1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afScratchMatrix, g_afViewProjMatrix);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
}

void SetupMenuProjection(f32 width, f32 height) {
    LOOK_AT_SCREEN(MENU_CENTER_X, MENU_CENTER_Y, MENU_DISTANCE);
    if (g_dwKeepCustomClipPlanes) {
        g_dwKeepCustomClipPlanes = FALSE;
    } else {
        g_fCameraNearScale = MENU_NEAR;
        g_fCameraFarDistance = MENU_FAR;
    }
    guPerspectiveF(g_afPerspectiveMatrix,
                   &g_wPerspNorm,
                   SCREEN_FOV,
                   ASPECT_4_3,
                   g_fCameraNearScale,
                   g_fCameraFarDistance,
                   1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afPerspectiveMatrix, g_afViewProjMatrix);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
}

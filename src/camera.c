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

/* The distances of the chase camera behind and above the vehicle. */
#define CHASE_DISTANCE 11.0f
#define CHASE_DISTANCE_SPLIT_SCREEN 19.8f
#define CHASE_HEIGHT 2.4f
#define CHASE_DISTANCE_AFTER_SEASON 13.0f

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
#define MENU_NEAR 1.0f
#define MENU_FAR 500.0f

/* The RSP takes physical addresses, which are the KSEG0 addresses less its base. */
#define TO_RSP_ADDRESS(p) ((u32)(p) - K0BASE)

/*
 * Find the nearest triangle of the cached slots that the line from start along direction crosses
 * between 0 and limit, and the point where it does.
 */
static TrackTriangle *FindTrackHit(
    s32 *slots, s32 slotCount, Vec3f *start, Vec3f *direction, f32 limit, Vec3f *hitPoint) {
    TrackTriangle *triangle, *end, *hit = NULL;
    Vec3f toVertex, point;
    f32 facing, t;
    s32 i;

    for (i = 0; i < slotCount; i++) {
        triangle = g_aTriCacheTriangles[slots[i]];
        end = triangle + g_awTriCacheTriCount[slots[i]];
        for (; triangle != end; triangle++) {
            facing = Vec3fDot(direction, &triangle->normal);
            if (!(facing < 0.0f)) {
                continue;
            }
            Vec3fSubtract(&toVertex, triangle->vertices[0], start);
            t = Vec3fDot(&toVertex, &triangle->normal) / facing;
            if (!(0.0f < t) || !(t < limit)) {
                continue;
            }
            Vec3fMulAdd(&point, start, direction, t);
            if (PointInTriangle3D(&point,
                                  triangle->vertices[0],
                                  triangle->vertices[1],
                                  triangle->vertices[2],
                                  &triangle->normal)) {
                limit = t;
                hit = triangle;
                *hitPoint = point;
            }
        }
    }
    return hit;
}

void ResolveVehicleTrackCollision(Vehicle *vehicle, CameraFrame *camera, Vec3f *target) {
    s32 slots[2];
    s32 slotCount;
    Vec3f direction, hitPoint, offset;
    TrackTriangle *hit;
    f32 length, limit, distance, newLength;

    g_bVehicleTrackCollisionCorrected = FALSE;
    slots[0] = BuildTrackTriangleCache(vehicle->lookAt.x, vehicle->lookAt.y);
    slots[1] = BuildTrackTriangleCache(camera->position.x, camera->position.y);
    slotCount = slots[0] == slots[1] ? 1 : 2;

    /* The line from the look-at point to just beyond the camera. */
    Vec3fSubtract(&direction, &camera->position, &vehicle->lookAt);
    length = Vec3fLength(&direction);
    limit = length == 0.0f ? 1.0f : (length + TRACK_CLEARANCE) / length;
    if (FindTrackHit(slots, slotCount, &vehicle->lookAt, &direction, limit, &hitPoint) == NULL) {
        return;
    }

    /* The line from the target to the camera. */
    Vec3fSubtract(&direction, &camera->position, target);
    hit = FindTrackHit(slots, slotCount, target, &direction, 1.0f, &hitPoint);
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

void UpdateCameraChase(Vehicle *vehicle, CameraFrame *camera, f32 share) {
    Vec3f previous, offset;
    f32 length;

    if (vehicle->flyMode) {
        Vec3fMulAdd(&camera->position, &vehicle->frame.position, &vehicle->frame.up, CHASE_HEIGHT);
        Vec3fMulAdd(&camera->position,
                    &camera->position,
                    &vehicle->frame.forward,
                    g_dwNumViewports == 1 ? -CHASE_DISTANCE : -CHASE_DISTANCE_SPLIT_SCREEN);
        return;
    }

    /* The original keeps a copy of the old position that it does not use. */
    previous = camera->position;
    (void)previous;

    /* The direction the camera was in from the vehicle, at the chase distance. */
    camera->position.z -= CHASE_HEIGHT;
    Vec3fSubtract(&offset, &camera->position, &vehicle->frame.position);
    length = Vec3fLength(&offset);
    if (length != 0.0f) {
        Vec3fScaleInPlace(&offset,
                          (g_dwNumViewports == 1 ? CHASE_DISTANCE : CHASE_DISTANCE_SPLIT_SCREEN) /
                              length);
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
    Vec3f worldUp;
    f32 length;

    Vec3fSubtract(&direction, &vehicle->lookAt, &camera->position);
    length = Vec3fLength(&direction);
    if (length != 0.0f) {
        Vec3fScaleByInverse(&camera->forward, &direction, length);
    } else if (Vec3fLength(&camera->forward) == 0.0f) {
        camera->forward = vehicle->frame.forward;
    }
    if (vehicle->flyMode) {
        Vec3fCross(&camera->side, &vehicle->frame.up, &camera->forward);
    } else {
        worldUp.x = 0.0f;
        worldUp.y = 0.0f;
        worldUp.z = 1.0f;
        Vec3fCross(&camera->side, &worldUp, &camera->forward);
    }
    Vec3fCross(&camera->up, &camera->forward, &camera->side);
}

void PlaceTrackCamera(Vehicle *vehicle) {
    CameraFrame *camera = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];

    vehicle->activeCamera = camera;
    Vec3fMulAdd(
        &camera->position, &vehicle->frame.position, &vehicle->frame.forward, TRACKSIDE_FORWARD);
    Vec3fMulAddInPlace(&camera->position, &vehicle->frame.side, TRACKSIDE_SIDE);
    Vec3fAddInPlace(&camera->position, &vehicle->frame.up);
    vehicle->cameraChanged = 2;
}

void UpdateCameraSystem(Vehicle *vehicle) {
    CameraFrame *chase = &vehicle->cameras[VEHICLE_CAMERA_CHASE];
    CameraFrame *cockpit = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
    CameraFrame *bumper = &vehicle->cameras[VEHICLE_CAMERA_BUMPER];
    CameraFrame *trackside = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    CameraFrame *mirror = &vehicle->cameras[VEHICLE_CAMERA_MIRROR];
    VehicleModel *model;
    Vec3f target, previousCockpit;
    f32 speed, lagSource, lag, lagNumerator, chaseTerm, lagTerm, pull, distance, zoom;

    /* The smoothed speed and lag of the chase camera. */
    target = chase->position;
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
    chase->position = vehicle->chasePosition;
    vehicle->chaseLag = vehicle->chaseLag * (1.0f - pull * CHASE_PULL_SCALE);

    lagNumerator = gInstantReplayActive ? CHASE_LAG_NUMERATOR_REPLAY : CHASE_LAG_NUMERATOR;
    chaseTerm = vehicle->chaseSpeed * CHASE_SPEED_COEFF;
    lagTerm = vehicle->chaseLag * (lagNumerator / CHASE_SPEED_MAX);
    if (CHASE_THRESHOLD < pull + (chaseTerm - lagTerm)) {
        UpdateCameraChase(vehicle, chase, 0.0f);
    } else {
        UpdateCameraChase(vehicle, chase, ((CHASE_THRESHOLD - pull) - chaseTerm) + lagTerm);
    }
    UpdateCameraLookAtTarget(vehicle);
    vehicle->chasePosition = chase->position;

    /* Keep the chase camera out of the track, and pull it in harder while it hits. */
    if (!vehicle->flyMode) {
        ResolveVehicleTrackCollision(vehicle, chase, &target);
        if (g_bVehicleTrackCollisionCorrected) {
            if (gInstantReplayActive) {
                g_dwReplayCameraHoldTimer = REPLAY_HOLD_FRAMES;
                if (vehicle->activeCamera == chase) {
                    vehicle->activeCamera = cockpit;
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

    ComputeCameraBasis(vehicle, chase);

    /* The cockpit camera sits at the eye of the driver and looks along the vehicle. */
    previousCockpit = cockpit->position;
    (void)previousCockpit;
    if (vehicle->flyMode) {
        memcpy(cockpit, &vehicle->frame, sizeof(CameraFrame));
    } else {
        model = vehicle->model;
        cockpit->position.x =
            model->cockpitUp * vehicle->frame.up.x +
            (vehicle->frame.position.x + vehicle->frame.forward.x * model->cockpitForward);
        cockpit->position.y =
            model->cockpitUp * vehicle->frame.up.y +
            (vehicle->frame.position.y + vehicle->frame.forward.y * model->cockpitForward);
        cockpit->position.z =
            model->cockpitUp * vehicle->frame.up.z +
            (vehicle->frame.position.z + vehicle->frame.forward.z * model->cockpitForward);
        Vec3fMulAddInPlace(&cockpit->forward, &vehicle->frame.forward, COCKPIT_AIM_DISTANCE);
        Vec3fNegate(&cockpit->forward, &cockpit->forward);
        Vec3fNormalize(&cockpit->forward);
        cockpit->side = vehicle->frame.side;
        Vec3fCross(&cockpit->up, &cockpit->forward, &cockpit->side);
    }

    /* The bumper camera is the cockpit camera moved forward. */
    memcpy(bumper, cockpit, sizeof(CameraFrame));
    Vec3fMulAddInPlace(&bumper->position, &vehicle->frame.forward, BUMPER_FORWARD);

    /* The trackside camera looks at the vehicle from where it stands, and zooms with distance. */
    trackside->up.x = 0.0f;
    trackside->up.y = 0.0f;
    trackside->up.z = 1.0f;
    Vec3fAdd(&trackside->forward, &vehicle->frame.position, &vehicle->frame.up);
    Vec3fSubtractInPlace(&trackside->forward, &trackside->position);
    distance = Vec3fLength(&trackside->forward);
    Vec3fDivideInPlace(&trackside->forward, distance);
    Vec3fCross(&trackside->side, &trackside->up, &trackside->forward);
    Vec3fCross(&trackside->up, &trackside->forward, &trackside->side);
    if (distance <= TRACKSIDE_NEAR) {
        distance = 0.0f;
    } else if (TRACKSIDE_FAR <= distance) {
        distance = TRACKSIDE_FAR_DISTANCE;
    } else {
        distance -= TRACKSIDE_NEAR;
    }
    zoom = (cosf((distance - 1.0f) * TRACKSIDE_COS_SCALE) * TRACKSIDE_COS_WEIGHT + TRACKSIDE_BIAS) +
           (TRACKSIDE_MID - distance) * TRACKSIDE_DISTANCE_SCALE;
    trackside->fov = zoom * g_fCameraFovRadians;
    bumper->fov = g_fCameraFovRadians;
    chase->fov = g_fCameraFovRadians;
    cockpit->fov = g_fCameraFovRadians;
    mirror->fov = g_fCameraFovRadians;

    /* The mirror looks back from above and ahead of the driver. */
    model = vehicle->model;
    mirror->position.x =
        model->cockpitUp * vehicle->frame.up.x +
        (vehicle->frame.position.x + (vehicle->frame.forward.x * model->mirrorForward +
                                      vehicle->frame.forward.x * model->mirrorForward));
    mirror->position.y =
        model->cockpitUp * vehicle->frame.up.y +
        (vehicle->frame.position.y + (vehicle->frame.forward.y * model->mirrorForward +
                                      vehicle->frame.forward.y * model->mirrorForward));
    mirror->position.z =
        model->cockpitUp * vehicle->frame.up.z +
        (vehicle->frame.position.z + (vehicle->frame.forward.z * model->mirrorForward +
                                      vehicle->frame.forward.z * model->mirrorForward));
    mirror->forward.x = -vehicle->frame.forward.x;
    mirror->forward.y = -vehicle->frame.forward.y;
    mirror->forward.z = -vehicle->frame.forward.z;
    mirror->side.x = -vehicle->frame.side.x;
    mirror->side.z = -vehicle->frame.side.z;
    mirror->side.y = -vehicle->frame.side.y;
    mirror->up = vehicle->frame.up;
    vehicle->previousCamera = vehicle->activeCamera;
}

void InitCameraForVehicle(Vehicle *vehicle) {
    CameraFrame *chase = &vehicle->cameras[VEHICLE_CAMERA_CHASE];

    if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        vehicle->activeCamera = chase;
        vehicle->previousCamera = chase;
    } else {
        vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
        vehicle->previousCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
    }
    Vec3fMulAdd(&chase->position, &vehicle->frame.position, &vehicle->frame.up, START_CHASE_HEIGHT);
    if (gInstantReplayActive) {
        Vec3fMulAddInPlace(&chase->position, &vehicle->frame.forward, START_CHASE_REPLAY_FORWARD);
    }
    Vec3fSubtractInPlace(&chase->position, &vehicle->frame.forward);
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].position = chase->position;
    vehicle->chasePosition = chase->position;
    vehicle->initialChasePosition = chase->position;
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
    g_avFrustumPoints[FRUSTUM_CAMERA] = camera->position;
    Vec3fMulAdd(
        &g_afFrustumCenter, &g_avFrustumPoints[FRUSTUM_CAMERA], &camera->forward, farDistance);
    Vec3fScale(&g_vFrustumHalfRight, &camera->side, halfWidth);
    Vec3fScale(&g_vFrustumHalfUp, &camera->up, halfHeight);
    g_avFrustumPoints[FRUSTUM_FAR] = g_afFrustumCenter;

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
static void LookAtScreen(f32 x, f32 y, f32 distance) {
    guLookAtF(g_ViewLookAtMtx, x, y, distance, x, y, 0.0f, 0.0f, 1.0f, 0.0f);
}

/* Load the view and projection of the current view into the display list. */
static void LoadViewProjection(void) {
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
}

void SetupProjectionMatrix(f32 width, f32 height) {
    LookAtScreen(SCREEN_CENTER_X, SCREEN_CENTER_Y, SCREEN_DISTANCE);
    guPerspectiveF(
        g_afScratchMatrix, &g_wPerspNorm, SCREEN_FOV, ASPECT_4_3, SCREEN_NEAR, SCREEN_FAR, 1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afScratchMatrix, g_afViewProjMatrix);
    LoadViewProjection();
}

void SetupMenuProjection(f32 width, f32 height) {
    LookAtScreen(MENU_CENTER_X, MENU_CENTER_Y, MENU_DISTANCE);
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
    LoadViewProjection();
}

#include <math.h>
#include <string.h>

#include "common/camera.h"
#include "common/game.h"
#include "common/vehicleaccess.h"

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

/* The zoom of the trackside camera with its distance from the vehicle. */
#define TRACKSIDE_NEAR 1.0f
#define TRACKSIDE_FAR 101.0f
#define TRACKSIDE_FAR_DISTANCE 100.0f
#define TRACKSIDE_COS_SCALE 0.0418879017f
#define TRACKSIDE_COS_WEIGHT 0.1f
#define TRACKSIDE_BIAS 0.7f
#define TRACKSIDE_MID 51.0f
#define TRACKSIDE_DISTANCE_SCALE 0.004f

/* The cockpit camera looks along a point this far behind the vehicle, reversed. */
#define COCKPIT_AIM_DISTANCE -20.0f
#if TGR_BOSS_RALLY
#define BUMPER_FORWARD 0.6f
#else
#define BUMPER_FORWARD 0.5f
#endif

#if TGR_BOSS_RALLY
#define COCKPIT_POSITION(axis)                                                                     \
    (((long double)model->cockpitUp * VEHICLE_GET(vehicle, Frame).up.axis +                        \
      (long double)model->cockpitForward * VEHICLE_GET(vehicle, Frame).forward.axis) +             \
     VEHICLE_GET(vehicle, Frame).position.axis)
#define MIRROR_POSITION(axis)                                                                      \
    (((long double)model->cockpitUp * VEHICLE_GET(vehicle, Frame).up.axis -                        \
      ((long double)VEHICLE_GET(vehicle, Frame).forward.axis * model->mirrorForward) * -2.0f) +    \
     VEHICLE_GET(vehicle, Frame).position.axis)
#else
#define COCKPIT_POSITION(axis)                                                                     \
    (model->cockpitUp * VEHICLE_GET(vehicle, Frame).up.axis +                                      \
     (VEHICLE_GET(vehicle, Frame).position.axis +                                                  \
      VEHICLE_GET(vehicle, Frame).forward.axis * model->cockpitForward))
#define MIRROR_POSITION(axis)                                                                      \
    (model->cockpitUp * VEHICLE_GET(vehicle, Frame).up.axis +                                      \
     (VEHICLE_GET(vehicle, Frame).position.axis +                                                  \
      (VEHICLE_GET(vehicle, Frame).forward.axis * model->mirrorForward +                           \
       VEHICLE_GET(vehicle, Frame).forward.axis * model->mirrorForward)))
#endif /* TGR_BOSS_RALLY */

void UpdateCameraSystem(Vehicle *vehicle) {
    f32 previousCockpit[3];
    Vec3f target;
    VehicleModel *model;
    f32 lagSource, lag, lagNumerator, pull;
#if TGR_BOSS_RALLY
    long double speed, chaseTerm, lagTerm, smoothedLag, adjusted, zoom;
    f32 distance;
#else
    f32 speed, chaseTerm, lagTerm;
#if TGR_REGION == TGR_REGION_US
    f32 distance, zoom;
#endif
#endif

    /* The smoothed speed and lag of the chase camera. */
    target.x = VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.x;
    target.y = VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.y;
    target.z = VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.z;
    speed = (f32)Vec3fLength(&VEHICLE_GET(vehicle, Body).state.angularVelocity);
    lagSource = Vec3fLength(&VEHICLE_GET(vehicle, Body).state.velocity) * CHASE_LAG_SCALE;
    lag = lagSource;
    speed = speed > CHASE_SPEED_FLOOR ? speed - CHASE_SPEED_FLOOR : 0.0f;
    if (CHASE_SPEED_MAX < speed) {
        speed = CHASE_SPEED_MAX;
    }
    if (CHASE_SPEED_MAX < lagSource) {
        lag = CHASE_SPEED_MAX;
    }
    if (VEHICLE_GET(vehicle, ChaseSpeed) < speed) {
        VEHICLE_SET(vehicle, ChaseSpeed, speed);
    } else {
#if TGR_BOSS_RALLY
        VEHICLE_SET(vehicle,
                    ChaseSpeed,
                    (long double)VEHICLE_GET(vehicle, ChaseSpeed) * CHASE_KEEP + speed * CHASE_NEW);
#else
        vehicle->chaseSpeed = vehicle->chaseSpeed * CHASE_KEEP + speed * CHASE_NEW;
#endif
    }
#if TGR_BOSS_RALLY
    smoothedLag =
        (long double)VEHICLE_GET(vehicle, ChaseLag) * CHASE_KEEP + (long double)lag * CHASE_NEW;
    VEHICLE_SET(vehicle, ChaseLag, (f32)smoothedLag);
#else
    vehicle->chaseLag = vehicle->chaseLag * CHASE_KEEP + lag * CHASE_NEW;
#endif
    pull = VEHICLE_GET(vehicle, CollisionPull);
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_CHASE].position.x = VEHICLE_GET(vehicle, ChasePosition).x;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_CHASE].position.y = VEHICLE_GET(vehicle, ChasePosition).y;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_CHASE].position.z = VEHICLE_GET(vehicle, ChasePosition).z;
#if TGR_BOSS_RALLY
    VEHICLE_SET(vehicle, ChaseLag, smoothedLag * (1.0f - (long double)pull * CHASE_PULL_SCALE));
#else
    vehicle->chaseLag = vehicle->chaseLag * (1.0f - pull * CHASE_PULL_SCALE);
#endif

    lagNumerator = gInstantReplayActive ? CHASE_LAG_NUMERATOR_REPLAY : CHASE_LAG_NUMERATOR;
#if TGR_BOSS_RALLY
    chaseTerm = (long double)VEHICLE_GET(vehicle, ChaseSpeed) * CHASE_SPEED_COEFF;
    lagTerm = ((long double)lagNumerator * 0.0318309888f) * VEHICLE_GET(vehicle, ChaseLag);
#else
    chaseTerm = vehicle->chaseSpeed * CHASE_SPEED_COEFF;
    lagTerm = vehicle->chaseLag * (lagNumerator / CHASE_SPEED_MAX);
#endif
    if (CHASE_THRESHOLD < pull + (chaseTerm - lagTerm)) {
        UpdateCameraChase(vehicle, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE], 0.0f);
    } else {
        UpdateCameraChase(vehicle,
                          &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE],
#if TGR_BOSS_RALLY
                          lagTerm - (((long double)pull + chaseTerm) - CHASE_THRESHOLD));
#else
                          ((CHASE_THRESHOLD - pull) - chaseTerm) + lagTerm);
#endif
    }
    UpdateCameraLookAtTarget(vehicle);
    VEHICLE_GET(vehicle, ChasePosition).x =
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.x;
    VEHICLE_GET(vehicle, ChasePosition).y =
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.y;
    VEHICLE_GET(vehicle, ChasePosition).z =
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.z;

    /* Keep the chase camera out of the track, and pull it in harder while it hits. */
    if (!VEHICLE_GET(vehicle, FlyMode)) {
        ResolveVehicleTrackCollision(
            vehicle, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE], &target);
        if (g_bVehicleTrackCollisionCorrected) {
            if (gInstantReplayActive) {
                g_dwReplayCameraHoldTimer = REPLAY_HOLD_FRAMES;
                if (VEHICLE_GET(vehicle, ActiveCamera) ==
                    &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE]) {
                    VEHICLE_SET(vehicle,
                                ActiveCamera,
                                &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT]);
                    VEHICLE_SET(vehicle, CameraChanged, 2);
                    g_dwReplayCameraSwitchTimer = REPLAY_SWITCH_FRAMES;
                }
            }
            if (VEHICLE_GET(vehicle, CollisionPull) < PULL_MIN) {
                VEHICLE_SET(vehicle, CollisionPull, PULL_MIN);
            } else {
                VEHICLE_SET(
                    vehicle, CollisionPull, VEHICLE_GET(vehicle, CollisionPull) + (PULL_STEP));
                if (PULL_MAX < VEHICLE_GET(vehicle, CollisionPull)) {
                    VEHICLE_SET(vehicle, CollisionPull, PULL_MAX);
                }
            }
            /* Whichever way it went, the original then sets the default pull. */
            VEHICLE_SET(vehicle, CollisionPull, PULL_DEFAULT);
        } else {
            if (g_dwReplayCameraHoldTimer != 0) {
                g_dwReplayCameraHoldTimer--;
            }
#if TGR_BOSS_RALLY
            adjusted = (long double)VEHICLE_GET(vehicle, CollisionPull) - PULL_DECAY;
            VEHICLE_SET(vehicle, CollisionPull, (f32)adjusted);
            if (adjusted < 0.0f) {
#else
            vehicle->collisionPull -= PULL_DECAY;
            if (vehicle->collisionPull < 0.0f) {
#endif
                VEHICLE_SET(vehicle, CollisionPull, 0.0f);
            }
        }
    }

    ComputeCameraBasis(vehicle, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE]);

    /* The cockpit camera sits at the eye of the driver and looks along the vehicle. */
    previousCockpit[0] = VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].position.x;
    previousCockpit[1] = VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].position.y;
    previousCockpit[2] = VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].position.z;
    (void)previousCockpit;
    if (VEHICLE_GET(vehicle, FlyMode)) {
        memcpy(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT],
               &VEHICLE_GET(vehicle, Frame),
               sizeof(CameraFrame));
    } else {
        model = VEHICLE_GET(vehicle, Model);
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].position.x = COCKPIT_POSITION(x);
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].position.y = COCKPIT_POSITION(y);
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].position.z = COCKPIT_POSITION(z);
        Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].forward,
                           &VEHICLE_GET(vehicle, Frame).forward,
                           COCKPIT_AIM_DISTANCE);
        Vec3fNegate(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].forward,
                    &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].forward);
        Vec3fNormalize(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].forward);
        VEHICLE_GET(vehicle, Cameras)
        [VEHICLE_CAMERA_COCKPIT].side = VEHICLE_GET(vehicle, Frame).side;
        Vec3fCross(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].up,
                   &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].forward,
                   &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].side);
    }

    /* The bumper camera is the cockpit camera moved forward. */
    memcpy(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_BUMPER],
           &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT],
           sizeof(CameraFrame));
    Vec3fMulAddInPlace(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_BUMPER].position,
                       &VEHICLE_GET(vehicle, Frame).forward,
                       BUMPER_FORWARD);

    /* The trackside camera looks at the vehicle from where it stands, and zooms with distance. The
     * European and Japanese releases call the routine that does the same. */
#if TGR_PLATFORM == TGR_PLATFORM_N64 && (TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP)
    UpdateCameraViewBasisAndFov(vehicle, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE]);
#else
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].up.x = 0.0f;
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].up.y = 0.0f;
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].up.z = 1.0f;
    Vec3fAdd(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].forward,
             &VEHICLE_GET(vehicle, Frame).position,
             &VEHICLE_GET(vehicle, Frame).up);
    Vec3fSubtractInPlace(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].forward,
                         &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].position);
    distance = Vec3fLength(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].forward);
    Vec3fDivideInPlace(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].forward, distance);
    Vec3fCross(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].side,
               &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].up,
               &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].forward);
    Vec3fCross(&VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].up,
               &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].forward,
               &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].side);
#if TGR_BOSS_RALLY
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
#if TGR_BOSS_RALLY
    zoom =
        cosl((f32)(((long double)distance - 1.0f) * TRACKSIDE_COS_SCALE)) * TRACKSIDE_COS_WEIGHT -
        ((long double)TRACKSIDE_MID - distance) * -TRACKSIDE_DISTANCE_SCALE;
    zoom -= -TRACKSIDE_BIAS;
#else
    zoom = (cosf((distance - 1.0f) * TRACKSIDE_COS_SCALE) * TRACKSIDE_COS_WEIGHT + TRACKSIDE_BIAS) +
           (TRACKSIDE_MID - distance) * TRACKSIDE_DISTANCE_SCALE;
#endif
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].fov = zoom * g_fCameraFovRadians;
#endif /* TGR_PLATFORM_N64 && (TGR_REGION_EU || TGR_REGION_JP) */
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_BUMPER].fov = g_fCameraFovRadians;
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].fov = g_fCameraFovRadians;
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].fov = g_fCameraFovRadians;
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR].fov = g_fCameraFovRadians;

    /* The mirror looks back from above and ahead of the driver. */
    model = VEHICLE_GET(vehicle, Model);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR].position.x = MIRROR_POSITION(x);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR].position.y = MIRROR_POSITION(y);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR].position.z = MIRROR_POSITION(z);
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_MIRROR].forward.x = -VEHICLE_GET(vehicle, Frame).forward.x;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_MIRROR].forward.y = -VEHICLE_GET(vehicle, Frame).forward.y;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_MIRROR].forward.z = -VEHICLE_GET(vehicle, Frame).forward.z;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_MIRROR].side.x = -VEHICLE_GET(vehicle, Frame).side.x;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_MIRROR].side.z = -VEHICLE_GET(vehicle, Frame).side.z;
    VEHICLE_GET(vehicle, Cameras)
    [VEHICLE_CAMERA_MIRROR].side.y = -VEHICLE_GET(vehicle, Frame).side.y;
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR].up = VEHICLE_GET(vehicle, Frame).up;
    VEHICLE_SET(vehicle, PreviousCamera, VEHICLE_GET(vehicle, ActiveCamera));
}

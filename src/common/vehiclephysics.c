#include "common/vehiclephysics.h"

#include <PR/gu.h>

#include "common/camera.h"
#include "common/frametime.h"
#include "common/physics.h"
#include "common/timing.h"
#include "common/vehicleaccess.h"
#include "common/vehiclereset.h"

#if TGR_BOSS_RALLY
#include <math.h>
#include <string.h>

#include "common/screen.h"
#include "common/settings.h"
#else
#include <PR/os_libc.h>
#endif

#if TGR_BOSS_RALLY

#define FRAME_PRODUCT(a, b) ((long double)(a) * (b))
#define FRAME_LESS(a, b) (!((a) >= (b)))
#define AddTimingMarker(...) NullStub()
#define SIGN_OF(x) (!((x) < 0.0f) && !((x) > 0.0f) ? 0.0f : ((x) > 0.0f ? 1.0f : -1.0f))
#else
#define FRAME_LESS(a, b) ((a) < (b))
#define SIGN_OF(x) ((x) == 0.0f ? 0.0f : (0.0f < (x) ? 1.0f : -1.0f))
#endif /* TGR_BOSS_RALLY */

#define PHYSICS_STEP (1.0f / 30.0f)
#define WHEEL_DRAW_DROP 0.254f
#define TWO_RAD_TO_DEG 114.5915527f
#define VECTOR_AXIS(v, axis) ((&(v)->x)[axis])
#define KEEP_UNREVERSED(velocity, damped)                                                          \
    ((velocity) = SIGN_OF(damped) == SIGN_OF(velocity) ? (damped) : 0.0f)

/* The rates of a flying vehicle. */
#define FLY_SPEED 100.0f
#define FLY_TURN_RATE 3.141592741f
#define FLY_TURN_SCALE 0.4f

/* The reverse toggle and the command to level a flying vehicle. */
#define INPUT_REVERSE_TOGGLE (1 << 17)
#define INPUT_FLY_LEVEL 0x10

/* The share of the drive the rear wheels take with rear-wheel drive, and the front ones. */
#define REAR_DRIVE_SHARE 2.0f
#define FRONT_DRIVE_SHARE 0.0f

/* The drive of a front wheel below which the brakes act on the front wheels as well. */
#define FRONT_BRAKE_DRIVE_LIMIT 1.0e-4f

/* The scale from the speed of a vehicle to the speed the display shows. */
#define SPEED_TO_DISPLAY 2.24f

#if TGR_BOSS_RALLY
s32 Vehicle::CastGroundRay() {
    return CastRayAgainstTrack(&frame.position,
                               &frame.up,
                               &frame.position,
                               groundInstances,
                               &groundInstanceCount,
                               nearbyInstances,
                               &nearbyInstanceCount,
                               &groundDistance,
                               &groundSurface);
}

void Vehicle::DriveRearWheel(RigidBody *body, RigidBody *wheel, f32 dt) {
    DriveWheel(body, wheel, &rearLoad, &rearSliding, dt);
}

void Vehicle::DriveFrontWheel(RigidBody *body, RigidBody *wheel, f32 dt) {
    DriveWheel(body, wheel, &frontLoad, &frontSliding, dt);
}

void Vehicle::ApplyWheelSideGrip(RigidBody *body, f32 dt) {
    ApplySideGrip(body, dt, &rearLoad, &frontLoad, &rearSliding, &frontSliding);
}

#define CAST_GROUND_RAY(vehicle) ((vehicle)->CastGroundRay())
#define DRIVE_REAR_WHEEL(vehicle, body, wheel, dt) ((vehicle)->DriveRearWheel(body, wheel, dt))
#define DRIVE_FRONT_WHEEL(vehicle, body, wheel, dt) ((vehicle)->DriveFrontWheel(body, wheel, dt))
#define APPLY_WHEEL_SIDE_GRIP(vehicle, body, dt) ((vehicle)->ApplyWheelSideGrip(body, dt))
#else
#define CAST_GROUND_RAY(vehicle)                                                                   \
    CastRayAgainstTrack(&(vehicle)->frame.position,                                                \
                        &(vehicle)->frame.up,                                                      \
                        &(vehicle)->frame.position,                                                \
                        (vehicle)->groundInstances,                                                \
                        &(vehicle)->groundInstanceCount,                                           \
                        (vehicle)->nearbyInstances,                                                \
                        &(vehicle)->nearbyInstanceCount,                                           \
                        &(vehicle)->groundDistance,                                                \
                        &(vehicle)->groundSurface)
#define DRIVE_REAR_WHEEL(vehicle, body, wheel, dt)                                                 \
    DriveWheel(body, wheel, &(vehicle)->rearLoad, &(vehicle)->rearSliding, dt)
#define DRIVE_FRONT_WHEEL(vehicle, body, wheel, dt)                                                \
    DriveWheel(body, wheel, &(vehicle)->frontLoad, &(vehicle)->frontSliding, dt)
#define APPLY_WHEEL_SIDE_GRIP(vehicle, body, dt)                                                   \
    ApplySideGrip(body,                                                                            \
                  dt,                                                                              \
                  &(vehicle)->rearLoad,                                                            \
                  &(vehicle)->frontLoad,                                                           \
                  &(vehicle)->rearSliding,                                                         \
                  &(vehicle)->frontSliding)
#endif /* TGR_BOSS_RALLY */

void UpdateVehiclePhysicsFrame(Vehicle *vehicle) {
    RigidBody **wheels = VEHICLE_GET(vehicle, Body).wheels;
    Vec3f step;
    Vec3f *velocity;
    f32 drive;
    s32 i;

    if (VEHICLE_GET(vehicle, FlyMode)) {
        /* Fly along the vehicle and turn with the controls. */
#if TGR_BOSS_RALLY
        Vec3fScale(&step,
                   &VEHICLE_GET(vehicle, Frame).forward,
                   FRAME_PRODUCT(VEHICLE_GET(vehicle, Accelerator), g_fFrameDeltaTime) * FLY_SPEED);
#else
        Vec3fScale(&step,
                   &VEHICLE_GET(vehicle, Frame).forward,
                   vehicle->accelerator * g_fFrameDeltaTime * FLY_SPEED);
#endif
        Vec3fAddInPlace(&VEHICLE_GET(vehicle, Frame).position, &step);
        SetVehiclePosition(vehicle,
                           VEHICLE_GET(vehicle, Frame).position.x,
                           VEHICLE_GET(vehicle, Frame).position.y,
                           VEHICLE_GET(vehicle, Frame).position.z);
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
        SetVehicleTurnRate(vehicle, 0.0f, 0.0f, 0.0f);
        VEHICLE_SET(vehicle, EngineRpm, 0.0f);
#if TGR_BOSS_RALLY
        RotateVehicleByEuler(vehicle,
                             FRAME_PRODUCT(VEHICLE_GET(vehicle, Steering), g_fFrameDeltaTime) *
                                 1.256637096405029296875f,
                             FRAME_PRODUCT(VEHICLE_GET(vehicle, PitchInput), g_fFrameDeltaTime) *
                                 1.256637096405029296875f,
                             FRAME_PRODUCT(VEHICLE_GET(vehicle, RollInput), g_fFrameDeltaTime) *
                                 1.256637096405029296875f);
#else
        RotateVehicleByEuler(
            vehicle,
            vehicle->steering * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->pitchInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->rollInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime);
#endif /* TGR_BOSS_RALLY */
        if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_FLY_LEVEL) {
            VEHICLE_GET(vehicle, Frame).up.x = 0.0f;
            VEHICLE_GET(vehicle, Frame).up.y = 0.0f;
            VEHICLE_GET(vehicle, Frame).up.z = 1.0f;
            Vec3fCross(&VEHICLE_GET(vehicle, Frame).side,
                       &VEHICLE_GET(vehicle, Frame).up,
                       &VEHICLE_GET(vehicle, Frame).forward);
            Vec3fCross(&VEHICLE_GET(vehicle, Frame).up,
                       &VEHICLE_GET(vehicle, Frame).forward,
                       &VEHICLE_GET(vehicle, Frame).side);
            TransferFlags(VEHICLE_GET(vehicle, Input), INPUT_FLY_LEVEL);
        }
    } else {
        /* Put the vehicle on the ground, then drive and brake its wheels and step its physics. */
        VEHICLE_SET(vehicle, GroundHits, CAST_GROUND_RAY(vehicle));
        wheels[1]->steerAngle = 0.0f;
        wheels[0]->steerAngle = wheels[1]->steerAngle;
        UpdateVehicleEngineAndGearbox(vehicle);
        wheels[3]->steerAngle = VEHICLE_GET(vehicle, SteerAngle);
        wheels[2]->steerAngle = VEHICLE_GET(vehicle, SteerAngle);
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->drive = 0.0f;
        }
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->brake = 0.0f;
        }
        if (0.0f < VEHICLE_GET(vehicle, Throttle)) {
#if TGR_BOSS_RALLY
            if (VEHICLE_GET(vehicle, Input)->buttons & INPUT_REVERSE_TOGGLE) {
#else
            if ((vehicle->input->buttons & INPUT_REVERSE_TOGGLE) && vehicle->gear == 1) {
#endif
                VEHICLE_SET(vehicle, Throttle, -VEHICLE_GET(vehicle, Throttle));
            }
#if TGR_BOSS_RALLY
            switch (VEHICLE_GET(vehicle, Model)->driveMode) {
            case 0:
                wheels[0]->drive = VEHICLE_GET(vehicle, Throttle) * 0.1f;
                wheels[1]->drive = VEHICLE_GET(vehicle, Throttle) * 0.1f;
                wheels[2]->drive = VEHICLE_GET(vehicle, Throttle) * -2.1f;
                wheels[3]->drive = VEHICLE_GET(vehicle, Throttle) * -2.1f;
                break;
            case 1:
                wheels[0]->drive = VEHICLE_GET(vehicle, Throttle) * -2.0f;
                wheels[1]->drive = VEHICLE_GET(vehicle, Throttle) * -2.0f;
                wheels[2]->drive = 0.0f;
                wheels[3]->drive = 0.0f;
                break;
            default:
                for (i = 0; i < VEHICLE_WHEELS; i++) {
                    wheels[i]->drive = -VEHICLE_GET(vehicle, Throttle);
                }
                break;
            }
#else
            if (vehicle->rearWheelDrive) {
                wheels[0]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[1]->drive = -vehicle->throttle * REAR_DRIVE_SHARE;
                wheels[2]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
                wheels[3]->drive = -vehicle->throttle * FRONT_DRIVE_SHARE;
            } else {
                for (i = 0; i < VEHICLE_WHEELS; i++) {
                    wheels[i]->drive = -vehicle->throttle;
                }
            }
#endif /* TGR_BOSS_RALLY */
        }
        if (FRAME_LESS(VEHICLE_GET(vehicle, Brake), 0.0f)) {
            wheels[0]->brake = -VEHICLE_GET(vehicle, Brake);
            wheels[1]->brake = -VEHICLE_GET(vehicle, Brake);
            wheels[0]->drive = 0.0f;
            wheels[1]->drive = 0.0f;
            drive = FRAME_LESS(wheels[2]->drive, 0.0f) ? -wheels[2]->drive : wheels[2]->drive;
            if (FRAME_LESS(drive, FRONT_BRAKE_DRIVE_LIMIT)) {
                wheels[2]->brake = -VEHICLE_GET(vehicle, Brake);
                wheels[3]->brake = -VEHICLE_GET(vehicle, Brake);
            }
        }
#if TGR_BOSS_RALLY
        if (ShouldStepVehiclePhysics(vehicle)) {
            SimulateVehiclePhysicsSubstep(vehicle);
        }
#else
        SimulateVehiclePhysicsSubstep(vehicle);
#endif
    }

#if TGR_BOSS_RALLY
    FinishVehiclePhysicsFrame(vehicle);
#else
    TransformAllWheelMatrices(vehicle);
#endif
    /* The speed changes while the first front wheel is on the ground. */
    if (VEHICLE_GET(vehicle, WheelBodies)[1].contactFrames) {
        velocity = &VEHICLE_GET(vehicle, Body).state.velocity;
#if TGR_BOSS_RALLY
        VEHICLE_SET(vehicle,
                    SpeedKph,
                    sqrtl((f32)(FRAME_PRODUCT(velocity->z, velocity->z) +
                                (FRAME_PRODUCT(velocity->x, velocity->x) +
                                 FRAME_PRODUCT(velocity->y, velocity->y)))) *
                        SPEED_TO_DISPLAY);
#else
        vehicle->speedKph = sqrtf(velocity->z * velocity->z +
                                  (velocity->x * velocity->x + velocity->y * velocity->y)) *
                            SPEED_TO_DISPLAY;
#endif /* TGR_BOSS_RALLY */
    }
#if TGR_BOSS_RALLY
    if (g_dwNetworkPlayMode) {
        return;
    }
#endif
    for (i = 0; i < g_dwNumViewports; i++) {
        if (VEHICLE_GET(vehicle, Index) == g_aViewportRects[i].vehicleIndex) {
            UpdateCameraSystem(vehicle);
            return;
        }
    }
}

void UpdateVehicleWheelTransforms(Vehicle *vehicle) {
    s32 i;

    UpdateWheelHeights(&VEHICLE_GET(vehicle, Body));
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        QuaternionToMatrix(VEHICLE_GET(vehicle, Body).wheels[i]->orientation,
                           &VEHICLE_GET(vehicle, Body).wheels[i]->state);
    }
}

/* Place the matrix of a wheel at the wheel, in the frame of the vehicle, transforming the
 * translation rows in place as points. */
#define PLACE_WHEEL(out, body, wheel)                                                              \
    Mtx4x4TransformPoint((Vec3f *)(out)[3], (body)->orientation, (Vec3f *)(wheel)->orientation[3])

/* Apply wheel roll before steering. */
#define STEER_WHEEL(out, wheel, roll, steer)                                                       \
    guRotateF(roll, (wheel)->rollAngle, 0.0f, 1.0f, 0.0f);                                         \
    guRotateF(steer, (wheel)->steerAngle *TWO_RAD_TO_DEG, 0.0f, 0.0f, 1.0f);                       \
    guMtxCatF(roll, steer, out)

/* Move the wheel bodies of a vehicle down, or back up, by the drop of the drawing. */
#define DROP_WHEELS(vehicle, op)                                                                   \
    VEHICLE_GET((vehicle), WheelBodies)                                                            \
    [0].orientation[3][2] =                                                                        \
        VEHICLE_GET((vehicle), WheelBodies)[0].orientation[3][2] op WHEEL_DRAW_DROP;               \
    VEHICLE_GET((vehicle), WheelBodies)                                                            \
    [1].orientation[3][2] =                                                                        \
        VEHICLE_GET((vehicle), WheelBodies)[1].orientation[3][2] op WHEEL_DRAW_DROP;               \
    VEHICLE_GET((vehicle), WheelBodies)                                                            \
    [2].orientation[3][2] =                                                                        \
        VEHICLE_GET((vehicle), WheelBodies)[2].orientation[3][2] op WHEEL_DRAW_DROP;               \
    VEHICLE_GET((vehicle), WheelBodies)                                                            \
    [3].orientation[3][2] =                                                                        \
        VEHICLE_GET((vehicle), WheelBodies)[3].orientation[3][2] op WHEEL_DRAW_DROP

void TransformAllWheelMatrices(Vehicle *vehicle) {
    f32 roll[4][4];
    f32 turn[4][4];
    f32 steer[4][4];

#if TGR_BOSS_RALLY
    UpdateVehicleReplayPose(vehicle);
#endif
    guRotateF(turn, VEHICLE_GET(vehicle, Body).rollAngle, 1.0f, 0.0f, 0.0f);
    guMtxCatF(turn, VEHICLE_GET(vehicle, Body).orientation, VEHICLE_GET(vehicle, Matrix));
    DROP_WHEELS(vehicle, +);
    /* The rear wheels by their roll, then the front ones by their roll and steering. */
    guRotateF(turn, VEHICLE_GET(vehicle, WheelBodies)[0].rollAngle, 0.0f, 1.0f, 0.0f);
    guMtxCatF(turn, VEHICLE_GET(vehicle, Body).orientation, VEHICLE_GET(vehicle, WheelMatrices)[2]);
    PLACE_WHEEL(VEHICLE_GET(vehicle, WheelMatrices)[2],
                &VEHICLE_GET(vehicle, Body),
                &VEHICLE_GET(vehicle, WheelBodies)[0]);
    guRotateF(turn, VEHICLE_GET(vehicle, WheelBodies)[2].rollAngle, 0.0f, 1.0f, 0.0f);
    guMtxCatF(turn, VEHICLE_GET(vehicle, Body).orientation, VEHICLE_GET(vehicle, WheelMatrices)[3]);
    PLACE_WHEEL(VEHICLE_GET(vehicle, WheelMatrices)[3],
                &VEHICLE_GET(vehicle, Body),
                &VEHICLE_GET(vehicle, WheelBodies)[2]);
    STEER_WHEEL(turn, &VEHICLE_GET(vehicle, WheelBodies)[1], roll, steer);
    guMtxCatF(turn, VEHICLE_GET(vehicle, Body).orientation, VEHICLE_GET(vehicle, WheelMatrices)[1]);
    PLACE_WHEEL(VEHICLE_GET(vehicle, WheelMatrices)[1],
                &VEHICLE_GET(vehicle, Body),
                &VEHICLE_GET(vehicle, WheelBodies)[1]);
    STEER_WHEEL(turn, &VEHICLE_GET(vehicle, WheelBodies)[3], roll, steer);
    guMtxCatF(turn, VEHICLE_GET(vehicle, Body).orientation, VEHICLE_GET(vehicle, WheelMatrices)[0]);
    PLACE_WHEEL(VEHICLE_GET(vehicle, WheelMatrices)[0],
                &VEHICLE_GET(vehicle, Body),
                &VEHICLE_GET(vehicle, WheelBodies)[3]);
    DROP_WHEELS(vehicle, -);
}

void SimulateVehiclePhysicsSubstep(Vehicle *vehicle) {
    RigidBody *body = &VEHICLE_GET(vehicle, Body);
    RigidBodyState *state = &body->state;
    s32 i;
    s16 axis;

    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    body->forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_0];
    body->wheels[0]->forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_0];
    body->wheels[1]->forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_2];
    body->wheels[2]->forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_1];
    body->wheels[3]->forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_3];
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        body->wheels[i]->forces->vector.x = 0.0f;
        body->wheels[i]->forces->vector.y = 0.0f;
        body->wheels[i]->forces->vector.z = 0.0f;
    }
    ComputeSpringForces(body);
    if (VEHICLE_GET(vehicle, SkipDrive)) {
        VEHICLE_SET(vehicle, SkipDrive, 0);
    } else {
        VEHICLE_SET(vehicle, FrontLoad, 0.0f);
        VEHICLE_SET(vehicle, RearLoad, 0.0f);
        VEHICLE_SET(vehicle, RearSliding, 0);
        DRIVE_REAR_WHEEL(vehicle, body, body->wheels[0], PHYSICS_STEP);
        DRIVE_REAR_WHEEL(vehicle, body, body->wheels[1], PHYSICS_STEP);
        DRIVE_FRONT_WHEEL(vehicle, body, body->wheels[2], PHYSICS_STEP);
        DRIVE_FRONT_WHEEL(vehicle, body, body->wheels[3], PHYSICS_STEP);
        VEHICLE_SET(vehicle, SkipDrive, 0);
    }
    body->force.x = 0.0f;
    body->force.y = 0.0f;
    body->force.z = 0.0f;
    body->torque.x = 0.0f;
    body->torque.y = 0.0f;
    body->torque.z = 0.0f;
    ComputeVehicleForces(body);
    ApplyRigidBodyImpulse(state, body, PHYSICS_STEP);
    APPLY_WHEEL_SIDE_GRIP(vehicle, body, PHYSICS_STEP);
    ComputeDeltaQuaternion(state);

    body->forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_0];
    body->wheels[0]->forces = NULL;
    body->wheels[2]->forces = NULL;
    body->wheels[1]->forces = NULL;
    body->wheels[3]->forces = NULL;
    ComputeDragForce(body, &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DRAG]);
    ComputeDampingForces(body);
    body->force.x = 0.0f;
    body->force.y = 0.0f;
    body->force.z = 0.0f;
    body->torque.x = 0.0f;
    body->torque.y = 0.0f;
    body->torque.z = 0.0f;
    ComputeVehicleForces(body);
    memcpy(&body->stepStates[1], state, sizeof(RigidBodyState));
    ApplyRigidBodyImpulse(&body->stepStates[1], body, PHYSICS_STEP);
    for (axis = 0; axis < 3; axis++) {
        KEEP_UNREVERSED(VECTOR_AXIS(&state->angularVelocity, axis),
                        VECTOR_AXIS(&body->stepStates[1].angularVelocity, axis));
        KEEP_UNREVERSED(VECTOR_AXIS(&state->velocity, axis),
                        VECTOR_AXIS(&body->stepStates[1].velocity, axis));
    }

    /* Move it through the frame against the track and the others, and hang the wheels. */
    memcpy(&body->stepStates[0], state, sizeof(RigidBodyState));
    /* The original updates the same step quaternion twice. */
    ComputeDeltaQuaternion(&body->stepStates[0]);
    ComputeDeltaQuaternion(&body->stepStates[0]);
#if TGR_BOSS_RALLY
    StepVehicleCollisions(vehicle, body);
#else
    StepVehicleCollisions(body);
#endif
    memcpy(state, &body->stepStates[1], sizeof(RigidBodyState));
    UpdateVehicleWheelTransforms(vehicle);
    AddTimingMarker(0, 0, 0x80, 0, 0xFF);
}

#include "vehiclephysics.h"

#include <PR/gu.h>

#include "camera.h"
#include "frametime.h"
#include "physics.h"
#include "timing.h"
#include "vehiclereset.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>
#include <string.h>

#include "platform/win/settings.h"
#include "screen.h"
#define FRAME_PRODUCT(a, b) ((long double)(a) * (b))
#define FRAME_LESS(a, b) (!((a) >= (b)))
#define AddTimingMarker(...) NullStub()
#define SIGN_OF(x) (!((x) < 0.0f) && !((x) > 0.0f) ? 0.0f : ((x) > 0.0f ? 1.0f : -1.0f))
#else
#include <PR/os_libc.h>
#define FRAME_LESS(a, b) ((a) < (b))
#define SIGN_OF(x) ((x) == 0.0f ? 0.0f : (0.0f < (x) ? 1.0f : -1.0f))
#endif

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

void UpdateVehiclePhysicsFrame(Vehicle *vehicle) {
    RigidBody **wheels = vehicle->body.wheels;
    Vec3f step;
    Vec3f *velocity;
    f32 drive;
    s32 i;

    if (vehicle->flyMode) {
        /* Fly along the vehicle and turn with the controls. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        Vec3fScale(&step,
                   &vehicle->frame.forward,
                   FRAME_PRODUCT(vehicle->accelerator, g_fFrameDeltaTime) * FLY_SPEED);
#else
        Vec3fScale(
            &step, &vehicle->frame.forward, vehicle->accelerator * g_fFrameDeltaTime * FLY_SPEED);
#endif
        Vec3fAddInPlace(&vehicle->frame.position, &step);
        SetVehiclePosition(vehicle,
                           vehicle->frame.position.x,
                           vehicle->frame.position.y,
                           vehicle->frame.position.z);
        SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
        SetVehicleTurnRate(vehicle, 0.0f, 0.0f, 0.0f);
        vehicle->engineRpm = 0.0f;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        RotateVehicleByEuler(
            vehicle,
            FRAME_PRODUCT(vehicle->steering, g_fFrameDeltaTime) * 1.256637096405029296875f,
            FRAME_PRODUCT(vehicle->pitchInput, g_fFrameDeltaTime) * 1.256637096405029296875f,
            FRAME_PRODUCT(vehicle->rollInput, g_fFrameDeltaTime) * 1.256637096405029296875f);
#else
        RotateVehicleByEuler(
            vehicle,
            vehicle->steering * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->pitchInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime,
            vehicle->rollInput * FLY_TURN_RATE * FLY_TURN_SCALE * g_fFrameDeltaTime);
#endif
        if (vehicle->input->buttons & INPUT_FLY_LEVEL) {
            vehicle->frame.up.x = 0.0f;
            vehicle->frame.up.y = 0.0f;
            vehicle->frame.up.z = 1.0f;
            Vec3fCross(&vehicle->frame.side, &vehicle->frame.up, &vehicle->frame.forward);
            Vec3fCross(&vehicle->frame.up, &vehicle->frame.forward, &vehicle->frame.side);
            TransferFlags(vehicle->input, INPUT_FLY_LEVEL);
        }
    } else {
        /* Put the vehicle on the ground, then drive and brake its wheels and step its physics. */
        vehicle->groundHits = CastRayAgainstTrack(&vehicle->frame.position,
                                                  &vehicle->frame.up,
                                                  &vehicle->frame.position,
                                                  vehicle->groundInstances,
                                                  &vehicle->groundInstanceCount,
                                                  vehicle->nearbyInstances,
                                                  &vehicle->nearbyInstanceCount,
                                                  &vehicle->groundDistance,
                                                  &vehicle->groundSurface);
        wheels[1]->steerAngle = 0.0f;
        wheels[0]->steerAngle = wheels[1]->steerAngle;
        UpdateVehicleEngineAndGearbox(vehicle);
        wheels[3]->steerAngle = vehicle->steerAngle;
        wheels[2]->steerAngle = vehicle->steerAngle;
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->drive = 0.0f;
        }
        for (i = 0; i < VEHICLE_WHEELS; i++) {
            wheels[i]->brake = 0.0f;
        }
        if (0.0f < vehicle->throttle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            if (vehicle->input->buttons & INPUT_REVERSE_TOGGLE) {
#else
            if ((vehicle->input->buttons & INPUT_REVERSE_TOGGLE) && vehicle->gear == 1) {
#endif
                vehicle->throttle = -vehicle->throttle;
            }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            switch (vehicle->model->driveMode) {
            case 0:
                wheels[0]->drive = vehicle->throttle * 0.1f;
                wheels[1]->drive = vehicle->throttle * 0.1f;
                wheels[2]->drive = vehicle->throttle * -2.1f;
                wheels[3]->drive = vehicle->throttle * -2.1f;
                break;
            case 1:
                wheels[0]->drive = vehicle->throttle * -2.0f;
                wheels[1]->drive = vehicle->throttle * -2.0f;
                wheels[2]->drive = 0.0f;
                wheels[3]->drive = 0.0f;
                break;
            default:
                for (i = 0; i < VEHICLE_WHEELS; i++) {
                    wheels[i]->drive = -vehicle->throttle;
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
#endif
        }
        if (FRAME_LESS(vehicle->brake, 0.0f)) {
            wheels[0]->brake = -vehicle->brake;
            wheels[1]->brake = -vehicle->brake;
            wheels[0]->drive = 0.0f;
            wheels[1]->drive = 0.0f;
            drive = FRAME_LESS(wheels[2]->drive, 0.0f) ? -wheels[2]->drive : wheels[2]->drive;
            if (FRAME_LESS(drive, FRONT_BRAKE_DRIVE_LIMIT)) {
                wheels[2]->brake = -vehicle->brake;
                wheels[3]->brake = -vehicle->brake;
            }
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (ShouldStepVehiclePhysics(vehicle)) {
            SimulateVehiclePhysicsSubstep(vehicle);
        }
#else
        SimulateVehiclePhysicsSubstep(vehicle);
#endif
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    FinishVehiclePhysicsFrame(vehicle);
#else
    TransformAllWheelMatrices(vehicle);
#endif
    /* The speed changes while the first front wheel is on the ground. */
    if (vehicle->wheelBodies[1].contactFrames) {
        velocity = &vehicle->body.state.velocity;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle->speedKph = sqrtl((f32)(FRAME_PRODUCT(velocity->z, velocity->z) +
                                        (FRAME_PRODUCT(velocity->x, velocity->x) +
                                         FRAME_PRODUCT(velocity->y, velocity->y)))) *
                            SPEED_TO_DISPLAY;
#else
        vehicle->speedKph = sqrtf(velocity->z * velocity->z +
                                  (velocity->x * velocity->x + velocity->y * velocity->y)) *
                            SPEED_TO_DISPLAY;
#endif
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwNetworkPlayMode) {
        return;
    }
#endif
    for (i = 0; i < g_dwNumViewports; i++) {
        if (vehicle->index == g_aViewportRects[i].vehicleIndex) {
            UpdateCameraSystem(vehicle);
            return;
        }
    }
}

void UpdateVehicleWheelTransforms(Vehicle *vehicle) {
    s32 i;

    UpdateWheelHeights(&vehicle->body);
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        QuaternionToMatrix(vehicle->body.wheels[i]->orientation, &vehicle->body.wheels[i]->state);
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
    (vehicle)->wheelBodies[0].orientation[3][2] =                                                  \
        (vehicle)->wheelBodies[0].orientation[3][2] op WHEEL_DRAW_DROP;                            \
    (vehicle)->wheelBodies[1].orientation[3][2] =                                                  \
        (vehicle)->wheelBodies[1].orientation[3][2] op WHEEL_DRAW_DROP;                            \
    (vehicle)->wheelBodies[2].orientation[3][2] =                                                  \
        (vehicle)->wheelBodies[2].orientation[3][2] op WHEEL_DRAW_DROP;                            \
    (vehicle)->wheelBodies[3].orientation[3][2] =                                                  \
        (vehicle)->wheelBodies[3].orientation[3][2] op WHEEL_DRAW_DROP

void TransformAllWheelMatrices(Vehicle *vehicle) {
    f32 roll[4][4];
    f32 turn[4][4];
    f32 steer[4][4];

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    UpdateVehicleReplayPose(vehicle);
#endif
    guRotateF(turn, vehicle->body.rollAngle, 1.0f, 0.0f, 0.0f);
    guMtxCatF(turn, vehicle->body.orientation, vehicle->matrix);
    DROP_WHEELS(vehicle, +);
    /* The rear wheels by their roll, then the front ones by their roll and steering. */
    guRotateF(turn, vehicle->wheelBodies[0].rollAngle, 0.0f, 1.0f, 0.0f);
    guMtxCatF(turn, vehicle->body.orientation, vehicle->wheelMatrices[2]);
    PLACE_WHEEL(vehicle->wheelMatrices[2], &vehicle->body, &vehicle->wheelBodies[0]);
    guRotateF(turn, vehicle->wheelBodies[2].rollAngle, 0.0f, 1.0f, 0.0f);
    guMtxCatF(turn, vehicle->body.orientation, vehicle->wheelMatrices[3]);
    PLACE_WHEEL(vehicle->wheelMatrices[3], &vehicle->body, &vehicle->wheelBodies[2]);
    STEER_WHEEL(turn, &vehicle->wheelBodies[1], roll, steer);
    guMtxCatF(turn, vehicle->body.orientation, vehicle->wheelMatrices[1]);
    PLACE_WHEEL(vehicle->wheelMatrices[1], &vehicle->body, &vehicle->wheelBodies[1]);
    STEER_WHEEL(turn, &vehicle->wheelBodies[3], roll, steer);
    guMtxCatF(turn, vehicle->body.orientation, vehicle->wheelMatrices[0]);
    PLACE_WHEEL(vehicle->wheelMatrices[0], &vehicle->body, &vehicle->wheelBodies[3]);
    DROP_WHEELS(vehicle, -);
}

void SimulateVehiclePhysicsSubstep(Vehicle *vehicle) {
    RigidBody *body = &vehicle->body;
    RigidBodyState *state = &body->state;
    s32 i;
    s16 axis;

    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    body->forces = &vehicle->forces[VEHICLE_FORCE_SPRING_0];
    body->wheels[0]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_0];
    body->wheels[1]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_2];
    body->wheels[2]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_1];
    body->wheels[3]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_3];
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        body->wheels[i]->forces->vector.x = 0.0f;
        body->wheels[i]->forces->vector.y = 0.0f;
        body->wheels[i]->forces->vector.z = 0.0f;
    }
    ComputeSpringForces(body);
    if (vehicle->skipDrive) {
        vehicle->skipDrive = 0;
    } else {
        vehicle->frontLoad = 0.0f;
        vehicle->rearLoad = 0.0f;
        vehicle->rearSliding = 0;
        DriveWheel(body, body->wheels[0], &vehicle->rearLoad, &vehicle->rearSliding, PHYSICS_STEP);
        DriveWheel(body, body->wheels[1], &vehicle->rearLoad, &vehicle->rearSliding, PHYSICS_STEP);
        DriveWheel(
            body, body->wheels[2], &vehicle->frontLoad, &vehicle->frontSliding, PHYSICS_STEP);
        DriveWheel(
            body, body->wheels[3], &vehicle->frontLoad, &vehicle->frontSliding, PHYSICS_STEP);
        vehicle->skipDrive = 0;
    }
    body->force.x = 0.0f;
    body->force.y = 0.0f;
    body->force.z = 0.0f;
    body->torque.x = 0.0f;
    body->torque.y = 0.0f;
    body->torque.z = 0.0f;
    ComputeVehicleForces(body);
    ApplyRigidBodyImpulse(state, body, PHYSICS_STEP);
    ApplySideGrip(body,
                  PHYSICS_STEP,
                  &vehicle->rearLoad,
                  &vehicle->frontLoad,
                  &vehicle->rearSliding,
                  &vehicle->frontSliding);
    ComputeDeltaQuaternion(state);

    body->forces = &vehicle->forces[VEHICLE_FORCE_DAMPER_0];
    body->wheels[0]->forces = NULL;
    body->wheels[2]->forces = NULL;
    body->wheels[1]->forces = NULL;
    body->wheels[3]->forces = NULL;
    ComputeDragForce(body, &vehicle->forces[VEHICLE_FORCE_DRAG]);
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    StepVehicleCollisions(vehicle, body);
#else
    StepVehicleCollisions(body);
#endif
    memcpy(state, &body->stepStates[1], sizeof(RigidBodyState));
    UpdateVehicleWheelTransforms(vehicle);
    AddTimingMarker(0, 0, 0x80, 0, 0xFF);
}

#include "vehiclephysics.h"

#include <PR/gu.h>

#include "physics.h"
#include "timing.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <string.h>

#include "screen.h"
#define AddTimingMarker(...) NullStub()
#define SIGN_OF(x) (!((x) < 0.0f) && !((x) > 0.0f) ? 0.0f : ((x) > 0.0f ? 1.0f : -1.0f))
#else
#include <PR/os_libc.h>
#define SIGN_OF(x) ((x) == 0.0f ? 0.0f : (0.0f < (x) ? 1.0f : -1.0f))
#endif

#define PHYSICS_STEP (1.0f / 30.0f)
#define WHEEL_DRAW_DROP 0.254f
#define TWO_RAD_TO_DEG 114.5915527f
#define VECTOR_AXIS(v, axis) ((&(v)->x)[axis])
#define KEEP_UNREVERSED(velocity, damped)                                                          \
    ((velocity) = SIGN_OF(damped) == SIGN_OF(velocity) ? (damped) : 0.0f)

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

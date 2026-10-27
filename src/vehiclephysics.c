#include "vehiclephysics.h"

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
#define VECTOR_AXIS(v, axis) ((&(v)->x)[axis])
#define KEEP_UNREVERSED(velocity, damped)                                                          \
    ((velocity) = SIGN_OF(damped) == SIGN_OF(velocity) ? (damped) : 0.0f)

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
    UpdateWheelHeights(body);
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        QuaternionToMatrix(body->wheels[i]->orientation, &body->wheels[i]->state);
    }
    AddTimingMarker(0, 0, 0x80, 0, 0xFF);
}

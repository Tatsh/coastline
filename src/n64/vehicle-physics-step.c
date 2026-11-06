#include <PR/os_libc.h>

#include "common/physics.h"
#include "common/timing.h"
#include "common/vehiclephysics.h"

#if TGR_REGION == TGR_REGION_EU
#define PHYSICS_STEP 0.04f
#else
#define PHYSICS_STEP 0.033333335f
#endif
#define VELOCITY_SIGN(value) ((value) == 0.0f ? 0.0f : ((value) > 0.0f ? 1.0f : -1.0f))

void SimulateVehiclePhysicsSubstep(Vehicle *vehicle) {
    RigidBodyState tmp;
    s16 axis;
    s32 unused[2];

    /* These unused locals preserve IDO's stack frame. */
    (void)tmp;
    (void)unused;
    AddTimingMarker(0, 0x80, 0x80, 0, 0xff);
    vehicle->body.forces = &vehicle->forces[VEHICLE_FORCE_SPRING_0];
    vehicle->body.wheels[0]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_0];
    vehicle->body.wheels[1]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_2];
    vehicle->body.wheels[2]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_1];
    vehicle->body.wheels[3]->forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_3];
    vehicle->body.wheels[0]->forces->vector.x = 0.0f;
    vehicle->body.wheels[0]->forces->vector.y = 0.0f;
    vehicle->body.wheels[0]->forces->vector.z = 0.0f;
    vehicle->body.wheels[1]->forces->vector.x = 0.0f;
    vehicle->body.wheels[1]->forces->vector.y = 0.0f;
    vehicle->body.wheels[1]->forces->vector.z = 0.0f;
    vehicle->body.wheels[2]->forces->vector.x = 0.0f;
    vehicle->body.wheels[2]->forces->vector.y = 0.0f;
    vehicle->body.wheels[2]->forces->vector.z = 0.0f;
    vehicle->body.wheels[3]->forces->vector.x = 0.0f;
    vehicle->body.wheels[3]->forces->vector.y = 0.0f;
    vehicle->body.wheels[3]->forces->vector.z = 0.0f;
    ComputeSpringForces(&vehicle->body);
    if (vehicle->skipDrive == 0) {
        vehicle->frontLoad = 0.0f;
        vehicle->rearLoad = 0.0f;
        vehicle->rearSliding = 0;
        DriveWheel(&vehicle->body,
                   vehicle->body.wheels[0],
                   &vehicle->rearLoad,
                   &vehicle->rearSliding,
                   PHYSICS_STEP);
        DriveWheel(&vehicle->body,
                   vehicle->body.wheels[1],
                   &vehicle->rearLoad,
                   &vehicle->rearSliding,
                   PHYSICS_STEP);
        DriveWheel(&vehicle->body,
                   vehicle->body.wheels[2],
                   &vehicle->frontLoad,
                   &vehicle->frontSliding,
                   PHYSICS_STEP);
        DriveWheel(&vehicle->body,
                   vehicle->body.wheels[3],
                   &vehicle->frontLoad,
                   &vehicle->frontSliding,
                   PHYSICS_STEP);
    }
    vehicle->skipDrive = 0;
    vehicle->body.force.x = 0.0f;
    vehicle->body.force.y = 0.0f;
    vehicle->body.force.z = 0.0f;
    vehicle->body.torque.x = 0.0f;
    vehicle->body.torque.y = 0.0f;
    vehicle->body.torque.z = 0.0f;
    ComputeVehicleForces(&vehicle->body);
    ApplyRigidBodyImpulse(&vehicle->body.state, &vehicle->body, PHYSICS_STEP);
    ApplySideGrip(&vehicle->body,
                  PHYSICS_STEP,
                  &vehicle->rearLoad,
                  &vehicle->frontLoad,
                  &vehicle->rearSliding,
                  &vehicle->frontSliding);
    ComputeDeltaQuaternion(&vehicle->body.state);
    vehicle->body.forces = &vehicle->forces[VEHICLE_FORCE_DAMPER_0];
    vehicle->body.wheels[0]->forces = NULL;
    vehicle->body.wheels[2]->forces = NULL;
    vehicle->body.wheels[1]->forces = NULL;
    vehicle->body.wheels[3]->forces = NULL;
    ComputeDragForce(&vehicle->body, &vehicle->forces[VEHICLE_FORCE_DRAG]);
    ComputeDampingForces(&vehicle->body);
    vehicle->body.force.x = 0.0f;
    vehicle->body.force.y = 0.0f;
    vehicle->body.force.z = 0.0f;
    vehicle->body.torque.x = 0.0f;
    vehicle->body.torque.y = 0.0f;
    vehicle->body.torque.z = 0.0f;
    ComputeVehicleForces(&vehicle->body);
    memcpy(&vehicle->body.stepStates[1], &vehicle->body.state, sizeof(RigidBodyState));
    ApplyRigidBodyImpulse(&vehicle->body.stepStates[1], &vehicle->body, PHYSICS_STEP);
    for (axis = 0; axis < 3; axis++) {
        if (VELOCITY_SIGN((&vehicle->body.state.angularVelocity.x)[axis]) !=
            VELOCITY_SIGN((&vehicle->body.stepStates[1].angularVelocity.x)[axis])) {
            (&vehicle->body.state.angularVelocity.x)[axis] = 0.0f;
        } else {
            (&vehicle->body.state.angularVelocity.x)[axis] =
                (&vehicle->body.stepStates[1].angularVelocity.x)[axis];
        }
        if (VELOCITY_SIGN((&vehicle->body.state.velocity.x)[axis]) !=
            VELOCITY_SIGN((&vehicle->body.stepStates[1].velocity.x)[axis])) {
            (&vehicle->body.state.velocity.x)[axis] = 0.0f;
        } else {
            (&vehicle->body.state.velocity.x)[axis] =
                (&vehicle->body.stepStates[1].velocity.x)[axis];
        }
    }
    memcpy(&vehicle->body.stepStates[0], &vehicle->body.state, sizeof(RigidBodyState));
    ComputeDeltaQuaternion(&vehicle->body.stepStates[0]);
    ComputeDeltaQuaternion(&vehicle->body.stepStates[0]);
    StepVehicleCollisions(&vehicle->body);
    memcpy(&vehicle->body.state, &vehicle->body.stepStates[1], sizeof(RigidBodyState));
    UpdateWheelHeights(&vehicle->body);
    QuaternionToMatrix(vehicle->body.wheels[0]->orientation, &vehicle->body.wheels[0]->state);
    QuaternionToMatrix(vehicle->body.wheels[1]->orientation, &vehicle->body.wheels[1]->state);
    QuaternionToMatrix(vehicle->body.wheels[2]->orientation, &vehicle->body.wheels[2]->state);
    QuaternionToMatrix(vehicle->body.wheels[3]->orientation, &vehicle->body.wheels[3]->state);
    AddTimingMarker(0, 0, 0x80, 0, 0xff);
}

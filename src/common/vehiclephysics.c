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

#define AddTimingMarker(...) NullStub()
#define SIGN_OF(x) (!((x) < 0.0f) && !((x) > 0.0f) ? 0.0f : ((x) > 0.0f ? 1.0f : -1.0f))
#else
#define SIGN_OF(x) ((x) == 0.0f ? 0.0f : (0.0f < (x) ? 1.0f : -1.0f))
#endif /* TGR_BOSS_RALLY */

#define PHYSICS_STEP (1.0f / 30.0f)
#define WHEEL_DRAW_DROP 0.254f
#define TWO_RAD_TO_DEG 114.5915527f
#define VECTOR_AXIS(v, axis) ((&(v)->x)[axis])
#define KEEP_UNREVERSED(velocity, damped)                                                          \
    ((velocity) = SIGN_OF(damped) == SIGN_OF(velocity) ? (damped) : 0.0f)

#if TGR_BOSS_RALLY
inline void Vehicle::DriveRearWheel(RigidBody *chassis, RigidBody *wheel, f32 dt) {
    DriveWheel(chassis, wheel, &rearLoad, &rearSliding, dt);
}

inline void Vehicle::DriveFrontWheel(RigidBody *chassis, RigidBody *wheel, f32 dt) {
    DriveWheel(chassis, wheel, &frontLoad, &frontSliding, dt);
}

inline void Vehicle::ApplyWheelSideGrip(RigidBody *chassis, f32 dt) {
    ApplySideGrip(chassis, dt, &rearLoad, &frontLoad, &rearSliding, &frontSliding);
}

#define DRIVE_REAR_WHEEL(vehicle, body, wheel, dt) ((vehicle)->DriveRearWheel(body, wheel, dt))
#define DRIVE_FRONT_WHEEL(vehicle, body, wheel, dt) ((vehicle)->DriveFrontWheel(body, wheel, dt))
#define APPLY_WHEEL_SIDE_GRIP(vehicle, body, dt) ((vehicle)->ApplyWheelSideGrip(body, dt))
#else
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

#if TGR_PLATFORM != TGR_PLATFORM_N64 || (!defined(__mips__) && !defined(__sgi))
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
#endif

#include "vehiclereset.h"

#include "camera.h"
#include "game.h"
#include "track.h"
#include "trackmenuitem.h"
#include "trackselect.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/settings.h"
#include "screen.h"
#endif

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#include <PR/os_libc.h>
#else
#include <math.h>
#include <string.h>
#endif

#define DEFAULT_CAMERA_FOV 0.5235987902f
#define HALF_PI 1.570796371f

/* Copy the orientation of the body into both of its step states, a coordinate at a time. */
#define COPY_ORIENTATION_TO_STEPS(vehicle)                                                         \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.w =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.w;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.w =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.w;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.x =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.x;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.x =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.x;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.y =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.y;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.y =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.y;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[1].orientation.z =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.z;                                          \
    VEHICLE_GET((vehicle), Body).stepStates[0].orientation.z =                                     \
        VEHICLE_GET((vehicle), Body).state.orientation.z

void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    VEHICLE_GET(vehicle, Matrix)[3][0] = x;
    VEHICLE_GET(vehicle, Matrix)[3][1] = y;
    VEHICLE_GET(vehicle, Matrix)[3][2] = z;
    VEHICLE_GET(vehicle, SprayOrigin).x = x;
    VEHICLE_GET(vehicle, SprayOrigin).y = y;
    VEHICLE_GET(vehicle, SprayOrigin).z = z;
    VEHICLE_GET(vehicle, Body).state.position.x = x;
    VEHICLE_GET(vehicle, Body).state.position.y = y;
    VEHICLE_GET(vehicle, Body).state.position.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[1].position.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[1].position.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[1].position.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[0].position.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[0].position.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[0].position.z = z;
    QuaternionToMatrix(VEHICLE_GET(vehicle, Body).orientation, &VEHICLE_GET(vehicle, Body).state);
}

void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle) {
    f32 forwardX = cosf(angle);
    f32 forwardY = sinf(angle);
    f32 sideX = cosf(angle + HALF_PI);
    f32 sideY = sinf(angle + HALF_PI);
    f32 half = angle * 0.5f;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    /* The original reserves stack for 44 bytes of locals it does not use. */
    f32 unused[11];

    (void)unused;
#endif
    VEHICLE_GET(vehicle, Matrix)[0][0] = forwardX;
    VEHICLE_GET(vehicle, Matrix)[0][1] = forwardY;
    VEHICLE_GET(vehicle, Matrix)[0][2] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[1][0] = sideX;
    VEHICLE_GET(vehicle, Matrix)[1][1] = sideY;
    VEHICLE_GET(vehicle, Matrix)[1][2] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[2][1] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[2][0] = 0.0f;
    VEHICLE_GET(vehicle, Matrix)[2][2] = 1.0f;
    VEHICLE_GET(vehicle, Body).state.orientation.w = cosf(half);
    VEHICLE_GET(vehicle, Body).state.orientation.y = 0.0f;
    VEHICLE_GET(vehicle, Body).state.orientation.x = 0.0f;
    VEHICLE_GET(vehicle, Body).state.orientation.z = sinf(half);
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(VEHICLE_GET(vehicle, Body).orientation, &VEHICLE_GET(vehicle, Body).state);
}

void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]) {
    memcpy(VEHICLE_GET(vehicle, Matrix), m, sizeof(VEHICLE_GET(vehicle, Matrix)));
    MatrixToQuaternion(m, &VEHICLE_GET(vehicle, Body).state.orientation);
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(VEHICLE_GET(vehicle, Body).orientation, &VEHICLE_GET(vehicle, Body).state);
}

void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll) {
    Quaternion turn;
    f32 sine;

    yaw *= 0.5f;
    pitch *= 0.5f;
    roll *= 0.5f;
    sine = sinf(yaw);
    turn.w = cosf(yaw);
    turn.x = 0.0f;
    turn.y = 0.0f;
    turn.z = sine;
    QuaternionMultiply(&VEHICLE_GET(vehicle, Body).state.orientation,
                       &VEHICLE_GET(vehicle, Body).state.orientation,
                       &turn);
    sine = sinf(pitch);
    turn.w = cosf(pitch);
    turn.x = 0.0f;
    turn.y = sine;
    turn.z = 0.0f;
    QuaternionMultiply(&VEHICLE_GET(vehicle, Body).state.orientation,
                       &VEHICLE_GET(vehicle, Body).state.orientation,
                       &turn);
    sine = sinf(roll);
    turn.w = cosf(roll);
    turn.x = sine;
    turn.y = 0.0f;
    turn.z = 0.0f;
    QuaternionMultiply(&VEHICLE_GET(vehicle, Body).state.orientation,
                       &VEHICLE_GET(vehicle, Body).state.orientation,
                       &turn);
    QuaternionNormalize(&VEHICLE_GET(vehicle, Body).state.orientation);
    COPY_ORIENTATION_TO_STEPS(vehicle);
}

void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    VEHICLE_GET(vehicle, Body).state.angularVelocity.x = x;
    VEHICLE_GET(vehicle, Body).state.angularVelocity.y = y;
    VEHICLE_GET(vehicle, Body).state.angularVelocity.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[1].angularVelocity.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[1].angularVelocity.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[1].angularVelocity.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[0].angularVelocity.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[0].angularVelocity.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[0].angularVelocity.z = z;
}

void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    Vec3f *placed = &VEHICLE_GET(vehicle, PlacedVelocity);

    VEHICLE_GET(vehicle, Body).state.velocity.x = x;
    VEHICLE_GET(vehicle, Body).state.velocity.y = y;
    VEHICLE_GET(vehicle, Body).state.velocity.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[1].velocity.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[1].velocity.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[1].velocity.z = z;
    VEHICLE_GET(vehicle, Body).stepStates[0].velocity.x = x;
    VEHICLE_GET(vehicle, Body).stepStates[0].velocity.y = y;
    VEHICLE_GET(vehicle, Body).stepStates[0].velocity.z = z;
    placed->x = x;
    placed->y = y;
    placed->z = z;
}

void ResetVehicleState(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    const VehicleModel *spec = VEHICLE_GET(vehicle, Model);
#else
    const CarPhysicsSpec *spec = &g_aCarPhysicsSpecs[vehicle->carModel];
#endif

    /* The frames of the vehicle and its cameras, all but camera 4, and of its wheels. */
    FinishFrameMatrix(VEHICLE_GET(vehicle, Matrix));
    FinishFrameMatrix((f32(*)[4]) & VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT]);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE]);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_BUMPER]);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_BUMPER].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE]);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR]);
    VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR].fov = DEFAULT_CAMERA_FOV;
    VEHICLE_SET(vehicle, ActiveCamera, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT]);
#if TGR_PLATFORM == TGR_PLATFORM_N64
    VEHICLE_GET(vehicle, Model) = NULL;
#endif
    FinishFrameMatrix(VEHICLE_GET(vehicle, WheelMatrices)[0]);
    FinishFrameMatrix(VEHICLE_GET(vehicle, WheelMatrices)[1]);
    FinishFrameMatrix(VEHICLE_GET(vehicle, WheelMatrices)[2]);
    FinishFrameMatrix(VEHICLE_GET(vehicle, WheelMatrices)[3]);
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    VEHICLE_SET(vehicle, Section, NULL);
    VEHICLE_SET(vehicle, PointIndex, 0);
    VEHICLE_SET(vehicle, PreviousCamera, NULL);

    memcpy(VEHICLE_GET(vehicle, GearRatios),
           spec->gearRatios,
           sizeof(VEHICLE_GET(vehicle, GearRatios)));
    VEHICLE_GET(vehicle, TorqueCurve)[0] = spec->torqueCurve[0];
    VEHICLE_GET(vehicle, TorqueCurve)[1] = spec->torqueCurve[1];
    VEHICLE_GET(vehicle, TorqueCurve)[2] = spec->torqueCurve[2];
    VEHICLE_GET(vehicle, TorqueCurve)[3] = spec->torqueCurve[3];
    VEHICLE_SET(vehicle, FinalDrive, spec->finalDrive);
    VEHICLE_SET(vehicle, GearCount, spec->gearCount);
    VEHICLE_SET(vehicle, Transmission, VEHICLE_GET(vehicle, ChosenTransmission));
    VEHICLE_SET(vehicle, RearWheelDrive, spec->rearWheelDrive);
    VEHICLE_GET(vehicle, Body).boxSize.x = spec->boxSize.x;
    VEHICLE_GET(vehicle, Body).boxSize.y = spec->boxSize.y;
    VEHICLE_GET(vehicle, Body).boxSize.z = spec->boxSize.z;
    VEHICLE_GET(vehicle, Body).boxCenterZ = spec->boxCenterZ;
    VEHICLE_SET(vehicle, TableIndex, spec->tableIndex);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    VEHICLE_SET(vehicle, Model, NULL);
#endif
}

/* Set the state of the body of a wheel at rest at a place under the vehicle. The statements share
 * the line of the use, as in the original. */
#define INIT_WHEEL_STATE(wheel, px, py)                                                            \
    (wheel)->state.position.x = (px);                                                              \
    (wheel)->state.position.y = (py);                                                              \
    (wheel)->state.position.z = -0.1f;                                                             \
    (wheel)->state.angularVelocity.x = 0.0f;                                                       \
    (wheel)->state.angularVelocity.y = 0.0f;                                                       \
    (wheel)->state.angularVelocity.z = 0.0f;                                                       \
    (wheel)->state.orientation.w = 1.0f;                                                           \
    (wheel)->state.orientation.x = 0.0f;                                                           \
    (wheel)->state.orientation.y = 0.0f;                                                           \
    (wheel)->state.orientation.z = 0.0f;                                                           \
    (wheel)->state.velocity.x = 0.0f;                                                              \
    (wheel)->state.velocity.y = 0.0f;                                                              \
    (wheel)->state.velocity.z = 0.0f;                                                              \
    QuaternionToMatrix((wheel)->orientation, &(wheel)->state)

/* A force at a point of the body, which the physics sets each step. */
#define INIT_BODY_FORCE(force, px, py)                                                             \
    SetRigidBodyForce(force, 0.0f, 0.0f, 0.0f, px, py, 0.0f, RIGID_BODY_FORCE_BODY)

void InitVehiclePhysics(Vehicle *vehicle) {
    VEHICLE_SET(vehicle, SkipDrive, 1);
    VEHICLE_SET(vehicle, Gear, 0);
    VEHICLE_SET(vehicle, SteerAngle, 0.0f);
    VEHICLE_SET(vehicle, EngineRpm, 0.0f);
    if (VEHICLE_GET(vehicle, Model) != NULL) {
        VEHICLE_GET(vehicle, Body).kind = RIGID_BODY_BOX_1;
        VEHICLE_GET(vehicle, Body).size.y = 2.0f;
        VEHICLE_GET(vehicle, Body).mass = 1000.0f;
        VEHICLE_GET(vehicle, Body).size.x = 3.5f;
        VEHICLE_GET(vehicle, Body).size.z = 1.5f;
        InitRigidBody(&VEHICLE_GET(vehicle, Body));
        VEHICLE_GET(vehicle, Body).state.position.x = 0.0f;
        VEHICLE_GET(vehicle, Body).state.position.y = 0.0f;
        VEHICLE_GET(vehicle, Body).state.position.z = 2.0f;
        VEHICLE_GET(vehicle, Body).state.velocity.x = 0.0f;
        VEHICLE_GET(vehicle, Body).state.velocity.y = 0.0f;
        VEHICLE_GET(vehicle, Body).state.velocity.z = 0.0f;
        VEHICLE_GET(vehicle, Body).state.orientation.x = 0.0f;
        VEHICLE_GET(vehicle, Body).state.orientation.y = 0.0f;
        VEHICLE_GET(vehicle, Body).state.orientation.z = 0.0f;
        VEHICLE_GET(vehicle, Body).state.angularVelocity.x = 0.0f;
        VEHICLE_GET(vehicle, Body).state.angularVelocity.y = 0.0f;
        VEHICLE_GET(vehicle, Body).state.angularVelocity.z = 0.0f;
        VEHICLE_GET(vehicle, Body).state.orientation.w = 1.0f;
        QuaternionToMatrix(VEHICLE_GET(vehicle, Body).orientation,
                           &VEHICLE_GET(vehicle, Body).state);
        VEHICLE_GET(vehicle, Body).springRate =
            ((f32)VEHICLE_GET(vehicle, SuspensionStiffness) * 4.0f + 20.0f) * 16000.0f;
        VEHICLE_GET(vehicle, Body).damping = -3000.0f;

        /* The rear and the front wheel of the model, and those of the other side mirrored. */
        VEHICLE_GET(vehicle, WheelBodies)[0].suspension = 0;
        VEHICLE_GET(vehicle, WheelBodies)[0].ground = NULL;
        VEHICLE_GET(vehicle, WheelBodies)[0].spinRate = 0;
        VEHICLE_GET(vehicle, WheelBodies)[0].kind = RIGID_BODY_NO_ROTATION;
        VEHICLE_GET(vehicle, WheelBodies)[0].mass = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[0].size.x = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[0].size.y = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[0].size.z = 0.0f;
        InitRigidBody(&VEHICLE_GET(vehicle, WheelBodies)[0]);
        INIT_WHEEL_STATE(&VEHICLE_GET(vehicle, WheelBodies)[0],
                         VEHICLE_GET(vehicle, Model)->wheels[2].x,
                         VEHICLE_GET(vehicle, Model)->wheels[2].y);
        VEHICLE_GET(vehicle, WheelBodies)[2].suspension = 0;
        VEHICLE_GET(vehicle, WheelBodies)[2].ground = NULL;
        VEHICLE_GET(vehicle, WheelBodies)[2].spinRate = 0;
        VEHICLE_GET(vehicle, WheelBodies)[2].kind = RIGID_BODY_NO_ROTATION;
        VEHICLE_GET(vehicle, WheelBodies)[2].mass = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[2].size.x = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[2].size.y = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[2].size.z = 0.0f;
        InitRigidBody(&VEHICLE_GET(vehicle, WheelBodies)[2]);
        INIT_WHEEL_STATE(&VEHICLE_GET(vehicle, WheelBodies)[2],
                         VEHICLE_GET(vehicle, Model)->wheels[2].x,
                         -VEHICLE_GET(vehicle, Model)->wheels[2].y);
        VEHICLE_GET(vehicle, WheelBodies)[1].suspension = 0;
        VEHICLE_GET(vehicle, WheelBodies)[1].ground = NULL;
        VEHICLE_GET(vehicle, WheelBodies)[1].spinRate = 0;
        VEHICLE_GET(vehicle, WheelBodies)[1].kind = RIGID_BODY_NO_ROTATION;
        VEHICLE_GET(vehicle, WheelBodies)[1].mass = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[1].size.x = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[1].size.y = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[1].size.z = 0.0f;
        InitRigidBody(&VEHICLE_GET(vehicle, WheelBodies)[1]);
        INIT_WHEEL_STATE(&VEHICLE_GET(vehicle, WheelBodies)[1],
                         VEHICLE_GET(vehicle, Model)->wheels[1].x,
                         VEHICLE_GET(vehicle, Model)->wheels[1].y);
        VEHICLE_GET(vehicle, WheelBodies)[3].suspension = 0;
        VEHICLE_GET(vehicle, WheelBodies)[3].ground = NULL;
        VEHICLE_GET(vehicle, WheelBodies)[3].spinRate = 0;
        VEHICLE_GET(vehicle, WheelBodies)[3].kind = RIGID_BODY_NO_ROTATION;
        VEHICLE_GET(vehicle, WheelBodies)[3].mass = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[3].size.x = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[3].size.y = 0.0f;
        VEHICLE_GET(vehicle, WheelBodies)[3].size.z = 0.0f;
        InitRigidBody(&VEHICLE_GET(vehicle, WheelBodies)[3]);
        INIT_WHEEL_STATE(&VEHICLE_GET(vehicle, WheelBodies)[3],
                         VEHICLE_GET(vehicle, Model)->wheels[1].x,
                         -VEHICLE_GET(vehicle, Model)->wheels[1].y);
        VEHICLE_GET(vehicle, Body).wheels[0] = &VEHICLE_GET(vehicle, WheelBodies)[0];
        VEHICLE_GET(vehicle, Body).wheels[1] = &VEHICLE_GET(vehicle, WheelBodies)[2];
        VEHICLE_GET(vehicle, Body).wheels[2] = &VEHICLE_GET(vehicle, WheelBodies)[1];
        VEHICLE_GET(vehicle, Body).wheels[3] = &VEHICLE_GET(vehicle, WheelBodies)[3];

        /* The springs over the wheels, then gravity. */
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_0], -1.5f, -1.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_2], -1.5f, 1.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_1], 1.5f, -1.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_3], 1.5f, 1.0f);
        SetRigidBodyForce(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_GRAVITY],
                          0.0f,
                          0.0f,
                          -15450.75f,
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        SetRigidBodyForce(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY],
                          0.0f,
                          0.0f,
                          -(10.5f * 9.81f),
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        SetRigidBodyForce(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_REAR_WHEEL_GRAVITY],
                          0.0f,
                          0.0f,
                          -(10.5f * 9.81f),
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_SPRING_0].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_2];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_SPRING_2].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_1];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_SPRING_1].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_3];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_SPRING_3].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_GRAVITY];
        VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_GRAVITY].next = NULL;
        VEHICLE_GET(vehicle, Body).forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_SPRING_0];

        /* The drive of each wheel, then the gravity of its axle. */
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_0], 0.0f, 0.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_2], 0.0f, 0.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_1], 0.0f, 0.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_3], 0.0f, 0.0f);
        VEHICLE_GET(vehicle, WheelBodies)
        [0].forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_0];
        VEHICLE_GET(vehicle, WheelBodies)
        [2].forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_2];
        VEHICLE_GET(vehicle, WheelBodies)
        [1].forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_1];
        VEHICLE_GET(vehicle, WheelBodies)
        [3].forces = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_WHEEL_3];
        VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_REAR_WHEEL_GRAVITY].next = NULL;
        VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY].next = NULL;
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_WHEEL_0].next =
            &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_REAR_WHEEL_GRAVITY];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_WHEEL_2].next =
            &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_REAR_WHEEL_GRAVITY];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_WHEEL_1].next =
            &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_WHEEL_3].next =
            &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY];

        /* The dampers over the wheels, then the drag. */
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_0], -1.5f, -1.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_2], -1.5f, 1.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_1], 1.5f, -1.0f);
        INIT_BODY_FORCE(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_3], 1.5f, 1.0f);
        SetRigidBodyForce(&VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DRAG],
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_DAMPER_0].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_2];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_DAMPER_2].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_1];
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_DAMPER_1].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DAMPER_3];
        VEHICLE_GET(vehicle, Body).tireType = VEHICLE_GET(vehicle, TireType);
        VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DRAG].next = NULL;
        VEHICLE_SET(vehicle, SteerReturn, 0);
        VEHICLE_SET(vehicle, FrontLoad, 0);
        VEHICLE_SET(vehicle, RearLoad, 0);
        VEHICLE_GET(vehicle, Forces)
        [VEHICLE_FORCE_DAMPER_3].next = &VEHICLE_GET(vehicle, Forces)[VEHICLE_FORCE_DRAG];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        NullStub();
        NullStub();
        NullStub();
        NullStub();
#endif
    }
    VEHICLE_SET(vehicle, RearSliding, 0);
    VEHICLE_SET(vehicle, FrontSliding, 0);
}

/* The spray of the wheels: where each starts its phase, and how far apart. */
#define SPRAY_FRAMES_STEP 0.137f
#define SPRAY_FRAMES_SPREAD 0.034f
#define SMOKE_TIMER_STEP 0.15f
#define SPRAY_VERTEX_ALPHA 0xFF

void ResetVehicleEffects(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    ConvertedModelVertex *vertex;
#else
    Vtx *vertex;
#endif
    s16 *velocity;
    s16 *surface;
    f32 start;
    s32 i;
    /* Each loop over the wheels has its own counter, and the original unrolls them. */
    s32 j;
    s32 k;
    s32 l;

    /* The staggered starts are written and at once cleared. */
    start = (f32)VEHICLE_GET(vehicle, Index) * SPRAY_FRAMES_STEP;
    VEHICLE_GET(vehicle, SprayFramesLeft)[0] = start;
    VEHICLE_GET(vehicle, SprayFramesLeft)[1] = start + SPRAY_FRAMES_SPREAD;
    VEHICLE_GET(vehicle, SprayFramesLeft)
    [2] = VEHICLE_GET(vehicle, SprayFramesLeft)[1] + SPRAY_FRAMES_SPREAD;
    VEHICLE_GET(vehicle, SprayFramesLeft)
    [3] = VEHICLE_GET(vehicle, SprayFramesLeft)[2] + SPRAY_FRAMES_SPREAD;
    for (j = 0; j < VEHICLE_WHEELS; j++) {
        VEHICLE_GET(vehicle, SprayAdvanced)[j] = 0;
        VEHICLE_GET(vehicle, SpraySurfaces)[j] = TRACK_SURFACE_START;
    }
    for (k = 0; k < VEHICLE_WHEELS; k++) {
        VEHICLE_GET(vehicle, SmokeTimers)[k] = (f32)k * SMOKE_TIMER_STEP;
    }
    for (l = 0; l < VEHICLE_WHEELS; l++) {
        VEHICLE_GET(vehicle, SprayPhases)[l] = 0.0f;
        VEHICLE_GET(vehicle, SprayFramesLeft)[l] = 0.0f;
    }

    velocity = &VEHICLE_GET(vehicle, SprayVelocities)[0][0][0][0];
    vertex = &VEHICLE_GET(vehicle, SprayVertices)[0][0][0];
    for (i = 0; i < VEHICLE_WHEELS * SPRAY_SEGMENTS * SPRAY_SEGMENT_VERTICES; i++) {
        velocity[2] = 0;
        velocity[1] = velocity[2];
        velocity[0] = velocity[2];
        velocity += 3;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vertex->position.x = 0.0f;
        vertex->position.y = 0.0f;
        vertex->position.z = 0.0f;
        vertex->normal.x = 0.0f;
        vertex->normal.y = 0.0f;
        vertex->normal.z = 0.0f;
#else
        vertex->v.ob[0] = 0;
        vertex->v.ob[1] = 0;
        vertex->v.ob[2] = 0;
        vertex->v.cn[0] = 0;
        /* Both kinds of vertex are cleared alike. */
        if (i & 1) {
            vertex->v.cn[1] = 0;
        } else {
            vertex->v.cn[1] = 0;
        }
        vertex->v.cn[2] = 0;
        vertex->v.cn[3] = SPRAY_VERTEX_ALPHA;
#endif
        vertex++;
    }

    surface = &VEHICLE_GET(vehicle, SpraySegmentSurfaces)[0][0];
    for (i = 0; i < VEHICLE_WHEELS * SPRAY_SEGMENTS; i++) {
        surface[i] = TRACK_SURFACE_START;
    }
}

#define RESPAWN_FALL_DEPTH 10.0f
#define RESPAWN_HEIGHT 2.0
#define RESPAWN_SPREAD 0.01f
#define RESPAWN_OPACITY 0.1f
#define AI_START_GRACE -180

void RespawnVehicle(Vehicle *vehicle) {
    if (VEHICLE_GET(vehicle, Body).uprightFrames >= 0 &&
        !(VEHICLE_GET(vehicle, Matrix)[3][2] < g_LoadedTrack.minZ - RESPAWN_FALL_DEPTH)) {
        return;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    ResetVehicleEffects(vehicle);
#endif
    InitVehiclePhysics(vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION == TGR_REGION_US
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
#else
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO) {
#endif
        SetVehiclePosition(
            vehicle,
            VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.x,
            VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.y,
            VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.z +
                RESPAWN_HEIGHT);
    } else {
        SetVehiclePosition(
            vehicle,
            VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.x +
                (f32)VEHICLE_GET(vehicle, Index) * RESPAWN_SPREAD,
            VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.y,
            VEHICLE_GET(vehicle, Section)->points[VEHICLE_GET(vehicle, PointIndex)].position.z +
                RESPAWN_HEIGHT);
    }
    InitVehicleOrientationFromAngle(
        vehicle,
        Atan2f(VEHICLE_GET(vehicle, TrackDirection).x, VEHICLE_GET(vehicle, TrackDirection).y));
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    SetVehicleTurnRate(vehicle, 0.0f, 0.0f, 0.0f);
    VEHICLE_GET(vehicle, Body).wheels[0]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[0]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[0]->surface = TRACK_SURFACE_START;
    VEHICLE_GET(vehicle, Body).wheels[1]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[1]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[1]->surface = TRACK_SURFACE_START;
    VEHICLE_GET(vehicle, Body).wheels[3]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[3]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[3]->surface = TRACK_SURFACE_START;
    VEHICLE_GET(vehicle, Body).wheels[2]->ground = NULL;
    VEHICLE_GET(vehicle, Body).wheels[2]->contactFrames = 0;
    VEHICLE_GET(vehicle, Body).wheels[2]->surface = TRACK_SURFACE_START;
    VEHICLE_SET(vehicle, ForcedForwardFrames, 0);
    VEHICLE_SET(vehicle, ForcedReverseFrames, 0);
    VEHICLE_SET(vehicle, ReverseFrames, 0);
    VEHICLE_SET(vehicle, ForwardFrames, AI_START_GRACE);
    VEHICLE_GET(vehicle, Body).uprightFrames = 0;
    VEHICLE_SET(vehicle, GhostMode, VEHICLE_GHOST);
    VEHICLE_SET(vehicle, Opacity, RESPAWN_OPACITY);
    UpdateCameraChase(vehicle, &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE], 1.0f);
    VEHICLE_SET(vehicle, CameraChanged, 1);
    VEHICLE_GET(vehicle, ChasePosition).x =
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.x;
    VEHICLE_GET(vehicle, ChasePosition).y =
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.y;
    VEHICLE_GET(vehicle, ChasePosition).z =
        VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_CHASE].position.z;
}

void InitVehicleColorAndPhysics(Vehicle *vehicle) {
    /* The Japanese release does not paint the model here. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION != TGR_REGION_JP
    SetMaterialColor(VEHICLE_GET(vehicle, Model),
                     VEHICLE_GET(vehicle, Color)[0] >> 3,
                     VEHICLE_GET(vehicle, Color)[1] >> 3,
                     VEHICLE_GET(vehicle, Color)[2] >> 3);
#endif
    InitVehiclePhysics(vehicle);
}

void SetVehicleModelSlot(Vehicle *vehicle, s32 slot) {
    VEHICLE_SET(vehicle, Model, &g_aVehicleModels[slot]);
    InitVehicleColorAndPhysics(vehicle);
}

void ClearVehicleDamageState(Vehicle *vehicle) {
    s32 i;

    for (i = 0; i < VEHICLE_DENT_SIDES; i++) {
        VEHICLE_GET(vehicle, Dents)[i] = 0;
    }
    VEHICLE_SET(vehicle, DentJitter, 0);
}

#define GRID_COLUMN_SPACING 3.0f
#define GRID_ROW_SPACING 8.0f
#define GRID_HEIGHT 0.1f
#define QUARTER_TURN 1.570796371f

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define SELECTED_SETUP_TRACK() (g_apTrackMenuItems[g_dwSelectedTrackId])
#else
#define SELECTED_SETUP_TRACK() ((TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])
#endif

void SetupVehicleForRaceStart(Vehicle *vehicle) {
    TrackSection *root;
    f32 row;
    f32 column;
    f32 forwardX;
    f32 forwardY;
    f32 sideX;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double sideY;
    s32 gridIndex;
#else
    f32 sideY;
#endif
    s16 weather;

    ResetVehicleState(vehicle);
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO ||
        (g_dwGameMode == GAME_MODE_PRACTICE && g_dwNumLocalPlayers == 1) ||
        g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        /* Alone on the grid, in the middle of the front row. */
        SetVehicleModelSlot(vehicle, VEHICLE_GET(vehicle, Index));
        row = 0.0f;
        column = 0.5f;
    } else {
        SetVehicleModelSlot(vehicle, VEHICLE_GET(vehicle, Index));
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        gridIndex =
            g_dwNetworkPlayMode ? VEHICLE_GET(vehicle, GridIndex) : VEHICLE_GET(vehicle, Index);
        row = (f32)(gridIndex >> 1);
        column = (f32)((gridIndex & 1) ^ 1);
#else
        row = (f32)(vehicle->index >> 1);
        column = (f32)((vehicle->index & 1) ^ 1);
#endif
    }
    forwardX = cosf(g_LoadedTrack.startHeading);
    forwardY = sinf(g_LoadedTrack.startHeading);
    sideX = cosf(g_LoadedTrack.startHeading + QUARTER_TURN);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    sideY = sinl((f32)(g_LoadedTrack.startHeading + QUARTER_TURN));
#else
    sideY = sinf(g_LoadedTrack.startHeading + QUARTER_TURN);
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    SetVehiclePosition(vehicle,
                       g_LoadedTrack.startPosition.x -
                           ((long double)sideX * (f32)(column - 0.5f)) * GRID_COLUMN_SPACING -
                           ((long double)forwardX * (f32)(row + 1.0f)) * GRID_ROW_SPACING,
                       g_LoadedTrack.startPosition.y -
                           (sideY * (f32)(column - 0.5f)) * GRID_COLUMN_SPACING -
                           ((long double)forwardY * (f32)(row + 1.0f)) * GRID_ROW_SPACING,
                       g_LoadedTrack.startPosition.z + GRID_HEIGHT);
#else
    SetVehiclePosition(
        vehicle,
        g_LoadedTrack.startPosition.x - sideX * GRID_COLUMN_SPACING * (column - 0.5f) -
            forwardX * GRID_ROW_SPACING * (row + 1.0f),
        g_LoadedTrack.startPosition.y - sideY * GRID_COLUMN_SPACING * (column - 0.5f) -
            forwardY * GRID_ROW_SPACING * (row + 1.0f),
        g_LoadedTrack.startPosition.z + GRID_HEIGHT);
#endif
    VEHICLE_SET(vehicle, Distance, (row + 0.5f) * -GRID_ROW_SPACING);
    InitVehicleOrientationFromAngle(vehicle, g_LoadedTrack.startHeading);
    VEHICLE_GET(vehicle, PreviousPosition) = VEHICLE_GET(vehicle, Frame).position;
    VEHICLE_GET(vehicle, PreviousListenerPosition) = VEHICLE_GET(vehicle, Frame).position;
    InitCameraForVehicle(vehicle);
    VEHICLE_SET(vehicle, ForcedForwardFrames, 0);
    VEHICLE_SET(vehicle, ForcedReverseFrames, 0);
    VEHICLE_SET(vehicle, ReverseFrames, 0);
    VEHICLE_SET(vehicle, ForwardFrames, AI_START_GRACE);
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);

    root = g_LoadedTrack.rootSection;
    if (root != NULL) {
        VEHICLE_SET(vehicle, LapTime, 0.0f);
        VEHICLE_SET(vehicle, TotalTime, 0.0f);
        VEHICLE_SET(vehicle, BestLapTime, 0.0f);
        weather = g_dwCurrentWeather - 1;
        if (weather >= ARCADE_WEATHERS || weather < 0) {
            weather = 0;
        }
        VEHICLE_SET(vehicle,
                    TimeLeft,
                    SELECTED_SETUP_TRACK()
                        ->arcadeTimes[VEHICLE_GET(vehicle, TableIndex)][weather]
                        .startTime);
        VEHICLE_SET(vehicle, PointIndex, 0);
        VEHICLE_SET(vehicle, Section, g_LoadedTrack.rootSection);
        Vec3fDirection(&VEHICLE_GET(vehicle, TrackDirection),
                       &g_LoadedTrack.rootSection->points[0].position,
                       &g_LoadedTrack.rootSection->points[1].position);
    } else {
        VEHICLE_GET(vehicle, TrackDirection).y = 0.0f;
        VEHICLE_GET(vehicle, TrackDirection).z = 0.0f;
        VEHICLE_GET(vehicle, TrackDirection).x = 1.0f;
    }

    VEHICLE_SET(vehicle, Lap, 0);
    VEHICLE_SET(vehicle, BestLap, 0);
    VEHICLE_SET(vehicle, RacePosition, g_dwNumVehicles - VEHICLE_GET(vehicle, Index) - 1);
    VEHICLE_SET(vehicle, MaxGate, 0);
    /* The players and the vehicles of these modes start behind the first gate. */
    if (VEHICLE_GET(vehicle, Index) < (s32)g_dwNumLocalPlayers ||
        g_dwGameMode == GAME_MODE_ARCADE || g_dwGameMode == GAME_MODE_TIME_ATTACK ||
        g_dwGameMode == GAME_MODE_DEMO
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        || g_dwGameMode == GAME_MODE_NETWORK
#endif
    ) {
        VEHICLE_SET(vehicle, DistanceLaps, -1);
        VEHICLE_SET(vehicle, Gate, -1);
    } else {
        VEHICLE_SET(vehicle, DistanceLaps, 0);
        VEHICLE_SET(vehicle, Gate, 0);
    }
    VEHICLE_SET(vehicle, GroundInstanceCount, 0);
    VEHICLE_SET(vehicle, NearbyInstanceCount, 0);
    VEHICLE_SET(vehicle, CameraHeading, 0.0f);
    VEHICLE_SET(vehicle, Odometer, 0.0f);
    VEHICLE_SET(vehicle, SpeedKph, 0.0f);
    VEHICLE_GET(vehicle, JoltPosition).x = 0.0f;
    VEHICLE_GET(vehicle, JoltPosition).y = 0.0f;
    VEHICLE_GET(vehicle, JoltPosition).z = 0.0f;
    VEHICLE_GET(vehicle, JoltVelocity).x = 0.0f;
    VEHICLE_GET(vehicle, JoltVelocity).y = 0.0f;
    VEHICLE_GET(vehicle, JoltVelocity).z = 0.0f;
    VEHICLE_GET(vehicle, JoltOffset).x = 0.0f;
    VEHICLE_GET(vehicle, JoltOffset).y = 0.0f;
    VEHICLE_GET(vehicle, JoltOffset).z = 0.0f;
    VEHICLE_SET(vehicle, GroundHits, 0);
    VEHICLE_SET(vehicle, SlotCooldown, 0);
    ResetVehicleEffects(vehicle);
    ClearVehicleDamageState(vehicle);
    VEHICLE_SET(vehicle, CameraChanged, 0);
    VEHICLE_SET(vehicle, EffectSound, 0);
    VEHICLE_SET(vehicle, EffectVolume, 0);
    VEHICLE_SET(vehicle, EffectPriority, 0);
    VEHICLE_SET(vehicle, DopplerFactor, 1.0f);
}

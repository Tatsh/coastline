#include "vehiclereset.h"

#include "track.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
    (vehicle)->body.stepStates[1].orientation.w = (vehicle)->body.state.orientation.w;             \
    (vehicle)->body.stepStates[0].orientation.w = (vehicle)->body.state.orientation.w;             \
    (vehicle)->body.stepStates[1].orientation.x = (vehicle)->body.state.orientation.x;             \
    (vehicle)->body.stepStates[0].orientation.x = (vehicle)->body.state.orientation.x;             \
    (vehicle)->body.stepStates[1].orientation.y = (vehicle)->body.state.orientation.y;             \
    (vehicle)->body.stepStates[0].orientation.y = (vehicle)->body.state.orientation.y;             \
    (vehicle)->body.stepStates[1].orientation.z = (vehicle)->body.state.orientation.z;             \
    (vehicle)->body.stepStates[0].orientation.z = (vehicle)->body.state.orientation.z

void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    vehicle->matrix[3][0] = x;
    vehicle->matrix[3][1] = y;
    vehicle->matrix[3][2] = z;
    vehicle->sprayOrigin.x = x;
    vehicle->sprayOrigin.y = y;
    vehicle->sprayOrigin.z = z;
    vehicle->body.state.position.x = x;
    vehicle->body.state.position.y = y;
    vehicle->body.state.position.z = z;
    vehicle->body.stepStates[1].position.x = x;
    vehicle->body.stepStates[1].position.y = y;
    vehicle->body.stepStates[1].position.z = z;
    vehicle->body.stepStates[0].position.x = x;
    vehicle->body.stepStates[0].position.y = y;
    vehicle->body.stepStates[0].position.z = z;
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
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
    vehicle->matrix[0][0] = forwardX;
    vehicle->matrix[0][1] = forwardY;
    vehicle->matrix[0][2] = 0.0f;
    vehicle->matrix[1][0] = sideX;
    vehicle->matrix[1][1] = sideY;
    vehicle->matrix[1][2] = 0.0f;
    vehicle->matrix[2][1] = 0.0f;
    vehicle->matrix[2][0] = 0.0f;
    vehicle->matrix[2][2] = 1.0f;
    vehicle->body.state.orientation.w = cosf(half);
    vehicle->body.state.orientation.y = 0.0f;
    vehicle->body.state.orientation.x = 0.0f;
    vehicle->body.state.orientation.z = sinf(half);
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
}

void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]) {
    memcpy(vehicle->matrix, m, sizeof(vehicle->matrix));
    MatrixToQuaternion(m, &vehicle->body.state.orientation);
    COPY_ORIENTATION_TO_STEPS(vehicle);
    QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
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
    QuaternionMultiply(&vehicle->body.state.orientation, &vehicle->body.state.orientation, &turn);
    sine = sinf(pitch);
    turn.w = cosf(pitch);
    turn.x = 0.0f;
    turn.y = sine;
    turn.z = 0.0f;
    QuaternionMultiply(&vehicle->body.state.orientation, &vehicle->body.state.orientation, &turn);
    sine = sinf(roll);
    turn.w = cosf(roll);
    turn.x = sine;
    turn.y = 0.0f;
    turn.z = 0.0f;
    QuaternionMultiply(&vehicle->body.state.orientation, &vehicle->body.state.orientation, &turn);
    QuaternionNormalize(&vehicle->body.state.orientation);
    COPY_ORIENTATION_TO_STEPS(vehicle);
}

void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    vehicle->body.state.angularVelocity.x = x;
    vehicle->body.state.angularVelocity.y = y;
    vehicle->body.state.angularVelocity.z = z;
    vehicle->body.stepStates[1].angularVelocity.x = x;
    vehicle->body.stepStates[1].angularVelocity.y = y;
    vehicle->body.stepStates[1].angularVelocity.z = z;
    vehicle->body.stepStates[0].angularVelocity.x = x;
    vehicle->body.stepStates[0].angularVelocity.y = y;
    vehicle->body.stepStates[0].angularVelocity.z = z;
}

void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z) {
    Vec3f *placed = &vehicle->placedVelocity;

    vehicle->body.state.velocity.x = x;
    vehicle->body.state.velocity.y = y;
    vehicle->body.state.velocity.z = z;
    vehicle->body.stepStates[1].velocity.x = x;
    vehicle->body.stepStates[1].velocity.y = y;
    vehicle->body.stepStates[1].velocity.z = z;
    vehicle->body.stepStates[0].velocity.x = x;
    vehicle->body.stepStates[0].velocity.y = y;
    vehicle->body.stepStates[0].velocity.z = z;
    placed->x = x;
    placed->y = y;
    placed->z = z;
}

void ResetVehicleState(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    const VehicleModel *spec = vehicle->model;
#else
    const CarPhysicsSpec *spec = &g_aCarPhysicsSpecs[vehicle->carModel];
#endif

    /* The frames of the vehicle and its cameras, all but camera 4, and of its wheels. */
    FinishFrameMatrix(vehicle->matrix);
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_COCKPIT]);
    vehicle->cameras[VEHICLE_CAMERA_COCKPIT].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_CHASE]);
    vehicle->cameras[VEHICLE_CAMERA_CHASE].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_BUMPER]);
    vehicle->cameras[VEHICLE_CAMERA_BUMPER].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]);
    vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE].fov = DEFAULT_CAMERA_FOV;
    FinishFrameMatrix((f32(*)[4]) & vehicle->cameras[VEHICLE_CAMERA_MIRROR]);
    vehicle->cameras[VEHICLE_CAMERA_MIRROR].fov = DEFAULT_CAMERA_FOV;
    vehicle->activeCamera = &vehicle->cameras[VEHICLE_CAMERA_COCKPIT];
#if TGR_PLATFORM == TGR_PLATFORM_N64
    vehicle->model = NULL;
#endif
    FinishFrameMatrix(vehicle->wheelMatrices[0]);
    FinishFrameMatrix(vehicle->wheelMatrices[1]);
    FinishFrameMatrix(vehicle->wheelMatrices[2]);
    FinishFrameMatrix(vehicle->wheelMatrices[3]);
    SetVehicleVelocity(vehicle, 0.0f, 0.0f, 0.0f);
    vehicle->section = NULL;
    vehicle->pointIndex = 0;
    vehicle->previousCamera = NULL;

    memcpy(vehicle->gearRatios, spec->gearRatios, sizeof(vehicle->gearRatios));
    vehicle->torqueCurve[0] = spec->torqueCurve[0];
    vehicle->torqueCurve[1] = spec->torqueCurve[1];
    vehicle->torqueCurve[2] = spec->torqueCurve[2];
    vehicle->torqueCurve[3] = spec->torqueCurve[3];
    vehicle->finalDrive = spec->finalDrive;
    vehicle->gearCount = spec->gearCount;
    vehicle->transmission = vehicle->chosenTransmission;
    vehicle->rearWheelDrive = spec->rearWheelDrive;
    vehicle->body.boxSize.x = spec->boxSize.x;
    vehicle->body.boxSize.y = spec->boxSize.y;
    vehicle->body.boxSize.z = spec->boxSize.z;
    vehicle->body.boxCenterZ = spec->boxCenterZ;
    vehicle->tableIndex = spec->tableIndex;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    vehicle->model = NULL;
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
    vehicle->skipDrive = 1;
    vehicle->gear = 0;
    vehicle->steerAngle = 0.0f;
    vehicle->engineRpm = 0.0f;
    if (vehicle->model != NULL) {
        vehicle->body.kind = RIGID_BODY_BOX_1;
        vehicle->body.size.y = 2.0f;
        vehicle->body.mass = 1000.0f;
        vehicle->body.size.x = 3.5f;
        vehicle->body.size.z = 1.5f;
        InitRigidBody(&vehicle->body);
        vehicle->body.state.position.x = 0.0f;
        vehicle->body.state.position.y = 0.0f;
        vehicle->body.state.position.z = 2.0f;
        vehicle->body.state.velocity.x = 0.0f;
        vehicle->body.state.velocity.y = 0.0f;
        vehicle->body.state.velocity.z = 0.0f;
        vehicle->body.state.orientation.x = 0.0f;
        vehicle->body.state.orientation.y = 0.0f;
        vehicle->body.state.orientation.z = 0.0f;
        vehicle->body.state.angularVelocity.x = 0.0f;
        vehicle->body.state.angularVelocity.y = 0.0f;
        vehicle->body.state.angularVelocity.z = 0.0f;
        vehicle->body.state.orientation.w = 1.0f;
        QuaternionToMatrix(vehicle->body.orientation, &vehicle->body.state);
        vehicle->body.springRate = ((f32)vehicle->suspensionStiffness * 4.0f + 20.0f) * 16000.0f;
        vehicle->body.damping = -3000.0f;

        /* The rear and the front wheel of the model, and those of the other side mirrored. */
        vehicle->wheelBodies[0].suspension = 0;
        vehicle->wheelBodies[0].ground = NULL;
        vehicle->wheelBodies[0].spinRate = 0;
        vehicle->wheelBodies[0].kind = RIGID_BODY_NO_ROTATION;
        vehicle->wheelBodies[0].mass = 0.0f;
        vehicle->wheelBodies[0].size.x = 0.0f;
        vehicle->wheelBodies[0].size.y = 0.0f;
        vehicle->wheelBodies[0].size.z = 0.0f;
        InitRigidBody(&vehicle->wheelBodies[0]);
        INIT_WHEEL_STATE(
            &vehicle->wheelBodies[0], vehicle->model->wheels[2].x, vehicle->model->wheels[2].y);
        vehicle->wheelBodies[2].suspension = 0;
        vehicle->wheelBodies[2].ground = NULL;
        vehicle->wheelBodies[2].spinRate = 0;
        vehicle->wheelBodies[2].kind = RIGID_BODY_NO_ROTATION;
        vehicle->wheelBodies[2].mass = 0.0f;
        vehicle->wheelBodies[2].size.x = 0.0f;
        vehicle->wheelBodies[2].size.y = 0.0f;
        vehicle->wheelBodies[2].size.z = 0.0f;
        InitRigidBody(&vehicle->wheelBodies[2]);
        INIT_WHEEL_STATE(
            &vehicle->wheelBodies[2], vehicle->model->wheels[2].x, -vehicle->model->wheels[2].y);
        vehicle->wheelBodies[1].suspension = 0;
        vehicle->wheelBodies[1].ground = NULL;
        vehicle->wheelBodies[1].spinRate = 0;
        vehicle->wheelBodies[1].kind = RIGID_BODY_NO_ROTATION;
        vehicle->wheelBodies[1].mass = 0.0f;
        vehicle->wheelBodies[1].size.x = 0.0f;
        vehicle->wheelBodies[1].size.y = 0.0f;
        vehicle->wheelBodies[1].size.z = 0.0f;
        InitRigidBody(&vehicle->wheelBodies[1]);
        INIT_WHEEL_STATE(
            &vehicle->wheelBodies[1], vehicle->model->wheels[1].x, vehicle->model->wheels[1].y);
        vehicle->wheelBodies[3].suspension = 0;
        vehicle->wheelBodies[3].ground = NULL;
        vehicle->wheelBodies[3].spinRate = 0;
        vehicle->wheelBodies[3].kind = RIGID_BODY_NO_ROTATION;
        vehicle->wheelBodies[3].mass = 0.0f;
        vehicle->wheelBodies[3].size.x = 0.0f;
        vehicle->wheelBodies[3].size.y = 0.0f;
        vehicle->wheelBodies[3].size.z = 0.0f;
        InitRigidBody(&vehicle->wheelBodies[3]);
        INIT_WHEEL_STATE(
            &vehicle->wheelBodies[3], vehicle->model->wheels[1].x, -vehicle->model->wheels[1].y);
        vehicle->body.wheels[0] = &vehicle->wheelBodies[0];
        vehicle->body.wheels[1] = &vehicle->wheelBodies[2];
        vehicle->body.wheels[2] = &vehicle->wheelBodies[1];
        vehicle->body.wheels[3] = &vehicle->wheelBodies[3];

        /* The springs over the wheels, then gravity. */
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_SPRING_0], -1.5f, -1.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_SPRING_2], -1.5f, 1.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_SPRING_1], 1.5f, -1.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_SPRING_3], 1.5f, 1.0f);
        SetRigidBodyForce(&vehicle->forces[VEHICLE_FORCE_GRAVITY],
                          0.0f,
                          0.0f,
                          -15450.75f,
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        SetRigidBodyForce(&vehicle->forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY],
                          0.0f,
                          0.0f,
                          -(10.5f * 9.81f),
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        SetRigidBodyForce(&vehicle->forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY],
                          0.0f,
                          0.0f,
                          -(10.5f * 9.81f),
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        vehicle->forces[VEHICLE_FORCE_SPRING_0].next = &vehicle->forces[VEHICLE_FORCE_SPRING_2];
        vehicle->forces[VEHICLE_FORCE_SPRING_2].next = &vehicle->forces[VEHICLE_FORCE_SPRING_1];
        vehicle->forces[VEHICLE_FORCE_SPRING_1].next = &vehicle->forces[VEHICLE_FORCE_SPRING_3];
        vehicle->forces[VEHICLE_FORCE_SPRING_3].next = &vehicle->forces[VEHICLE_FORCE_GRAVITY];
        vehicle->forces[VEHICLE_FORCE_GRAVITY].next = NULL;
        vehicle->body.forces = &vehicle->forces[VEHICLE_FORCE_SPRING_0];

        /* The drive of each wheel, then the gravity of its axle. */
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_WHEEL_0], 0.0f, 0.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_WHEEL_2], 0.0f, 0.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_WHEEL_1], 0.0f, 0.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_WHEEL_3], 0.0f, 0.0f);
        vehicle->wheelBodies[0].forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_0];
        vehicle->wheelBodies[2].forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_2];
        vehicle->wheelBodies[1].forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_1];
        vehicle->wheelBodies[3].forces = &vehicle->forces[VEHICLE_FORCE_WHEEL_3];
        vehicle->forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY].next = NULL;
        vehicle->forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY].next = NULL;
        vehicle->forces[VEHICLE_FORCE_WHEEL_0].next =
            &vehicle->forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY];
        vehicle->forces[VEHICLE_FORCE_WHEEL_2].next =
            &vehicle->forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY];
        vehicle->forces[VEHICLE_FORCE_WHEEL_1].next =
            &vehicle->forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY];
        vehicle->forces[VEHICLE_FORCE_WHEEL_3].next =
            &vehicle->forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY];

        /* The dampers over the wheels, then the drag. */
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_DAMPER_0], -1.5f, -1.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_DAMPER_2], -1.5f, 1.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_DAMPER_1], 1.5f, -1.0f);
        INIT_BODY_FORCE(&vehicle->forces[VEHICLE_FORCE_DAMPER_3], 1.5f, 1.0f);
        SetRigidBodyForce(&vehicle->forces[VEHICLE_FORCE_DRAG],
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          0.0f,
                          RIGID_BODY_FORCE_WORLD);
        vehicle->forces[VEHICLE_FORCE_DAMPER_0].next = &vehicle->forces[VEHICLE_FORCE_DAMPER_2];
        vehicle->forces[VEHICLE_FORCE_DAMPER_2].next = &vehicle->forces[VEHICLE_FORCE_DAMPER_1];
        vehicle->forces[VEHICLE_FORCE_DAMPER_1].next = &vehicle->forces[VEHICLE_FORCE_DAMPER_3];
        vehicle->body.tireType = vehicle->tireType;
        vehicle->forces[VEHICLE_FORCE_DRAG].next = NULL;
        vehicle->steerReturn = 0;
        vehicle->frontLoad = 0;
        vehicle->rearLoad = 0;
        vehicle->forces[VEHICLE_FORCE_DAMPER_3].next = &vehicle->forces[VEHICLE_FORCE_DRAG];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        NullStub();
        NullStub();
        NullStub();
        NullStub();
#endif
    }
    vehicle->rearSliding = 0;
    vehicle->frontSliding = 0;
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
    start = (f32)vehicle->index * SPRAY_FRAMES_STEP;
    vehicle->sprayFramesLeft[0] = start;
    vehicle->sprayFramesLeft[1] = start + SPRAY_FRAMES_SPREAD;
    vehicle->sprayFramesLeft[2] = vehicle->sprayFramesLeft[1] + SPRAY_FRAMES_SPREAD;
    vehicle->sprayFramesLeft[3] = vehicle->sprayFramesLeft[2] + SPRAY_FRAMES_SPREAD;
    for (j = 0; j < VEHICLE_WHEELS; j++) {
        vehicle->sprayAdvanced[j] = 0;
        vehicle->spraySurfaces[j] = TRACK_SURFACE_START;
    }
    for (k = 0; k < VEHICLE_WHEELS; k++) {
        vehicle->smokeTimers[k] = (f32)k * SMOKE_TIMER_STEP;
    }
    for (l = 0; l < VEHICLE_WHEELS; l++) {
        vehicle->sprayPhases[l] = 0.0f;
        vehicle->sprayFramesLeft[l] = 0.0f;
    }

    velocity = &vehicle->sprayVelocities[0][0][0][0];
    vertex = &vehicle->sprayVertices[0][0][0];
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

    surface = &vehicle->spraySegmentSurfaces[0][0];
    for (i = 0; i < VEHICLE_WHEELS * SPRAY_SEGMENTS; i++) {
        surface[i] = TRACK_SURFACE_START;
    }
}

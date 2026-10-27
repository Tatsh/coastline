#include "vehiclereset.h"

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

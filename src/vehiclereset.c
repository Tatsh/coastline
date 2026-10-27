#include "vehiclereset.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os_libc.h>
#else
#include <string.h>
#endif

#define DEFAULT_CAMERA_FOV 0.5235987902f

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

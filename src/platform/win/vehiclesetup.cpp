#include <PR/gu.h>

#include "camera.h"
#include "game.h"
#include "trackselect.h"
#include "vehicle.h"
#include "vehiclechoice.h"
#include "vehiclereset.h"

void Vehicle::SetVehicleVelocity(f32 x, f32 y, f32 z) {
    ::SetVehicleVelocity(this, x, y, z);
}

void Vehicle::ResetVehicleState(void) {
    ::ResetVehicleState(this);
}

void Vehicle::InitVehiclePhysics(void) {
    ::InitVehiclePhysics(this);
}

void Vehicle::InitVehicleColorAndPhysics(void) {
    ::InitVehicleColorAndPhysics(this);
}

void Vehicle::SetVehicleModelSlot(s32 slot) {
    ::SetVehicleModelSlot(this, slot);
}

void Vehicle::ClearVehicleDamageState(void) {
    ::ClearVehicleDamageState(this);
}

void Vehicle::ResetVehicleEffects(void) {
    ::ResetVehicleEffects(this);
}

void Vehicle::RespawnVehicle(void) {
    ::RespawnVehicle(this);
}

void Vehicle::UpdateCameraChase(CameraFrame *camera, f32 share) {
    ::UpdateCameraChase(this, camera, share);
}

void Vehicle::UpdateCameraLookAtTarget(void) {
    ::UpdateCameraLookAtTarget(this);
}

void Vehicle::ComputeCameraBasis(CameraFrame *camera) {
    ::ComputeCameraBasis(this, camera);
}

void Vehicle::PlaceTrackCamera(void) {
    ::PlaceTrackCamera(this);
}

void Vehicle::InitCameraForVehicle(void) {
    ::InitCameraForVehicle(this);
}

void Vehicle::UpdateCameraSystem(void) {
    ::UpdateCameraSystem(this);
}

void Vehicle::ResolveVehicleTrackCollision(CameraFrame *camera, Vec3f *target) {
    ::ResolveVehicleTrackCollision(this, camera, target);
}

void Vehicle::SetVehiclePosition(f32 x, f32 y, f32 z) {
    ::SetVehiclePosition(this, x, y, z);
}

void Vehicle::InitVehicleOrientationFromAngle(f32 angle) {
    ::InitVehicleOrientationFromAngle(this, angle);
}

void Vehicle::SetVehicleFrameMatrix(f32 m[4][4]) {
    ::SetVehicleFrameMatrix(this, m);
}

void Vehicle::RotateVehicleByEuler(f32 yaw, f32 pitch, f32 roll) {
    ::RotateVehicleByEuler(this, yaw, pitch, roll);
}

void Vehicle::SetVehicleTurnRate(f32 x, f32 y, f32 z) {
    ::SetVehicleTurnRate(this, x, y, z);
}
void Vehicle::BindPlayerInputToVehicle(void) {
    input = &g_aPlayerInputState[this - g_aVehicles];
    guMtxIdentF(inputMatrix);
}

void Vehicle::SetVehicleCarId(s32 carId) {
    if (carId >= 16) {
        customCarModel = 1;
        carModel = carId - 16;
    } else {
        customCarModel = 0;
        carModel = carId;
    }
}

void Vehicle::ChooseVehicleCar(void) {
    s32 carId;
    VehicleCarChoice choice = ChooseVehicleCarId(this, &carId);

    if (choice == CAR_CHOICE_COPIED) {
        carModel = carId;
    } else if (choice == CAR_CHOICE_SELECTED) {
        SetVehicleCarId(carId);
    }
}

void Vehicle::InitPlayerVehicleSlot(void) {
    BindPlayerInputToVehicle();
    index = this - g_aVehicles;
    if (g_dwGameMode != GAME_MODE_NETWORK) {
        color[0] = g_abPlayerColorsRgb[index][0];
        color[1] = g_abPlayerColorsRgb[index][1];
        color[2] = g_abPlayerColorsRgb[index][2];
    }
    if (index < 2) {
        profile = &g_aSeasonSaveBuffers[index];
        InitPlayerProfileDefaults(profile, g_dwNumTracks);
    } else {
        profile = nullptr;
    }
    ChooseVehicleCar();
    customDecals = 0;
}

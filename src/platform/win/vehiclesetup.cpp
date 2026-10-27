#include <PR/gu.h>

#include "game.h"
#include "trackselect.h"
#include "vehicle.h"
#include "vehiclereset.h"

void Vehicle::SetVehicleVelocity(f32 x, f32 y, f32 z) {
    ::SetVehicleVelocity(this, x, y, z);
}

void Vehicle::ResetVehicleState(void) {
    ::ResetVehicleState(this);
}
#include "vehiclechoice.h"

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

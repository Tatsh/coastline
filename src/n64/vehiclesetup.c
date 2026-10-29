#include <PR/gu.h>

#include "common/camera.h"
#include "common/game.h"
#include "common/input.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/vehiclechoice.h"
#include "common/weather.h"
#include "racemenu.h"

/* The profiles a controller pak keeps, one for each of the first two players. */
#define SAVED_PROFILES 2

void ChooseVehicleCar(Vehicle *vehicle) {
    s32 car;

    if (ChooseVehicleCarId(vehicle, &car) != CAR_CHOICE_UNCHANGED) {
        vehicle->carModel = car;
    }
}

void InitVehicleColor(Vehicle *vehicle) {
    u8 red, green, blue;

    ResetVehicleState(vehicle);
    red = g_abPlayerColorsRgb[vehicle->index][0];
    green = g_abPlayerColorsRgb[vehicle->index][1];
    blue = g_abPlayerColorsRgb[vehicle->index][2];
    vehicle->ghostMode = 0;
    vehicle->otherLook = 0;
    vehicle->driver = NULL;
    vehicle->color[0] = red;
    vehicle->color[1] = green;
    vehicle->color[2] = blue;
}

void InitPlayerVehicleSlot(Vehicle *vehicle) {
    PlayerProfile *profile;

    BindPlayerInputToVehicle(vehicle);
    vehicle->index = vehicle - g_aVehicles;
    if (vehicle->index < SAVED_PROFILES) {
        profile = &g_aSeasonSaveBuffers[vehicle->index];
        vehicle->profile = profile;
        InitPlayerProfileDefaults(profile, g_dwNumTracks);
    } else {
        vehicle->profile = NULL;
    }
    ChooseVehicleCar(vehicle);
    vehicle->customDecals = 0;
}

void UnusedRaceStartStub(s32 unused) {
}

#include <PR/gu.h>

#include "common/camera.h"
#include "common/game.h"
#include "common/input.h"
#include "common/seasons.h"
#include "common/track.h"
#include "common/trackselect.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/weather.h"
#include "racemenu.h"

/* The profiles a controller pak keeps, one for each of the first two players. */
#define SAVED_PROFILES 2

void ChooseVehicleCar(Vehicle *vehicle) {
    s32 car, season;
    s16 unlocks;

    season = g_aVehicles[0].profile->season;
    if (season > 3) {
        season = 3;
    }
    if (vehicle->index >= g_nNumLocalPlayersSigned) {
        if (g_dwGameMode == GAME_MODE_ARCADE) {
            vehicle->carModel = g_aVehicles[0].carModel;
            return;
        }
        unlocks = g_SeasonTable[season].unlocks;
        for (car = 0; car < 16; car++) {
            if (unlocks & 1) {
                break;
            }
            unlocks >>= 1;
        }
        if (car == 16) {
            car = 5;
        }
        if (vehicle->index > g_nNumLocalPlayersSigned) {
            vehicle->carModel = car;
        } else {
            vehicle->carModel = g_SeasonTable[g_aVehicles[0].profile->season]
                                    .cars[vehicle->index - g_nNumLocalPlayersSigned];
        }
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        vehicle->carModel = g_SeasonTable[g_aVehicles[0].profile->season].cars[0];
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
    s32 i;

    BindPlayerInputToVehicle(vehicle);
    vehicle->index = vehicle - g_aVehicles;
    if (vehicle->index < SAVED_PROFILES) {
        vehicle->profile = &g_aSeasonSaveBuffers[vehicle->index];
        vehicle->profile->championshipFlags = 0;
        vehicle->profile->season = 0;
        vehicle->profile->seasonPoints[0] = 0;
        vehicle->profile->round = 0;
        vehicle->profile->unlocks = 0x102;
        vehicle->profile->unlockedTracks = 4;
        vehicle->profile->unlockedWeathers = 1;
        vehicle->profile->transmission = 1;
        vehicle->profile->handling = 0;
        vehicle->profile->suspension = 1;
        vehicle->profile->tires = 2;
        vehicle->profile->controllerLayout = 0;
        for (i = 0; i < g_dwNumTracks; i++) {
            vehicle->profile->lapRecords[i] = 0.0f;
            vehicle->profile->raceRecords[i] = 0.0f;
        }
    } else {
        vehicle->profile = NULL;
    }
    ChooseVehicleCar(vehicle);
    vehicle->customDecals = 0;
}

void UnusedRaceStartStub(s32 unused) {
}

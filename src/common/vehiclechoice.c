#include "common/vehiclechoice.h"

#include "common/game.h"
#include "common/seasons.h"
#include "common/vehicleaccess.h"

VehicleCarChoice ChooseVehicleCarId(Vehicle *vehicle, s32 *carId) {
    s32 season = VEHICLE_GET(&(g_aVehicles[0]), Profile)->season;
    s32 unlockSeason = season;
    s16 unlocks;
    s32 car;

    if (season >= 4) {
        unlockSeason = 3;
    }
    if (VEHICLE_GET(vehicle, Index) >= (s32)g_dwNumLocalPlayers) {
        if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PC
            || g_dwGameMode == GAME_MODE_NETWORK
#endif
        ) {
            *carId = VEHICLE_GET(&(g_aVehicles[0]), CarModel);
            return CAR_CHOICE_COPIED;
        }
        unlocks = g_SeasonTable[unlockSeason].unlocks;
        for (car = 0; car < 16; car++) {
            if (unlocks & 1) {
                break;
            }
            unlocks >>= 1;
        }
        if (car == 16) {
            car = 5;
        }
        if ((s32)g_dwNumLocalPlayers < VEHICLE_GET(vehicle, Index)) {
            *carId = car;
            return CAR_CHOICE_SELECTED;
        }
        car = g_SeasonTable[season].cars[VEHICLE_GET(vehicle, Index) - g_dwNumLocalPlayers];
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        car = g_SeasonTable[season].cars[0];
    } else {
        return CAR_CHOICE_UNCHANGED;
    }
#if TGR_PC
    car = (s8)car;
#endif
    *carId = car;
    return CAR_CHOICE_SELECTED;
}

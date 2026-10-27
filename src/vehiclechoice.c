#include "vehiclechoice.h"

#include "game.h"
#include "seasons.h"

VehicleCarChoice ChooseVehicleCarId(Vehicle *vehicle, s32 *carId) {
    s32 season = g_aVehicles[0].profile->season;
    s32 unlockSeason = season;
    s16 unlocks;
    s32 car;

    if (season >= 4) {
        unlockSeason = 3;
    }
    if (vehicle->index >= (s32)g_dwNumLocalPlayers) {
        if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            || g_dwGameMode == GAME_MODE_NETWORK
#endif
        ) {
            *carId = g_aVehicles[0].carModel;
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
        if ((s32)g_dwNumLocalPlayers < vehicle->index) {
            *carId = car;
            return CAR_CHOICE_SELECTED;
        }
        car = g_SeasonTable[season].cars[vehicle->index - g_dwNumLocalPlayers];
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        car = g_SeasonTable[season].cars[0];
    } else {
        return CAR_CHOICE_UNCHANGED;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    car = (s8)car;
#endif
    *carId = car;
    return CAR_CHOICE_SELECTED;
}

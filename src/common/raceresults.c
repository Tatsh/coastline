#include "common/game.h"
#include "common/profile.h"
#include "common/race.h"
#include "common/vehicleaccess.h"

#if TGR_BOSS_RALLY
#include "common/screen.h"
#else
#include <PR/os.h>
#endif

void SaveRaceResultsToSlots(void) {
    s32 i, j;

    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        g_adwSavedResultPositions[i] = VEHICLE_GET(&(g_aVehicles[i]), RacePosition);
        g_afSavedResultTimes[i] = VEHICLE_GET(&(g_aVehicles[i]), TotalTime);
        g_afSavedResultBestLapTimes[i] = VEHICLE_GET(&(g_aVehicles[i]), BestLapTime);
        g_adwSavedResultBestLaps[i] = VEHICLE_GET(&(g_aVehicles[i]), BestLap);
        g_adwSavedResultLaps[i] = VEHICLE_GET(&(g_aVehicles[i]), Lap);
        for (j = 0; j < g_dwTotalLaps; j++) {
            g_aafSavedResultLapTimes[i][j] = VEHICLE_GET(&(g_aVehicles[i]), LapTimes)[j];
        }
    }
    g_dwSavedResultsValid = TRUE;
}

void RestoreRaceResultsFromSlots(void) {
    PlayerProfile *profile;
    s32 i, j;

    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && g_dwSavedResultsValid) {
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            profile = VEHICLE_GET(&(g_aVehicles[i]), Profile);
#if TGR_BOSS_RALLY
            profile->seasonRacePoints[profile->season][profile->round] =
                (s8)g_abRacePositionPoints[g_adwSavedResultPositions[i]];
#else
            profile->seasonPoints[profile->season] +=
                g_abRacePositionPoints[g_adwSavedResultPositions[i]];
#endif
            profile = VEHICLE_GET(&(g_aVehicles[i]), Profile);
            profile->seasonPlaces[profile->season][profile->round] = g_adwSavedResultPositions[i];
            profile = VEHICLE_GET(&(g_aVehicles[i]), Profile);
            profile->seasonTimes[profile->season][profile->round] = g_afSavedResultTimes[i];
            profile = VEHICLE_GET(&(g_aVehicles[i]), Profile);
#if TGR_BOSS_RALLY
            NullStub();
#else
            osSyncPrintf("points = %d\n", profile->seasonPoints[profile->season]);
#endif
        }
    }
    /* The best lap time is kept but not given back. */
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        for (j = 0; j < g_dwTotalLaps; j++) {
            VEHICLE_GET(&(g_aVehicles[i]), LapTimes)[j] = g_aafSavedResultLapTimes[i][j];
        }
        VEHICLE_SET(&(g_aVehicles[i]), Lap, g_adwSavedResultLaps[i]);
        VEHICLE_SET(&(g_aVehicles[i]), BestLap, g_adwSavedResultBestLaps[i]);
        VEHICLE_SET(&(g_aVehicles[i]), TotalTime, g_afSavedResultTimes[i]);
        VEHICLE_SET(&(g_aVehicles[i]), RacePosition, g_adwSavedResultPositions[i]);
    }
}

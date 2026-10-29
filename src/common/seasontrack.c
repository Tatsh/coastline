#include "common/profile.h"
#include "common/race.h"
#include "common/seasons.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "common/vehicleaccess.h"
#include "common/weather.h"

void SetTrackFromSeason(void) {
#if TGR_BOSS_RALLY
    PlayerProfile *profile = VEHICLE_GET(&(g_aVehicles[0]), Profile);
#else
    PlayerProfile *profile = g_pActiveProfile;
#endif

    g_dwSelectedTrackId = g_SeasonTable[profile->season].races[profile->round][0];
    if (profile->championshipFlags & 1) {
        if ((s32)g_dwSelectedTrackId < TRACK_MIRRORED) {
            g_dwSelectedTrackId += TRACK_MIRRORED;
        } else {
            g_dwSelectedTrackId -= TRACK_MIRRORED;
        }
    }
    g_dwCurrentWeather = g_SeasonTable[profile->season].races[profile->round][1];
}

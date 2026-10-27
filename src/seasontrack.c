#include "profile.h"
#include "race.h"
#include "seasons.h"
#include "trackselect.h"
#include "vehicle.h"
#include "weather.h"

void SetTrackFromSeason(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    PlayerProfile *profile = g_aVehicles[0].profile;
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

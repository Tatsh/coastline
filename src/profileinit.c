#include "profile.h"

void InitPlayerProfileDefaults(PlayerProfile *profile, s32 trackCount) {
    s32 i;

    profile->championshipFlags = 0;
    profile->season = 0;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    profile->seasonPoints[0] = 0;
#endif
    profile->round = 0;
    profile->unlocks = 0x102;
    profile->unlockedTracks = 4;
    profile->unlockedWeathers = 1;
    profile->transmission = 1;
    profile->handling = 0;
    profile->suspension = 1;
    profile->tires = 2;
    profile->controllerLayout = 0;
#if TGR_PC
    /* The Windows initializer overwrites the common controller default. */
    profile->controllerLayout = 5;
#endif
    for (i = 0; i < trackCount; i++) {
        profile->lapRecords[i] = 0.0f;
        profile->raceRecords[i] = 0.0f;
    }
}

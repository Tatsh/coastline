#ifndef COASTLINE_RACER_MOTION_H
#define COASTLINE_RACER_MOTION_H

#include "common/racerslot.h"

#if TGR_BOSS_RALLY
#include "common/racerslotaccess.h"
#include "common/settings.h"
#include "common/soundsystem.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Drive an assigned vehicle or advance an unassigned racer along the route. */
void UpdateRacerSlotMotion(RacerSlot *slot);

#if TGR_BOSS_RALLY
/** Whether network play disables an unassigned racer. */
static inline s32 IsNetworkRacerMotionDisabled(RacerSlot *slot) {
    return g_dwNetworkPlayMode && RACER_SLOT_GET(slot, Vehicle) == NULL;
}
/** Clear frequency and stereo levels for the first five requested sound channels. */
static inline void ClearRacerSoundChannels(void) {
    for (s32 i = 0; i < 5; i++) {
        g_aSoundChannelRequested[i].llFrequency = 0;
        g_aSoundChannelRequested[i].dwStereoLevels = 0;
    }
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACER_MOTION_H */

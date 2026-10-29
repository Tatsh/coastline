#ifndef COASTLINE_RACER_MOTION_H
#define COASTLINE_RACER_MOTION_H

#include "common/racerslot.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Drive an assigned vehicle or advance an unassigned racer along the route. */
void UpdateRacerSlotMotion(RacerSlot *slot);

#if TGR_BOSS_RALLY
/** Whether network play disables an unassigned racer. */
s32 IsNetworkRacerMotionDisabled(RacerSlot *slot);
/** Clear frequency and stereo levels for the first five requested sound channels. */
void ClearRacerSoundChannels(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACER_MOTION_H */

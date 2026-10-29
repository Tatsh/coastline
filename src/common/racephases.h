#ifndef COASTLINE_RACE_PHASES_H
#define COASTLINE_RACE_PHASES_H
#include "common/race.h"

#ifdef __cplusplus
extern "C" {
#endif
#if TGR_BOSS_RALLY
/**
 * Whether the race has started: when the player presses the start binding, at the start time of a
 * network game, and from the outset of the demo.
 *
 * @ghidraAddress BRD3D: 0x1022af14
 * @ghidraAddress BRGlide: 0x10226a44
 */
extern int g_bRaceStartReached;
#else
/** Advance the race phase when network readiness permits it. */
void AdvanceRacePhase(void);
#endif /* TGR_BOSS_RALLY */

/** Update the countdown, racing, results, and exit phases. */
void StepRacePhases(void);

#ifdef __cplusplus
}
#endif
#endif

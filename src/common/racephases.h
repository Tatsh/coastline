#ifndef COASTLINE_RACE_PHASES_H
#define COASTLINE_RACE_PHASES_H
#include "common/race.h"

#ifdef __cplusplus
extern "C" {
#endif
/** Advance the race phase when network readiness permits it. */
void AdvanceRacePhase(void);
/** Update the countdown, racing, results, and exit phases. */
void StepRacePhases(void);

#ifdef __cplusplus
}
#endif
#endif

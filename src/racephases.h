#ifndef COASTLINE_RACE_PHASES_H
#define COASTLINE_RACE_PHASES_H
#include "race.h"
/** Advance the race phase when network readiness permits it. */
void AdvanceRacePhase(void);
/** Update the countdown, racing, results, and exit phases. */
void StepRacePhases(void);
#endif

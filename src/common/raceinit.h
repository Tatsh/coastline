#ifndef COASTLINE_RACE_INIT_H
#define COASTLINE_RACE_INIT_H

/**
 * @file
 * The setup of the vehicles and the input recordings at the start of a race.
 */

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !TGR_BOSS_RALLY
/**
 * Apply the control options from the current input recording.
 *
 * @param vehicle The vehicle whose steering mode, gearbox, tyres, and suspension are read from
 * the header of the recording its input plays back, with the button layout of its input.
 */
void SetupFromRecording(Vehicle *vehicle);
#endif

/** Set up the vehicles, controller recordings, and replays of the current race mode. */
void SetupRaceVehicles(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_INIT_H */

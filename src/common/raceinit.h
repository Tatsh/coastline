#ifndef COASTLINE_RACE_INIT_H
#define COASTLINE_RACE_INIT_H

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !TGR_PC
/** Apply the control options from the current input recording. */
void SetupFromRecording(Vehicle *vehicle);
#endif

/** Set up the vehicles, controller recordings, and replays of the current race mode. */
void SetupRaceVehicles(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_INIT_H */

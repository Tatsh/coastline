#ifndef COASTLINE_RACE_INIT_H
#define COASTLINE_RACE_INIT_H

#include "vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/** Apply the control options from the current input recording. */
void SetupFromRecording(Vehicle *vehicle);
#endif

/** Set up the vehicles, controller recordings, and replays of the current race mode. */
void SetupRaceVehicles(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_INIT_H */

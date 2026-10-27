#ifndef COASTLINE_RACE_INIT_H
#define COASTLINE_RACE_INIT_H

#include "vehicle.h"

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/** Apply the control options from the current input recording. */
void SetupFromRecording(Vehicle *vehicle);
#endif

#endif

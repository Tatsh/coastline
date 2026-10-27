#ifndef COASTLINE_VEHICLERESET_H
#define COASTLINE_VEHICLERESET_H

#include "vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Reset vehicle transforms and apply the selected car's physics. */
void ResetVehicleState(Vehicle *vehicle);
/** Set velocity in the rigid-body integration states and the recorded placement. */
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z);

#ifdef __cplusplus
}
#endif

#endif

#ifndef COASTLINE_VEHICLEPHYSICS_H
#define COASTLINE_VEHICLEPHYSICS_H

#include "vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Advance spring, tyre, damping, and collision calculations for a vehicle. */
void SimulateVehiclePhysicsSubstep(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif

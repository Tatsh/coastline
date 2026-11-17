#ifndef COASTLINE_VEHICLEEFFECTS_H
#define COASTLINE_VEHICLEEFFECTS_H

/**
 * @file
 * The dent sides, the spray segments, and the reset of the wheel effects of a vehicle.
 */

#include "common/vehiclefwd.h"

/** The sides around a car that dent apart, by the direction of the blow. */
#define VEHICLE_DENT_SIDES 8

/** The segments of the spray a wheel throws. */
#define SPRAY_SEGMENTS 9
/** The vertices of each segment of the spray, of which the first two are drawn. */
#define SPRAY_SEGMENT_VERTICES 4

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reset wheel spray and smoke timers.
 *
 * @param vehicle The vehicle whose spray and smoke are reset.
 */
void ResetVehicleEffects(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEEFFECTS_H */

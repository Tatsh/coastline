#ifndef COASTLINE_VEHICLEEFFECTS_H
#define COASTLINE_VEHICLEEFFECTS_H

#include "vehiclefwd.h"

/** The sides around a car that dent apart, by the direction of the blow. */
#define VEHICLE_DENT_SIDES 8

/** The segments of the spray a wheel throws. */
#define SPRAY_SEGMENTS 9
/** The vertices of each segment of the spray, of which the first two are drawn. */
#define SPRAY_SEGMENT_VERTICES 4

#ifdef __cplusplus
extern "C" {
#endif

/** Reset wheel spray and smoke timers. */
void ResetVehicleEffects(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEEFFECTS_H */

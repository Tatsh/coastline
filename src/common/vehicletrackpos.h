#ifndef COASTLINE_VEHICLE_TRACK_POSITION_H
#define COASTLINE_VEHICLE_TRACK_POSITION_H

#include "common/types.h"
#include "common/vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Find the nearest point of the route to a vehicle and add how far it went along the route to its
 * distance. A vehicle that has not crossed out of its point and stays near it keeps it.
 *
 * @param vehicle The vehicle.
 * @return 0 if no point is near enough, 1 otherwise.
 * @ghidraAddress USA: 0x8021eb50
 * @ghidraAddress EUR: 0x8021fe50
 * @ghidraAddress JPN: 0x8021f640
 */
s32 UpdateVehicleTrackPosition(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLE_TRACK_POSITION_H */

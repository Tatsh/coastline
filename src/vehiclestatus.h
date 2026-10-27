#ifndef COASTLINE_VEHICLE_STATUS_H
#define COASTLINE_VEHICLE_STATUS_H

#include "vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run the clocks of a vehicle as the mode of play keeps them.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f0a8
 * @ghidraAddress EUR: 0x802203a8
 * @ghidraAddress JPN: 0x8021fb98
 */
void UpdateRaceTimers(Vehicle *vehicle);

/**
 * Count down the time of the message of a vehicle, or failing one its smaller message, and clear it
 * at the end.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f158
 * @ghidraAddress EUR: 0x80220458
 * @ghidraAddress JPN: 0x8021fc48
 */
void UpdateHudMessageTimers(Vehicle *vehicle);

/**
 * Show "WRONG WAY", flashing, once a vehicle has faced against the track for a while.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f1f0
 * @ghidraAddress EUR: 0x802204f0
 * @ghidraAddress JPN: 0x8021fce0
 */
void CheckWrongWayWarning(Vehicle *vehicle);

/**
 * Find the cell of the grid a vehicle is in, then its place on the route.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8021f2d8
 * @ghidraAddress EUR: 0x802205d8
 * @ghidraAddress JPN: 0x8021fdc8
 */
void UpdateVehicleGridPosition(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLE_STATUS_H */

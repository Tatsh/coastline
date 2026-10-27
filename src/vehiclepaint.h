#ifndef COASTLINE_VEHICLE_PAINT_H
#define COASTLINE_VEHICLE_PAINT_H

#include "vehiclefwd.h"
#include "vehiclemodel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set vehicle paint from the first CI4 paint-palette colour.
 *
 * @ghidraAddress USA: 0x8021d2a0
 * @ghidraAddress EUR: 0x8021e4a0
 * @ghidraAddress JPN: 0x8021dd70
 * @ghidraAddress BRD3D: 0x10035452
 */
void ExtractVehicleBaseColor(Vehicle *vehicle, VehicleModel *model);

#ifdef __cplusplus
}
#endif

#endif

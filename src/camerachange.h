#ifndef COASTLINE_CAMERA_CHANGE_H
#define COASTLINE_CAMERA_CHANGE_H

#include "vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clear the flag of a vehicle that marks the change of its camera.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022bcac
 * @ghidraAddress EUR: 0x8022d1bc
 * @ghidraAddress JPN: 0x8022c974
 */
void ClearCameraChanged(Vehicle *vehicle);

#ifdef __cplusplus
}
#endif

#endif

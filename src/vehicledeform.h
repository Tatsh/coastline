#ifndef COASTLINE_VEHICLE_DEFORM_H
#define COASTLINE_VEHICLE_DEFORM_H

#include "vecmath.h"
#include "vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The accumulated dent at which further mesh deformation stops.
 *
 * @ghidraAddress BRD3D: 0x100ac6a4
 * @ghidraAddress BRGlide: 0x100abe44
 */
extern s16 g_wVehicleDeformMaxDent;
#endif

/**
 * Dent the model on the side of an impact unless the side is already dented deeply enough.
 *
 * @param vehicle The vehicle.
 * @param direction The direction of the impact.
 * @param amount The strength of the impact.
 * @ghidraAddress USA: 0x80233880
 * @ghidraAddress EUR: 0x80234d90
 * @ghidraAddress JPN: 0x80234b90
 */
void DeformVehicleMeshByImpact(Vehicle *vehicle, Vec3f *direction, s16 amount);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLE_DEFORM_H */

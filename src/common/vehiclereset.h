#ifndef COASTLINE_VEHICLERESET_H
#define COASTLINE_VEHICLERESET_H

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reset vehicle transforms and apply the selected car's physics.
 *
 * @param vehicle The vehicle whose model and setup choices are already selected.
 */
void ResetVehicleState(Vehicle *vehicle);
/**
 * Initialise the body, wheels, and force chains for the selected model.
 *
 * @param vehicle The vehicle to initialise; a null model skips body and wheel initialisation.
 */
void InitVehiclePhysics(Vehicle *vehicle);
/**
 * Apply release-specific model paint and initialise physics.
 *
 * @param vehicle The vehicle whose model and colour selections have been populated.
 */
void InitVehicleColorAndPhysics(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Bind a model slot and initialise its colour and physics.
 *
 * @param vehicle The vehicle receiving the selected model.
 * @param slot A valid index in g_aVehicleModels.
 */
void SetVehicleModelSlot(Vehicle *vehicle, s32 slot);
#endif
/**
 * Clear dents and the dent variation.
 *
 * @param vehicle The vehicle whose visual damage is reset.
 */
void ClearVehicleDamageState(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Place the vehicle on the starting grid and initialise race state.
 *
 * @param vehicle The configured vehicle with a loaded model and assigned grid position.
 */
void SetupVehicleForRaceStart(Vehicle *vehicle);
#endif
/**
 * Return an overturned or fallen vehicle to its current route point.
 *
 * @param vehicle The active vehicle with a valid track section and point index.
 */
void RespawnVehicle(Vehicle *vehicle);
/**
 * Set velocity in the rigid-body integration states and the recorded placement.
 *
 * @param vehicle The vehicle whose linear velocity changes.
 * @param x The world x velocity, in world units per second.
 * @param y The world y velocity, in world units per second.
 * @param z The world z velocity, in world units per second.
 */
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z);
/**
 * Set the position in the frame, spray origin, and rigid-body integration states.
 *
 * @param vehicle The vehicle to reposition.
 * @param x The world x coordinate.
 * @param y The world y coordinate.
 * @param z The world z coordinate.
 */
void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z);
/**
 * Initialise the vehicle and integration orientations from a heading in radians.
 *
 * @param vehicle The vehicle whose orientation changes.
 * @param angle The heading in the world XY plane, in radians about the z axis.
 */
void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle);
/**
 * Set the frame matrix and copy its orientation to the integration states.
 *
 * @param vehicle The vehicle whose frame and body orientation change.
 * @param m The readable 4-by-4 frame matrix, copied during this call.
 */
void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]);
/**
 * Apply yaw, pitch, and roll rotations in radians to the integration orientations.
 *
 * @param vehicle The vehicle whose orientation changes.
 * @param yaw The rotation about the local z axis, applied first, in radians.
 * @param pitch The rotation about the local y axis, applied second, in radians.
 * @param roll The rotation about the local x axis, applied last, in radians.
 */
void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll);
/**
 * Set angular velocity in the rigid-body integration states.
 *
 * @param vehicle The vehicle whose angular velocity changes.
 * @param x The x component of angular velocity, in radians per second.
 * @param y The y component of angular velocity, in radians per second.
 * @param z The z component of angular velocity, in radians per second.
 */
void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLERESET_H */

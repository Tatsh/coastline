#ifndef COASTLINE_VEHICLERESET_H
#define COASTLINE_VEHICLERESET_H

#include "vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Reset vehicle transforms and apply the selected car's physics. */
void ResetVehicleState(Vehicle *vehicle);
/** Initialise the body, wheels, and force chains for the selected model. */
void InitVehiclePhysics(Vehicle *vehicle);
/** Paint the selected model and initialise its physics. */
void InitVehicleColorAndPhysics(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_N64
/** Bind a model slot and initialise its colour and physics. */
void SetVehicleModelSlot(Vehicle *vehicle, s32 slot);
#endif
/** Clear dents and the dent variation. */
void ClearVehicleDamageState(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_N64
/** Place the vehicle on the starting grid and initialise race state. */
void SetupVehicleForRaceStart(Vehicle *vehicle);
#endif
/** Return an overturned or fallen vehicle to its current route point. */
void RespawnVehicle(Vehicle *vehicle);
/** Set velocity in the rigid-body integration states and the recorded placement. */
void SetVehicleVelocity(Vehicle *vehicle, f32 x, f32 y, f32 z);
/** Set the position in the frame, spray origin, and rigid-body integration states. */
void SetVehiclePosition(Vehicle *vehicle, f32 x, f32 y, f32 z);
/** Initialise the vehicle and integration orientations from a heading in radians. */
void InitVehicleOrientationFromAngle(Vehicle *vehicle, f32 angle);
/** Set the frame matrix and copy its orientation to the integration states. */
void SetVehicleFrameMatrix(Vehicle *vehicle, f32 (*m)[4]);
/** Apply yaw, pitch, and roll rotations in radians to the integration orientations. */
void RotateVehicleByEuler(Vehicle *vehicle, f32 yaw, f32 pitch, f32 roll);
/** Set angular velocity in the rigid-body integration states. */
void SetVehicleTurnRate(Vehicle *vehicle, f32 x, f32 y, f32 z);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLERESET_H */

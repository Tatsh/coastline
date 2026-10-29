#ifndef COASTLINE_VEHICLEPHYSICS_H
#define COASTLINE_VEHICLEPHYSICS_H

#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Advance the player-controlled vehicle for one frame. */
void UpdatePhysicsStep(Vehicle *vehicle);

/** Advance spring, tyre, damping, and collision calculations for a vehicle. */
void SimulateVehiclePhysicsSubstep(Vehicle *vehicle);

/** Drive or fly a vehicle for a frame, then update its drawing transforms and cameras. */
void UpdateVehiclePhysicsFrame(Vehicle *vehicle);

/** Update steering, engine torque, and gearbox selection. */
void UpdateVehicleEngineAndGearbox(Vehicle *vehicle);

/**
 * Update wheel heights and rebuild their rigid-body orientation matrices.
 *
 * @param vehicle The vehicle to update.
 * @ghidraAddress BRD3D: 0x10061be0
 * @ghidraAddress BRGlide: 0x1005ac60
 */
void UpdateVehicleWheelTransforms(Vehicle *vehicle);

/** Transform the body and wheel matrices for drawing. */
void TransformAllWheelMatrices(Vehicle *vehicle);

#if TGR_BOSS_RALLY
/** Whether the selected binding for a driving control is an axis. */
s32 IsVehicleControlAnalogue(s32 control);

/** Whether speed reduces analogue steering lock. */
s32 IsVehicleSteeringSpeedSensitive(void);

/** Restore replay motion or record the current pose before drawing. */
void UpdateVehicleReplayPose(Vehicle *vehicle);

/** Whether replay and race conditions permit a physics step. */
s32 ShouldStepVehiclePhysics(Vehicle *vehicle);

/** Update drawing transforms or send the local driver's motion. */
void FinishVehiclePhysicsFrame(Vehicle *vehicle);
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEPHYSICS_H */

#ifndef COASTLINE_VEHICLEPHYSICS_H
#define COASTLINE_VEHICLEPHYSICS_H

#include "common/vehicle.h"

#if TGR_BOSS_RALLY
#include "common/game.h"
#include "common/ghost.h"
#include "common/instantreplay.h"
#include "common/racephases.h"
#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/vehicleaccess.h"
#endif

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
static inline s32 IsVehicleControlAnalogue(s32 control) {
    return (g_GlobalRallyConfig.pActiveInputBindings[control].wControl & 0x8000) != 0;
}

/** Whether speed reduces analogue steering lock. */
static inline s32 IsVehicleSteeringSpeedSensitive(void) {
    return g_bSpeedSensitiveSteering;
}

/** Restore replay motion or record the current pose before drawing. */
static inline void UpdateVehicleReplayPose(Vehicle *vehicle) {
    if (gInstantReplayActive ||
        (g_dwGameMode == GAME_MODE_TIME_ATTACK && VEHICLE_GET(vehicle, Index) == 1 &&
         VEHICLE_GET(vehicle, Input)->playbackBuffer)) {
        ApplyGhostKeyframeForVehicle(vehicle);
        UpdateVehicleWheelTransforms(vehicle);
    } else if (g_dwGameMode == GAME_MODE_DEMO && VEHICLE_GET(vehicle, Index) == 0 &&
               VEHICLE_GET(vehicle, Input)->playbackBuffer) {
        ApplyGhostKeyframeToVehicle(vehicle, 1);
        UpdateVehicleWheelTransforms(vehicle);
    } else {
        RecordGhostKeyframe(vehicle);
    }
}

/** Whether replay and race conditions permit a physics step. */
static inline s32 ShouldStepVehiclePhysics(Vehicle *vehicle) {
    if (gInstantReplayActive ||
        (g_dwGameMode == GAME_MODE_TIME_ATTACK && VEHICLE_GET(vehicle, Index) == 1 &&
         VEHICLE_GET(vehicle, Input)->playbackBuffer) ||
        (g_dwGameMode == GAME_MODE_DEMO && VEHICLE_GET(vehicle, Index) == 0 &&
         VEHICLE_GET(vehicle, Input)->playbackBuffer)) {
        return 0;
    }
    return g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
           g_bRaceStartReached || VEHICLE_GET(vehicle, WheelBodies)[1].contactFrames <= 90;
}

/** Update drawing transforms or send the local driver's motion. */
static inline void FinishVehiclePhysicsFrame(Vehicle *vehicle) {
    if (g_dwNetworkPlayMode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (VEHICLE_GET(vehicle, Driver) == PlayerVehicleControllerCallback &&
            !gInstantReplayActive) {
            RecordVehicleKeyframe(vehicle);
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    } else if (!gInstantReplayActive) {
        vehicle->TransformAllWheelMatrices();
    }
}
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEPHYSICS_H */

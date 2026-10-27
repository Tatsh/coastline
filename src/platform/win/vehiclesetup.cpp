#include <PR/gu.h>

#include "boot/rallyconfig.h"
#include "camera.h"
#include "directplay.h"
#include "driving.h"
#include "game.h"
#include "ghost.h"
#include "input.h"
#include "instantreplay.h"
#include "settings.h"
#include "trackselect.h"
#include "vehicle.h"
#include "vehiclechoice.h"
#include "vehiclephysics.h"
#include "vehiclereset.h"

void AiVehicleControllerCallback(Vehicle *vehicle) {
    vehicle->ComputeAiVehicleInput();
}

void Vehicle::SetVehiclePaintColorRgb(u8 red, u8 green, u8 blue) {
    color[0] = red;
    color[1] = green;
    color[2] = blue;
}

void PlayerVehicleControllerCallback(Vehicle *vehicle) {
    vehicle->ApplyPlayerVehicleInput();
}

void Vehicle::ApplyPlayerVehicleInput(void) {
    ::ProcessVehicleInputAndPhysics(this);
}

void ComputeTrackPosition(Vehicle *vehicle) {
    vehicle->ComputeTrackPosition();
}

void ApplyPlayerVehicleEffects(Vehicle *vehicle) {
    if (g_dwInputButtonMask & 0x10000) {
        vehicle->body.unreconstructed202 = 0x80;
    }
    if (g_dwInputButtonMask & 0x20000) {
        vehicle->body.unreconstructed203 = 0x80;
    }
    if (g_dwInputButtonMask & 0x40000) {
        vehicle->body.unreconstructed204 = 0x80;
    }
    if (g_dwInputButtonMask & 0x80000) {
        vehicle->body.replayEffects[0] = 0x80;
    }
    if (g_dwInputButtonMask & 0x80) {
        vehicle->body.replayEffects[1] = 0x80;
    }
}

void ApplyPlayerVehicleRespawnRequest(Vehicle *vehicle) {
    if (g_bRespawnRequested) {
        if (!(vehicle->racer->status & 3) && !gInstantReplayActive) {
            vehicle->body.uprightFrames = -1;
        }
        g_bRespawnRequested = 0;
    }
}

void Vehicle::SimulateVehiclePhysicsSubstep(void) {
    ::SimulateVehiclePhysicsSubstep(this);
}

void Vehicle::UpdateVehiclePhysicsFrame(void) {
    ::UpdateVehiclePhysicsFrame(this);
}

void Vehicle::UpdateVehicleEngineAndGearbox(void) {
    ::UpdateVehicleEngineAndGearbox(this);
}

s32 IsVehicleControlAnalogue(s32 control) {
    return (g_GlobalRallyConfig.pActiveInputBindings[control].wControl & 0x8000) != 0;
}

s32 IsVehicleSteeringSpeedSensitive(void) {
    return g_bSpeedSensitiveSteering;
}

s32 ShouldStepVehiclePhysics(Vehicle *vehicle) {
    if (gInstantReplayActive ||
        (g_dwGameMode == GAME_MODE_TIME_ATTACK && vehicle->index == 1 &&
         vehicle->input->playbackBuffer) ||
        (g_dwGameMode == GAME_MODE_DEMO && vehicle->index == 0 && vehicle->input->playbackBuffer)) {
        return 0;
    }
    return g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
           g_bNetRaceStartReached || vehicle->wheelBodies[1].contactFrames <= 90;
}

void FinishVehiclePhysicsFrame(Vehicle *vehicle) {
    if (g_dwNetworkPlayMode) {
        if (vehicle->driver == PlayerVehicleControllerCallback && !gInstantReplayActive) {
            RecordVehicleKeyframe(vehicle);
        }
    } else if (!gInstantReplayActive) {
        vehicle->TransformAllWheelMatrices();
    }
}

void UpdateVehicleReplayPose(Vehicle *vehicle) {
    if (gInstantReplayActive || (g_dwGameMode == GAME_MODE_TIME_ATTACK && vehicle->index == 1 &&
                                 vehicle->input->playbackBuffer)) {
        ApplyGhostKeyframeForVehicle(vehicle);
        UpdateVehicleWheelTransforms(vehicle);
    } else if (g_dwGameMode == GAME_MODE_DEMO && vehicle->index == 0 &&
               vehicle->input->playbackBuffer) {
        ApplyGhostKeyframeToVehicle(vehicle, 1);
        UpdateVehicleWheelTransforms(vehicle);
    } else {
        RecordGhostKeyframe(vehicle);
    }
}

void Vehicle::TransformAllWheelMatrices(void) {
    ::TransformAllWheelMatrices(this);
}

void Vehicle::SetVehicleVelocity(f32 x, f32 y, f32 z) {
    ::SetVehicleVelocity(this, x, y, z);
}

void Vehicle::ResetVehicleState(void) {
    ::ResetVehicleState(this);
}

void Vehicle::InitVehiclePhysics(void) {
    ::InitVehiclePhysics(this);
}

void Vehicle::InitVehicleColorAndPhysics(void) {
    ::InitVehicleColorAndPhysics(this);
}

void Vehicle::SetVehicleModelSlot(s32 slot) {
    ::SetVehicleModelSlot(this, slot);
}

void Vehicle::ClearVehicleDamageState(void) {
    ::ClearVehicleDamageState(this);
}

void Vehicle::SetupVehicleForRaceStart(void) {
    ::SetupVehicleForRaceStart(this);
}

void Vehicle::ResetVehicleEffects(void) {
    ::ResetVehicleEffects(this);
}

void Vehicle::RespawnVehicle(void) {
    ::RespawnVehicle(this);
}

void Vehicle::UpdateCameraChase(CameraFrame *camera, f32 share) {
    ::UpdateCameraChase(this, camera, share);
}

void Vehicle::UpdateCameraLookAtTarget(void) {
    ::UpdateCameraLookAtTarget(this);
}

void Vehicle::ComputeCameraBasis(CameraFrame *camera) {
    ::ComputeCameraBasis(this, camera);
}

void Vehicle::PlaceTrackCamera(void) {
    ::PlaceTrackCamera(this);
}

void Vehicle::InitCameraForVehicle(void) {
    ::InitCameraForVehicle(this);
}

void Vehicle::UpdateCameraSystem(void) {
    ::UpdateCameraSystem(this);
}

void Vehicle::ResolveVehicleTrackCollision(CameraFrame *camera, Vec3f *target) {
    ::ResolveVehicleTrackCollision(this, camera, target);
}

void Vehicle::SetVehiclePosition(f32 x, f32 y, f32 z) {
    ::SetVehiclePosition(this, x, y, z);
}

void Vehicle::InitVehicleOrientationFromAngle(f32 angle) {
    ::InitVehicleOrientationFromAngle(this, angle);
}

void Vehicle::SetVehicleFrameMatrix(f32 m[4][4]) {
    ::SetVehicleFrameMatrix(this, m);
}

void Vehicle::RotateVehicleByEuler(f32 yaw, f32 pitch, f32 roll) {
    ::RotateVehicleByEuler(this, yaw, pitch, roll);
}

void Vehicle::SetVehicleTurnRate(f32 x, f32 y, f32 z) {
    ::SetVehicleTurnRate(this, x, y, z);
}
void Vehicle::BindPlayerInputToVehicle(void) {
    input = &g_aPlayerInputState[this - g_aVehicles];
    guMtxIdentF(inputMatrix);
}

void Vehicle::SetVehicleCarId(s32 carId) {
    if (carId >= 16) {
        customCarModel = 1;
        carModel = carId - 16;
    } else {
        customCarModel = 0;
        carModel = carId;
    }
}

void Vehicle::ChooseVehicleCar(void) {
    s32 carId;
    VehicleCarChoice choice = ChooseVehicleCarId(this, &carId);

    if (choice == CAR_CHOICE_COPIED) {
        carModel = carId;
    } else if (choice == CAR_CHOICE_SELECTED) {
        SetVehicleCarId(carId);
    }
}

void Vehicle::InitPlayerVehicleSlot(void) {
    BindPlayerInputToVehicle();
    index = this - g_aVehicles;
    if (g_dwGameMode != GAME_MODE_NETWORK) {
        color[0] = g_abPlayerColorsRgb[index][0];
        color[1] = g_abPlayerColorsRgb[index][1];
        color[2] = g_abPlayerColorsRgb[index][2];
    }
    if (index < 2) {
        profile = &g_aSeasonSaveBuffers[index];
        InitPlayerProfileDefaults(profile, g_dwNumTracks);
    } else {
        profile = nullptr;
    }
    ChooseVehicleCar();
    customDecals = 0;
}

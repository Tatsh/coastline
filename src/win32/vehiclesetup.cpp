#include <cstring>

#include <PR/gu.h>

#include "common/camera.h"
#include "common/driving.h"
#include "common/game.h"
#include "common/ghost.h"
#include "common/instantreplay.h"
#include "common/raceprogress.h"
#include "common/racermotion.h"
#include "common/racers.h"
#include "common/racersharing.h"
#include "common/racerslotaccess.h"
#include "common/settings.h"
#include "common/strings.h"
#include "common/trackselect.h"
#include "common/vehicleaccess.h"
#include "common/vehiclechoice.h"
#include "common/vehiclephysics.h"
#include "common/vehiclereset.h"
#include "directplay.h"
#include "input.h"
#include "paintshop.h"
#include "render.h"
#include "sound.h"
#include "vehicle.h"
#include "win32/rallyconfig.h"

void AiVehicleControllerCallback(Vehicle *vehicle) {
    vehicle->ComputeAiVehicleInput();
}

void Vehicle::SetVehiclePaintColorRgb(u8 red, u8 green, u8 blue) {
    color[0] = red;
    color[1] = green;
    color[2] = blue;
}

void Vehicle::ComputeAiVehicleInput(void) {
    ::UpdateAiVehicleBehavior(this);
}

void Vehicle::ManageAiRacerSlots(void) {
    ::ManageAiRacerSlots(this);
}

void RacerSlot::UpdateRacerSlotMotion(void) {
    ::UpdateRacerSlotMotion(this);
}

void RacerSlot::UpdateRaceProgress(void) {
    ::UpdateRaceProgress(this);
}

const char *GetRaceProgressMessage(s32 id) {
    return GetStringResource(id);
}

const char *GetNetworkRaceFinishMessage(void) {
    return g_szEmptyUiString;
}

s32 IsNetworkRacerMotionDisabled(RacerSlot *slot) {
    return g_dwNetworkPlayMode && RACER_SLOT_GET(slot, Vehicle) == nullptr;
}

void ClearRacerSoundChannels(void) {
    for (s32 i = 0; i < 5; i++) {
        g_aSoundChannelRequested[i].llFrequency = 0;
        g_aSoundChannelRequested[i].dwStereoLevels = 0;
    }
}

void PlayerVehicleControllerCallback(Vehicle *vehicle) {
    vehicle->ApplyPlayerVehicleInput();
}

void Vehicle::ApplyPlayerVehicleInput(void) {
    ::ProcessVehicleInputAndPhysics(this);
}

void Vehicle::ComputeTrackPosition(void) {
    ::ComputeTrackPosition(this);
}

s32 Vehicle::TraceTrackSplineRecursive(s32 depth, s32 pointIndex, TrackSection *section) {
    return ::TraceTrackSplineRecursive(this, depth, pointIndex, section);
}

void ApplyPlayerVehicleEffects(Vehicle *vehicle) {
    if (g_dwInputButtonMask & 0x10000) {
        VEHICLE_GET(vehicle, Body).unreconstructed202 = 0x80;
    }
    if (g_dwInputButtonMask & 0x20000) {
        VEHICLE_GET(vehicle, Body).unreconstructed203 = 0x80;
    }
    if (g_dwInputButtonMask & 0x40000) {
        VEHICLE_GET(vehicle, Body).unreconstructed204 = 0x80;
    }
    if (g_dwInputButtonMask & 0x80000) {
        VEHICLE_GET(vehicle, Body).replayEffects[0] = 0x80;
    }
    if (g_dwInputButtonMask & 0x80) {
        VEHICLE_GET(vehicle, Body).replayEffects[1] = 0x80;
    }
}

void ApplyPlayerVehicleRespawnRequest(Vehicle *vehicle) {
    if (g_bRespawnRequested) {
        if (!(RACER_SLOT_GET(VEHICLE_GET(vehicle, Racer), Status) & 3) && !gInstantReplayActive) {
            VEHICLE_GET(vehicle, Body).uprightFrames = -1;
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
        (g_dwGameMode == GAME_MODE_TIME_ATTACK && VEHICLE_GET(vehicle, Index) == 1 &&
         VEHICLE_GET(vehicle, Input)->playbackBuffer) ||
        (g_dwGameMode == GAME_MODE_DEMO && VEHICLE_GET(vehicle, Index) == 0 &&
         VEHICLE_GET(vehicle, Input)->playbackBuffer)) {
        return 0;
    }
    return g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
           g_bNetRaceStartReached || VEHICLE_GET(vehicle, WheelBodies)[1].contactFrames <= 90;
}

void FinishVehiclePhysicsFrame(Vehicle *vehicle) {
    if (g_dwNetworkPlayMode) {
        if (VEHICLE_GET(vehicle, Driver) == PlayerVehicleControllerCallback &&
            !gInstantReplayActive) {
            RecordVehicleKeyframe(vehicle);
        }
    } else if (!gInstantReplayActive) {
        vehicle->TransformAllWheelMatrices();
    }
}

void UpdateVehicleReplayPose(Vehicle *vehicle) {
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

void Vehicle::InstallVehicleModel(s32 slot, s32 car) {
    SetColorTintMultipliers(color[0], color[1], color[2]);
    LoadCar(slot, car, nullptr);
    CopyDamageTextureEntries(damageTextureEntries);
    std::memset(damageTextureLevels, 0, sizeof(damageTextureLevels));
    SetVehicleModelSlot(slot);
}

void Vehicle::ApplyImpactDamageTexture(s32 side) {
    SetColorTintMultipliers(color[0], color[1], color[2]);
    ApplyRandomAdjacentDamageTexture(side, damageTextureEntries, damageTextureLevels);
}

void Vehicle::ClearVehicleDamageState(void) {
    ::ClearVehicleDamageState(this);
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
    guMtxIdentF(sprayMtx);
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

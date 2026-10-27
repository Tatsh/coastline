#pragma once

#include <render.h>

#include "carphysics.h"
#include "convertedvertex.h"
#include "playerinput.h"
#include "profile.h"
#include "racerslot.h"
#include "rigidbody.h"
#include "track.h"
#include "types.h"
#include "vehiclecamera.h"
#include "vehicleeffects.h"
#include "vehicleforces.h"
#include "vehiclefwd.h"
#include "vehiclemodel.h"
#include "wheelframe.h"

#define VEHICLE_GHOST 2
#define MAX_VEHICLES 16
#define VEHICLE_LAP_TIMES 12

#ifdef __cplusplus
/** Recovered fields of a Windows vehicle. */
class Vehicle {
private:
    union {
        CameraFrame frame;
        struct {
            f32 matrix[4][4];
            union {
                f32 wheelMatrices[4][4][4];
                WheelFrame wheelFrames[4];
            };
        };
    };
    s32 index;
    s32 gridIndex;
    u8 abUnknown148[0x1a]; /* +0x148 */
    s16 skidFrames;
    RigidBody body;
    RigidBody wheelBodies[4];
    RigidBodyForce forces[VEHICLE_FORCES];
    u8 abUnknownDa0[0x80]; /* +0xda0 */
    f32 steerAngle;
    f32 engineRpm;
    f32 gearRatios[CAR_GEAR_RATIOS];
    f32 torqueCurve[4];
    f32 finalDrive;
    s32 gearCount;
    s32 rearWheelDrive;
    s32 transmission;
    s32 tableIndex;
    f32 throttle;
    f32 brake;
    s32 gear;
    f32 frontLoad;
    u8 frontSliding;
    u8 abUnknownE79[3]; /* +0xe79 */
    f32 rearLoad;
    u8 rearSliding;
    s8 steerReturn;
    u8 abUnknownE82[2]; /* +0xe82 */
    s32 skipDrive;
    s32 customDecals;
    PlayerProfile *profile;
    s32 tireType;
    s32 suspensionStiffness;
    s32 steeringMode;
    s32 chosenTransmission;
    s32 forcedForwardFrames;
    s32 forcedReverseFrames;
    s32 reverseFrames;
    s32 forwardFrames;
    RacerSlot *nearestRacers[RACER_SLOTS];
    RacerSlot *racer;
    s32 slotCooldown;
    void (*driver)(Vehicle *vehicle);
    Vec3f aiTarget;
    Vec3f trackPoint;
    Vec3f trackForward;
    Vec3f trackSide;
    Vec3f trackUp;
    f32 trackSideDistance;
    Vec3f trackOffset;
    f32 trackSideOffset;
    Vec3f previousListenerPosition;
    s32 effectSound;
    s32 effectVolume;
    s32 effectPriority;
    f32 dopplerFactor;
    s32 cameraChanged;
    s32 flyMode;
    Vec3f previousPosition;
    struct TrackSection *section;
    s32 pointIndex;
    Vec3f trackDirection;
    s32 maxGate;
    s32 gate;
    s32 lap;
    s32 distanceLaps;
    f32 lapTime;
    f32 lapTimes[VEHICLE_LAP_TIMES];
    f32 bestLapTime;
    s32 bestLap;
    f32 totalTime;
    f32 timeLeft;
    f32 distance;
    s32 racePosition;
    const char *message;
    f32 messageTime;
    const char *subMessage;
    f32 subMessageTime;
    char messageText[20];
    s32 groundHits;
    Vec3f placedVelocity;
    f32 speedKph;
    f32 odometer;
    Vec3f joltPosition;
    Vec3f joltVelocity;
    Vec3f joltOffset;
    f32 exhaustTimer;
    Vec3f exhaustPosition;
    f32 smokeTimers[VEHICLE_WHEELS];
    Vec3f smokePositions[VEHICLE_WHEELS];
    f32 sprayFramesLeft[VEHICLE_WHEELS];
    s32 spraySurfaces[VEHICLE_WHEELS];
    f32 sprayPhases[VEHICLE_WHEELS];
    s32 sprayAdvanced[VEHICLE_WHEELS];
    Vec3f sprayPositions[VEHICLE_WHEELS];
    u8 abUnknown111c[4]; /* +0x111c */
    ConvertedModelVertex sprayVertices[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES];
    s16 sprayVelocities[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES][3];
    s16 spraySegmentSurfaces[VEHICLE_WHEELS][SPRAY_SEGMENTS];
    Vec3f sprayOrigin;
    f32 sprayMtx[4][4];
    s32 inTunnel;
    f32 cameraHeading;
    u8 abUnknown271c[4]; /* +0x271c */
    f32 steering;
    f32 pitchInput;
    f32 accelerator;
    f32 rollInput;
    f32 fogFactor;
    CameraFrame *activeCamera;
    CameraFrame *previousCamera;
    CameraFrame cameras[VEHICLE_CAMERAS];
    f32 chaseSpeed;
    f32 chaseLag;
    f32 lookAhead;
    Vec3f lookAt;
    Vec3f chasePosition;
    f32 collisionPull;
    u8 abUnknown28fc[4]; /* +0x28fc */
    Vec3f initialChasePosition;
    u16 groundInstances[RAY_INSTANCES];
    s32 groundInstanceCount;
    u16 nearbyInstances[RAY_INSTANCES];
    s32 nearbyInstanceCount;
    f32 groundDistance;
    s32 groundSurface;
    s16 shadowCorners[2][2];
    u8 abUnknown29a4[4]; /* +0x29a4 */
    s32 carModel;
    u8 color[3];
    u8 ghostMode;
    f32 opacity;
    s32 customCarModel;
    s32 wrongWayFrames;
    u8 gridX;
    u8 gridY;
    u8 abUnknown29be[2]; /* +0x29be */
    PlayerInputState *input;
    VehicleModel *model;
    s16 dents[VEHICLE_DENT_SIDES];
    s16 dentJitter;
    u8 abUnknown29da[2]; /* +0x29da */
    s32 shadowCornerValid[4];
    u8 abUnknown29ec[4]; /* +0x29ec */
    ConvertedModelVertex shadowQuad[4];
    s32 damageTextureEntries[VEHICLE_DENT_SIDES];
    s32 damageTextureLevels[VEHICLE_DENT_SIDES];
    Vec3f previousCollisionPosition;
    char checkpointMessageText[20];
    u8 abUnknown2ad0[0x98]; /* +0x2ad0 */
public:
    /**
     * @ghidraAddress BRD3D: 0x10039020
     */
    void SpawnExhaustParticle();
    /**
     * @ghidraAddress BRD3D: 0x10039f20
     */
    void SpawnTireSmoke();
    /**
     * @ghidraAddress BRD3D: 0x10039200
     */
    void EmitTireSpray();
    void SetExhaustTimer(f32 value) {
        exhaustTimer = value;
    }
    s32 GetWrongWayFrames() const {
        return wrongWayFrames;
    }
    s32 SetWrongWayFrames(s32 value) {
        return wrongWayFrames = value;
    }
    u8 GetGridX() const {
        return gridX;
    }
    u8 SetGridX(u8 value) {
        return gridX = value;
    }
    u8 GetGridY() const {
        return gridY;
    }
    u8 SetGridY(u8 value) {
        return gridY = value;
    }
    f32 GetMessageTime() const {
        return messageTime;
    }
    f32 GetSubMessageTime() const {
        return subMessageTime;
    }
    /** @ghidraAddress BRD3D: 0x10075780 */
    void UpdateRaceTimers();
    /** @ghidraAddress BRD3D: 0x10075810 */
    void UpdateHudMessageTimers();
    /** @ghidraAddress BRD3D: 0x100758a0 */
    void CheckWrongWayWarning();
    /** @ghidraAddress BRD3D: 0x10075960 */
    void UpdateVehicleGridPosition();
    /** @ghidraAddress BRD3D: 0x10076a70 */
    void InstallVehicleModel(s32 slot, s32 car);
    /** @ghidraAddress BRD3D: 0x1000ef80 */
    void DeformVehicleMeshByImpact(Vec3f *direction, s16 amount);
    void ApplyImpactDamageTexture(s32 side);
    s32 AdvanceSkidFrames() {
        return skidFrames++;
    }
    s32 CastGroundRay();
    void DriveRearWheel(RigidBody *body, RigidBody *wheel, f32 dt);
    void DriveFrontWheel(RigidBody *body, RigidBody *wheel, f32 dt);
    void ApplyWheelSideGrip(RigidBody *body, f32 dt);
    f32 GetAccelerator() const {
        return accelerator;
    }
    f32 SetAccelerator(f32 value) {
        return accelerator = value;
    }
    CameraFrame *GetActiveCamera() const {
        return activeCamera;
    }
    CameraFrame *SetActiveCamera(CameraFrame *value) {
        return activeCamera = value;
    }
    auto &GetAiTarget() {
        return aiTarget;
    }
    const auto &GetAiTarget() const {
        return aiTarget;
    }
    s32 SetBestLap(s32 value) {
        return bestLap = value;
    }
    f32 GetBestLapTime() const {
        return bestLapTime;
    }
    f32 SetBestLapTime(f32 value) {
        return bestLapTime = value;
    }
    auto &GetBody() {
        return body;
    }
    const auto &GetBody() const {
        return body;
    }
    f32 GetBrake() const {
        return brake;
    }
    f32 SetBrake(f32 value) {
        return brake = value;
    }
    s32 GetCameraChanged() const {
        return cameraChanged;
    }
    s32 SetCameraChanged(s32 value) {
        return cameraChanged = value;
    }
    f32 GetCameraHeading() const {
        return cameraHeading;
    }
    f32 SetCameraHeading(f32 value) {
        return cameraHeading = value;
    }
    auto &GetCameras() {
        return cameras;
    }
    const auto &GetCameras() const {
        return cameras;
    }
    s32 GetCarModel() const {
        return carModel;
    }
    f32 GetChaseLag() const {
        return chaseLag;
    }
    f32 SetChaseLag(f32 value) {
        return chaseLag = value;
    }
    auto &GetChasePosition() {
        return chasePosition;
    }
    const auto &GetChasePosition() const {
        return chasePosition;
    }
    f32 GetChaseSpeed() const {
        return chaseSpeed;
    }
    f32 SetChaseSpeed(f32 value) {
        return chaseSpeed = value;
    }
    auto &GetCheckpointMessageText() {
        return checkpointMessageText;
    }
    const auto &GetCheckpointMessageText() const {
        return checkpointMessageText;
    }
    s32 GetChosenTransmission() const {
        return chosenTransmission;
    }
    f32 GetCollisionPull() const {
        return collisionPull;
    }
    f32 SetCollisionPull(f32 value) {
        return collisionPull = value;
    }
    auto &GetColor() {
        return color;
    }
    const auto &GetColor() const {
        return color;
    }
    s32 GetCustomCarModel() const {
        return customCarModel;
    }
    s16 GetDentJitter() const {
        return dentJitter;
    }
    s16 SetDentJitter(s16 value) {
        return dentJitter = value;
    }
    auto &GetDents() {
        return dents;
    }
    const auto &GetDents() const {
        return dents;
    }
    f32 GetDistance() const {
        return distance;
    }
    f32 SetDistance(f32 value) {
        return distance = value;
    }
    s32 GetDistanceLaps() const {
        return distanceLaps;
    }
    s32 SetDistanceLaps(s32 value) {
        return distanceLaps = value;
    }
    f32 GetDopplerFactor() const {
        return dopplerFactor;
    }
    f32 SetDopplerFactor(f32 value) {
        return dopplerFactor = value;
    }
    auto GetDriver() const {
        return driver;
    }
    s32 GetEffectPriority() const {
        return effectPriority;
    }
    s32 SetEffectPriority(s32 value) {
        return effectPriority = value;
    }
    s32 GetEffectSound() const {
        return effectSound;
    }
    s32 SetEffectSound(s32 value) {
        return effectSound = value;
    }
    s32 GetEffectVolume() const {
        return effectVolume;
    }
    s32 SetEffectVolume(s32 value) {
        return effectVolume = value;
    }
    f32 GetEngineRpm() const {
        return engineRpm;
    }
    f32 SetEngineRpm(f32 value) {
        return engineRpm = value;
    }
    f32 GetFinalDrive() const {
        return finalDrive;
    }
    f32 SetFinalDrive(f32 value) {
        return finalDrive = value;
    }
    s32 GetFlyMode() const {
        return flyMode;
    }
    f32 GetFogFactor() const {
        return fogFactor;
    }
    f32 SetFogFactor(f32 value) {
        return fogFactor = value;
    }
    s32 GetForcedForwardFrames() const {
        return forcedForwardFrames;
    }
    s32 SetForcedForwardFrames(s32 value) {
        return forcedForwardFrames = value;
    }
    s32 GetForcedReverseFrames() const {
        return forcedReverseFrames;
    }
    s32 SetForcedReverseFrames(s32 value) {
        return forcedReverseFrames = value;
    }
    auto &GetForces() {
        return forces;
    }
    const auto &GetForces() const {
        return forces;
    }
    s32 GetForwardFrames() const {
        return forwardFrames;
    }
    s32 SetForwardFrames(s32 value) {
        return forwardFrames = value;
    }
    auto &GetFrame() {
        return frame;
    }
    const auto &GetFrame() const {
        return frame;
    }
    f32 SetFrontLoad(f32 value) {
        return frontLoad = value;
    }
    u8 SetFrontSliding(u8 value) {
        return frontSliding = value;
    }
    s32 GetGate() const {
        return gate;
    }
    s32 SetGate(s32 value) {
        return gate = value;
    }
    s32 GetGear() const {
        return gear;
    }
    s32 SetGear(s32 value) {
        return gear = value;
    }
    s32 GetGearCount() const {
        return gearCount;
    }
    s32 SetGearCount(s32 value) {
        return gearCount = value;
    }
    auto &GetGearRatios() {
        return gearRatios;
    }
    const auto &GetGearRatios() const {
        return gearRatios;
    }
    u8 GetGhostMode() const {
        return ghostMode;
    }
    u8 SetGhostMode(u8 value) {
        return ghostMode = value;
    }
    s32 GetGridIndex() const {
        return gridIndex;
    }
    s32 SetGroundHits(s32 value) {
        return groundHits = value;
    }
    s32 GetGroundInstanceCount() const {
        return groundInstanceCount;
    }
    s32 SetGroundInstanceCount(s32 value) {
        return groundInstanceCount = value;
    }
    const auto &GetGroundInstances() const {
        return groundInstances;
    }
    s32 GetInTunnel() const {
        return inTunnel;
    }
    s32 SetInTunnel(s32 value) {
        return inTunnel = value;
    }
    s32 GetIndex() const {
        return index;
    }
    s32 SetIndex(s32 value) {
        return index = value;
    }
    auto &GetInitialChasePosition() {
        return initialChasePosition;
    }
    const auto &GetInitialChasePosition() const {
        return initialChasePosition;
    }
    PlayerInputState *GetInput() const {
        return input;
    }
    auto &GetJoltOffset() {
        return joltOffset;
    }
    const auto &GetJoltOffset() const {
        return joltOffset;
    }
    auto &GetJoltPosition() {
        return joltPosition;
    }
    const auto &GetJoltPosition() const {
        return joltPosition;
    }
    auto &GetJoltVelocity() {
        return joltVelocity;
    }
    const auto &GetJoltVelocity() const {
        return joltVelocity;
    }
    s32 GetLap() const {
        return lap;
    }
    s32 SetLap(s32 value) {
        return lap = value;
    }
    f32 GetLapTime() const {
        return lapTime;
    }
    f32 SetLapTime(f32 value) {
        return lapTime = value;
    }
    auto &GetLapTimes() {
        return lapTimes;
    }
    const auto &GetLapTimes() const {
        return lapTimes;
    }
    f32 GetLookAhead() const {
        return lookAhead;
    }
    f32 SetLookAhead(f32 value) {
        return lookAhead = value;
    }
    auto &GetLookAt() {
        return lookAt;
    }
    const auto &GetLookAt() const {
        return lookAt;
    }
    auto &GetMatrix() {
        return matrix;
    }
    const auto &GetMatrix() const {
        return matrix;
    }
    s32 GetMaxGate() const {
        return maxGate;
    }
    s32 SetMaxGate(s32 value) {
        return maxGate = value;
    }
    const char *GetMessage() const {
        return message;
    }
    const char *SetMessage(const char *value) {
        return message = value;
    }
    auto &GetMessageText() {
        return messageText;
    }
    const auto &GetMessageText() const {
        return messageText;
    }
    f32 SetMessageTime(f32 value) {
        return messageTime = value;
    }
    VehicleModel *GetModel() const {
        return model;
    }
    VehicleModel *SetModel(VehicleModel *value) {
        return model = value;
    }
    s32 SetNearbyInstanceCount(s32 value) {
        return nearbyInstanceCount = value;
    }
    auto &GetNearestRacers() {
        return nearestRacers;
    }
    const auto &GetNearestRacers() const {
        return nearestRacers;
    }
    f32 GetOdometer() const {
        return odometer;
    }
    f32 SetOdometer(f32 value) {
        return odometer = value;
    }
    f32 GetOpacity() const {
        return opacity;
    }
    f32 SetOpacity(f32 value) {
        return opacity = value;
    }
    f32 GetPitchInput() const {
        return pitchInput;
    }
    f32 SetPitchInput(f32 value) {
        return pitchInput = value;
    }
    auto &GetPlacedVelocity() {
        return placedVelocity;
    }
    const auto &GetPlacedVelocity() const {
        return placedVelocity;
    }
    s32 GetPointIndex() const {
        return pointIndex;
    }
    s32 SetPointIndex(s32 value) {
        return pointIndex = value;
    }
    CameraFrame *SetPreviousCamera(CameraFrame *value) {
        return previousCamera = value;
    }
    auto &GetPreviousCollisionPosition() {
        return previousCollisionPosition;
    }
    const auto &GetPreviousCollisionPosition() const {
        return previousCollisionPosition;
    }
    auto &GetPreviousListenerPosition() {
        return previousListenerPosition;
    }
    const auto &GetPreviousListenerPosition() const {
        return previousListenerPosition;
    }
    auto &GetPreviousPosition() {
        return previousPosition;
    }
    const auto &GetPreviousPosition() const {
        return previousPosition;
    }
    PlayerProfile *GetProfile() const {
        return profile;
    }
    PlayerProfile *SetProfile(PlayerProfile *value) {
        return profile = value;
    }
    s32 GetRacePosition() const {
        return racePosition;
    }
    s32 SetRacePosition(s32 value) {
        return racePosition = value;
    }
    RacerSlot *GetRacer() const {
        return racer;
    }
    RacerSlot *SetRacer(RacerSlot *value) {
        return racer = value;
    }
    f32 SetRearLoad(f32 value) {
        return rearLoad = value;
    }
    u8 SetRearSliding(u8 value) {
        return rearSliding = value;
    }
    s32 SetRearWheelDrive(s32 value) {
        return rearWheelDrive = value;
    }
    s32 GetReverseFrames() const {
        return reverseFrames;
    }
    s32 SetReverseFrames(s32 value) {
        return reverseFrames = value;
    }
    f32 GetRollInput() const {
        return rollInput;
    }
    f32 SetRollInput(f32 value) {
        return rollInput = value;
    }
    struct TrackSection *GetSection() const {
        return section;
    }
    struct TrackSection *SetSection(struct TrackSection *value) {
        return section = value;
    }
    auto &GetShadowCornerValid() {
        return shadowCornerValid;
    }
    const auto &GetShadowCornerValid() const {
        return shadowCornerValid;
    }
    const auto &GetShadowCorners() const {
        return shadowCorners;
    }
    auto &GetShadowQuad() {
        return shadowQuad;
    }
    const auto &GetShadowQuad() const {
        return shadowQuad;
    }
    s16 GetSkidFrames() const {
        return skidFrames;
    }
    s16 SetSkidFrames(s16 value) {
        return skidFrames = value;
    }
    s32 GetSkipDrive() const {
        return skipDrive;
    }
    s32 SetSkipDrive(s32 value) {
        return skipDrive = value;
    }
    s32 GetSlotCooldown() const {
        return slotCooldown;
    }
    s32 SetSlotCooldown(s32 value) {
        return slotCooldown = value;
    }
    auto &GetSmokeTimers() {
        return smokeTimers;
    }
    const auto &GetSmokeTimers() const {
        return smokeTimers;
    }
    f32 GetSpeedKph() const {
        return speedKph;
    }
    f32 SetSpeedKph(f32 value) {
        return speedKph = value;
    }
    auto &GetSprayAdvanced() {
        return sprayAdvanced;
    }
    const auto &GetSprayAdvanced() const {
        return sprayAdvanced;
    }
    auto &GetSprayFramesLeft() {
        return sprayFramesLeft;
    }
    const auto &GetSprayFramesLeft() const {
        return sprayFramesLeft;
    }
    auto &GetSprayOrigin() {
        return sprayOrigin;
    }
    const auto &GetSprayOrigin() const {
        return sprayOrigin;
    }
    auto &GetSprayPhases() {
        return sprayPhases;
    }
    const auto &GetSprayPhases() const {
        return sprayPhases;
    }
    auto &GetSpraySegmentSurfaces() {
        return spraySegmentSurfaces;
    }
    const auto &GetSpraySegmentSurfaces() const {
        return spraySegmentSurfaces;
    }
    auto &GetSpraySurfaces() {
        return spraySurfaces;
    }
    const auto &GetSpraySurfaces() const {
        return spraySurfaces;
    }
    auto &GetSprayVelocities() {
        return sprayVelocities;
    }
    const auto &GetSprayVelocities() const {
        return sprayVelocities;
    }
    auto &GetSprayVertices() {
        return sprayVertices;
    }
    const auto &GetSprayVertices() const {
        return sprayVertices;
    }
    f32 GetSteerAngle() const {
        return steerAngle;
    }
    f32 SetSteerAngle(f32 value) {
        return steerAngle = value;
    }
    s8 GetSteerReturn() const {
        return steerReturn;
    }
    s8 SetSteerReturn(s8 value) {
        return steerReturn = value;
    }
    f32 GetSteering() const {
        return steering;
    }
    f32 SetSteering(f32 value) {
        return steering = value;
    }
    s32 GetSteeringMode() const {
        return steeringMode;
    }
    const char *SetSubMessage(const char *value) {
        return subMessage = value;
    }
    f32 SetSubMessageTime(f32 value) {
        return subMessageTime = value;
    }
    s32 GetSuspensionStiffness() const {
        return suspensionStiffness;
    }
    s32 GetTableIndex() const {
        return tableIndex;
    }
    s32 SetTableIndex(s32 value) {
        return tableIndex = value;
    }
    f32 GetThrottle() const {
        return throttle;
    }
    f32 SetThrottle(f32 value) {
        return throttle = value;
    }
    f32 GetTimeLeft() const {
        return timeLeft;
    }
    f32 SetTimeLeft(f32 value) {
        return timeLeft = value;
    }
    s32 GetTireType() const {
        return tireType;
    }
    auto &GetTorqueCurve() {
        return torqueCurve;
    }
    const auto &GetTorqueCurve() const {
        return torqueCurve;
    }
    f32 GetTotalTime() const {
        return totalTime;
    }
    f32 SetTotalTime(f32 value) {
        return totalTime = value;
    }
    auto &GetTrackDirection() {
        return trackDirection;
    }
    const auto &GetTrackDirection() const {
        return trackDirection;
    }
    auto &GetTrackForward() {
        return trackForward;
    }
    const auto &GetTrackForward() const {
        return trackForward;
    }
    auto &GetTrackOffset() {
        return trackOffset;
    }
    const auto &GetTrackOffset() const {
        return trackOffset;
    }
    auto &GetTrackPoint() {
        return trackPoint;
    }
    const auto &GetTrackPoint() const {
        return trackPoint;
    }
    auto &GetTrackSide() {
        return trackSide;
    }
    const auto &GetTrackSide() const {
        return trackSide;
    }
    f32 GetTrackSideDistance() const {
        return trackSideDistance;
    }
    f32 SetTrackSideDistance(f32 value) {
        return trackSideDistance = value;
    }
    f32 GetTrackSideOffset() const {
        return trackSideOffset;
    }
    f32 SetTrackSideOffset(f32 value) {
        return trackSideOffset = value;
    }
    auto &GetTrackUp() {
        return trackUp;
    }
    const auto &GetTrackUp() const {
        return trackUp;
    }
    s32 GetTransmission() const {
        return transmission;
    }
    s32 SetTransmission(s32 value) {
        return transmission = value;
    }
    auto &GetWheelBodies() {
        return wheelBodies;
    }
    const auto &GetWheelBodies() const {
        return wheelBodies;
    }
    const auto &GetWheelFrames() const {
        return wheelFrames;
    }
    auto &GetWheelMatrices() {
        return wheelMatrices;
    }
    const auto &GetWheelMatrices() const {
        return wheelMatrices;
    }

    /** @ghidraAddress BRD3D: 0x10063840 */
    void ApplyPlayerVehicleInput(void);

private:
    /**
     * @ghidraAddress BRD3D: 0x10075360
     */
    s32 UpdateVehicleTrackPosition();
    /**
     * @ghidraAddress BRD3D: 0x10064350
     */
    void ComputeTrackPosition(void);

public:
    /** @ghidraAddress BRD3D: 0x10075f10 */
    void UpdateVehiclePhysicsFrame(void);

private:
    /**
     * @ghidraAddress BRD3D: 0x10061f70
     */
    void UpdateVehicleEngineAndGearbox(void);

public:
    /**
     * Store the vehicle's three paint colour bytes.
     *
     * @ghidraAddress BRD3D: 0x10035ca0
     * @ghidraAddress BRGlide: 0x1002f340
     */
    void SetVehiclePaintColorRgb(u8 red, u8 green, u8 blue);

private:
    /**
     * Advance spring, tyre, damping, and collision calculations.
     *
     * @ghidraAddress BRD3D: 0x10061720
     */
    void SimulateVehiclePhysicsSubstep(void);

public:
    /**
     * Transform the body and wheel matrices for drawing.
     *
     * @ghidraAddress BRD3D: 0x10061c60
     */
    void TransformAllWheelMatrices(void);

    /**
     * Drive the vehicle with its computer controller.
     *
     * @ghidraAddress BRD3D: 0x10064700
     */
    void ComputeAiVehicleInput(void);

private:
    /**
     * Assign computer vehicles to racers near this player.
     *
     * @ghidraAddress BRD3D: 0x10066650
     */
    void ManageAiRacerSlots(void);

    /**
     * Update engine, event, skid, and rolling sounds.
     *
     * @ghidraAddress BRD3D: 0x10068400
     */
    void UpdateVehicleSounds(void);

    /** @ghidraAddress BRD3D: 0x10063ff0 */
    s32 TraceTrackSplineRecursive(s32 depth, s32 pointIndex, TrackSection *section);

public:
    /**
     * Initialise the player slot, profile defaults, and vehicle selection.
     *
     * @ghidraAddress BRD3D: 0x100634f0
     */
    void InitPlayerVehicleSlot(void);

private:
    /**
     * @ghidraAddress BRD3D: 0x10076c90
     */
    void BindPlayerInputToVehicle(void);
    /**
     * @ghidraAddress BRD3D: 0x10063420
     */
    void ChooseVehicleCar(void);
    /**
     * @ghidraAddress BRD3D: 0x10076ae0
     */
    void SetVehicleCarId(s32 carId);
    /**
     * @ghidraAddress BRD3D: 0x100767a0
     */
    void SetVehicleVelocity(f32 x, f32 y, f32 z);
    /**
     * @ghidraAddress BRD3D: 0x10076b20
     */
    void ResetVehicleState(void);
    /**
     * @ghidraAddress BRD3D: 0x10062c50
     */
    void InitVehiclePhysics(void);
    /**
     * @ghidraAddress BRD3D: 0x10076a00
     */
    void InitVehicleColorAndPhysics(void);
    /**
     * @ghidraAddress BRD3D: 0x10076a40
     */
    void SetVehicleModelSlot(s32 slot);
    /**
     * @ghidraAddress BRD3D: 0x10065710
     */
    void ClearVehicleDamageState(void);
    /**
     * @ghidraAddress BRD3D: 0x10065740
     */
    void SetupVehicleForRaceStart(void);
    /**
     * @ghidraAddress BRD3D: 0x10065630
     */
    void ResetVehicleEffects(void);
    /**
     * @ghidraAddress BRD3D: 0x10063660
     */
    void RespawnVehicle(void);
    /**
     * @ghidraAddress BRD3D: 0x100015d0
     */
    void UpdateCameraChase(CameraFrame *camera, f32 share);
    /**
     * @ghidraAddress BRD3D: 0x10001760
     */
    void UpdateCameraLookAtTarget(void);
    /**
     * @ghidraAddress BRD3D: 0x10001890
     */
    void ComputeCameraBasis(CameraFrame *camera);
    /**
     * @ghidraAddress BRD3D: 0x10001970
     */
    void PlaceTrackCamera(void);
    /**
     * @ghidraAddress BRD3D: 0x10001ff0
     */
    void InitCameraForVehicle(void);

public:
    /**
     * @ghidraAddress BRD3D: 0x100019d0
     */
    void UpdateCameraSystem(void);

private:
    /**
     * @ghidraAddress BRD3D: 0x100683c0
     */
    void ClearCameraChanged(void);
    /**
     * @ghidraAddress BRD3D: 0x100011f0
     */
    void ResolveVehicleTrackCollision(CameraFrame *camera, Vec3f *target);
    /**
     * @ghidraAddress BRD3D: 0x10076420
     */
    void SetVehiclePosition(f32 x, f32 y, f32 z);
    /**
     * @ghidraAddress BRD3D: 0x100764c0
     */
    void InitVehicleOrientationFromAngle(f32 angle);
    /**
     * @ghidraAddress BRD3D: 0x10076700
     */
    void SetVehicleFrameMatrix(f32 m[4][4]);
    /**
     * @ghidraAddress BRD3D: 0x10076820
     */
    void RotateVehicleByEuler(f32 yaw, f32 pitch, f32 roll);
    /**
     * @ghidraAddress BRD3D: 0x100769a0
     */
    void SetVehicleTurnRate(f32 x, f32 y, f32 z);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Dispatch the computer controller through its vehicle method.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress BRD3D: 0x10065620
 */
void AiVehicleControllerCallback(Vehicle *vehicle);

/**
 * Refresh displayed speed when the second wheel has contact, then advance vehicle physics.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress BRD3D: 0x1002c430
 * @ghidraAddress BRGlide: 0x100199a0
 */
void UpdateVehicleSpeedAndPhysics(Vehicle *vehicle);

/**
 * @ghidraAddress BRD3D: 0x10063fe0
 */
void PlayerVehicleControllerCallback(Vehicle *vehicle);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * Vehicles of the Windows race.
 *
 * @ghidraAddress BRD3D: 0x10acdea8
 */
extern Vehicle g_aVehicles[MAX_VEHICLES];
#endif

/**
 * @ghidraAddress BRD3D: 0x100b37d0
 */
extern u8 g_abPlayerColorsRgb[][3];

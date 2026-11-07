#pragma once

#include <common/render.h>

#include "common/carphysics.h"
#include "common/convertedvertex.h"
#include "common/playerinput.h"
#include "common/profile.h"
#include "common/racerslot.h"
#include "common/rigidbody.h"
#include "common/track.h"
#include "common/types.h"
#include "common/vehiclecamera.h"
#include "common/vehicleeffects.h"
#include "common/vehicleforces.h"
#include "common/vehiclefwd.h"
#include "common/vehiclemodel.h"
#include "common/wheelframe.h"

/** The translucent, non-colliding mode used by ghost vehicles and respawn fades. */
#define VEHICLE_GHOST 2
/** The capacity of the race engine's vehicle array. */
#define MAX_VEHICLES 16
/** The number of completed lap times stored per vehicle. */
#define VEHICLE_LAP_TIMES 12

#ifdef __cplusplus
/**
 * Recovered fields of a Windows vehicle.
 * Direct setters store values without validation; non-void setters return the stored value.
 * Array and vector references alias this object's storage until destruction or replacement.
 * Pointer setters borrow their arguments without transferring or releasing resources.
 */
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
    char playerName[0x1a];
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
    f32 rearLoad;
    u8 rearSliding;
    s8 steerReturn;
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
    s32 selectedCar;
    s32 carModel;
    u8 color[3];
    u8 ghostMode;
    f32 opacity;
    s32 customCarModel;
    s32 wrongWayFrames;
    u8 gridX;
    u8 gridY;
    PlayerInputState *input;
    VehicleModel *model;
    s16 dents[VEHICLE_DENT_SIDES];
    s16 dentJitter;
    s32 shadowCornerValid[4];
    u8 abUnknown29ec[4]; /* +0x29ec */
    ConvertedModelVertex shadowQuad[4];
    s32 damageTextureEntries[VEHICLE_DENT_SIDES];
    s32 damageTextureLevels[VEHICLE_DENT_SIDES];
    Vec3f previousCollisionPosition;
    char checkpointMessageText[20];
    u8 abUnknown2ad0[0x98]; /* +0x2ad0 */
    void ColorSpraySegment(s32 wheel);
    void AgeSpraySegment(s32 wheel, s32 age);

    /**
     * @ghidraAddress BRD3D: 0x10075360
     * @ghidraAddress BRGlide: 0x1006e5c0
     */
    s32 UpdateVehicleTrackPosition();
    /**
     * @ghidraAddress BRD3D: 0x10064350
     * @ghidraAddress BRGlide: 0x1005d3c0
     */
    void ComputeTrackPosition(void);

    /**
     * @ghidraAddress BRD3D: 0x10061f70
     * @ghidraAddress BRGlide: 0x1005aff0
     */
    void UpdateVehicleEngineAndGearbox(void);

    /**
     * Advance spring, tyre, damping, and collision calculations.
     *
     * @ghidraAddress BRD3D: 0x10061720
     * @ghidraAddress BRGlide: 0x1005a7a0
     */
    void SimulateVehiclePhysicsSubstep(void);

    /**
     * @ghidraAddress BRD3D: 0x10063ff0
     * @ghidraAddress BRGlide: 0x1005d060
     */
    s32 TraceTrackSplineRecursive(s32 depth, s32 pointIndex, TrackSection *section);

    /**
     * @ghidraAddress BRD3D: 0x10076c90
     * @ghidraAddress BRGlide: 0x1006ff00
     */
    void BindPlayerInputToVehicle(void);
    /**
     * @ghidraAddress BRD3D: 0x10063420
     * @ghidraAddress BRGlide: 0x1005c490
     */
    void ChooseVehicleCar(void);

    /**
     * @ghidraAddress BRD3D: 0x100767a0
     * @ghidraAddress BRGlide: 0x1006fa10
     */
    void SetVehicleVelocity(f32 x, f32 y, f32 z);
    /**
     * @ghidraAddress BRD3D: 0x10076b20
     * @ghidraAddress BRGlide: 0x1006fd90
     */
    void ResetVehicleState(void);
    /**
     * @ghidraAddress BRD3D: 0x10062c50
     * @ghidraAddress BRGlide: 0x1005bcc0
     */
    void InitVehiclePhysics(void);
    /**
     * @ghidraAddress BRD3D: 0x10076a00
     * @ghidraAddress BRGlide: 0x1006fc70
     */
    void InitVehicleColorAndPhysics(void);
    /**
     * @ghidraAddress BRD3D: 0x10076a40
     * @ghidraAddress BRGlide: 0x1006fcb0
     */
    void SetVehicleModelSlot(s32 slot);
    /**
     * @ghidraAddress BRD3D: 0x10065710
     * @ghidraAddress BRGlide: 0x1005e780
     */
    void ClearVehicleDamageState(void);
    /**
     * @ghidraAddress BRD3D: 0x10065740
     * @ghidraAddress BRGlide: 0x1005e7b0
     */
    void SetupVehicleForRaceStart(void);
    /**
     * @ghidraAddress BRD3D: 0x10065630
     * @ghidraAddress BRGlide: 0x1005e6a0
     */
    void ResetVehicleEffects(void);
    /**
     * @ghidraAddress BRD3D: 0x10063660
     * @ghidraAddress BRGlide: 0x1005c6d0
     */
    void RespawnVehicle(void);
    /**
     * @ghidraAddress BRD3D: 0x100015d0
     * @ghidraAddress BRGlide: 0x100018f0
     */
    void UpdateCameraChase(CameraFrame *camera, f32 share);
    /**
     * @ghidraAddress BRD3D: 0x10001760
     * @ghidraAddress BRGlide: 0x10001a80
     */
    void UpdateCameraLookAtTarget(void);
    /**
     * @ghidraAddress BRD3D: 0x10001890
     * @ghidraAddress BRGlide: 0x10001bb0
     */
    void ComputeCameraBasis(CameraFrame *camera);
    /**
     * @ghidraAddress BRD3D: 0x10001970
     * @ghidraAddress BRGlide: 0x10001c90
     */
    void PlaceTrackCamera(void);
    /**
     * @ghidraAddress BRD3D: 0x10001ff0
     * @ghidraAddress BRGlide: 0x10002310
     */
    void InitCameraForVehicle(void);

    /**
     * @ghidraAddress BRD3D: 0x100011f0
     * @ghidraAddress BRGlide: 0x10001510
     */
    void ResolveVehicleTrackCollision(CameraFrame *camera, Vec3f *target);
    /**
     * @ghidraAddress BRD3D: 0x10076420
     * @ghidraAddress BRGlide: 0x1006f680
     */
    void SetVehiclePosition(f32 x, f32 y, f32 z);
    /**
     * @ghidraAddress BRD3D: 0x100764c0
     * @ghidraAddress BRGlide: 0x1006f720
     */
    void InitVehicleOrientationFromAngle(f32 angle);
    /**
     * @ghidraAddress BRD3D: 0x10076700
     * @ghidraAddress BRGlide: 0x1006f970
     */
    void SetVehicleFrameMatrix(f32 m[4][4]);
    /**
     * @ghidraAddress BRD3D: 0x10076820
     * @ghidraAddress BRGlide: 0x1006fa90
     */
    void RotateVehicleByEuler(f32 yaw, f32 pitch, f32 roll);
    /**
     * @ghidraAddress BRD3D: 0x100769a0
     * @ghidraAddress BRGlide: 0x1006fc10
     */
    void SetVehicleTurnRate(f32 x, f32 y, f32 z);

public:
    /**
     * Clear the camera-change flag after a frame consumes a camera transition.
     *
     * @ghidraAddress BRD3D: 0x100683c0
     * @ghidraAddress BRGlide: 0x10061430
     */
    void ClearCameraChanged(void);

    /**
     * Assign computer vehicles to racers near this player.
     *
     * @ghidraAddress BRD3D: 0x10066650
     * @ghidraAddress BRGlide: 0x1005f6c0
     */
    void ManageAiRacerSlots(void);

    /**
     * Update engine, event, skid, and rolling sounds.
     *
     * @ghidraAddress BRD3D: 0x10068400
     * @ghidraAddress BRGlide: 0x10061470
     */
    void UpdateVehicleSounds(void);

    /** Apply the control options from the current input recording. */
    void SetupFromRecording(void);
    /**
     * Configure race input from the active player's profile or computer driver.
     *
     * @param localPlayer Whether the vehicle uses local player controls.
     */
    inline void ConfigureRaceControls(bool localPlayer);
    /**
     * Select a race driver and ghost mode, install the model, and prepare the starting grid.
     *
     * @param vehicleIndex The slot in g_aVehicles, from 0 through MAX_VEHICLES - 1.
     */
    inline void PrepareRaceVehicle(s32 vehicleIndex);
    /** Initialise a camera-only vehicle for a race view without an active racer. */
    inline void PrepareEmptyRaceView(void);

    /**
     * Select custom decal usage without changing the currently loaded textures.
     * @param value Whether to use custom decal textures for this vehicle.
     */
    void SetCustomDecals(s32 value) {
        customDecals = value;
    }
    /**
     * Assign the custom-model flag without loading a model.
     * @param value Whether the car uses custom model and texture handling.
     */
    void SetCustomCarModel(s32 value) {
        customCarModel = value;
    }
    /**
     * Assign the selected car without loading its model.
     * @param value The selected car index used during model setup.
     */
    void SetSelectedCar(s32 value) {
        selectedCar = value;
    }

    /**
     * Copy all vehicle storage and remap recognised internal camera pointers into this vehicle.
     * Other non-null camera pointers become null; other borrowed pointers remain unchanged.
     *
     * @param from The vehicle whose captured pose, effects, and camera data are copied.
     */
    void CopySnapshotState(const Vehicle &from);
    /**
     * Blend chassis poses, wheel positions, and camera poses between race simulation steps.
     *
     * @param from The earlier captured vehicle.
     * @param to The later captured vehicle.
     * @param share The interpolation fraction, from 0 for @p from through 1 for @p to.
     */
    void InterpolateSnapshotState(const Vehicle &from, const Vehicle &to, long double share);
    /**
     * Start last-lap input recording in spare vehicle-model storage and snapshot the race.
     * @ghidraAddress BRD3D: 0x100679c0
     * @ghidraAddress BRGlide: 0x10060a30
     */
    void SaveLastLapSnapshot(void);
    /**
     * Mark every nearby track instance without clearing existing mask bits.
     *
     * @param mask Writable bytes indexed by all stored nearby instance indices.
     * @param bit The bits ORed into each referenced instance's mask byte.
     */
    void MarkNearbyInstanceShadows(u8 *mask, u8 bit) const {
        for (s32 index = 0; index < nearbyInstanceCount; ++index) {
            mask[nearbyInstances[index]] |= bit;
        }
    }
    /**
     * Project the ground beneath the car into screen bounds and clip to the current view.
     * @param viewports Viewport rectangles indexed by g_dwCurrentSplitScreenIndex.
     */
    inline void UpdateShadowScreenRect(const ViewportRect *viewports);
    /**
     * Age and move existing tyre spray while recentring its relative-coordinate origin.
     * @param step The elapsed step in fixed-point seconds with SPRAY_STEP_SHIFT fraction bits.
     * @param fall The gravitational velocity decrement in SPRAY_UNITS for this step.
     */
    inline void MoveTireSpraySegments(s32 step, s16 fall);
    /**
     * Emit exhaust particles when weather, speed, and the emission timer permit.
     *
     * @ghidraAddress BRD3D: 0x10039020
     * @ghidraAddress BRGlide: 0x100326a0
     */
    void SpawnExhaustParticle();
    /**
     * Emit wheel smoke using tyre contact, sliding state, and the surface beneath each wheel.
     *
     * @ghidraAddress BRD3D: 0x10039f20
     * @ghidraAddress BRGlide: 0x100335a0
     */
    void SpawnTireSmoke();
    /**
     * Extend tyre spray from wheel motion and the current surface conditions.
     *
     * @ghidraAddress BRD3D: 0x10039200
     * @ghidraAddress BRGlide: 0x10032880
     */
    void EmitTireSpray();
    /**
     * Assign the exhaust emission timer without emitting a particle.
     * @param value The exhaust emission timer consumed by the particle update.
     */
    void SetExhaustTimer(f32 value) {
        exhaustTimer = value;
    }
    /** @return The accumulated simulation frames travelling against the route direction. */
    s32 GetWrongWayFrames() const {
        return wrongWayFrames;
    }
    /**
     * @param value The replacement wrong-way frame count.
     * @return The stored count.
     */
    s32 SetWrongWayFrames(s32 value) {
        return wrongWayFrames = value;
    }
    /** @return The current column in the track's collision grid. */
    u8 GetGridX() const {
        return gridX;
    }
    /**
     * @param value The track collision-grid column.
     * @return The stored column.
     */
    u8 SetGridX(u8 value) {
        return gridX = value;
    }
    /** @return The current row in the track's collision grid. */
    u8 GetGridY() const {
        return gridY;
    }
    /**
     * @param value The track collision-grid row.
     * @return The stored row.
     */
    u8 SetGridY(u8 value) {
        return gridY = value;
    }
    /** @return The remaining primary HUD-message display time, in seconds. */
    f32 GetMessageTime() const {
        return messageTime;
    }
    /** @return The remaining secondary HUD-message display time, in seconds. */
    f32 GetSubMessageTime() const {
        return subMessageTime;
    }
    /**
     * Advance lap and total timers while the racer's clock runs; practice advances only lap time.
     * Reduce the remaining time in arcade and network modes.
     *
     * @ghidraAddress BRD3D: 0x10075780
     * @ghidraAddress BRGlide: 0x1006e9e0
     */
    void UpdateRaceTimers();
    /**
     * Reduce the primary HUD timer, or the secondary timer when the primary timer is inactive.
     * Clear each message when its timer expires.
     *
     * @ghidraAddress BRD3D: 0x10075810
     * @ghidraAddress BRGlide: 0x1006ea70
     */
    void UpdateHudMessageTimers();
    /**
     * Count frames travelling against the route and display the wrong-way warning when required.
     *
     * @ghidraAddress BRD3D: 0x100758a0
     * @ghidraAddress BRGlide: 0x1006eb00
     */
    void CheckWrongWayWarning();
    /**
     * Convert the vehicle's world position into clamped track collision-grid coordinates.
     *
     * @ghidraAddress BRD3D: 0x10075960
     * @ghidraAddress BRGlide: 0x1006ebc0
     */
    void UpdateVehicleGridPosition();
    /**
     * Load a car into a model slot, prepare damage textures, and initialise its colour and physics.
     *
     * @param slot A valid slot in g_aVehicleModels.
     * @param car The car identifier used by LoadCar().
     * @ghidraAddress BRD3D: 0x10076a70
     * @ghidraAddress BRGlide: 0x1006fce0
     */
    void InstallVehicleModel(s32 slot, s32 car);
    /**
     * Dent the model on the impacted side unless its existing dent already exceeds the impact.
     *
     * @param direction The impact direction used to select and displace the damaged side.
     * @param amount The signed impact strength in the dent calculation's units.
     * @ghidraAddress BRD3D: 0x1000ef80
     * @ghidraAddress BRGlide: 0x1000c4e0
     */
    void DeformVehicleMeshByImpact(Vec3f *direction, s16 amount);
    /**
     * Apply paint tint and advance a random damage texture adjacent to the impacted side.
     * @param side The damage-region index, from 0 through VEHICLE_DENT_SIDES - 1.
     */
    inline void ApplyImpactDamageTexture(s32 side);
    /** @return The previous skid counter, incrementing the stored 16-bit counter afterwards. */
    s32 AdvanceSkidFrames() {
        return skidFrames++;
    }
    /**
     * Cast vertically through the chassis, replacing chassis height and up vector with track data.
     * Refresh ground and nearby instance candidates; use ceiling data when ground is absent.
     * @return The number of ground triangles found by CastRayAgainstTrack().
     */
    inline s32 CastGroundRay();
    /**
     * Apply a rear wheel's longitudinal force using the cached rear load and sliding flag.
     * @param body The chassis receiving the driving force.
     * @param wheel The rear wheel whose contact and drive are processed.
     * @param dt The physics step duration, in seconds.
     */
    inline void DriveRearWheel(RigidBody *body, RigidBody *wheel, f32 dt);
    /**
     * Apply a front wheel's longitudinal force using the cached front load and sliding flag.
     * @param body The chassis receiving the driving force.
     * @param wheel The front wheel whose contact and drive are processed.
     * @param dt The physics step duration, in seconds.
     */
    inline void DriveFrontWheel(RigidBody *body, RigidBody *wheel, f32 dt);
    /**
     * Apply lateral tyre friction, updating both axle loads and sliding flags.
     * @param body The chassis receiving the friction force.
     * @param dt The physics step duration, in seconds.
     */
    inline void ApplyWheelSideGrip(RigidBody *body, f32 dt);
    /** @return The accelerator input before engine and transmission processing. */
    f32 GetAccelerator() const {
        return accelerator;
    }
    /**
     * @param value The accelerator input before engine and transmission processing.
     * @return The stored input.
     */
    f32 SetAccelerator(f32 value) {
        return accelerator = value;
    }
    /** @return The borrowed active race camera, or null before camera initialisation. */
    CameraFrame *GetActiveCamera() const {
        return activeCamera;
    }
    /**
     * @param value The active camera, valid while race rendering uses it.
     * @return The stored camera pointer.
     */
    CameraFrame *SetActiveCamera(CameraFrame *value) {
        return activeCamera = value;
    }
    /** @return The mutable world-space target of the computer driver. */
    auto &GetAiTarget() {
        return aiTarget;
    }
    /** @return The world-space target of the computer driver. */
    const auto &GetAiTarget() const {
        return aiTarget;
    }
    /** @return The recorded best-lap index. */
    s32 GetBestLap() const {
        return bestLap;
    }
    /**
     * @param value The best-lap index.
     * @return The stored index.
     */
    s32 SetBestLap(s32 value) {
        return bestLap = value;
    }
    /** @return The fastest completed lap time, in seconds. */
    f32 GetBestLapTime() const {
        return bestLapTime;
    }
    /**
     * @param value The fastest completed lap time, in seconds.
     * @return The stored time.
     */
    f32 SetBestLapTime(f32 value) {
        return bestLapTime = value;
    }
    /** @return The mutable chassis rigid body, including integration and collision state. */
    auto &GetBody() {
        return body;
    }
    /** @return The chassis rigid body, including integration and collision state. */
    const auto &GetBody() const {
        return body;
    }
    /** @return The applied braking control used by wheel physics. */
    f32 GetBrake() const {
        return brake;
    }
    /**
     * @param value The applied braking control used by wheel physics.
     * @return The stored control.
     */
    f32 SetBrake(f32 value) {
        return brake = value;
    }
    /** @return Whether a camera change requires a discontinuous view transition. */
    s32 GetCameraChanged() const {
        return cameraChanged;
    }
    /**
     * @param value Whether a camera change requires a discontinuous view transition.
     * @return The stored flag.
     */
    s32 SetCameraChanged(s32 value) {
        return cameraChanged = value;
    }
    /** @return The camera's tracked heading in radians. */
    f32 GetCameraHeading() const {
        return cameraHeading;
    }
    /**
     * @param value The camera's tracked heading in radians.
     * @return The stored heading.
     */
    f32 SetCameraHeading(f32 value) {
        return cameraHeading = value;
    }
    /** @return Mutable storage for VEHICLE_CAMERAS camera frames. */
    auto &GetCameras() {
        return cameras;
    }
    /** @return The VEHICLE_CAMERAS camera frames. */
    const auto &GetCameras() const {
        return cameras;
    }
    /** @return The car-model identifier selected for loading. */
    s32 GetCarModel() const {
        return carModel;
    }
    /** @return The chase-camera lag term derived from smoothed linear speed. */
    f32 GetChaseLag() const {
        return chaseLag;
    }
    /**
     * @param value The chase-camera lag term derived from linear speed.
     * @return The stored lag term.
     */
    f32 SetChaseLag(f32 value) {
        return chaseLag = value;
    }
    /** @return The mutable chase-camera world position. */
    auto &GetChasePosition() {
        return chasePosition;
    }
    /** @return The chase-camera world position. */
    const auto &GetChasePosition() const {
        return chasePosition;
    }
    /** @return The chase-camera response term derived from angular speed. */
    f32 GetChaseSpeed() const {
        return chaseSpeed;
    }
    /**
     * @param value The chase-camera response term derived from angular speed.
     * @return The stored response term.
     */
    f32 SetChaseSpeed(f32 value) {
        return chaseSpeed = value;
    }
    /** @return The mutable 20-byte checkpoint HUD-message buffer. */
    auto &GetCheckpointMessageText() {
        return checkpointMessageText;
    }
    /** @return The 20-byte checkpoint HUD-message buffer. */
    const auto &GetCheckpointMessageText() const {
        return checkpointMessageText;
    }
    /** @return The player's selected transmission mode before temporary driving overrides. */
    s32 GetChosenTransmission() const {
        return chosenTransmission;
    }
    /** @return The chase camera's collision correction factor. */
    f32 GetCollisionPull() const {
        return collisionPull;
    }
    /**
     * @param value The chase camera's collision correction factor.
     * @return The stored factor.
     */
    f32 SetCollisionPull(f32 value) {
        return collisionPull = value;
    }
    /** @return The mutable three-byte paint colour in RGB order. */
    auto &GetColor() {
        return color;
    }
    /** @return The three-byte paint colour in RGB order. */
    const auto &GetColor() const {
        return color;
    }
    /** @return Whether the car uses custom model and texture handling. */
    s32 GetCustomCarModel() const {
        return customCarModel;
    }
    /** @return The damage texture level for each of VEHICLE_DENT_SIDES regions. */
    const auto &GetDamageTextureLevels() const {
        return damageTextureLevels;
    }
    /** @return The current variation applied to visible body dents. */
    s16 GetDentJitter() const {
        return dentJitter;
    }
    /**
     * @param value The variation applied to visible body dents.
     * @return The stored variation.
     */
    s16 SetDentJitter(s16 value) {
        return dentJitter = value;
    }
    /** @return Mutable dent amounts for VEHICLE_DENT_SIDES body regions. */
    auto &GetDents() {
        return dents;
    }
    /** @return Dent amounts for VEHICLE_DENT_SIDES body regions. */
    const auto &GetDents() const {
        return dents;
    }
    /** @return The route-distance value used when ranking the vehicle. */
    f32 GetDistance() const {
        return distance;
    }
    /**
     * @param value The route-distance value used for race ranking.
     * @return The stored distance.
     */
    f32 SetDistance(f32 value) {
        return distance = value;
    }
    /** @return The completed-lap count contributing to race-distance ranking. */
    s32 GetDistanceLaps() const {
        return distanceLaps;
    }
    /**
     * @param value The completed-lap count contributing to race-distance ranking.
     * @return The stored count.
     */
    s32 SetDistanceLaps(s32 value) {
        return distanceLaps = value;
    }
    /** @return The sound frequency multiplier computed for relative listener motion. */
    f32 GetDopplerFactor() const {
        return dopplerFactor;
    }
    /**
     * @param value The sound frequency multiplier for relative listener motion.
     * @return The stored multiplier.
     */
    f32 SetDopplerFactor(f32 value) {
        return dopplerFactor = value;
    }
    /** @return The driving callback, or null when driving is disabled. */
    auto GetDriver() const {
        return driver;
    }
    /** Disable the driving callback without changing vehicle input or velocity. */
    void ClearDriver() {
        driver = nullptr;
    }
    /**
     * Select the callback invoked to drive this vehicle during race steps.
     * @param value The callback receiving this vehicle, or null to disable driving.
     */
    void SetDriver(void (*value)(Vehicle *vehicle)) {
        driver = value;
    }
    /** @return The priority of the selected positional sound effect. */
    s32 GetEffectPriority() const {
        return effectPriority;
    }
    /**
     * @param value The selected positional sound effect's priority.
     * @return The stored priority.
     */
    s32 SetEffectPriority(s32 value) {
        return effectPriority = value;
    }
    /** @return The selected positional sound effect's sample identifier. */
    s32 GetEffectSound() const {
        return effectSound;
    }
    /**
     * @param value The selected positional sound effect's sample identifier.
     * @return The stored identifier.
     */
    s32 SetEffectSound(s32 value) {
        return effectSound = value;
    }
    /** @return The selected positional sound effect's computed volume. */
    s32 GetEffectVolume() const {
        return effectVolume;
    }
    /**
     * @param value The selected positional sound effect's computed volume.
     * @return The stored volume.
     */
    s32 SetEffectVolume(s32 value) {
        return effectVolume = value;
    }
    /** @return The engine speed, in revolutions per minute. */
    f32 GetEngineRpm() const {
        return engineRpm;
    }
    /**
     * @param value The engine speed, in revolutions per minute.
     * @return The stored speed.
     */
    f32 SetEngineRpm(f32 value) {
        return engineRpm = value;
    }
    /** @return The final drive ratio applied after the selected gear ratio. */
    f32 GetFinalDrive() const {
        return finalDrive;
    }
    /**
     * @param value The final drive ratio.
     * @return The stored ratio.
     */
    f32 SetFinalDrive(f32 value) {
        return finalDrive = value;
    }
    /** @return Whether the vehicle uses flying controls. */
    s32 GetFlyMode() const {
        return flyMode;
    }
    /** @return The normalised fog amount, normally from 0 for clear through 1 for full fog. */
    f32 GetFogFactor() const {
        return fogFactor;
    }
    /**
     * @param value The normalised fog amount, stored without clamping.
     * @return The stored factor.
     */
    f32 SetFogFactor(f32 value) {
        return fogFactor = value;
    }
    /** @return The remaining simulation frames forcing forward transmission input. */
    s32 GetForcedForwardFrames() const {
        return forcedForwardFrames;
    }
    /**
     * @param value The remaining forced-forward simulation frames.
     * @return The stored count.
     */
    s32 SetForcedForwardFrames(s32 value) {
        return forcedForwardFrames = value;
    }
    /** @return The remaining simulation frames forcing reverse transmission input. */
    s32 GetForcedReverseFrames() const {
        return forcedReverseFrames;
    }
    /**
     * @param value The remaining forced-reverse simulation frames.
     * @return The stored count.
     */
    s32 SetForcedReverseFrames(s32 value) {
        return forcedReverseFrames = value;
    }
    /** @return The mutable VEHICLE_FORCES rigid-body force records. */
    auto &GetForces() {
        return forces;
    }
    /** @return The VEHICLE_FORCES rigid-body force records. */
    const auto &GetForces() const {
        return forces;
    }
    /** @return The simulation-frame count of forward transmission input. */
    s32 GetForwardFrames() const {
        return forwardFrames;
    }
    /**
     * @param value The forward transmission input frame count.
     * @return The stored count.
     */
    s32 SetForwardFrames(s32 value) {
        return forwardFrames = value;
    }
    /** @return The mutable chassis frame in world space. */
    auto &GetFrame() {
        return frame;
    }
    /** @return The chassis frame in world space. */
    const auto &GetFrame() const {
        return frame;
    }
    /**
     * @param value The cached front-axle load used by wheel friction.
     * @return The stored load.
     */
    f32 SetFrontLoad(f32 value) {
        return frontLoad = value;
    }
    /**
     * @param value Whether front-wheel friction indicates sliding.
     * @return The stored flag.
     */
    u8 SetFrontSliding(u8 value) {
        return frontSliding = value;
    }
    /** @return The most recently crossed checkpoint gate index. */
    s32 GetGate() const {
        return gate;
    }
    /**
     * @param value The most recently crossed checkpoint gate index.
     * @return The stored index.
     */
    s32 SetGate(s32 value) {
        return gate = value;
    }
    /** @return The active gear index, with zero for neutral. */
    s32 GetGear() const {
        return gear;
    }
    /**
     * @param value The active gear index, with zero for neutral.
     * @return The stored index.
     */
    s32 SetGear(s32 value) {
        return gear = value;
    }
    /** @return The number of forward gears in the selected model's transmission. */
    s32 GetGearCount() const {
        return gearCount;
    }
    /**
     * @param value The number of forward gears.
     * @return The stored count.
     */
    s32 SetGearCount(s32 value) {
        return gearCount = value;
    }
    /** @return Mutable CAR_GEAR_RATIOS ratios, with neutral at index zero. */
    auto &GetGearRatios() {
        return gearRatios;
    }
    /** @return The CAR_GEAR_RATIOS ratios, with neutral at index zero. */
    const auto &GetGearRatios() const {
        return gearRatios;
    }
    /** @return The ghost mode, with VEHICLE_GHOST used for time attack and respawn fades. */
    u8 GetGhostMode() const {
        return ghostMode;
    }
    /**
     * @param value The ghost mode, with VEHICLE_GHOST used for time attack and respawn fades.
     * @return The stored mode.
     */
    u8 SetGhostMode(u8 value) {
        return ghostMode = value;
    }
    /** @return The starting-grid slot used by network race setup. */
    s32 GetGridIndex() const {
        return gridIndex;
    }
    /**
     * @param value The starting-grid slot used by network race setup.
     * @return The stored index.
     */
    s32 SetGridIndex(s32 value) {
        return gridIndex = value;
    }
    /** @return The mutable 26-byte player display-name buffer. */
    char *GetPlayerName() {
        return playerName;
    }
    /** @return The 26-byte player display-name buffer. */
    const char *GetPlayerName() const {
        return playerName;
    }
    /**
     * @param value The cached ground-contact count.
     * @return The stored count.
     */
    s32 SetGroundHits(s32 value) {
        return groundHits = value;
    }
    /** @return The populated entries of the ground-collision instance array. */
    s32 GetGroundInstanceCount() const {
        return groundInstanceCount;
    }
    /**
     * @param value The populated ground-instance count, from 0 through RAY_INSTANCES.
     * @return The stored count.
     */
    s32 SetGroundInstanceCount(s32 value) {
        return groundInstanceCount = value;
    }
    /**
     * @return The RAY_INSTANCES collision-candidate indices, used up to GetGroundInstanceCount().
     */
    const auto &GetGroundInstances() const {
        return groundInstances;
    }
    /** @return The RAY_INSTANCES nearby track-instance indices, used up to nearbyInstanceCount. */
    const auto &GetNearbyInstances() const {
        return nearbyInstances;
    }
    /** @return Whether the vehicle occupies a tunnel section. */
    s32 GetInTunnel() const {
        return inTunnel;
    }
    /**
     * @param value Whether the vehicle occupies a tunnel section.
     * @return The stored flag.
     */
    s32 SetInTunnel(s32 value) {
        return inTunnel = value;
    }
    /** @return The vehicle's slot in g_aVehicles. */
    s32 GetIndex() const {
        return index;
    }
    /**
     * @param value The slot in g_aVehicles, from 0 through MAX_VEHICLES - 1.
     * @return The stored index.
     */
    s32 SetIndex(s32 value) {
        return index = value;
    }
    /** @return The mutable initial world position of the chase camera. */
    auto &GetInitialChasePosition() {
        return initialChasePosition;
    }
    /** @return The initial world position of the chase camera. */
    const auto &GetInitialChasePosition() const {
        return initialChasePosition;
    }
    /** @return The borrowed player input record, or null before input binding. */
    PlayerInputState *GetInput() const {
        return input;
    }
    /** @return The mutable current camera-jolt displacement. */
    auto &GetJoltOffset() {
        return joltOffset;
    }
    /** @return The current camera-jolt displacement. */
    const auto &GetJoltOffset() const {
        return joltOffset;
    }
    /** @return The mutable position of the camera-jolt spring simulation. */
    auto &GetJoltPosition() {
        return joltPosition;
    }
    /** @return The position of the camera-jolt spring simulation. */
    const auto &GetJoltPosition() const {
        return joltPosition;
    }
    /** @return The mutable velocity of the camera-jolt spring simulation. */
    auto &GetJoltVelocity() {
        return joltVelocity;
    }
    /** @return The velocity of the camera-jolt spring simulation. */
    const auto &GetJoltVelocity() const {
        return joltVelocity;
    }
    /** @return The current lap index used by checkpoint processing. */
    s32 GetLap() const {
        return lap;
    }
    /**
     * @param value The current lap index.
     * @return The stored index.
     */
    s32 SetLap(s32 value) {
        return lap = value;
    }
    /** @return The current lap's elapsed time, in seconds. */
    f32 GetLapTime() const {
        return lapTime;
    }
    /**
     * @param value The current lap's elapsed time, in seconds.
     * @return The stored time.
     */
    f32 SetLapTime(f32 value) {
        return lapTime = value;
    }
    /** @return Mutable storage for VEHICLE_LAP_TIMES completed lap durations, in seconds. */
    auto &GetLapTimes() {
        return lapTimes;
    }
    /** @return The VEHICLE_LAP_TIMES completed lap durations, in seconds. */
    const auto &GetLapTimes() const {
        return lapTimes;
    }
    /** @return The chase camera's forward target offset in world units. */
    f32 GetLookAhead() const {
        return lookAhead;
    }
    /**
     * @param value The chase camera's forward target offset in world units.
     * @return The stored offset.
     */
    f32 SetLookAhead(f32 value) {
        return lookAhead = value;
    }
    /** @return The mutable world-space point the camera tracks. */
    auto &GetLookAt() {
        return lookAt;
    }
    /** @return The world-space point the camera tracks. */
    const auto &GetLookAt() const {
        return lookAt;
    }
    /** @return The mutable chassis frame matrix, aliased with GetFrame(). */
    auto &GetMatrix() {
        return matrix;
    }
    /** @return The chassis frame matrix, aliased with GetFrame(). */
    const auto &GetMatrix() const {
        return matrix;
    }
    /** @return The furthest checkpoint gate contributing to race progress. */
    s32 GetMaxGate() const {
        return maxGate;
    }
    /**
     * @param value The furthest checkpoint gate contributing to progress.
     * @return The stored gate index.
     */
    s32 SetMaxGate(s32 value) {
        return maxGate = value;
    }
    /** @return The borrowed primary HUD message, or null when no message is selected. */
    const char *GetMessage() const {
        return message;
    }
    /** @return The borrowed secondary HUD message, or null when no message is selected. */
    const char *GetSubMessage() const {
        return subMessage;
    }
    /**
     * @param value The primary HUD text, valid until replaced or its display time expires.
     * @return The stored text pointer.
     */
    const char *SetMessage(const char *value) {
        return message = value;
    }
    /** @return The mutable 20-byte primary HUD-message formatting buffer. */
    auto &GetMessageText() {
        return messageText;
    }
    /** @return The 20-byte primary HUD-message formatting buffer. */
    const auto &GetMessageText() const {
        return messageText;
    }
    /**
     * @param value The primary HUD message's remaining display time, in seconds.
     * @return The stored duration.
     */
    f32 SetMessageTime(f32 value) {
        return messageTime = value;
    }
    /** @return The borrowed loaded car model, or null before model installation. */
    VehicleModel *GetModel() const {
        return model;
    }
    /**
     * @param value The model, valid while physics or rendering accesses this vehicle.
     * @return The stored model pointer.
     */
    VehicleModel *SetModel(VehicleModel *value) {
        return model = value;
    }
    /**
     * @param value The populated nearby-instance count, from 0 through RAY_INSTANCES.
     * @return The stored count.
     */
    s32 SetNearbyInstanceCount(s32 value) {
        return nearbyInstanceCount = value;
    }
    /** @return Mutable nearest-racer pointers, with RACER_SLOTS entries. */
    auto &GetNearestRacers() {
        return nearestRacers;
    }
    /** @return Nearest-racer pointers, with RACER_SLOTS entries. */
    const auto &GetNearestRacers() const {
        return nearestRacers;
    }
    /** @return The accumulated travel used to advance wheel rotation. */
    f32 GetOdometer() const {
        return odometer;
    }
    /**
     * @param value The accumulated travel used to advance wheel rotation.
     * @return The stored travel.
     */
    f32 SetOdometer(f32 value) {
        return odometer = value;
    }
    /** @return The rendering opacity, with 0 for transparent and 1 for opaque. */
    f32 GetOpacity() const {
        return opacity;
    }
    /**
     * @param value The rendering opacity, normally from 0 through 1.
     * @return The stored opacity, without clamping.
     */
    f32 SetOpacity(f32 value) {
        return opacity = value;
    }
    /** @return The pitch control used by flying-mode input. */
    f32 GetPitchInput() const {
        return pitchInput;
    }
    /**
     * @param value The pitch control used by flying-mode input.
     * @return The stored control.
     */
    f32 SetPitchInput(f32 value) {
        return pitchInput = value;
    }
    /** @return The mutable world-space velocity recorded when repositioning the vehicle. */
    auto &GetPlacedVelocity() {
        return placedVelocity;
    }
    /** @return The world-space velocity recorded when repositioning the vehicle. */
    const auto &GetPlacedVelocity() const {
        return placedVelocity;
    }
    /** @return The current route point index within GetSection(). */
    s32 GetPointIndex() const {
        return pointIndex;
    }
    /**
     * @param value The route point index within the current section.
     * @return The stored index.
     */
    s32 SetPointIndex(s32 value) {
        return pointIndex = value;
    }
    /**
     * @param value The borrowed camera used for the previous view transition.
     * @return The stored camera pointer.
     */
    CameraFrame *SetPreviousCamera(CameraFrame *value) {
        return previousCamera = value;
    }
    /** @return The mutable previous chassis position used by collision processing. */
    auto &GetPreviousCollisionPosition() {
        return previousCollisionPosition;
    }
    /** @return The previous chassis position used by collision processing. */
    const auto &GetPreviousCollisionPosition() const {
        return previousCollisionPosition;
    }
    /** @return The mutable previous audio listener position used for Doppler calculations. */
    auto &GetPreviousListenerPosition() {
        return previousListenerPosition;
    }
    /** @return The previous audio listener position used for Doppler calculations. */
    const auto &GetPreviousListenerPosition() const {
        return previousListenerPosition;
    }
    /** @return The mutable previous world position used by route and motion updates. */
    auto &GetPreviousPosition() {
        return previousPosition;
    }
    /** @return The previous world position used by route and motion updates. */
    const auto &GetPreviousPosition() const {
        return previousPosition;
    }
    /** @return The borrowed player's profile, or null before profile assignment. */
    PlayerProfile *GetProfile() const {
        return profile;
    }
    /**
     * @param value The profile, valid while race setup and results processing use it.
     * @return The stored profile pointer.
     */
    PlayerProfile *SetProfile(PlayerProfile *value) {
        return profile = value;
    }
    /** @return The race ranking assigned by racer-slot processing. */
    s32 GetRacePosition() const {
        return racePosition;
    }
    /**
     * @param value The race ranking assigned by racer-slot processing.
     * @return The stored ranking.
     */
    s32 SetRacePosition(s32 value) {
        return racePosition = value;
    }
    /** @return The borrowed racer slot assigned to this vehicle, or null when unassigned. */
    RacerSlot *GetRacer() const {
        return racer;
    }
    /**
     * @param value The racer slot, valid until the association is cleared or replaced.
     * @return The stored racer pointer.
     */
    RacerSlot *SetRacer(RacerSlot *value) {
        return racer = value;
    }
    /**
     * @param value The cached rear-axle load used by wheel friction.
     * @return The stored load.
     */
    f32 SetRearLoad(f32 value) {
        return rearLoad = value;
    }
    /**
     * @param value Whether rear-wheel friction indicates sliding.
     * @return The stored flag.
     */
    u8 SetRearSliding(u8 value) {
        return rearSliding = value;
    }
    /**
     * @param value Whether the selected physics specification uses rear-wheel drive.
     * @return The stored flag.
     */
    s32 SetRearWheelDrive(s32 value) {
        return rearWheelDrive = value;
    }
    /** @return The simulation-frame count of reverse transmission input. */
    s32 GetReverseFrames() const {
        return reverseFrames;
    }
    /**
     * @param value The reverse transmission input frame count.
     * @return The stored count.
     */
    s32 SetReverseFrames(s32 value) {
        return reverseFrames = value;
    }
    /** @return The roll control used by flying-mode input. */
    f32 GetRollInput() const {
        return rollInput;
    }
    /**
     * @param value The roll control used by flying-mode input.
     * @return The stored control.
     */
    f32 SetRollInput(f32 value) {
        return rollInput = value;
    }
    /** @return The borrowed current track section, or null before route association. */
    struct TrackSection *GetSection() const {
        return section;
    }
    /**
     * @param value The current track section, backed by loaded track storage.
     * @return The stored section pointer.
     */
    struct TrackSection *SetSection(struct TrackSection *value) {
        return section = value;
    }
    /** @return Four mutable flags identifying projected car-shadow corners with valid ground. */
    auto &GetShadowCornerValid() {
        return shadowCornerValid;
    }
    /** @return Four flags identifying projected car-shadow corners with valid ground. */
    const auto &GetShadowCornerValid() const {
        return shadowCornerValid;
    }
    /** @return The two-by-two screen-space bounds of the projected car shadow. */
    const auto &GetShadowCorners() const {
        return shadowCorners;
    }
    /** @return Four mutable converted vertices of the projected car-shadow quad. */
    auto &GetShadowQuad() {
        return shadowQuad;
    }
    /** @return Four converted vertices of the projected car-shadow quad. */
    const auto &GetShadowQuad() const {
        return shadowQuad;
    }
    /** @return The frame counter used when extending tyre skid marks. */
    s16 GetSkidFrames() const {
        return skidFrames;
    }
    /**
     * @param value The frame counter used when extending tyre skid marks.
     * @return The stored count.
     */
    s16 SetSkidFrames(s16 value) {
        return skidFrames = value;
    }
    /** @return Whether normal drive-force processing is temporarily suppressed. */
    s32 GetSkipDrive() const {
        return skipDrive;
    }
    /**
     * @param value Whether to suppress normal drive-force processing.
     * @return The stored flag.
     */
    s32 SetSkipDrive(s32 value) {
        return skipDrive = value;
    }
    /** @return The racer-slot reassignment cooldown in simulation frames. */
    s32 GetSlotCooldown() const {
        return slotCooldown;
    }
    /**
     * @param value The racer-slot reassignment cooldown in simulation frames.
     * @return The stored cooldown.
     */
    s32 SetSlotCooldown(s32 value) {
        return slotCooldown = value;
    }
    /** @return Mutable smoke emission timers, one per wheel. */
    auto &GetSmokeTimers() {
        return smokeTimers;
    }
    /** @return Smoke emission timers, one per wheel. */
    const auto &GetSmokeTimers() const {
        return smokeTimers;
    }
    /** @return The cached HUD speed, using the legacy world-speed multiplier of 2.24. */
    f32 GetSpeedKph() const {
        return speedKph;
    }
    /**
     * @param value The cached speed in legacy HUD display units.
     * @return The stored speed.
     */
    f32 SetSpeedKph(f32 value) {
        return speedKph = value;
    }
    /** @return Mutable per-wheel flags recording whether spray advanced during this update. */
    auto &GetSprayAdvanced() {
        return sprayAdvanced;
    }
    /** @return Per-wheel flags recording whether spray advanced during this update. */
    const auto &GetSprayAdvanced() const {
        return sprayAdvanced;
    }
    /** @return Mutable remaining spray emission lifetimes, one per wheel. */
    auto &GetSprayFramesLeft() {
        return sprayFramesLeft;
    }
    /** @return Remaining spray emission lifetimes, one per wheel. */
    const auto &GetSprayFramesLeft() const {
        return sprayFramesLeft;
    }
    /** @return The mutable world-space origin used for relative spray geometry. */
    auto &GetSprayOrigin() {
        return sprayOrigin;
    }
    /** @return The world-space origin used for relative spray geometry. */
    const auto &GetSprayOrigin() const {
        return sprayOrigin;
    }
    /** @return Mutable per-wheel progression through the current spray segment. */
    auto &GetSprayPhases() {
        return sprayPhases;
    }
    /** @return Per-wheel progression through the current spray segment. */
    const auto &GetSprayPhases() const {
        return sprayPhases;
    }
    /** @return Mutable surface identifiers for each wheel's SPRAY_SEGMENTS recorded segments. */
    auto &GetSpraySegmentSurfaces() {
        return spraySegmentSurfaces;
    }
    /** @return Surface identifiers for each wheel's SPRAY_SEGMENTS recorded segments. */
    const auto &GetSpraySegmentSurfaces() const {
        return spraySegmentSurfaces;
    }
    /** @return Mutable current spray surface identifiers, one per wheel. */
    auto &GetSpraySurfaces() {
        return spraySurfaces;
    }
    /** @return The current spray surface identifiers, one per wheel. */
    const auto &GetSpraySurfaces() const {
        return spraySurfaces;
    }
    /** @return Mutable signed XYZ velocities for every vertex of each wheel's spray segments. */
    auto &GetSprayVelocities() {
        return sprayVelocities;
    }
    /** @return Signed XYZ velocities for every vertex of each wheel's spray segments. */
    const auto &GetSprayVelocities() const {
        return sprayVelocities;
    }
    /** @return Mutable converted vertices for each wheel's SPRAY_SEGMENTS spray segments. */
    auto &GetSprayVertices() {
        return sprayVertices;
    }
    /** @return Converted vertices for each wheel's SPRAY_SEGMENTS spray segments. */
    const auto &GetSprayVertices() const {
        return sprayVertices;
    }
    /** @return The transform from relative spray coordinates into world space. */
    const auto &GetSprayMatrix() const {
        return sprayMtx;
    }
    /** @return The wheel steering angle, in radians. */
    f32 GetSteerAngle() const {
        return steerAngle;
    }
    /**
     * @param value The wheel steering angle, in radians.
     * @return The stored angle.
     */
    f32 SetSteerAngle(f32 value) {
        return steerAngle = value;
    }
    /** @return The steering return-direction indicator. */
    s8 GetSteerReturn() const {
        return steerReturn;
    }
    /**
     * @param value The steering return-direction indicator.
     * @return The stored indicator.
     */
    s8 SetSteerReturn(s8 value) {
        return steerReturn = value;
    }
    /** @return The current steering control before wheel-angle processing. */
    f32 GetSteering() const {
        return steering;
    }
    /**
     * @param value The steering control before wheel-angle processing.
     * @return The stored control.
     */
    f32 SetSteering(f32 value) {
        return steering = value;
    }
    /** @return The configured steering and handling mode index. */
    s32 GetSteeringMode() const {
        return steeringMode;
    }
    /**
     * @param value The secondary HUD text, valid until replaced or its display time expires.
     * @return The stored text pointer.
     */
    const char *SetSubMessage(const char *value) {
        return subMessage = value;
    }
    /**
     * @param value The secondary HUD message's remaining display time, in seconds.
     * @return The stored duration.
     */
    f32 SetSubMessageTime(f32 value) {
        return subMessageTime = value;
    }
    /** @return The selected suspension stiffness option index. */
    s32 GetSuspensionStiffness() const {
        return suspensionStiffness;
    }
    /** @return The car class used to select arcade record-time tables. */
    s32 GetTableIndex() const {
        return tableIndex;
    }
    /**
     * @param value The car class used by arcade record-time tables.
     * @return The stored index.
     */
    s32 SetTableIndex(s32 value) {
        return tableIndex = value;
    }
    /** @return The processed throttle control supplied to drivetrain physics. */
    f32 GetThrottle() const {
        return throttle;
    }
    /**
     * @param value The processed throttle control supplied to drivetrain physics.
     * @return The stored control.
     */
    f32 SetThrottle(f32 value) {
        return throttle = value;
    }
    /** @return The remaining arcade race time, in seconds. */
    f32 GetTimeLeft() const {
        return timeLeft;
    }
    /**
     * @param value The remaining arcade race time, in seconds.
     * @return The stored duration.
     */
    f32 SetTimeLeft(f32 value) {
        return timeLeft = value;
    }
    /** @return The selected tyre option index. */
    s32 GetTireType() const {
        return tireType;
    }
    /** @return Mutable cubic engine torque coefficients, highest power first. */
    auto &GetTorqueCurve() {
        return torqueCurve;
    }
    /** @return The cubic engine torque coefficients, highest power first. */
    const auto &GetTorqueCurve() const {
        return torqueCurve;
    }
    /** @return The vehicle's elapsed race time, in seconds. */
    f32 GetTotalTime() const {
        return totalTime;
    }
    /**
     * @param value The vehicle's elapsed race time, in seconds.
     * @return The stored time.
     */
    f32 SetTotalTime(f32 value) {
        return totalTime = value;
    }
    /** @return The mutable route direction used to detect wrong-way driving. */
    auto &GetTrackDirection() {
        return trackDirection;
    }
    /** @return The route direction used to detect wrong-way driving. */
    const auto &GetTrackDirection() const {
        return trackDirection;
    }
    /** @return The mutable forward vector of the current track reference frame. */
    auto &GetTrackForward() {
        return trackForward;
    }
    /** @return The forward vector of the current track reference frame. */
    const auto &GetTrackForward() const {
        return trackForward;
    }
    /** @return The mutable vehicle offset from the current track reference point. */
    auto &GetTrackOffset() {
        return trackOffset;
    }
    /** @return The vehicle offset from the current track reference point. */
    const auto &GetTrackOffset() const {
        return trackOffset;
    }
    /** @return The mutable world-space route reference point. */
    auto &GetTrackPoint() {
        return trackPoint;
    }
    /** @return The world-space route reference point. */
    const auto &GetTrackPoint() const {
        return trackPoint;
    }
    /** @return The mutable lateral vector of the current track reference frame. */
    auto &GetTrackSide() {
        return trackSide;
    }
    /** @return The lateral vector of the current track reference frame. */
    const auto &GetTrackSide() const {
        return trackSide;
    }
    /** @return The lateral distance measured in the current track reference frame. */
    f32 GetTrackSideDistance() const {
        return trackSideDistance;
    }
    /**
     * @param value The lateral distance in the current track reference frame.
     * @return The stored distance.
     */
    f32 SetTrackSideDistance(f32 value) {
        return trackSideDistance = value;
    }
    /** @return The cached lateral route offset used by track-position processing. */
    f32 GetTrackSideOffset() const {
        return trackSideOffset;
    }
    /**
     * @param value The cached lateral route offset.
     * @return The stored offset.
     */
    f32 SetTrackSideOffset(f32 value) {
        return trackSideOffset = value;
    }
    /** @return The mutable up vector of the current track reference frame. */
    auto &GetTrackUp() {
        return trackUp;
    }
    /** @return The up vector of the current track reference frame. */
    const auto &GetTrackUp() const {
        return trackUp;
    }
    /** @return The active transmission mode after temporary driving overrides. */
    s32 GetTransmission() const {
        return transmission;
    }
    /**
     * @param value The active transmission mode.
     * @return The stored mode.
     */
    s32 SetTransmission(s32 value) {
        return transmission = value;
    }
    /** @return The four mutable wheel rigid bodies. */
    auto &GetWheelBodies() {
        return wheelBodies;
    }
    /** @return The four wheel rigid bodies. */
    const auto &GetWheelBodies() const {
        return wheelBodies;
    }
    /** @return The four mutable wheel render frames. */
    auto &GetWheelFrames() {
        return wheelFrames;
    }
    /** @return The four wheel render frames. */
    const auto &GetWheelFrames() const {
        return wheelFrames;
    }
    /** @return The four mutable wheel matrices, aliased with GetWheelFrames(). */
    auto &GetWheelMatrices() {
        return wheelMatrices;
    }
    /** @return The four wheel matrices, aliased with GetWheelFrames(). */
    const auto &GetWheelMatrices() const {
        return wheelMatrices;
    }

    /**
     * Translate player input into throttle, braking, steering, transmission, and flying controls.
     *
     * @ghidraAddress BRD3D: 0x10063840
     * @ghidraAddress BRGlide: 0x1005c8b0
     */
    void ApplyPlayerVehicleInput(void);

    /**
     * Advance the vehicle's physics, route progress, timers, camera, and effects for a race step.
     *
     * @ghidraAddress BRD3D: 0x10075f10
     * @ghidraAddress BRGlide: 0x1006f170
     */
    void UpdateVehiclePhysicsFrame(void);

    /**
     * Store the vehicle's three paint colour bytes.
     *
     * @param red The red channel, from 0 through 255.
     * @param green The green channel, from 0 through 255.
     * @param blue The blue channel, from 0 through 255.
     * @ghidraAddress BRD3D: 0x10035ca0
     * @ghidraAddress BRGlide: 0x1002f340
     */
    void SetVehiclePaintColorRgb(u8 red, u8 green, u8 blue);

    /**
     * Transform the body and wheel matrices for drawing.
     *
     * @ghidraAddress BRD3D: 0x10061c60
     * @ghidraAddress BRGlide: 0x1005ace0
     */
    void TransformAllWheelMatrices(void);

    /**
     * Drive the vehicle with its computer controller.
     *
     * @ghidraAddress BRD3D: 0x10064700
     * @ghidraAddress BRGlide: 0x1005d770
     */
    void ComputeAiVehicleInput(void);

    /**
     * Initialise the player slot, profile defaults, and vehicle selection.
     *
     * @ghidraAddress BRD3D: 0x100634f0
     * @ghidraAddress BRGlide: 0x1005c560
     */
    void InitPlayerVehicleSlot(void);

    /**
     * Decode a combined car identifier into the model identifier and custom-model flag.
     *
     * @param carId The model identifier, plus 16 when custom model handling is required.
     * @ghidraAddress BRD3D: 0x10076ae0
     * @ghidraAddress BRGlide: 0x1006fd50
     */
    void SetVehicleCarId(s32 carId);

    /**
     * Update active camera placement and transitions from current vehicle and track conditions.
     *
     * @ghidraAddress BRD3D: 0x100019d0
     * @ghidraAddress BRGlide: 0x10001cf0
     */
    void UpdateCameraSystem(void);
};
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Dispatch the computer controller through its vehicle method.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress BRD3D: 0x10065620
 * @ghidraAddress BRGlide: 0x1005e690
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
 * Translate player controls, advance vehicle physics, and process player effects and respawning.
 *
 * @param vehicle The active vehicle with a valid input record.
 * @ghidraAddress BRD3D: 0x10063fe0
 * @ghidraAddress BRGlide: 0x1005d050
 */
void PlayerVehicleControllerCallback(Vehicle *vehicle);

/**
 * Player and computer vehicle paint colours, indexed by player or race slot, in RGB byte order.
 *
 * @ghidraAddress BRD3D: 0x100b37d0
 * @ghidraAddress BRGlide: 0x100b2fd8
 */
extern u8 g_abPlayerColorsRgb[20][3];
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
 * Vehicles of the Windows race.
 *
 * @ghidraAddress BRD3D: 0x10acdea8
 * @ghidraAddress BRGlide: 0x10af1208
 */
extern Vehicle g_aVehicles[MAX_VEHICLES];
#endif

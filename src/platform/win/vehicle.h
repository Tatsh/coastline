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
#include "vehiclemodel.h"
#include "wheelframe.h"

#define VEHICLE_GHOST 2
#define MAX_VEHICLES 16
#define VEHICLE_LAP_TIMES 12

/** Recovered fields of a Windows vehicle. */
typedef struct Vehicle {
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
    struct RacerSlot *nearestRacers[RACER_SLOTS];
    struct RacerSlot *racer;
    s32 slotCooldown;
    void (*driver)(struct Vehicle *vehicle);
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
    u8 abUnknown105c[0x10]; /* +0x105c */
    f32 smokeTimers[VEHICLE_WHEELS];
    u8 abUnknown107c[0x30]; /* +0x107c */
    f32 sprayFramesLeft[VEHICLE_WHEELS];
    s32 spraySurfaces[VEHICLE_WHEELS];
    f32 sprayPhases[VEHICLE_WHEELS];
    s32 sprayAdvanced[VEHICLE_WHEELS];
    u8 abUnknown10ec[0x34]; /* +0x10ec */
    ConvertedModelVertex sprayVertices[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES];
    s16 sprayVelocities[VEHICLE_WHEELS][SPRAY_SEGMENTS][SPRAY_SEGMENT_VERTICES][3];
    s16 spraySegmentSurfaces[VEHICLE_WHEELS][SPRAY_SEGMENTS];
    Vec3f sprayOrigin;
    f32 inputMatrix[4][4];
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
    u8 abUnknown299c[0xc]; /* +0x299c */
    s32 carModel;
    u8 color[3];
    u8 ghostMode;
    f32 opacity;
    s32 customCarModel;
    u8 abUnknown29b8[8]; /* +0x29b8 */
    PlayerInputState *input;
    VehicleModel *model;
    s16 dents[VEHICLE_DENT_SIDES];
    s16 dentJitter;
    u8 abUnknown29da[0xd6]; /* +0x29da */
    Vec3f previousCollisionPosition;
    char checkpointMessageText[20];
    u8 abUnknown2ad0[0x98]; /* +0x2ad0 */
#ifdef __cplusplus
    /** @ghidraAddress BRD3D: 0x10063840 */
    void ApplyPlayerVehicleInput(void);

    /**
     * @ghidraAddress BRD3D: 0x10064350
     */
    void ComputeTrackPosition(void);

    /** @ghidraAddress BRD3D: 0x10075f10 */
    void UpdateVehiclePhysicsFrame(void);

    /**
     * @ghidraAddress BRD3D: 0x10061f70
     */
    void UpdateVehicleEngineAndGearbox(void);

    /**
     * Store the vehicle's three paint colour bytes.
     *
     * @ghidraAddress BRD3D: 0x10035ca0
     * @ghidraAddress BRGlide: 0x1002f340
     */
    void SetVehiclePaintColorRgb(u8 red, u8 green, u8 blue);

    /**
     * Advance spring, tyre, damping, and collision calculations.
     *
     * @ghidraAddress BRD3D: 0x10061720
     */
    void SimulateVehiclePhysicsSubstep(void);

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
    /**
     * Initialise the player slot, profile defaults, and vehicle selection.
     *
     * @ghidraAddress BRD3D: 0x100634f0
     */
    void InitPlayerVehicleSlot(void);
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
    /**
     * @ghidraAddress BRD3D: 0x100019d0
     */
    void UpdateCameraSystem(void);

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
#endif
} Vehicle;

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

/**
 * Vehicles of the Windows race.
 *
 * @ghidraAddress BRD3D: 0x10acdea8
 */
extern Vehicle g_aVehicles[MAX_VEHICLES];

/**
 * @ghidraAddress BRD3D: 0x100b37d0
 */
extern u8 g_abPlayerColorsRgb[][3];

#pragma once

#include <render.h>

#include "carphysics.h"
#include "convertedvertex.h"
#include "playerinput.h"
#include "profile.h"
#include "racerslot.h"
#include "rigidbody.h"
#include "types.h"
#include "vehiclecamera.h"
#include "vehicleeffects.h"
#include "vehicleforces.h"
#include "vehiclemodel.h"

#define VEHICLE_GHOST 2

/** Recovered fields of a Windows vehicle. */
typedef struct Vehicle {
    union {
        CameraFrame frame;
        struct {
            f32 matrix[4][4];
            f32 wheelMatrices[4][4][4];
        };
    };
    s32 index;
    u8 abUnknown144[0x20]; /* +0x144 */
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
    s32 mouseSensitivity;
    s32 chosenTransmission;
    s32 forcedForwardFrames;
    s32 forcedReverseFrames;
    s32 reverseFrames;
    s32 forwardFrames;
    u8 abUnknownEb0[0x50]; /* +0xeb0 */
    struct RacerSlot *racer;
    u8 abUnknownF04[0x74]; /* +0xf04 */
    s32 cameraChanged;
    s32 flyMode;
    u8 abUnknownF80[0xc]; /* +0xf80 */
    struct TrackSection *section;
    s32 pointIndex;
    Vec3f trackDirection;
    u8 abUnknownFa0[8]; /* +0xfa0 */
    s32 lap;
    u8 abUnknownFac[0x38]; /* +0xfac */
    f32 bestLapTime;
    u8 abUnknownFe8[4]; /* +0xfe8 */
    f32 raceTime;
    u8 abUnknownFf0[4]; /* +0xff0 */
    f32 progress;
    u8 abUnknownFf8[0x2c]; /* +0xff8 */
    Vec3f placedVelocity;
    u8 abUnknown1030[0x3c]; /* +0x1030 */
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
    u8 abUnknown2714[0x20]; /* +0x2714 */
    CameraFrame *activeCamera;
    CameraFrame *previousCamera;
    CameraFrame cameras[VEHICLE_CAMERAS];
    u8 abUnknown28d4[0x18]; /* +0x28d4 */
    Vec3f chasePosition;
    u8 abUnknown28f8[0xb0]; /* +0x28f8 */
    s32 carModel;
    u8 color[3];
    u8 ghostMode;
    f32 opacity;
    s32 customCarModel;
    u8 abUnknown29b8[8]; /* +0x29b8 */
    PlayerInputState *input;
    VehicleModel *model;
    u8 abUnknown29c8[0x1a0]; /* +0x29c8 */
#ifdef __cplusplus
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

/**
 * Vehicles of the Windows race.
 *
 * @ghidraAddress BRD3D: 0x10acdea8
 */
extern Vehicle g_aVehicles[16];

/**
 * @ghidraAddress BRD3D: 0x100b37d0
 */
extern u8 g_abPlayerColorsRgb[][3];

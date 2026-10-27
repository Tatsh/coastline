#pragma once

#include <render.h>

#include "carphysics.h"
#include "playerinput.h"
#include "profile.h"
#include "racerslot.h"
#include "rigidbody.h"
#include "types.h"
#include "vehiclecamera.h"
#include "vehiclemodel.h"

#define VEHICLE_GHOST 2

/** Recovered fields of a Windows vehicle. */
typedef struct Vehicle {
    f32 matrix[4][4];
    f32 wheelMatrices[4][4][4];
    s32 index;
    u8 abUnknown144[0x20]; /* +0x144 */
    RigidBody body;
    RigidBody wheelBodies[4];
    u8 abUnknownBa0[0x284]; /* +0xba0 */
    f32 sector;
    f32 gearRatios[CAR_GEAR_RATIOS];
    f32 torqueCurve[4];
    f32 finalDrive;
    s32 gearCount;
    s32 rearWheelDrive;
    s32 transmission;
    s32 tableIndex;
    f32 throttle;
    u8 abUnknownE6c[0x1c]; /* +0xe6c */
    s32 customDecals;
    PlayerProfile *profile;
    u8 abUnknownE90[8]; /* +0xe90 */
    s32 mouseSensitivity;
    s32 chosenTransmission;
    u8 abUnknownEa0[0x60]; /* +0xea0 */
    struct RacerSlot *racer;
    u8 abUnknownF04[0x88]; /* +0xf04 */
    struct TrackSection *section;
    s32 pointIndex;
    u8 abUnknownF94[0x14]; /* +0xf94 */
    s32 lap;
    u8 abUnknownFac[0x38]; /* +0xfac */
    f32 bestLapTime;
    u8 abUnknownFe8[4]; /* +0xfe8 */
    f32 raceTime;
    u8 abUnknownFf0[4]; /* +0xff0 */
    f32 progress;
    u8 abUnknownFf8[0x2c]; /* +0xff8 */
    Vec3f placedVelocity;
    u8 abUnknown1030[0x16a4]; /* +0x1030 */
    f32 inputMatrix[4][4];
    u8 abUnknown2714[0x20]; /* +0x2714 */
    CameraFrame *activeCamera;
    CameraFrame *previousCamera;
    CameraFrame cameras[VEHICLE_CAMERAS];
    u8 abUnknown28d4[0xd4]; /* +0x28d4 */
    s32 carModel;
    u8 color[3];
    u8 ghostMode;
    u8 abUnknown29b0[4]; /* +0x29b0 */
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

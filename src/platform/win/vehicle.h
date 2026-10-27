#pragma once

#include "playerinput.h"
#include "profile.h"
#include "racerslot.h"
#include "rigidbody.h"
#include "types.h"
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
    u8 abUnknownE28[0x40]; /* +0xe28 */
    f32 throttle;
    u8 abUnknownE6c[0x1c]; /* +0xe6c */
    s32 customDecals;
    PlayerProfile *profile;
    u8 abUnknownE90[8]; /* +0xe90 */
    s32 mouseSensitivity;
    u8 abUnknownE9c[0x10c]; /* +0xe9c */
    s32 lap;
    u8 abUnknownFac[0x48]; /* +0xfac */
    f32 progress;
    u8 abUnknownFf8[0x16dc]; /* +0xff8 */
    f32 inputMatrix[4][4];
    u8 abUnknown2714[0x294]; /* +0x2714 */
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

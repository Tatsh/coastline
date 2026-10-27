#pragma once

#include "playerinput.h"
#include "profile.h"
#include "rigidbody.h"
#include "types.h"

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
    u8 abUnknownE6c[0x20]; /* +0xe6c */
    PlayerProfile *profile;
    u8 abUnknownE90[8]; /* +0xe90 */
    s32 mouseSensitivity;
    u8 abUnknownE9c[0x10c]; /* +0xe9c */
    s32 lap;
    u8 abUnknownFac[0x48]; /* +0xfac */
    f32 progress;
    u8 abUnknownFf8[0x19b4]; /* +0xff8 */
    u8 color[3];
    u8 abUnknown29af[0x11]; /* +0x29af */
    PlayerInputState *input;
    u8 abUnknown29c4[0x1a4]; /* +0x29c4 */
#ifdef __cplusplus
    /**
     * Initialise the player slot, profile defaults, and vehicle selection.
     *
     * @ghidraAddress BRD3D: 0x100634f0
     * @stub
     */
    void InitPlayerVehicleSlot(void);
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

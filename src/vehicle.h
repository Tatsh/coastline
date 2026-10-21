#ifndef COASTLINE_VEHICLE_H
#define COASTLINE_VEHICLE_H

/**
 * @file
 * The vehicles of a race.
 */

#include <PR/ultratypes.h>

/** The number of vehicles a race can hold. */
#define MAX_VEHICLES 4

/**
 * The replay state of a vehicle. Its fields join this structure as the routines that use them are
 * reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0x44];
    /** The ghost recording the vehicle plays back, or NULL. */
    u8 *ghostData;
    u8 unreconstructed48[0x4];
    u32 ghostSize;
} ReplayState;

/**
 * A vehicle. The game prints its size, 0x2090 bytes, at start-up. Its fields join this structure as
 * the routines that use them are reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0x2074];
    ReplayState *replay;
    u8 unreconstructed2078[0x2090 - 0x2078];
} Vehicle;

/** @ghidraAddress USA: 0x8031b760 */
extern Vehicle g_aVehicles[MAX_VEHICLES];

/**
 * Bind a vehicle to the input of its player and reset it for a race.
 *
 * @ghidraAddress USA: 0x80226100
 */
void InitPlayerVehicleSlot(Vehicle *vehicle);

#endif

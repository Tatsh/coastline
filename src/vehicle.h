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
 * The saved settings and progress of a player. Its fields join this structure as the routines that
 * use them are reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0xCC];
    /** The unlocked cars, a bit for each from bit 0, and further unlocks above them. */
    u16 unlocks;
    /** The unlocked tracks, a bit for each. */
    u16 unlockedTracks;
    u8 unreconstructedD0[0xE4 - 0xD0];
    /** The button layout the player chose on the controller screen. */
    s32 controllerLayout;
} PlayerProfile;

/**
 * A vehicle. The game prints its size, 0x2090 bytes, at start-up. Its fields join this structure as
 * the routines that use them are reconstructed.
 */
typedef struct {
    u8 unreconstructed0[0xE5C];
    /** The profile of the player who drives the vehicle. */
    PlayerProfile *profile;
    u8 unreconstructedE60[0x2074 - 0xE60];
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

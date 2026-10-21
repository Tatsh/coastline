#ifndef COASTLINE_INPUT_H
#define COASTLINE_INPUT_H

/**
 * @file
 * The input of each player, the cheat codes, and the translation of controller data.
 */

#include <PR/ultratypes.h>

/** The number of players the input state holds. */
#define MAX_PLAYERS 4

/**
 * The input of one player. Its fields join this structure as the routines that use them are
 * reconstructed.
 */
typedef struct {
    u8 raw[0x15C];
} PlayerInputState;

/** @ghidraAddress USA: 0x8036a8e0 */
extern PlayerInputState g_aPlayerInputState[MAX_PLAYERS];

/** The number of players at this console. @ghidraAddress USA: 0x8026ff08 */
extern u32 g_dwNumLocalPlayers;

/**
 * Reset the input of a player.
 *
 * @ghidraAddress USA: 0x80255b54
 */
void InitPlayerInputState(PlayerInputState *state);

#endif

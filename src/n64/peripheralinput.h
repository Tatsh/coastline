#ifndef COASTLINE_N64_PERIPHERALINPUT_H
#define COASTLINE_N64_PERIPHERALINPUT_H

/**
 * @file
 * Polling of the N64 mouse and the Randnet keyboard, merged into the input of the players.
 */

#include <PR/ultratypes.h>

#include "common/playerinput.h"

#define RANDNET_KEY_CONTROL 0x1107     /*!< The Control key's matrix position. */
#define RANDNET_KEY_Z 0x0D08           /*!< The Z key's matrix position. */
#define RANDNET_KEY_LEFT_SHIFT 0x0E01  /*!< The left Shift key's matrix position. */
#define RANDNET_KEY_RIGHT_SHIFT 0x0E06 /*!< The right Shift key's matrix position. */

/**
 * Read three keys from the Randnet keyboard on a port, or clear its key history if absent.
 *
 * @param port The controller port, from 0.
 * @param keys Receives the matrix positions of up to three pressed keys, written only on success.
 * @return Non-zero when a keyboard on the port replied without an error.
 */
s32 ReadRandnetKeyboard(s32 port, u16 keys[3]);

/**
 * Check whether a key was down during the previous poll of a port.
 *
 * @param port The controller port, from 0.
 * @param code The matrix position of the key.
 * @return Non-zero when the key was among the keys last recorded for the port.
 */
s32 WasRandnetKeyDown(s32 port, u16 code);

/**
 * Record the keys from the latest poll of a port.
 *
 * @param port The controller port, from 0.
 * @param keys The matrix positions of the three keys read from the port.
 */
void RememberRandnetKeys(s32 port, const u16 keys[3]);

/**
 * Check whether either Shift key is down on any Randnet keyboard.
 *
 * @return Non-zero when a Shift key is among the keys last recorded for any port.
 */
s32 IsRandnetShiftDown(void);

/**
 * Merge keyboard controls into menus and races.
 *
 * @param state The input of the first player, receiving the buttons and the steering of every
 *     keyboard found.
 */
void UpdateGameKeyboardInput(PlayerInputState *state);

/**
 * Poll N64 peripherals for a player and merge their input.
 *
 * @param player The player, from 0. Player 0 also refreshes the status of the ports every 16
 *     frames.
 */
void UpdateN64PeripheralInputForPlayer(s32 player);

#endif /* COASTLINE_N64_PERIPHERALINPUT_H */

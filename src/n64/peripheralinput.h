#ifndef COASTLINE_N64_PERIPHERALINPUT_H
#define COASTLINE_N64_PERIPHERALINPUT_H

#include <PR/ultratypes.h>

#include "common/playerinput.h"

#define RANDNET_KEY_CONTROL 0x1107     /*!< The Control key's matrix position. */
#define RANDNET_KEY_Z 0x0D08           /*!< The Z key's matrix position. */
#define RANDNET_KEY_LEFT_SHIFT 0x0E01  /*!< The left Shift key's matrix position. */
#define RANDNET_KEY_RIGHT_SHIFT 0x0E06 /*!< The right Shift key's matrix position. */

/** Read three keys from the Randnet keyboard on a port, or clear its key history if absent. */
s32 ReadRandnetKeyboard(s32 port, u16 keys[3]);

/** Check whether a key was down during the previous poll of a port. */
s32 WasRandnetKeyDown(s32 port, u16 code);

/** Record the keys from the latest poll of a port. */
void RememberRandnetKeys(s32 port, const u16 keys[3]);

/** Check whether either Shift key is down on any Randnet keyboard. */
s32 IsRandnetShiftDown(void);

/** Merge keyboard controls into menus and races. */
void UpdateGameKeyboardInput(PlayerInputState *state);

/** Poll N64 peripherals for a player and merge their input. */
void UpdateN64PeripheralInputForPlayer(s32 player);

#endif /* COASTLINE_N64_PERIPHERALINPUT_H */

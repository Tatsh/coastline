#ifndef COASTLINE_N64_PAINTINPUT_H
#define COASTLINE_N64_PAINTINPUT_H

/**
 * @file
 * The N64 mouse and Randnet keyboard input of the paint shop.
 */

#include "common/playerinput.h"

/**
 * Merge an N64 mouse and Randnet keyboard into the paint shop player's input.
 *
 * @param state The player whose paint shop is open.
 */
void UpdatePaintPeripheralInput(PlayerInputState *state);

#endif /* COASTLINE_N64_PAINTINPUT_H */

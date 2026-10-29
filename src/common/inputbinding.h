#pragma once

/**
 * @file
 * The bindings of the controls of the game, which the settings save.
 */

/** The number of controls a binding table has. */
#define INPUT_BINDING_COUNT 28

/** The binding of one control of the game to a key, a button, or an axis. */
struct InputBinding {
    /** The key or button in the low byte and the kind of device in the high. */
    unsigned short wControl;
    /** More controls of the defaults that select the binding, 0 where another binding uses one. */
    unsigned short awAlternateControls[2];
};

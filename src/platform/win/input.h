#pragma once

/**
 * @file
 * The control bindings of the PC port.
 */

#include <windows.h>

/** The number of controls a binding table has. */
#define INPUT_BINDING_COUNT 28

/** The binding of one control of the game to a key, a button, or an axis. */
struct InputBinding {
    WORD wControl; /*!< The key or button in the low byte and the kind of device in the high. */
    BYTE abUnknown2[4]; /* +0x02 */
};

/**
 * The default bindings of the keyboard.
 *
 * @ghidraAddress BRD3D: 0x100b4098
 * @ghidraAddress BRGlide: 0x100b38a0
 */
extern InputBinding g_awDefaultKeyBindingsDevice0[INPUT_BINDING_COUNT];

/**
 * The default bindings of the first game controller.
 *
 * @ghidraAddress BRD3D: 0x100b4140
 * @ghidraAddress BRGlide: 0x100b3948
 */
extern InputBinding g_awDefaultKeyBindingsDevice1[INPUT_BINDING_COUNT];

/**
 * The default bindings of the second game controller.
 *
 * @ghidraAddress BRD3D: 0x100b41e8
 * @ghidraAddress BRGlide: 0x100b39f0
 */
extern InputBinding g_awDefaultKeyBindingsDevice2[INPUT_BINDING_COUNT];

/**
 * The default bindings of the third game controller.
 *
 * @ghidraAddress BRD3D: 0x100b4290
 * @ghidraAddress BRGlide: 0x100b3a98
 */
extern InputBinding g_awDefaultKeyBindingsDevice3[INPUT_BINDING_COUNT];

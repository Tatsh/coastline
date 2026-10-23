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
 * The bindings of the keyboard, the first member of g_GlobalRallyConfig.
 *
 * @ghidraAddress BRD3D: 0x10b4df30
 */
extern InputBinding g_aInputBindingsKeyboard[INPUT_BINDING_COUNT];

/**
 * The bindings of the first game controller.
 *
 * @ghidraAddress BRD3D: 0x10b4dfd8
 */
extern InputBinding g_aInputBindingsDevice1[INPUT_BINDING_COUNT];

/**
 * The bindings of the second game controller.
 *
 * @ghidraAddress BRD3D: 0x10b4e080
 */
extern InputBinding g_aInputBindingsDevice2[INPUT_BINDING_COUNT];

/**
 * The bindings of the third game controller.
 *
 * @ghidraAddress BRD3D: 0x10b4e128
 */
extern InputBinding g_aInputBindingsDevice3[INPUT_BINDING_COUNT];

/**
 * The device the game reads, 1 to 3 for a game controller and any other value for the keyboard.
 *
 * @ghidraAddress BRD3D: 0x10b4e1d0
 */
extern DWORD g_dwInputDeviceType;

/**
 * The bindings of the device the game reads.
 *
 * @ghidraAddress BRD3D: 0x10b4e1d4
 */
extern InputBinding *g_pActiveInputBindings;

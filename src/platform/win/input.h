#pragma once

/**
 * @file
 * The control bindings and the DirectInput devices of the PC port.
 */

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0500
#endif
#include <windows.h>
// The headers below need the types of windows.h.
#include <dinput.h>

/** The mouse of the game. The layout is not recovered yet. */
class MouseDevice {
public:
    /**
     * Release the DirectInput device of the mouse.
     *
     * @ghidraAddress BRD3D: 0x100602b0
     * @ghidraAddress BRGlide: 0x10059320
     */
    void ReleaseMouseDevice();
};

/**
 * DirectInput.
 *
 * @ghidraAddress BRD3D: 0x118abd70
 * @ghidraAddress BRGlide: 0x118eee88
 */
extern IDirectInputA *g_pDirectInput;

/**
 * The users of DirectInput. The last ShutdownDirectInput() releases it.
 *
 * @ghidraAddress BRD3D: 0x118abde8
 * @ghidraAddress BRGlide: 0x118eef00
 */
extern int g_dwDirectInputRefCount;

/**
 * The mouse, created with DirectInput.
 *
 * @ghidraAddress BRD3D: 0x10aa2e80
 * @ghidraAddress BRGlide: 0x10ac61e0
 */
extern MouseDevice *g_pMouseDevice;

/**
 * The DirectInput device of the keyboard.
 *
 * @ghidraAddress BRD3D: 0x118abdd0
 * @ghidraAddress BRGlide: 0x118eeee8
 */
extern IDirectInputDeviceA *g_pKeyboardDevice;

/**
 * The users of the keyboard device.
 *
 * @ghidraAddress BRD3D: 0x118abdd8
 * @ghidraAddress BRGlide: 0x118eeef0
 */
extern int g_dwKeyboardDeviceRefCount;

/**
 * The DirectInput device of the game controller.
 *
 * @ghidraAddress BRD3D: 0x118abdd4
 * @ghidraAddress BRGlide: 0x118eeeec
 */
extern IDirectInputDevice2A *g_pJoystickDevice;

/**
 * The users of the game controller device.
 *
 * @ghidraAddress BRD3D: 0x118abe00
 * @ghidraAddress BRGlide: 0x118eef18
 */
extern int g_dwJoystickDeviceRefCount;

/**
 * The jolt effect of force feedback of the game controller.
 *
 * @ghidraAddress BRD3D: 0x118abdfc
 * @ghidraAddress BRGlide: 0x118eef14
 */
extern IDirectInputEffect *g_pJoltEffect;

/**
 * The condition effect of force feedback of the game controller.
 *
 * @ghidraAddress BRD3D: 0x118abdec
 * @ghidraAddress BRGlide: 0x118eef04
 */
extern IDirectInputEffect *g_pConditionEffect;

/** The number of controls a binding table has. */
#define INPUT_BINDING_COUNT 28

/** The binding of one control of the game to a key, a button, or an axis. */
struct InputBinding {
    /** The key or button in the low byte and the kind of device in the high. */
    unsigned short wControl;
    /** More controls of the defaults that select the binding, 0 where another binding uses one. */
    unsigned short awAlternateControls[2];
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

/**
 * Drop a user of the game controller device. The last user releases the effects of force feedback
 * and the device. A count that falls below 0 is set back to 0.
 *
 * @ghidraAddress BRD3D: 0x10079550
 * @ghidraAddress BRGlide: 0x10072840
 */
void ReleaseJoystickDevice(void);

/**
 * Drop a user of the keyboard device. The last user releases the device. A count that falls below
 * 0 is set back to 0.
 *
 * @ghidraAddress BRD3D: 0x10078bc0
 * @ghidraAddress BRGlide: 0x10071eb0
 */
void ReleaseKeyboardDevice(void);

/**
 * Drop a user of DirectInput. The last user frees the mouse and releases DirectInput.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10078db0
 * @ghidraAddress BRGlide: 0x100720a0
 */
bool ShutdownDirectInput(void);

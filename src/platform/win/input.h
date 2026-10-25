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

/** The mouse of the game. The layout is not recovered past the members below. */
class MouseDevice {
public:
    int nCursorX;                   /*!< The horizontal position of the cursor. */
    int nCursorY;                   /*!< The vertical position of the cursor. */
    int nWheel;                     /*!< The position of the wheel. */
    int nPrevCursorX;               /*!< nCursorX when the mouse was read before. */
    int nPrevCursorY;               /*!< nCursorY when the mouse was read before. */
    char abUnknown14[0x10];         /* +0x14 */
    unsigned char abButtons[4];     /*!< The buttons that are down, 0x80 for down. */
    unsigned char abPrevButtons[4]; /*!< abButtons when the mouse was read before. */
    int abHeld[4];                  /*!< Whether each button is held down. */
    int abClicked[4];               /*!< Whether each button was let go since the read before. */
    int bPressPending;              /*!< Set while a button is down, cleared when one is let go. */
    IDirectInputDeviceA *pDevice;   /*!< The DirectInput device of the mouse. */

    /**
     * Clear the state of the mouse.
     *
     * @ghidraAddress BRD3D: 0x10060280
     * @ghidraAddress BRGlide: 0x100592f0
     */
    MouseDevice();

    /**
     * Create the DirectInput device of the system mouse for a window, in the format of a mouse and
     * with exclusive use in the foreground. A step that fails shows a message.
     *
     * @param hWnd The window.
     * @return Whether the device was created.
     * @ghidraAddress BRD3D: 0x100602e0
     * @ghidraAddress BRGlide: 0x10059350
     */
    bool CreateMouseDevice(HWND hWnd);

    /**
     * Unacquire and release the DirectInput device of the mouse.
     *
     * @ghidraAddress BRD3D: 0x100602b0
     * @ghidraAddress BRGlide: 0x10059320
     */
    void ReleaseMouseDevice();

    /**
     * Read the mouse and the arrows of the keyboard for the menus. The cursor moves within the
     * screen, F11 and F12 select the track of music before and after, the up and down arrows
     * move the hover of a menu at a repeat of 120 milliseconds, and the left and right arrows act
     * as the first two buttons. A button counts as clicked when it is let go.
     *
     * @param hWnd Not read.
     * @ghidraAddress BRD3D: 0x100603a0
     * @ghidraAddress BRGlide: 0x10059410
     */
    void PollMouseDevice(HWND hWnd);
};

/** The state of the menus the mouse and the arrows of the keyboard move. */
struct MouseMenuState {
    int bScrollDecPending;       /*!< Whether the hover of a menu moves up. */
    int bScrollIncPending;       /*!< Whether the hover of a menu moves down. */
    int bKeyLeftAsButton0;       /*!< Whether the left arrow acts as the first button. */
    int bKeyRightAsButton1;      /*!< Whether the right arrow acts as the second button. */
    int bScrollRepeatReady;      /*!< Whether the repeat of the hover may move it again. */
    DWORD dwPollLastTimeMs;      /*!< The time of the last poll, in milliseconds. */
    DWORD dwScrollRepeatTimerMs; /*!< The time since the hover last moved, in milliseconds. */
};

/**
 * The state of the menus the mouse and the arrows of the keyboard move.
 *
 * @ghidraAddress BRD3D: 0x10aa3398
 * @ghidraAddress BRGlide: 0x10ac66f8
 */
extern MouseMenuState g_MouseMenuState;

/**
 * Whether the input of characters is held back, which stops the poll of the mouse.
 *
 * @ghidraAddress BRD3D: 0x10aa2844
 * @ghidraAddress BRGlide: 0x10ac5b9c
 */
extern bool g_fCharInputSuppressed;

/**
 * The direction the hover of a menu moved last, -1 up and 1 down.
 *
 * @ghidraAddress BRD3D: 0x100ab3dc
 * @ghidraAddress BRGlide: 0x100aab7c
 */
extern short g_dwMouseScrollDirection;

/**
 * The item of a menu under the hover.
 *
 * @ghidraAddress BRD3D: 0x10aa286c
 * @ghidraAddress BRGlide: 0x10ac5bc4
 */
extern unsigned short g_wMenuHoverIndex;

/**
 * The time of the last change of the mouse, in milliseconds.
 *
 * @ghidraAddress BRD3D: 0x10aa33e8
 * @ghidraAddress BRGlide: 0x10ac6748
 */
extern DWORD g_dwLastInputActivityTime;

/**
 * The mouse that was read last.
 *
 * @ghidraAddress BRD3D: 0x10aa2a78
 * @ghidraAddress BRGlide: 0x10ac5dd8
 */
extern MouseDevice *g_pMousePosition;

/**
 * Read the mouse for the main window.
 *
 * @ghidraAddress BRD3D: 0x10060260
 * @ghidraAddress BRGlide: 0x100592d0
 */
void UpdateMouseInput(void);

/**
 * Show a message of the input with the caption of string 0xaa.
 *
 * @param hWnd The window that owns the message.
 * @param hResult Not read.
 * @param pszText The message.
 * @return The result of MessageBoxA().
 * @ghidraAddress BRD3D: 0x10060030
 * @ghidraAddress BRGlide: 0x100590a0
 */
int ShowInfoMessageBox(HWND hWnd, HRESULT hResult, const char *pszText);

/**
 * Add a user of DirectInput. The first user creates DirectInput, version 5, and the mouse, and a
 * DirectInput that is not created shows a message.
 *
 * @return Whether DirectInput exists.
 * @ghidraAddress BRD3D: 0x10078cd0
 * @ghidraAddress BRGlide: 0x10071fc0
 */
bool InitializeDirectInput(void);

/**
 * Acquire the DirectInput device of the game controller.
 *
 * @return Whether the device was acquired, and false without a device.
 * @ghidraAddress BRD3D: 0x100773d0
 * @ghidraAddress BRGlide: 0x100706b0
 */
bool TryAcquireJoystickDevice(void);

/**
 * Set a property of a DIPROPDWORD on a DirectInput device.
 *
 * @param pDevice The device.
 * @param guidProperty The property.
 * @param dwObject The object of the device, or 0 for the device.
 * @param dwHow How dwObject is read.
 * @param dwData The value.
 * @return The result of SetProperty().
 * @ghidraAddress BRD3D: 0x10078c80
 * @ghidraAddress BRGlide: 0x10071f70
 */
HRESULT SetJoystickPropertyDword(
    IDirectInputDevice2A *pDevice, REFGUID guidProperty, DWORD dwObject, DWORD dwHow, DWORD dwData);

/**
 * Set a property of a DIPROPRANGE on a DirectInput device.
 *
 * @param pDevice The device.
 * @param guidProperty The property.
 * @param dwObject The object of the device, or 0 for the device.
 * @param dwHow How dwObject is read.
 * @param lMin The low end of the range.
 * @param lMax The high end of the range.
 * @return The result of SetProperty().
 * @ghidraAddress BRD3D: 0x10078c30
 * @ghidraAddress BRGlide: 0x10071f20
 */
HRESULT SetJoystickAxisRange(IDirectInputDevice2A *pDevice,
                             REFGUID guidProperty,
                             DWORD dwObject,
                             DWORD dwHow,
                             LONG lMin,
                             LONG lMax);

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

/** The number of keys of the state of a keyboard. */
#define KEYBOARD_KEY_COUNT 256

/** The state of a mouse the game polls. The layout is not recovered yet. */
struct MouseInputState {
    char abUnknown0[0x1c]; /* +0x00 */
};

/**
 * The two states of the keyboard, the current one and the one before.
 *
 * @ghidraAddress BRD3D: 0x118ab8b8
 * @ghidraAddress BRGlide: 0x118ee9d0
 */
extern unsigned char g_aabKeyboardStateBuffers[2][KEYBOARD_KEY_COUNT];

/**
 * The entry of g_aabKeyboardStateBuffers that is current.
 *
 * @ghidraAddress BRD3D: 0x118abad8
 * @ghidraAddress BRGlide: 0x118eebf0
 */
extern int g_dwKeyboardStateCurIndex;

/**
 * The entry of g_aabKeyboardStateBuffers from before.
 *
 * @ghidraAddress BRD3D: 0x118ab8b4
 * @ghidraAddress BRGlide: 0x118ee9cc
 */
extern int g_dwKeyboardStatePrevIndex;

/**
 * The two states of the game controller, the current one and the one before.
 *
 * @ghidraAddress BRD3D: 0x118abae0
 * @ghidraAddress BRGlide: 0x118eebf8
 */
extern DIJOYSTATE2 g_aJoystickStateBuffers[2];

/**
 * The entry of g_aJoystickStateBuffers that is current.
 *
 * @ghidraAddress BRD3D: 0x118abab8
 * @ghidraAddress BRGlide: 0x118eebd0
 */
extern int g_dwJoystickStateCurIndex;

/**
 * The entry of g_aJoystickStateBuffers from before.
 *
 * @ghidraAddress BRD3D: 0x118abd7c
 * @ghidraAddress BRGlide: 0x118eee94
 */
extern int g_dwJoystickStatePrevIndex;

/**
 * The two states of the mouse, the current one and the one before.
 *
 * @ghidraAddress BRD3D: 0x118abd38
 * @ghidraAddress BRGlide: 0x118eee50
 */
extern MouseInputState g_aMouseStateBuffers[2];

/**
 * The entry of g_aMouseStateBuffers that is current.
 *
 * @ghidraAddress BRD3D: 0x118abd80
 * @ghidraAddress BRGlide: 0x118eee98
 */
extern int g_dwMouseStateCurIndex;

/**
 * The entry of g_aMouseStateBuffers from before.
 *
 * @ghidraAddress BRD3D: 0x118abad4
 * @ghidraAddress BRGlide: 0x118eebec
 */
extern int g_dwMouseStatePrevIndex;

/**
 * The keys of the keyboard the game read last, as DirectInput reports them.
 *
 * @ghidraAddress BRD3D: 0x10aa3288
 * @ghidraAddress BRGlide: 0x10ac65e8
 */
extern unsigned char g_abKeyboardState[KEYBOARD_KEY_COUNT];

/**
 * Whether each key went down since the keyboard was read before.
 *
 * @ghidraAddress BRD3D: 0x10aa2a80
 * @ghidraAddress BRGlide: 0x10ac5de0
 */
extern int g_adwKeyPressedEdge[KEYBOARD_KEY_COUNT];

/**
 * Whether each key was down when the keyboard was read before.
 *
 * @ghidraAddress BRD3D: 0x10aa2e88
 * @ghidraAddress BRGlide: 0x10ac61e8
 */
extern int g_adwKeyPrevDown[KEYBOARD_KEY_COUNT];

/** The number of buttons of the mouse the game reads. */
#define MOUSE_BUTTON_COUNT 4

/**
 * Whether each button of the mouse was down when the mouse was read before.
 *
 * @ghidraAddress BRD3D: 0x10aa3388
 * @ghidraAddress BRGlide: 0x10ac66e8
 */
extern int g_adwMouseButtonPrev[MOUSE_BUTTON_COUNT];

/**
 * Whether each button of the mouse is down.
 *
 * @ghidraAddress BRD3D: 0x10aa33c0
 * @ghidraAddress BRGlide: 0x10ac6720
 */
extern int g_adwMouseButtonRaw[MOUSE_BUTTON_COUNT];

/**
 * Whether each button of the mouse went down since the mouse was read before.
 *
 * @ghidraAddress BRD3D: 0x10aa33d0
 * @ghidraAddress BRGlide: 0x10ac6730
 */
extern int g_adwMouseButtonPressed[MOUSE_BUTTON_COUNT];

/**
 * The width of the screen the cursor of the mouse moves in. Its name is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10aa33b8
 * @ghidraAddress BRGlide: 0x10ac6718
 */
extern int g_dwEntityHalfWidth;

/**
 * The height of the screen the cursor of the mouse moves in. Its name is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10aa33b4
 * @ghidraAddress BRGlide: 0x10ac6714
 */
extern int g_dwEntityHalfHeight;

/**
 * Find the buttons of the mouse that went down since the mouse was read before.
 *
 * @ghidraAddress BRD3D: 0x1005fff0
 * @ghidraAddress BRGlide: 0x10059060
 */
void UpdateMouseButtonEdges(void);

/**
 * Put the cursor of the mouse in the centre of the screen and clear g_MouseMenuState.
 *
 * @param nUnused Not read.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10060210
 * @ghidraAddress BRGlide: 0x10059280
 */
bool WINAPI ResetMouseCursorToCenter(int nUnused);

/** The control of a button of a game controller GetPressedJoystickControl() reports. */
#define JOYSTICK_CONTROL_BUTTON 0x100
/** The control of the left of the X axis of a game controller. */
#define JOYSTICK_CONTROL_X_NEGATIVE 0x8000
/** The control of the right of the X axis of a game controller. */
#define JOYSTICK_CONTROL_X_POSITIVE 0x8100
/** The control of the top of the Y axis of a game controller. */
#define JOYSTICK_CONTROL_Y_NEGATIVE 0x8200
/** The control of the bottom of the Y axis of a game controller. */
#define JOYSTICK_CONTROL_Y_POSITIVE 0x8300

/**
 * Read the buttons of the mouse and find the first one that went down.
 *
 * @return The button, or -1 for none or without a mouse.
 * @ghidraAddress BRD3D: 0x10077310
 * @ghidraAddress BRGlide: 0x100705f0
 */
int GetPressedMouseButton(void);

/**
 * Read the game controller and find a control that is pressed, to bind it. A game controller
 * that does not exist is created as the first one.
 *
 * @param pwControl Receives JOYSTICK_CONTROL_BUTTON for a button, or the control of an axis
 * pushed past 50.
 * @return The button pressed, 0 for an axis, -1 for no control, or 1 without a game controller.
 * @ghidraAddress BRD3D: 0x10077200
 * @ghidraAddress BRGlide: 0x100704e0
 */
int GetPressedJoystickControl(unsigned *pwControl);

/**
 * Clear both states of the mouse.
 *
 * @ghidraAddress BRD3D: 0x100770c0
 * @ghidraAddress BRGlide: 0x100703a0
 */
void ResetMouseState(void);

/**
 * Add a user of the keyboard. The first user clears the states of the keyboard, the game
 * controller, and the mouse, and creates and acquires the device of the system keyboard for the
 * main window, shared in the foreground.
 *
 * @return Whether the keyboard device exists.
 * @ghidraAddress BRD3D: 0x100770f0
 * @ghidraAddress BRGlide: 0x100703d0
 */
bool InitializeKeyboardDevice(void);

/**
 * Read the state of the keyboard, acquiring the device again and reading once more when the input
 * was lost.
 *
 * @param pabState Receives the state of each key.
 * @return The result of the last read, or 1 without a device.
 * @ghidraAddress BRD3D: 0x100771b0
 * @ghidraAddress BRGlide: 0x10070490
 */
HRESULT ReadKeyboardState(unsigned char *pabState);

/**
 * Clear the state of the keyboard. Only the first 64 entries of g_adwKeyPressedEdge and
 * g_adwKeyPrevDown are cleared, as in the original.
 *
 * @ghidraAddress BRD3D: 0x1005ff30
 * @ghidraAddress BRGlide: 0x10058fa0
 */
void ClearKeyboardStateBuffers(void);

/**
 * Find the keys that went down since the keyboard was read before.
 *
 * @ghidraAddress BRD3D: 0x1005ff60
 * @ghidraAddress BRGlide: 0x10058fd0
 */
void UpdateKeyboardEdgeState(void);

/**
 * Whether the game controller has force feedback.
 *
 * @ghidraAddress BRD3D: 0x118abdbc
 * @ghidraAddress BRGlide: 0x118eeed4
 */
extern bool g_bForceFeedbackAvailable;

/**
 * The condition effect of force feedback, a spring on X and Y.
 *
 * @ghidraAddress BRD3D: 0x118ab880
 * @ghidraAddress BRGlide: 0x118ee998
 */
extern DIEFFECT g_DiConditionEffect;

/**
 * The springs of X and Y of g_DiConditionEffect.
 *
 * @ghidraAddress BRD3D: 0x118abd08
 * @ghidraAddress BRGlide: 0x118eee20
 */
extern DICONDITION g_aDiConditionParams[2];

/**
 * The direction of g_DiConditionEffect.
 *
 * @ghidraAddress BRD3D: 0x118abdf0
 * @ghidraAddress BRGlide: 0x118eef08
 */
extern LONG g_alConditionEffectDirection[2];

/**
 * The strength of the spring of X once the condition effect exists.
 *
 * @ghidraAddress BRD3D: 0x118abd78
 * @ghidraAddress BRGlide: 0x118eee90
 */
extern int g_dwForceFeedbackMagnitude;

/**
 * Whether the condition effect does not play. Set once the effect is created.
 *
 * @ghidraAddress BRD3D: 0x118abdf8
 * @ghidraAddress BRGlide: 0x118eef10
 */
extern bool g_bForceFeedbackForceStopped;

/**
 * The jolt effect of force feedback, a square wave.
 *
 * @ghidraAddress BRD3D: 0x118abd88
 * @ghidraAddress BRGlide: 0x118eeea0
 */
extern DIEFFECT g_DiJoltEffect;

/**
 * The square wave of g_DiJoltEffect.
 *
 * @ghidraAddress BRD3D: 0x118abac0
 * @ghidraAddress BRGlide: 0x118eebd8
 */
extern DIPERIODIC g_DiJoltPeriodic;

/**
 * The duration of the jolt in microseconds, 125000.
 *
 * @ghidraAddress BRD3D: 0x100bd438
 * @ghidraAddress BRGlide: 0x100bcc40
 */
extern DWORD g_dwForceFeedbackDuration;

/**
 * The direction of the jolt, along the negative X axis.
 *
 * @ghidraAddress BRD3D: 0x100bd430
 * @ghidraAddress BRGlide: 0x100bcc38
 */
extern LONG g_adwForceFeedbackDirection[2];

/**
 * Create the condition effect, a spring on X and Y that lasts until it stops, and the jolt
 * effect, a square wave, of force feedback of the game controller.
 *
 * @param nCoefficientX The strength of the spring of X, 1000 as the game passes it.
 * @param nCoefficientY The strength of the spring of Y, 8000 as the game passes it.
 * @return The result of the creation of the jolt.
 * @ghidraAddress BRD3D: 0x10079390
 * @ghidraAddress BRGlide: 0x10072680
 */
HRESULT CreateForceFeedbackEffects(int nCoefficientX, int nCoefficientY);

/**
 * Create the device of the first attached game controller DirectInput enumerates, as its second
 * interface, with the cooperation the enumeration passes and the format of DIJOYSTATE2. A step
 * that fails is written to the debugger and releases the device.
 *
 * @param pInstance The game controller.
 * @param pContext The flags of cooperation.
 * @return Always DIENUM_STOP.
 * @ghidraAddress BRD3D: 0x100790e0
 * @ghidraAddress BRGlide: 0x100723d0
 */
BOOL CALLBACK EnumJoysticksCallback_CreateDevice(const DIDEVICEINSTANCEA *pInstance,
                                                 void *pContext);

/**
 * Add a user of the game controller of dwInputDeviceType. The first user creates the device,
 * exclusively with force feedback when it is on and available and shared otherwise, and sets the
 * range of X and Y to -128 to 128 with no dead zone. A property that is not set releases the
 * device.
 *
 * @return dwInputDeviceType of g_GlobalRallyConfig, or 0 without a game controller.
 * @ghidraAddress BRD3D: 0x100791d0
 * @ghidraAddress BRGlide: 0x100724c0
 */
int InitializeJoystickDevice(void);

/**
 * Create and release the first game controller with force feedback on, to find whether one is
 * attached. The type of the input device, its bindings, and force feedback are restored after.
 *
 * @ghidraAddress BRD3D: 0x100795d0
 * @ghidraAddress BRGlide: 0x100728c0
 */
void ProbeJoystickDevice(void);

/**
 * Read the keyboard into g_abKeyboardState and find the keys that went down.
 *
 * @ghidraAddress BRD3D: 0x1005ffb0
 * @ghidraAddress BRGlide: 0x10059020
 */
void PollKeyboardState(void);

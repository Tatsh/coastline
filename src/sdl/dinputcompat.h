#pragma once

/**
 * @file
 * The part of DirectInput 5 that the input of the PC release calls, with the values of the
 * DirectInput headers, on the keyboard, the mouse, and the gamepads of SDL. The first gamepad is
 * the joystick, and its effects are the rumble of the gamepad.
 */

#include <cstdint>

#include "sdl/win32types.h"

#define DIRECTINPUT_VERSION 0x0500

#define DI_OK ((HRESULT)0)                    /*!< Success. */
#define DIERR_INPUTLOST ((HRESULT)0x8007001E) /*!< The device must be acquired again. */
#define DIERR_GENERIC ((HRESULT)0x80004005)   /*!< A failure. */
#define DIENUM_STOP 0                         /*!< Stop an enumeration. */
#define DIENUM_CONTINUE 1                     /*!< Continue an enumeration. */
#define DISCL_EXCLUSIVE 0x00000001            /*!< Exclusive access. */
#define DISCL_NONEXCLUSIVE 0x00000002         /*!< Shared access. */
#define DISCL_FOREGROUND 0x00000004           /*!< Access in the foreground. */
#define DISCL_BACKGROUND 0x00000008           /*!< Access in the background. */
#define DIPH_DEVICE 0                         /*!< A property of the device. */
#define DIPH_BYOFFSET 1                       /*!< A property of an object by its offset. */
#define DIPH_BYID 2                           /*!< A property of an object by its identifier. */
#define DIDEVTYPE_JOYSTICK 4                  /*!< Joysticks. */
#define DIEDFL_ATTACHEDONLY 0x00000001        /*!< Attached devices only. */
#define DIEDFL_FORCEFEEDBACK 0x00000100       /*!< Devices with force feedback only. */
#define DIEB_NOTRIGGER 0xFFFFFFFF             /*!< An effect no button triggers. */
#define DIPROPAUTOCENTER_OFF 0                /*!< No centring spring. */
#define DIEP_DURATION 0x00000001              /*!< The duration of an effect. */
#define DIEP_TYPESPECIFICPARAMS 0x00000100    /*!< The parameters of the type of an effect. */
#define DIEP_DIRECTION 0x00000040             /*!< The direction of an effect. */
#define DIEP_START 0x20000000                 /*!< Start the effect. */
#define DIEFF_CARTESIAN 0x00000010            /*!< Cartesian directions. */
#define DIEFF_OBJECTOFFSETS 0x00000002        /*!< Axes by offset. */
#define DIJOFS_X 0                            /*!< The offset of the x axis. */
#define DIJOFS_Y 4                            /*!< The offset of the y axis. */
#define DIJOFS_Z 8                            /*!< The offset of the z axis. */
#define DI_FFNOMINALMAX 10000                 /*!< The greatest magnitude of an effect. */
#define DI_SECONDS 1000000                    /*!< Microseconds a second. */
#define INFINITE_EFFECT 0xFFFFFFFF            /*!< An effect without an end. */
#define VK_ESCAPE 0x1B                        /*!< The escape key. */

/** The keys of DirectInput, the scan codes of the keyboard, that the game names. */
enum DirectInputKey : unsigned {
    DIK_ESCAPE = 0x01,      /*!< Escape. */
    DIK_RETURN = 0x1C,      /*!< Enter. */
    DIK_NUMPADENTER = 0x9C, /*!< Enter of the numeric keypad. */
    DIK_F5 = 0x3F,          /*!< F5. */
    DIK_F11 = 0x57,         /*!< F11. */
    DIK_F12 = 0x58,         /*!< F12. */
    DIK_UP = 0xC8,          /*!< The up arrow. */
    DIK_LEFT = 0xCB,        /*!< The left arrow. */
    DIK_RIGHT = 0xCD,       /*!< The right arrow. */
    DIK_DOWN = 0xD0,        /*!< The down arrow. */
};

/** The header of a property. */
struct DIPROPHEADER {
    DWORD dwSize;       /*!< The size of the whole property. */
    DWORD dwHeaderSize; /*!< The size of the header. */
    DWORD dwObj;        /*!< The object. */
    DWORD dwHow;        /*!< How dwObj names it, a DIPH_ value. */
};

/** A property of one value. */
struct DIPROPDWORD {
    DIPROPHEADER diph; /*!< The header. */
    DWORD dwData;      /*!< The value. */
};

/** A property of a range. */
struct DIPROPRANGE {
    DIPROPHEADER diph; /*!< The header. */
    LONG lMin;         /*!< The least value. */
    LONG lMax;         /*!< The greatest value. */
};

/** The state of a mouse. */
struct DIMOUSESTATE {
    LONG lX;            /*!< The motion along x. */
    LONG lY;            /*!< The motion along y. */
    LONG lZ;            /*!< The motion of the wheel. */
    BYTE rgbButtons[4]; /*!< The buttons, 0x80 when down. */
};

/** The state of a joystick. */
struct DIJOYSTATE2 {
    LONG lX;              /*!< The x axis. */
    LONG lY;              /*!< The y axis. */
    LONG lZ;              /*!< The z axis. */
    LONG lRx;             /*!< The rotation about x. */
    LONG lRy;             /*!< The rotation about y. */
    LONG lRz;             /*!< The rotation about z. */
    LONG rglSlider[2];    /*!< The sliders. */
    DWORD rgdwPOV[4];     /*!< The hats. */
    BYTE rgbButtons[128]; /*!< The buttons, 0x80 when down. */
    LONG lVX;             /*!< The velocity along x. */
    LONG lVY;             /*!< The velocity along y. */
    LONG lVZ;             /*!< The velocity along z. */
    LONG lVRx;            /*!< The angular velocity about x. */
    LONG lVRy;            /*!< The angular velocity about y. */
    LONG lVRz;            /*!< The angular velocity about z. */
    LONG rglVSlider[2];   /*!< The velocities of the sliders. */
    LONG lAX;             /*!< The acceleration along x. */
    LONG lAY;             /*!< The acceleration along y. */
    LONG lAZ;             /*!< The acceleration along z. */
    LONG lARx;            /*!< The angular acceleration about x. */
    LONG lARy;            /*!< The angular acceleration about y. */
    LONG lARz;            /*!< The angular acceleration about z. */
    LONG rglASlider[2];   /*!< The accelerations of the sliders. */
    LONG lFX;             /*!< The force along x. */
    LONG lFY;             /*!< The force along y. */
    LONG lFZ;             /*!< The force along z. */
    LONG lFRx;            /*!< The torque about x. */
    LONG lFRy;            /*!< The torque about y. */
    LONG lFRz;            /*!< The torque about z. */
    LONG rglFSlider[2];   /*!< The forces of the sliders. */
};

/** A device the enumeration found. */
struct DIDEVICEINSTANCEA {
    DWORD dwSize;              /*!< The size of the structure. */
    GUID guidInstance;         /*!< The instance. */
    GUID guidProduct;          /*!< The product. */
    DWORD dwDevType;           /*!< The type. */
    CHAR tszInstanceName[260]; /*!< The name of the instance. */
    CHAR tszProductName[260];  /*!< The name of the product. */
    GUID guidFFDriver;         /*!< The force feedback driver. */
    WORD wUsagePage;           /*!< The HID usage page. */
    WORD wUsage;               /*!< The HID usage. */
};

/** The envelope of an effect. */
struct DIENVELOPE {
    DWORD dwSize;        /*!< The size of the structure. */
    DWORD dwAttackLevel; /*!< The level at the start. */
    DWORD dwAttackTime;  /*!< The time of the attack. */
    DWORD dwFadeLevel;   /*!< The level at the end. */
    DWORD dwFadeTime;    /*!< The time of the fade. */
};

/** A condition, such as a spring, along an axis. */
struct DICONDITION {
    LONG lOffset;               /*!< The centre. */
    LONG lPositiveCoefficient;  /*!< The coefficient above the centre. */
    LONG lNegativeCoefficient;  /*!< The coefficient below the centre. */
    DWORD dwPositiveSaturation; /*!< The greatest force above the centre. */
    DWORD dwNegativeSaturation; /*!< The greatest force below the centre. */
    LONG lDeadBand;             /*!< The dead band. */
};

/** A periodic force. */
struct DIPERIODIC {
    DWORD dwMagnitude; /*!< The magnitude. */
    LONG lOffset;      /*!< The offset. */
    DWORD dwPhase;     /*!< The phase. */
    DWORD dwPeriod;    /*!< The period. */
};

/** An effect. */
struct DIEFFECT {
    DWORD dwSize;                  /*!< The size of the structure. */
    DWORD dwFlags;                 /*!< The DIEFF_ flags. */
    DWORD dwDuration;              /*!< The duration in microseconds. */
    DWORD dwSamplePeriod;          /*!< The period of the samples. */
    DWORD dwGain;                  /*!< The gain, to DI_FFNOMINALMAX. */
    DWORD dwTriggerButton;         /*!< The button that triggers it. */
    DWORD dwTriggerRepeatInterval; /*!< The interval of the repeats. */
    DWORD cAxes;                   /*!< The axes. */
    DWORD *rgdwAxes;               /*!< The axes. */
    LONG *rglDirection;            /*!< The direction. */
    DIENVELOPE *lpEnvelope;        /*!< The envelope. */
    DWORD cbTypeSpecificParams;    /*!< The size of the parameters of the type. */
    LPVOID lpvTypeSpecificParams;  /*!< The parameters of the type. */
};

/** The format of the data of a device, which only names the device. */
struct DIDATAFORMAT {
    int nKind; /*!< 0 for the keyboard, 1 for the mouse, 2 for a joystick. */
};

extern const DIDATAFORMAT c_dfDIKeyboard;  /*!< The format of the keyboard. */
extern const DIDATAFORMAT c_dfDIMouse;     /*!< The format of the mouse. */
extern const DIDATAFORMAT c_dfDIJoystick2; /*!< The format of a joystick. */

extern const GUID GUID_SysKeyboard;              /*!< The keyboard. */
extern const GUID GUID_SysMouse;                 /*!< The mouse. */
extern const GUID GUID_Square;                   /*!< A square wave. */
extern const GUID GUID_Spring;                   /*!< A spring. */
extern const GUID IID_IDirectInputDevice2A;      /*!< The second interface of a device. */
extern const GUID DIPROP_RANGE_GUID;             /*!< The range of an axis. */
extern const GUID DIPROP_DEADZONE_GUID;          /*!< The dead zone of an axis. */
extern const GUID DIPROP_AUTOCENTER_GUID;        /*!< The centring spring. */
#define DIPROP_RANGE DIPROP_RANGE_GUID           /*!< The range of an axis. */
#define DIPROP_DEADZONE DIPROP_DEADZONE_GUID     /*!< The dead zone of an axis. */
#define DIPROP_AUTOCENTER DIPROP_AUTOCENTER_GUID /*!< The centring spring. */

/** An effect of a joystick, the rumble of an SDL gamepad. */
class IDirectInputEffect {
public:
    /** @return DI_OK, with the effect's magnitude and duration taken, and started on DIEP_START. */
    HRESULT SetParameters(const DIEFFECT *effect, DWORD flags);
    /** @return DI_OK, with the rumble started. */
    HRESULT Start(DWORD iterations, DWORD flags);
    /** @return DI_OK, with the rumble stopped. */
    HRESULT Stop();
    /** @return 0, with the effect deleted. */
    ULONG Release();

private:
    friend class IDirectInputDevice2A;
    float m_strength = 0.0f;
    DWORD m_durationMs = 0;
};

/** A device of DirectInput: the keyboard, the mouse, or a joystick. */
class IDirectInputDeviceA {
public:
    virtual ~IDirectInputDeviceA() = default;
    /** @return DI_OK, with the state of the device in its format. */
    HRESULT GetDeviceState(DWORD size, LPVOID data);
    /** @return DI_OK. */
    HRESULT Acquire() {
        return DI_OK;
    }
    /** @return DI_OK. */
    HRESULT Unacquire() {
        return DI_OK;
    }
    /** @return DI_OK, with the kind of device the format names. */
    HRESULT SetDataFormat(const DIDATAFORMAT *format);
    /** @return DI_OK. */
    HRESULT SetCooperativeLevel(HWND window, DWORD flags) {
        return DI_OK;
    }
    /** @return DI_OK, with the range of the axes of a joystick taken. */
    HRESULT SetProperty(REFGUID property, const DIPROPHEADER *header);
    /** @return DI_OK, with this device as its second interface. */
    HRESULT QueryInterface(REFGUID iid, void **object);
    /** @return 0, with the device deleted. */
    ULONG Release();

protected:
    int m_kind = 0;
    LONG m_min = 0;
    LONG m_max = 65535;
};

/** The second interface of a device, which polls and has effects. */
class IDirectInputDevice2A : public IDirectInputDeviceA {
public:
    /** @return DI_OK. */
    HRESULT Poll() {
        return DI_OK;
    }
    /** @return DI_OK, with the effect created. */
    HRESULT CreateEffect(REFGUID type,
                         const DIEFFECT *parameters,
                         IDirectInputEffect **effect,
                         void *outer);
};

/** The enumeration callback of devices. */
typedef BOOL(CALLBACK *LPDIENUMDEVICESCALLBACKA)(const DIDEVICEINSTANCEA *instance, LPVOID context);

/** DirectInput, which creates the devices. */
class IDirectInputA {
public:
    /** @return DI_OK, with the device of the GUID, or DIERR_GENERIC for an unknown one. */
    HRESULT CreateDevice(REFGUID guid, IDirectInputDeviceA **device, void *outer);
    /** @return DI_OK, with the callback called for each gamepad of SDL as a joystick. */
    HRESULT EnumDevices(DWORD type, LPDIENUMDEVICESCALLBACKA callback, LPVOID context, DWORD flags);
    /** @return 0, with DirectInput deleted. */
    ULONG Release();
};

/**
 * Create DirectInput.
 *
 * @return DI_OK.
 */
HRESULT DirectInputCreateA(HINSTANCE instance, DWORD version, IDirectInputA **input, void *outer);

/**
 * Report whether a key is down, with the low bit set when it went down since the last call for
 * the key.
 *
 * @param key A virtual key: an upper-case letter, a digit, or VK_ESCAPE.
 * @return 0x8000 while the key is down, with 1 for a new press.
 */
short GetAsyncKeyState(int key);

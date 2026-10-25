#include "platform/win/input.h"

#include <cstring>
#include <new>

#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/settings.h"

IDirectInputA *g_pDirectInput;
int g_dwDirectInputRefCount;
MouseDevice *g_pMouseDevice;
IDirectInputDeviceA *g_pKeyboardDevice;
int g_dwKeyboardDeviceRefCount;
IDirectInputDevice2A *g_pJoystickDevice;
int g_dwJoystickDeviceRefCount;
IDirectInputEffect *g_pJoltEffect;
IDirectInputEffect *g_pConditionEffect;

unsigned char g_aabKeyboardStateBuffers[2][KEYBOARD_KEY_COUNT];
int g_dwKeyboardStateCurIndex;
int g_dwKeyboardStatePrevIndex;
DIJOYSTATE2 g_aJoystickStateBuffers[2];
int g_dwJoystickStateCurIndex;
int g_dwJoystickStatePrevIndex;
MouseInputState g_aMouseStateBuffers[2];
int g_dwMouseStateCurIndex;
int g_dwMouseStatePrevIndex;
unsigned char g_abKeyboardState[KEYBOARD_KEY_COUNT];
int g_adwKeyPressedEdge[KEYBOARD_KEY_COUNT];
int g_adwKeyPrevDown[KEYBOARD_KEY_COUNT];

// The bit of a key in the state of DirectInput that is set while the key is down.
#define KEY_DOWN_BIT 7
// The entries of the edges of the keys ClearKeyboardStateBuffers() clears.
#define KEY_EDGE_CLEAR_COUNT 64

int g_adwMouseButtonPrev[MOUSE_BUTTON_COUNT];
int g_adwMouseButtonRaw[MOUSE_BUTTON_COUNT];
int g_adwMouseButtonPressed[MOUSE_BUTTON_COUNT];
int g_dwEntityHalfWidth;
int g_dwEntityHalfHeight;
int g_adwEntityMetricScratch[MOUSE_METRIC_COUNT];

void UpdateMouseButtonEdges(void) {
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        g_adwMouseButtonPressed[i] = (g_adwMouseButtonPrev[i] == 0) & g_adwMouseButtonRaw[i];
        g_adwMouseButtonPrev[i] = g_adwMouseButtonRaw[i];
    }
}

bool WINAPI ResetMouseCursorToCenter(int) {
    g_dwEntityHalfWidth = g_dwConfigFramebufferWidth;
    g_pMouseDevice->nCursorX = g_dwConfigFramebufferWidth / 2;
    g_dwEntityHalfHeight = g_dwConfigFramebufferHeight;
    g_pMouseDevice->nCursorY = g_dwConfigFramebufferHeight / 2;
    memset(g_adwEntityMetricScratch, 0, sizeof(g_adwEntityMetricScratch));
    return true;
}

// The input device type of the first game controller.
#define INPUT_DEVICE_FIRST_CONTROLLER 2
// The bit of a button in a state of DirectInput that is set while the button is down.
#define BUTTON_DOWN_MASK 0x80
// The distance an axis is pushed for GetPressedJoystickControl() to report it.
#define JOYSTICK_AXIS_THRESHOLD 50

int GetPressedMouseButton(void) {
    DIMOUSESTATE state;
    HRESULT hResult;

    if (g_pMouseDevice == nullptr || g_pMouseDevice->pDevice == nullptr) {
        return -1;
    }
    for (int &bDown : g_adwMouseButtonRaw) {
        bDown = 0;
    }
    hResult = g_pMouseDevice->pDevice->GetDeviceState(sizeof(state), &state);
    // A lost device is acquired again and the state that was not read is used.
    if (hResult == DIERR_INPUTLOST) {
        g_pMouseDevice->pDevice->Acquire();
    }
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (state.rgbButtons[i] & BUTTON_DOWN_MASK) {
            g_adwMouseButtonRaw[i] = 1;
        }
    }
    UpdateMouseButtonEdges();
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (g_adwMouseButtonPressed[i] != 0) {
            return i;
        }
    }
    return -1;
}

int GetPressedJoystickControl(unsigned *pwControl) {
    DIJOYSTATE2 state;
    HRESULT hResult;

    if (g_pJoystickDevice == nullptr) {
        g_GlobalRallyConfig.dwInputDeviceType = INPUT_DEVICE_FIRST_CONTROLLER;
        g_GlobalRallyConfig.SelectActiveInputBindings();
        InitializeJoystickDevice();
        if (g_pJoystickDevice == nullptr) {
            return 1;
        }
    }
    g_pJoystickDevice->Poll();
    hResult = g_pJoystickDevice->GetDeviceState(sizeof(state), &state);
    if (hResult == DIERR_INPUTLOST) {
        TryAcquireJoystickDevice();
    }
    for (int i = 0; i < static_cast<int>(sizeof(state.rgbButtons)); i++) {
        if (state.rgbButtons[i] & BUTTON_DOWN_MASK) {
            *pwControl = JOYSTICK_CONTROL_BUTTON;
            return i;
        }
    }
    if (state.lX < -JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_X_NEGATIVE;
    } else if (state.lX > JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_X_POSITIVE;
    } else if (state.lY < -JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_Y_NEGATIVE;
    } else if (state.lY > JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_Y_POSITIVE;
    } else {
        return -1;
    }
    return 0;
}

void ResetMouseState(void) {
    memset(g_aMouseStateBuffers, 0, sizeof(g_aMouseStateBuffers));
    g_dwMouseStatePrevIndex = 0;
    g_dwMouseStateCurIndex = 1;
}

bool InitializeKeyboardDevice(void) {
    if (++g_dwKeyboardDeviceRefCount != 1) {
        return true;
    }
    g_dwKeyboardStateCurIndex = 1;
    memset(g_aabKeyboardStateBuffers, 0, sizeof(g_aabKeyboardStateBuffers));
    memset(g_aJoystickStateBuffers, 0, sizeof(g_aJoystickStateBuffers));
    g_dwKeyboardStatePrevIndex = 0;
    g_dwJoystickStatePrevIndex = 0;
    g_dwJoystickStateCurIndex = 1;
    ResetMouseState();
    if (FAILED(g_pDirectInput->CreateDevice(GUID_SysKeyboard, &g_pKeyboardDevice, nullptr)) ||
        FAILED(g_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard)) ||
        FAILED(g_pKeyboardDevice->SetCooperativeLevel(g_hMainWindow,
                                                      DISCL_NONEXCLUSIVE | DISCL_FOREGROUND))) {
        return false;
    }
    if (g_pKeyboardDevice != nullptr) {
        g_pKeyboardDevice->Acquire();
    }
    return true;
}

HRESULT ReadKeyboardState(unsigned char *pabState) {
    HRESULT hResult;

    if (g_pKeyboardDevice == nullptr) {
        return 1;
    }
    hResult = g_pKeyboardDevice->GetDeviceState(KEYBOARD_KEY_COUNT, pabState);
    if (hResult == DIERR_INPUTLOST) {
        hResult = g_pKeyboardDevice->Acquire();
        if (SUCCEEDED(hResult)) {
            hResult = g_pKeyboardDevice->GetDeviceState(KEYBOARD_KEY_COUNT, pabState);
        }
    }
    return hResult;
}

void ClearKeyboardStateBuffers(void) {
    memset(g_abKeyboardState, 0, sizeof(g_abKeyboardState));
    memset(g_adwKeyPressedEdge, 0, KEY_EDGE_CLEAR_COUNT * sizeof(g_adwKeyPressedEdge[0]));
    memset(g_adwKeyPrevDown, 0, KEY_EDGE_CLEAR_COUNT * sizeof(g_adwKeyPrevDown[0]));
}

void UpdateKeyboardEdgeState(void) {
    for (int i = 0; i < KEYBOARD_KEY_COUNT; i++) {
        int bDown = (g_abKeyboardState[i] >> KEY_DOWN_BIT) & 1;

        g_adwKeyPressedEdge[i] = (g_adwKeyPrevDown[i] == 0) & bDown;
        g_adwKeyPrevDown[i] = bDown;
    }
}

void PollKeyboardState(void) {
    if (SUCCEEDED(ReadKeyboardState(g_abKeyboardState))) {
        UpdateKeyboardEdgeState();
    }
}

MouseDevice::MouseDevice() {
    pDevice = nullptr;
    for (int &nState : anState) {
        nState = 0;
    }
}

int ShowInfoMessageBox(HWND hWnd, HRESULT, const char *pszText) {
    return MessageBoxA(hWnd, pszText, GetStringResource(0xaa), MB_OK);
}

bool MouseDevice::CreateMouseDevice(HWND hWnd) {
    HRESULT hResult;

    hResult = g_pDirectInput->CreateDevice(GUID_SysMouse, &pDevice, nullptr);
    if (FAILED(hResult)) {
        ShowInfoMessageBox(hWnd, hResult, GetStringResource(0xac));
        return false;
    }
    hResult = pDevice->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hResult)) {
        ShowInfoMessageBox(hWnd, hResult, GetStringResource(0xad));
        return false;
    }
    hResult = pDevice->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if (FAILED(hResult)) {
        ShowInfoMessageBox(hWnd, hResult, GetStringResource(0xae));
        return false;
    }
    return true;
}

void MouseDevice::ReleaseMouseDevice() {
    if (pDevice != nullptr) {
        pDevice->Unacquire();
        pDevice->Release();
        pDevice = nullptr;
    }
}

bool InitializeDirectInput(void) {
    if (++g_dwDirectInputRefCount != 1) {
        return true;
    }
    if (FAILED(DirectInputCreateA(g_hAppInstance, DIRECTINPUT_VERSION, &g_pDirectInput, nullptr))) {
        MessageBoxA(
            g_hMainWindow, GetStringResource(0x127), GetStringResource(0x126), MB_ICONERROR);
        return false;
    }
    g_pMouseDevice = new (std::nothrow) MouseDevice();
    // The device is created through the pointer whether or not the allocation succeeded.
    g_pMouseDevice->CreateMouseDevice(g_hMainWindow);
    return true;
}

bool TryAcquireJoystickDevice(void) {
    if (g_pJoystickDevice == nullptr) {
        return false;
    }
    return SUCCEEDED(g_pJoystickDevice->Acquire());
}

HRESULT SetJoystickPropertyDword(IDirectInputDevice2A *pDevice,
                                 REFGUID guidProperty,
                                 DWORD dwObject,
                                 DWORD dwHow,
                                 DWORD dwData) {
    DIPROPDWORD property;

    property.diph.dwSize = sizeof(property);
    property.diph.dwHeaderSize = sizeof(property.diph);
    property.diph.dwObj = dwObject;
    property.diph.dwHow = dwHow;
    property.dwData = dwData;
    return pDevice->SetProperty(guidProperty, &property.diph);
}

HRESULT SetJoystickAxisRange(IDirectInputDevice2A *pDevice,
                             REFGUID guidProperty,
                             DWORD dwObject,
                             DWORD dwHow,
                             LONG lMin,
                             LONG lMax) {
    DIPROPRANGE property;

    property.diph.dwSize = sizeof(property);
    property.diph.dwHeaderSize = sizeof(property.diph);
    property.diph.dwObj = dwObject;
    property.diph.dwHow = dwHow;
    property.lMin = lMin;
    property.lMax = lMax;
    return pDevice->SetProperty(guidProperty, &property.diph);
}

bool g_bForceFeedbackAvailable;

// The range of an axis of the game controller.
#define JOYSTICK_AXIS_MIN -128
#define JOYSTICK_AXIS_MAX 128

BOOL CALLBACK EnumJoysticksCallback_CreateDevice(const DIDEVICEINSTANCEA *pInstance,
                                                 void *pContext) {
    IDirectInputDeviceA *pDevice;
    GUID guidInstance = pInstance->guidInstance;
    HRESULT hResult;
    const char *pszError;

    if (FAILED(g_pDirectInput->CreateDevice(guidInstance, &pDevice, nullptr))) {
        OutputDebugStringA("Error: Failed to create device.\n");
        return DIENUM_STOP;
    }
    hResult = pDevice->QueryInterface(IID_IDirectInputDevice2A,
                                      reinterpret_cast<void **>(&g_pJoystickDevice));
    pDevice->Release();
    if (FAILED(hResult)) {
        OutputDebugStringA("Error: Failed to obtain interface.\n");
        return DIENUM_STOP;
    }
    if (FAILED(g_pJoystickDevice->SetCooperativeLevel(
            g_hMainWindow, static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(pContext))))) {
        pszError = "Error: Failed to set cooperative level.\n";
    } else if (FAILED(g_pJoystickDevice->SetDataFormat(&c_dfDIJoystick2))) {
        pszError = "Error: Failed to set game device data format.\n";
    } else {
        return DIENUM_STOP;
    }
    OutputDebugStringA(pszError);
    g_pJoystickDevice->Release();
    g_pJoystickDevice = nullptr;
    return DIENUM_STOP;
}

// A property that is not set releases the game controller.
static inline int ReleaseJoystickAfterFailure(const char *pszError) {
    OutputDebugStringA(pszError);
    g_pJoystickDevice->Unacquire();
    g_pJoystickDevice->Release();
    g_pJoystickDevice = nullptr;
    return 0;
}

int InitializeJoystickDevice(void) {
    DIPROPDWORD autoCenter;

    if (g_GlobalRallyConfig.dwInputDeviceType == 0 || ++g_dwJoystickDeviceRefCount != 1) {
        return g_GlobalRallyConfig.dwInputDeviceType;
    }
    if (g_GlobalRallyConfig.bForceFeedbackEnabled &&
        g_pDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
                                    EnumJoysticksCallback_CreateDevice,
                                    reinterpret_cast<void *>(DISCL_EXCLUSIVE | DISCL_FOREGROUND),
                                    DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK) == DI_OK &&
        g_pJoystickDevice != nullptr) {
        g_bForceFeedbackAvailable = true;
        autoCenter.diph.dwSize = sizeof(autoCenter);
        autoCenter.diph.dwHeaderSize = sizeof(autoCenter.diph);
        autoCenter.diph.dwObj = 0;
        autoCenter.diph.dwHow = DIPH_DEVICE;
        autoCenter.dwData = DIPROPAUTOCENTER_OFF;
        if (FAILED(g_pJoystickDevice->SetProperty(DIPROP_AUTOCENTER, &autoCenter.diph))) {
            OutputDebugStringA("Error: Failed to change device property.\n");
        }
        TryAcquireJoystickDevice();
        CreateForceFeedbackEffects(1000, 8000);
    } else {
        g_pDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
                                    EnumJoysticksCallback_CreateDevice,
                                    reinterpret_cast<void *>(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND),
                                    DIEDFL_ATTACHEDONLY);
        TryAcquireJoystickDevice();
        g_bForceFeedbackAvailable = false;
    }
    if (g_pJoystickDevice == nullptr) {
        return 0;
    }
    if (FAILED(SetJoystickAxisRange(g_pJoystickDevice,
                                    DIPROP_RANGE,
                                    DIJOFS_X,
                                    DIPH_BYOFFSET,
                                    JOYSTICK_AXIS_MIN,
                                    JOYSTICK_AXIS_MAX))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_RANGE) FAILED\n");
    }
    if (FAILED(SetJoystickPropertyDword(
            g_pJoystickDevice, DIPROP_DEADZONE, DIJOFS_X, DIPH_BYOFFSET, 0))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_WORD) FAILED\n");
    }
    if (FAILED(SetJoystickAxisRange(g_pJoystickDevice,
                                    DIPROP_RANGE,
                                    DIJOFS_Y,
                                    DIPH_BYOFFSET,
                                    JOYSTICK_AXIS_MIN,
                                    JOYSTICK_AXIS_MAX))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_RANGE) FAILED\n");
    }
    if (FAILED(SetJoystickPropertyDword(
            g_pJoystickDevice, DIPROP_DEADZONE, DIJOFS_Y, DIPH_BYOFFSET, 0))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_WORD) FAILED\n");
    }
    return g_GlobalRallyConfig.dwInputDeviceType;
}

void ProbeJoystickDevice(void) {
    int dwInputDeviceType = g_GlobalRallyConfig.dwInputDeviceType;
    int bForceFeedbackEnabled = g_GlobalRallyConfig.bForceFeedbackEnabled;

    g_GlobalRallyConfig.dwInputDeviceType = INPUT_DEVICE_FIRST_CONTROLLER;
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_GlobalRallyConfig.bForceFeedbackEnabled = true;
    InitializeJoystickDevice();
    ReleaseJoystickDevice();
    g_GlobalRallyConfig.dwInputDeviceType = dwInputDeviceType;
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_GlobalRallyConfig.bForceFeedbackEnabled = bForceFeedbackEnabled;
}

void ReleaseKeyboardDevice(void) {
    if (--g_dwKeyboardDeviceRefCount < 0) {
        g_dwKeyboardDeviceRefCount = 0;
        return;
    }
    if (g_dwKeyboardDeviceRefCount == 0 && g_pKeyboardDevice != nullptr) {
        g_pKeyboardDevice->Unacquire();
        g_pKeyboardDevice->Release();
        g_pKeyboardDevice = nullptr;
    }
}

void ReleaseJoystickDevice(void) {
    if (--g_dwJoystickDeviceRefCount < 0) {
        g_dwJoystickDeviceRefCount = 0;
        return;
    }
    if (g_dwJoystickDeviceRefCount != 0) {
        return;
    }
    if (g_pJoltEffect != nullptr) {
        g_pJoltEffect->Release();
        g_pJoltEffect = nullptr;
    }
    if (g_pConditionEffect != nullptr) {
        g_pConditionEffect->Release();
        g_pConditionEffect = nullptr;
    }
    if (g_pJoystickDevice != nullptr) {
        g_pJoystickDevice->Unacquire();
        g_pJoystickDevice->Release();
        g_pJoystickDevice = nullptr;
    }
}

bool ShutdownDirectInput(void) {
    if (--g_dwDirectInputRefCount == 0 && g_pDirectInput != nullptr) {
        if (g_pMouseDevice != nullptr) {
            g_pMouseDevice->ReleaseMouseDevice();
            ::operator delete(g_pMouseDevice);
        }
        g_pMouseDevice = nullptr;
        g_pDirectInput->Release();
        g_pDirectInput = nullptr;
    }
    return true;
}

void UpdateAppActiveStateFromActivate(WPARAM wParam) {
    g_dwAppActive = wParam;
    g_dwActivateState = LOWORD(wParam);
    g_dwActivateMinimized = HIWORD(wParam);
    // The original calls NullStub() when the window goes inactive or is minimised.
    if (g_dwActivateState == WA_INACTIVE || g_dwActivateMinimized) {
        NullStub();
    }
}

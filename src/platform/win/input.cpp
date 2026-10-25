#include "platform/win/input.h"

#include <cstdio>
#include <cstring>
#include <new>

#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"

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
MouseMenuState g_MouseMenuState;
bool g_fCharInputSuppressed;
short g_dwMouseScrollDirection = 1;
unsigned short g_wMenuHoverIndex;
DWORD g_dwLastInputActivityTime;
MouseDevice *g_pMousePosition;

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
    memset(&g_MouseMenuState, 0, sizeof(g_MouseMenuState));
    return true;
}

// The input device type of the first game controller.
#define INPUT_DEVICE_FIRST_CONTROLLER 2
// The bit of a button in a state of DirectInput that is set while the button is down.
#define BUTTON_DOWN_MASK 0x80
// The distance an axis is pushed for GetPressedJoystickControl() to report it.
#define JOYSTICK_AXIS_THRESHOLD 50

JoystickControlName g_aJoystickButtonNamesA[JOYSTICK_NAME_COUNT_A];
JoystickControlName g_aJoystickButtonNamesB[JOYSTICK_NAME_COUNT_B];

// The bytes of each table of names that InitJoystickBindingNames() clears.
#define JOYSTICK_NAMES_CLEARED_A 0xdc
#define JOYSTICK_NAMES_CLEARED_B 0x158
// The buttons named in each table, and the first code of its axes.
#define JOYSTICK_BUTTON_COUNT_A 4
#define JOYSTICK_AXIS_CODE_A 0x86
#define JOYSTICK_BUTTON_COUNT_B 128
#define JOYSTICK_AXIS_CODE_B 0x80
// The string of the format of the name of a button, and the first string of an axis.
#define JOYSTICK_BUTTON_NAME_STRING 0xc3
#define JOYSTICK_AXIS_NAME_STRING 0xc4

static inline void
FillJoystickNames(JoystickControlName *pNames, int nCount, int nButtonCount, int nAxisCode) {
    for (int i = 0; i < nCount; i++) {
        if (i < nButtonCount) {
            pNames[i].nCode = i;
            sprintf(pNames[i].szName, GetStringResource(JOYSTICK_BUTTON_NAME_STRING), i);
        } else {
            pNames[i].nCode = nAxisCode + (i - nButtonCount);
            // The original passes the name to sprintf() as the format.
            sprintf(pNames[i].szName,
                    "%s",
                    GetStringResource(JOYSTICK_AXIS_NAME_STRING + (i - nButtonCount)));
        }
    }
}

void InitJoystickBindingNames(void) {
    memset(g_aJoystickButtonNamesA, 0, JOYSTICK_NAMES_CLEARED_A);
    memset(g_aJoystickButtonNamesB, 0, JOYSTICK_NAMES_CLEARED_B);
    FillJoystickNames(g_aJoystickButtonNamesA,
                      JOYSTICK_NAME_COUNT_A,
                      JOYSTICK_BUTTON_COUNT_A,
                      JOYSTICK_AXIS_CODE_A);
    FillJoystickNames(g_aJoystickButtonNamesB,
                      JOYSTICK_NAME_COUNT_B,
                      JOYSTICK_BUTTON_COUNT_B,
                      JOYSTICK_AXIS_CODE_B);
}

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
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        abHeld[i] = 0;
        abClicked[i] = 0;
    }
    bPressPending = 0;
}

// The time the hover of a menu waits before it moves again, in milliseconds.
#define MENU_SCROLL_REPEAT_MS 120

// The cursor of the mouse stays within the screen, the far edge included.
static inline int ClampCursor(int nPosition, int nLimit) {
    if (nPosition < 0) {
        return 0;
    }
    return nPosition >= nLimit ? nLimit : nPosition;
}

void MouseDevice::PollMouseDevice(HWND) {
    DIMOUSESTATE state;
    DWORD dwNow;
    HRESULT hResult;
    bool bKeyLeft;
    bool bAnyDown;

    if (g_fCharInputSuppressed) {
        return;
    }
    dwNow = GetHighResolutionTime();
    g_MouseMenuState.dwScrollRepeatTimerMs += dwNow - g_MouseMenuState.dwPollLastTimeMs;
    g_MouseMenuState.dwPollLastTimeMs = dwNow;
    if (static_cast<int>(g_MouseMenuState.dwScrollRepeatTimerMs) > MENU_SCROLL_REPEAT_MS) {
        g_MouseMenuState.bScrollRepeatReady = true;
    }
    if (pDevice == nullptr) {
        return;
    }
    pDevice->Acquire();
    hResult = pDevice->GetDeviceState(sizeof(state), &state);
    // The device is acquired twice before each read that follows a loss.
    while (hResult == DIERR_INPUTLOST && SUCCEEDED(pDevice->Acquire())) {
        pDevice->Acquire();
        hResult = pDevice->GetDeviceState(sizeof(state), &state);
    }
    nCursorX += state.lX;
    nCursorY += state.lY;
    nWheel += state.lZ;
    nCursorX = ClampCursor(nCursorX, g_dwEntityHalfWidth);
    nCursorY = ClampCursor(nCursorY, g_dwEntityHalfHeight);
    bAnyDown = false;
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        abButtons[i] = state.rgbButtons[i] & BUTTON_DOWN_MASK;
        bAnyDown = bAnyDown || abButtons[i] != 0;
    }
    if (bAnyDown) {
        bPressPending = 1;
    }

    if (g_adwKeyPressedEdge[DIK_F11]) {
        SelectPreviousMusicTrack();
    }
    if (g_adwKeyPressedEdge[DIK_F12]) {
        SelectNextMusicTrack();
    }
    if (g_MouseMenuState.bScrollRepeatReady) {
        if (g_abKeyboardState[DIK_UP] & BUTTON_DOWN_MASK) {
            g_MouseMenuState.bScrollDecPending = true;
            g_dwMouseScrollDirection = -1;
            g_MouseMenuState.dwScrollRepeatTimerMs = 0;
        }
        if (g_abKeyboardState[DIK_DOWN] & BUTTON_DOWN_MASK) {
            g_MouseMenuState.bScrollIncPending = true;
            g_dwMouseScrollDirection = 1;
            g_MouseMenuState.dwScrollRepeatTimerMs = 0;
        }
    }
    if (g_adwKeyPressedEdge[DIK_LEFT]) {
        bKeyLeft = true;
        g_dwMouseScrollDirection = -1;
        g_MouseMenuState.dwScrollRepeatTimerMs = 0;
    } else {
        bKeyLeft = g_MouseMenuState.bKeyLeftAsButton0;
    }
    if (g_adwKeyPressedEdge[DIK_RIGHT]) {
        g_MouseMenuState.bKeyRightAsButton1 = true;
        g_dwMouseScrollDirection = 1;
        g_MouseMenuState.dwScrollRepeatTimerMs = 0;
    }
    if (bKeyLeft) {
        abButtons[0] = 1;
        bPressPending = 0;
        g_MouseMenuState.bScrollRepeatReady = false;
    }
    if (g_MouseMenuState.bKeyRightAsButton1) {
        abButtons[1] = 1;
        bPressPending = 0;
        g_MouseMenuState.bScrollRepeatReady = false;
    }
    if (g_MouseMenuState.bScrollRepeatReady) {
        if (g_MouseMenuState.bScrollDecPending) {
            g_wMenuHoverIndex--;
            g_MouseMenuState.bScrollRepeatReady = false;
        }
        if (g_MouseMenuState.bScrollIncPending) {
            g_wMenuHoverIndex++;
            g_MouseMenuState.bScrollRepeatReady = false;
        }
    }
    g_MouseMenuState.bKeyRightAsButton1 = false;
    g_MouseMenuState.bKeyLeftAsButton0 = false;
    g_MouseMenuState.bScrollIncPending = false;
    g_MouseMenuState.bScrollDecPending = false;

    if (nCursorX != nPrevCursorX || nCursorY != nPrevCursorY ||
        memcmp(abButtons, abPrevButtons, sizeof(abButtons)) != 0) {
        g_dwLastInputActivityTime = GetHighResolutionTime();
    }
    nPrevCursorX = nCursorX;
    nPrevCursorY = nCursorY;
    memcpy(abPrevButtons, abButtons, sizeof(abButtons));

    for (int &bDown : g_adwMouseButtonRaw) {
        bDown = 0;
    }
    // A button counts as pressed for the game when it is let go.
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (abButtons[i] && !abHeld[i]) {
            abHeld[i] = 1;
        } else if (!abButtons[i] && abHeld[i]) {
            abHeld[i] = 0;
            abClicked[i] = 1;
            bPressPending = 0;
            g_adwMouseButtonRaw[i] = 1;
        } else {
            abClicked[i] = 0;
        }
    }
    g_pMousePosition = this;
    UpdateMouseButtonEdges();
}

void UpdateMouseInput(void) {
    g_pMouseDevice->PollMouseDevice(g_hMainWindow);
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
DIEFFECT g_DiConditionEffect;
DICONDITION g_aDiConditionParams[2];
LONG g_alConditionEffectDirection[2];
int g_dwForceFeedbackMagnitude;
bool g_bForceFeedbackForceStopped;
DIEFFECT g_DiJoltEffect;
DIPERIODIC g_DiJoltPeriodic;
DWORD g_dwForceFeedbackDuration = 125000;
LONG g_adwForceFeedbackDirection[2] = {-1, 0};

// The saturation and the gain of the effects of force feedback.
#define FORCE_FEEDBACK_FULL 10000
// The period of the square wave of the jolt in microseconds.
#define JOLT_PERIOD 250000

// The effects play on X and Y by offsets, in Cartesian directions, and with no trigger.
static inline void SetUpForceFeedbackEffect(DIEFFECT *pEffect,
                                            DWORD dwDuration,
                                            DWORD *pdwAxes,
                                            LONG *plDirection,
                                            DWORD cbParams,
                                            void *pParams) {
    pEffect->dwSize = sizeof(*pEffect);
    pEffect->dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    pEffect->dwDuration = dwDuration;
    pEffect->dwSamplePeriod = 0;
    pEffect->dwGain = FORCE_FEEDBACK_FULL;
    pEffect->dwTriggerButton = DIEB_NOTRIGGER;
    pEffect->dwTriggerRepeatInterval = 0;
    pEffect->cAxes = 2;
    pEffect->rgdwAxes = pdwAxes;
    pEffect->rglDirection = plDirection;
    pEffect->lpEnvelope = nullptr;
    pEffect->cbTypeSpecificParams = cbParams;
    pEffect->lpvTypeSpecificParams = pParams;
}

static inline void SetSpring(DICONDITION *pCondition, LONG lCoefficient) {
    pCondition->lOffset = 0;
    pCondition->lPositiveCoefficient = lCoefficient;
    pCondition->lNegativeCoefficient = lCoefficient;
    pCondition->dwPositiveSaturation = FORCE_FEEDBACK_FULL;
    pCondition->dwNegativeSaturation = FORCE_FEEDBACK_FULL;
    pCondition->lDeadBand = 0;
}

HRESULT CreateForceFeedbackEffects(int nCoefficientX, int nCoefficientY) {
    // The effects retain a pointer to the axes after the routine returns.
    DWORD adwAxes[2] = {DIJOFS_X, DIJOFS_Y};

    SetSpring(&g_aDiConditionParams[0], nCoefficientX);
    SetSpring(&g_aDiConditionParams[1], nCoefficientY);
    SetUpForceFeedbackEffect(&g_DiConditionEffect,
                             INFINITE,
                             adwAxes,
                             g_alConditionEffectDirection,
                             sizeof(g_aDiConditionParams),
                             g_aDiConditionParams);
    if (g_pJoystickDevice->CreateEffect(
            GUID_Spring, &g_DiConditionEffect, &g_pConditionEffect, nullptr) == DI_OK) {
        g_dwForceFeedbackMagnitude = nCoefficientX;
        g_bForceFeedbackForceStopped = true;
    }
    g_DiJoltPeriodic.dwMagnitude = FORCE_FEEDBACK_FULL;
    g_DiJoltPeriodic.lOffset = 0;
    g_DiJoltPeriodic.dwPhase = 0;
    g_DiJoltPeriodic.dwPeriod = JOLT_PERIOD;
    SetUpForceFeedbackEffect(&g_DiJoltEffect,
                             g_dwForceFeedbackDuration,
                             adwAxes,
                             g_adwForceFeedbackDirection,
                             sizeof(g_DiJoltPeriodic),
                             &g_DiJoltPeriodic);
    return g_pJoystickDevice->CreateEffect(GUID_Square, &g_DiJoltEffect, &g_pJoltEffect, nullptr);
}

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

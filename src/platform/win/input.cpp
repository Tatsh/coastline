#include "platform/win/input.h"

#include <cstring>
#include <new>

#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/strings.h"

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

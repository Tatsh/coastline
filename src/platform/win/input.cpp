#include "platform/win/input.h"

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

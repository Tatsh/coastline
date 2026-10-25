#include "platform/win/input.h"

#include <new>

#include <windows.h>

#include "platform/win/boot/app.h"

IDirectInputA *g_pDirectInput;
int g_dwDirectInputRefCount;
MouseDevice *g_pMouseDevice;
IDirectInputDeviceA *g_pKeyboardDevice;
int g_dwKeyboardDeviceRefCount;
IDirectInputDevice2A *g_pJoystickDevice;
int g_dwJoystickDeviceRefCount;
IDirectInputEffect *g_pJoltEffect;
IDirectInputEffect *g_pConditionEffect;

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

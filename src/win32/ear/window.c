#include "ear/window.h"

#include "ear/error.h"

HWND g_hEarNotifyWindow;
UINT g_uEarNotifyMessage;
char g_earProductName[] = "EAR Interactive Around-Sound";

void __stdcall ShowEarMessage(const char *message) {
    MessageBoxA(g_hEarNotifyWindow, message, g_earProductName, MB_OK);
}

BOOL __stdcall PostEarWindowMessage(WPARAM event, LPARAM notification) {
    return PostMessageA(g_hEarNotifyWindow, g_uEarNotifyMessage, event, notification);
}

/** @ghidraAddress EAR: 0x1000bab0 */
static BOOL CALLBACK FindEarProcessWindowCallback(HWND window, LPARAM context) {
    DWORD processId;
    GetWindowThreadProcessId(window, &processId);
    if (processId == GetCurrentProcessId()) {
        g_hEarNotifyWindow = window;
        return FALSE;
    }
    return TRUE;
}

BOOL __stdcall EarAssignHwndExportWrapper(HWND window) {
    return EAR_DLL_AssignHwnd(window);
}

BOOL __stdcall EAR_DLL_AssignHwnd(HWND window) {
    SetEarLastError(0);
    if (IsWindow(window)) {
        g_hEarNotifyWindow = window;
    } else {
        EnumWindows(FindEarProcessWindowCallback, 0);
    }
    if (!IsWindow(g_hEarNotifyWindow)) {
        SetEarLastError(0x21);
        g_hEarNotifyWindow = nullptr;
    }
    return IsWindow(g_hEarNotifyWindow);
}

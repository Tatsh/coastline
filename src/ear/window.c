#include "ear/window.h"

#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include <SDL3/SDL.h>
#endif

#include "ear/error.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
HWND g_hEarNotifyWindow;
#else
SDL_Window *g_hEarNotifyWindow;
#endif
UINT g_uEarNotifyMessage;
char g_earProductName[] = "EAR Interactive Around-Sound";

void __stdcall ShowEarMessage(const char *message) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    MessageBoxA(g_hEarNotifyWindow, message, g_earProductName, MB_OK);
#else
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_INFORMATION, g_earProductName, message, g_hEarNotifyWindow);
#endif
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#else
BOOL __stdcall PostEarWindowMessage(uintptr_t event, intptr_t notification) {
    SDL_Event message;
    SDL_zero(message);
    message.type = g_uEarNotifyMessage;
    message.user.windowID = g_hEarNotifyWindow ? SDL_GetWindowID(g_hEarNotifyWindow) : 0;
    message.user.code = (Sint32)notification;
    message.user.data1 = (void *)event;
    return SDL_PushEvent(&message);
}

BOOL __stdcall EarAssignHwndExportWrapper(SDL_Window *window) {
    return EAR_DLL_AssignHwnd(window);
}

BOOL __stdcall EAR_DLL_AssignHwnd(SDL_Window *window) {
    SetEarLastError(0);
    // SDL does not list the windows of other processes, and the window of the game is the one
    // given.
    g_hEarNotifyWindow = window;
    if (!g_hEarNotifyWindow) {
        SetEarLastError(0x21);
    }
    return g_hEarNotifyWindow != NULL;
}
#endif

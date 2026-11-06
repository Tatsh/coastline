#include "ear/window.h"

#include <SDL3/SDL.h>

#include "ear/error.h"

SDL_Window *g_hEarNotifyWindow;
UINT g_uEarNotifyMessage;
char g_earProductName[] = "EAR Interactive Around-Sound";

void __stdcall ShowEarMessage(const char *message) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_INFORMATION, g_earProductName, message, g_hEarNotifyWindow);
}

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

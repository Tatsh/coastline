#ifndef COASTLINE_EAR_WINDOW_H
#define COASTLINE_EAR_WINDOW_H

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
#else
#include <stdint.h>

#include <SDL3/SDL_video.h>

#include "common/types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress EAR: 0x1003b618 */
extern HWND g_hEarNotifyWindow;

/** @ghidraAddress EAR: 0x1003b3c0 */
extern UINT g_uEarNotifyMessage;
#else
/** @ghidraAddress EAR: 0x1003b618 */
extern SDL_Window *g_hEarNotifyWindow;

/**
 * The type of the SDL events that tell the game an event started or stopped. The code of each is
 * the notification, and its first data is the handle of the event.
 *
 * @ghidraAddress EAR: 0x1003b3c0
 */
extern UINT g_uEarNotifyMessage;
#endif

/**
 * The caption of the messages of the library.
 *
 * @ghidraAddress EAR: 0x10036150
 */
extern char g_earProductName[];

/**
 * Show a message over the window of the game.
 *
 * @param message The message.
 * @ghidraAddress EAR: 0x10003cbc
 */
void __stdcall ShowEarMessage(const char *message);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress EAR: 0x10004ae6 */
BOOL __stdcall PostEarWindowMessage(WPARAM event, LPARAM notification);

/** @ghidraAddress EAR: 0x1000baa0 */
BOOL __stdcall EarAssignHwndExportWrapper(HWND window);

/** @ghidraAddress EAR: 0x1000bae5 */
BOOL __stdcall EAR_DLL_AssignHwnd(HWND window);
#else
/**
 * Tell the game that an event started or stopped, with an event of g_uEarNotifyMessage.
 *
 * @param event The handle of the event.
 * @param notification 1 when the event started, or 2 when it stopped.
 * @return Whether the event was queued.
 * @ghidraAddress EAR: 0x10004ae6
 */
BOOL __stdcall PostEarWindowMessage(uintptr_t event, intptr_t notification);

/**
 * @param window The window of the game.
 * @return Whether the window was assigned.
 * @ghidraAddress EAR: 0x1000baa0
 */
BOOL __stdcall EarAssignHwndExportWrapper(SDL_Window *window);

/**
 * Assign the window the messages of the library show over.
 *
 * @param window The window of the game.
 * @return Whether the window was assigned.
 * @ghidraAddress EAR: 0x1000bae5
 */
BOOL __stdcall EAR_DLL_AssignHwnd(SDL_Window *window);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_WINDOW_H */

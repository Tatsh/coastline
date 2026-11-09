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
/**
 * Borrowed Windows notification and message-dialog window handle, or null when unassigned.
 * @ghidraAddress EAR: 0x1003b618
 */
/** @cond */
extern HWND g_hEarNotifyWindow;

/**
 * Windows message identifier for playback notifications; WPARAM is the event handle and LPARAM
 * is the notification code, one for start or two for end.
 * @ghidraAddress EAR: 0x1003b3c0
 */
extern UINT g_uEarNotifyMessage;
#else
/**
 * Borrowed SDL notification and message-dialog window, valid until reassigned or cleared.
 * The caller must preserve the window while assigned.
 * @ghidraAddress EAR: 0x1003b618
 */
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
/** @endcond */

/**
 * Show a message over the window of the game.
 *
 * @param message A non-null terminated message string, borrowed until the modal dialog closes.
 * @ghidraAddress EAR: 0x10003cbc
 */
void __stdcall ShowEarMessage(const char *message);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Post an asynchronous playback notification to the assigned Windows window.
 * @param event The event handle passed as WPARAM.
 * @param notification The notification code passed as LPARAM, normally one for start or two
 * for end.
 * @return Whether Windows queued the message.
 * @ghidraAddress EAR: 0x10004ae6
 */
BOOL __stdcall PostEarWindowMessage(WPARAM event, LPARAM notification);

/**
 * Forward notification-window assignment to EAR_DLL_AssignHwnd().
 * @param window A borrowed window handle, or an invalid handle to search this process's windows.
 * @return Whether a valid notification window is assigned after the attempt.
 * @ghidraAddress EAR: 0x1000baa0
 */
BOOL __stdcall EarAssignHwndExportWrapper(HWND window);

/**
 * Assign a valid window handle or search for the first enumerated window of this process.
 * An unsuccessful search preserves a previously assigned valid window.
 * Failure clears the window and records EAR error 0x21; success clears the last EAR error.
 * @param window A borrowed handle, valid while assigned, or an invalid handle to request discovery.
 * @return Whether a valid notification window is assigned after the attempt.
 * @ghidraAddress EAR: 0x1000bae5
 */
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
 * @param window A borrowed game window that remains valid while assigned, or null to clear it.
 * @return Whether the window was assigned.
 * @ghidraAddress EAR: 0x1000baa0
 */
BOOL __stdcall EarAssignHwndExportWrapper(SDL_Window *window);

/**
 * Assign the window the messages of the library show over.
 *
 * @param window A borrowed game window that remains valid while assigned, or null to clear it.
 * @return TRUE for a non-null window; FALSE with EAR error 0x21 for null.
 * @ghidraAddress EAR: 0x1000bae5
 */
BOOL __stdcall EAR_DLL_AssignHwnd(SDL_Window *window);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_WINDOW_H */

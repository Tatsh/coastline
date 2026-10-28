#ifndef COASTLINE_EAR_WINDOW_H
#define COASTLINE_EAR_WINDOW_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1003b618 */
extern HWND g_hEarNotifyWindow;

/** @ghidraAddress EAR: 0x1003b3c0 */
extern UINT g_uEarNotifyMessage;

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

/** @ghidraAddress EAR: 0x10004ae6 */
BOOL __stdcall PostEarWindowMessage(WPARAM event, LPARAM notification);

/** @ghidraAddress EAR: 0x1000baa0 */
BOOL __stdcall EarAssignHwndExportWrapper(HWND window);

/** @ghidraAddress EAR: 0x1000bae5 */
BOOL __stdcall EAR_DLL_AssignHwnd(HWND window);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_WINDOW_H */

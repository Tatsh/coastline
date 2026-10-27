#ifndef COASTLINE_EAR_WINDOW_H
#define COASTLINE_EAR_WINDOW_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1003b618 */
extern HWND g_hEarNotifyWindow;

/** @ghidraAddress EAR: 0x1000baa0 */
BOOL __stdcall EarAssignHwndExportWrapper(HWND window);

/** @ghidraAddress EAR: 0x1000bae5 */
BOOL __stdcall EAR_DLL_AssignHwnd(HWND window);

#ifdef __cplusplus
}
#endif

#endif

#pragma once

#include <windows.h>

/** The screen of the front end that is showing. The layout is not recovered yet. */
struct FrontEndScreen {
    char abUnknown0[0x68]; /* +0x00 */
    void *pUnknown68;      /*!< Set while the screen reads the messages of the main window. */
};

/**
 * Load the resources of the front end menus.
 *
 * @return Whether every resource loaded.
 * @ghidraAddress BRD3D: 0x1005d440
 * @ghidraAddress BRGlide: 0x10056260
 */
bool InitializeFrontEndResources(void);

/**
 * The screen of the front end that is showing.
 *
 * @ghidraAddress BRD3D: 0x10aa2904
 */
extern FrontEndScreen *g_pActiveStringTable;

/**
 * A buffer of the front end, passed to FrontEndWindowProc() and not read by it.
 *
 * @ghidraAddress BRD3D: 0x10aa2900
 */
extern void *g_pFrontEndScratchBuffer;

/**
 * Give a message of the main window to the front end.
 *
 * @param pUnused Not read.
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @return The result of the message for the front end.
 * @ghidraAddress BRD3D: 0x10060060
 */
LRESULT WINAPI
FrontEndWindowProc(void *pUnused, HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

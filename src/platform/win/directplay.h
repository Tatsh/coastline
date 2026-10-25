#pragma once

#include <windows.h>

/**
 * Initialise DirectPlay for the main window.
 *
 * @ghidraAddress BRD3D: 0x1000c6d0
 * @ghidraAddress BRGlide: 0x10009c00
 */
void InitializeDirectPlayForDesktopWindow(void);

/**
 * Whether a DirectPlay session is open.
 *
 * @ghidraAddress BRD3D: 0x10a9cffc
 */
extern bool g_dwDirectPlaySessionActive;

/**
 * Give a message of the main window to the network play.
 *
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @ghidraAddress BRD3D: 0x1003c3a0
 */
void WINAPI HandleNetworkWindowMessage(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

#pragma once

/**
 * @file
 * The sound of the PC port.
 */

#include <windows.h>

/**
 * Start the sound of the game for a window, when the game plays music in mode 2.
 *
 * @param hWnd The window.
 * @ghidraAddress BRD3D: 0x1002adc0
 */
void InitializeEarSound(HWND hWnd);

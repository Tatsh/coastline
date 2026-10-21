#ifndef COASTLINE_WEATHERSELECT_H
#define COASTLINE_WEATHERSELECT_H

/**
 * @file
 * The weather selection screen.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"

/**
 * The items of the screen, in the order of the weathers, ending with NULL.
 *
 * @ghidraAddress USA: 0x802722a4
 */
extern MenuItem *g_apWeatherMenuItems[];

/**
 * The state of the screen: 0 before it starts, 1 while it runs, and -1 once it is left.
 *
 * @ghidraAddress USA: 0x802722bc
 */
extern s32 g_dwWeatherSelectState;

/** @ghidraAddress USA: 0x80316370 */
extern s32 g_dwWeatherMenuItemCount;

/** @ghidraAddress USA: 0x80316374 */
extern s32 g_dwWeatherMenuSelection;

/**
 * Return whether a weather may be chosen. Sunny is offered only to one player and off tracks 3
 * and 8.
 *
 * @ghidraAddress USA: 0x80210f80
 */
s32 IsWeatherSelectable(s32 weather);

/**
 * Run one frame of the weather selection screen.
 *
 * @ghidraAddress USA: 0x80210fc8
 */
void ProcessWeatherSelect(void);

#endif

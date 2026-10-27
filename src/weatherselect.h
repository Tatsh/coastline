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
 * @ghidraAddress EUR: 0x80274284
 * @ghidraAddress JPN: 0x80273d34
 */
extern MenuItem *g_apWeatherMenuItems[];

/**
 * Return whether a weather may be chosen. Sunny is offered only to one player and off tracks 3
 * and 8.
 *
 * @param weather The weather, a `WEATHER_*` value.
 * @return Non-zero when the weather may be chosen.
 * @ghidraAddress USA: 0x80210f80
 * @ghidraAddress EUR: 0x80210de0
 * @ghidraAddress JPN: 0x80210cd0
 */
s32 IsWeatherSelectable(s32 weather);

/**
 * Run one frame of the weather selection screen.
 *
 * @ghidraAddress USA: 0x80210fc8
 * @ghidraAddress EUR: 0x80210e28
 * @ghidraAddress JPN: 0x80210d18
 */
void ProcessWeatherSelect(void);

#endif /* COASTLINE_WEATHERSELECT_H */

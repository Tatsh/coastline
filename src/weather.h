#ifndef COASTLINE_WEATHER_H
#define COASTLINE_WEATHER_H

/**
 * @file
 * The weather of a race and the flags the renderer tests.
 */

#include <PR/ultratypes.h>

/** The weathers, in the order of the weather menu. */
enum {
    WEATHER_SUNNY,
    WEATHER_FOG,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_NIGHT,
};

/** The weather the player chose. @ghidraAddress USA: 0x8028c800 */
extern u32 g_dwCurrentWeather;

/** Whether the weather is anything but sunny. @ghidraAddress USA: 0x8028aa78 */
extern u32 g_dwWeatherActive;

/** @ghidraAddress USA: 0x8028aa80 */
extern u32 g_dwNightMode;

/** @ghidraAddress USA: 0x8028aa84 */
extern u32 g_dwSnowMode;

/** @ghidraAddress USA: 0x8028aa8c */
extern u32 g_dwRainMode;

#endif

#include "weatherselect.h"

#include "carselect.h"
#include "input.h"
#include "racemenu.h"
#include "system.h"
#include "trackselect.h"
#include "weather.h"

s32 IsWeatherSelectable(s32 weather) {
    return weather != WEATHER_SUNNY ||
           (g_dwSelectedTrackId != 3 && g_dwSelectedTrackId != 8 && g_dwNumLocalPlayers == 1);
}

void ProcessWeatherSelect(void) {
    if (g_dwWeatherSelectState <= 0) {
        ResetAndInitDisplay(g_apWeatherMenuItems, &g_dwWeatherMenuItemCount);
        if (g_dwWeatherSelectState == 0) {
            g_dwWeatherMenuSelection = g_dwCurrentWeather;
        }
        /* Move off a weather that may not be chosen. The original steps through as many entries
         * as there are tracks, though only the first step can occur. */
        while (!IsWeatherSelectable(g_dwWeatherMenuSelection)) {
            g_dwWeatherMenuSelection = (g_dwWeatherMenuSelection + 1) % g_dwNumTracks;
        }
        g_dwWeatherSelectState = 1;
    }

    switch (ProcessScreenState("WEATHER SELECT",
                               g_dwWeatherMenuItemCount,
                               g_apWeatherMenuItems,
                               &g_dwWeatherMenuSelection,
                               IsWeatherSelectable,
                               0x00,
                               0x00,
                               0x00,
                               0x20,
                               0x00,
                               0xA0)) {
    case MENU_RESULT_CHOSEN:
        g_dwCurrentWeather = g_dwWeatherMenuSelection;
        SetCurrentScreenCallback(ProcessCarSelectScreen);
        g_dwWeatherSelectState = -1;
        break;
    case MENU_RESULT_CANCELLED:
        SetCurrentScreenCallback(ProcessTrackMenu);
        g_dwWeatherSelectState = -1;
        break;
    }
}

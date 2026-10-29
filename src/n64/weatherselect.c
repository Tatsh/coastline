#include "weatherselect.h"

#include "carselect.h"
#include "common/input.h"
#include "common/system.h"
#include "common/trackselect.h"
#include "common/weather.h"
#include "racemenu.h"

s32 IsWeatherSelectable(s32 weather) {
    return weather != WEATHER_SUNNY ||
           (g_dwSelectedTrackId != 3 && g_dwSelectedTrackId != 8 && g_dwNumLocalPlayers == 1);
}

void ProcessWeatherSelect(void) {
    /* The state of the screen: 0 before it starts, 1 while it runs, and -1 once it is left. */
    /**
     * @ghidraAddress USA: 0x802722bc
     * @ghidraAddress EUR: 0x8027429c
     * @ghidraAddress JPN: 0x80273d4c
     */
    static s32 g_dwWeatherSelectState = 0;
    /**
     * @ghidraAddress USA: 0x80316370
     * @ghidraAddress EUR: 0x80318050
     * @ghidraAddress JPN: 0x80316c80
     */
    static s32 g_dwWeatherMenuItemCount;
    /**
     * @ghidraAddress USA: 0x80316374
     * @ghidraAddress EUR: 0x80318054
     * @ghidraAddress JPN: 0x80316c84
     */
    static s32 g_dwWeatherMenuSelection;

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

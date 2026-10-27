#include "displaylistops.h"

#include <stddef.h>

#include "track.h"
#include "trackselect.h"
#include "vehicle.h"
#include "vehiclecount.h"
#include "weather.h"

#define ARRAY_COUNT(array) (s32)(sizeof(array) / sizeof((array)[0]))
#define SNOW_PRIM_COLOR 0x60789000
#define SNOW_ENV_COLOR 0x8C9CA800

s32 ProcessDisplayList(DisplayListCommand *displayList, DisplayListCommand table[][4]) {
    s32 clear, column, translucent, snowTint, row, i;

    clear = !g_dwNightMode && !g_dwRainMode;
    column = g_dwWeatherActive + !clear + 1;
    translucent = FALSE;
    snowTint = FALSE;
    if (displayList != NULL) {
        /* The original masks the opcode after the shift. */
        for (; ((displayList->words.w0 >> 24) & 0xFF) != G_ENDDL; displayList++) {
            switch ((displayList->words.w0 >> 24) & 0xFF) {
            case G_SETOTHERMODE_L:
                for (row = 0; row < RENDER_MODE_ROWS; row++) {
                    if (displayList->words.w0 == table[row][0].words.w0 &&
                        displayList->words.w1 == table[row][0].words.w1) {
                        *displayList = table[row][column];
                        if (row >= RENDER_MODE_TRANSLUCENT_ROW) {
                            translucent = TRUE;
                        }
                        break;
                    }
                }
                break;
            case G_SETCOMBINE:
                if (clear) {
                    for (i = 0; i < ARRAY_COUNT(g_aDlClearCombinePatch); i++) {
                        if (displayList->words.w0 == g_aDlClearCombinePatch[i][0].words.w0 &&
                            displayList->words.w1 == g_aDlClearCombinePatch[i][0].words.w1) {
                            *displayList = g_aDlClearCombinePatch[i][1];
                            break;
                        }
                    }
                }
                if (g_dwSnowMode && g_bDisplayListSnowTint) {
                    for (i = 0; i < ARRAY_COUNT(g_aDlSnowTintCombines); i++) {
                        if (displayList->words.w0 == g_aDlSnowTintCombines[i].words.w0 &&
                            displayList->words.w1 == g_aDlSnowTintCombines[i].words.w1) {
                            break;
                        }
                    }
                    snowTint = i < ARRAY_COUNT(g_aDlSnowTintCombines);
                }
                break;
            case G_SETPRIMCOLOR:
                if (snowTint && g_dwSnowMode) {
                    displayList->words.w1 = SNOW_PRIM_COLOR;
                }
                break;
            case G_SETENVCOLOR:
                if (snowTint && g_dwSnowMode) {
                    displayList->words.w1 = SNOW_ENV_COLOR;
                }
                break;
            }
        }
    }
    g_bDisplayListSnowTint = FALSE;
    return translucent;
}

void ProcessAllDisplayLists(void) {
    Vehicle *vehicle;
    s32 i, lod, k;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    s32 snowTint;

    snowTint =
        g_dwSelectedTrackId != TRACK_COASTLINE && g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE;
    for (i = 0; i < g_LoadedTrack.instanceCount; i++) {
        if (!(g_LoadedTrack.instances[i].flags & TRACK_INSTANCE_TWO_SIDED)) {
            g_bDisplayListSnowTint = snowTint;
        }
        if (ProcessDisplayList(g_LoadedTrack.instances[i].displayList, g_aaDlTrackRenderModes)) {
            g_LoadedTrack.instances[i].flags |= TRACK_INSTANCE_TRANSLUCENT;
        }
    }
#else
    g_bDisplayListSnowTint = FALSE;
#endif
    /* The models the vehicles share are patched for good. */
    for (i = 0, vehicle = g_aVehicles; i < g_dwNumActiveVehicles; i++, vehicle++) {
        for (lod = 0; lod < VEHICLE_LODS; lod++) {
            if (vehicle->ghostMode == VEHICLE_GHOST) {
                for (k = 0; k < VEHICLE_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(vehicle->model->displayLists[lod][k],
                                       g_aaDlGhostRenderModes);
                }
                for (k = 0; k < VEHICLE_WHEEL_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(vehicle->model->wheelDisplayLists[lod][k],
                                       g_aaDlGhostWheelRenderModes);
                }
            } else {
                for (k = 0; k < VEHICLE_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(vehicle->model->displayLists[lod][k],
                                       g_aaDlTrackRenderModes);
                }
                for (k = 0; k < VEHICLE_WHEEL_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(vehicle->model->wheelDisplayLists[lod][k],
                                       g_aaDlTrackRenderModes);
                }
            }
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_pfnRenderBackendUpdateFog();
#endif
}

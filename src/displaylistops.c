#include "displaylistops.h"

#include <stddef.h>

#include "track.h"
#include "trackselect.h"
#include "vehicle.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
namespace {
    /**
     * Whether ProcessDisplayList() tints the colour combines of the snow. It clears this.
     *
     * @ghidraAddress USA: 0x8028ade8
     * @ghidraAddress EUR: 0x8028d098
     * @ghidraAddress JPN: 0x8028cac8
     * @ghidraAddress BRD3D: 0x106c666c
     * @ghidraAddress BRGlide: 0x106ed6fc
     */
    s32 g_bDisplayListSnowTint;
} // namespace
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

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

void ProcessTrackNodeDisplayList(TrackInstance *instance) {
    s32 snowTint;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_bDisplayListSnowTint = FALSE;
#endif
    snowTint = g_dwSelectedTrackId != TRACK_COASTLINE &&
               g_dwSelectedTrackId != TRACK_COASTLINE + TRACK_MIRRORED;
    if (!(instance->flags & TRACK_INSTANCE_TWO_SIDED)) {
        g_bDisplayListSnowTint = snowTint;
    }
    if (ProcessDisplayList(instance->displayList, g_aaDlTrackRenderModes)) {
        instance->flags |= TRACK_INSTANCE_TRANSLUCENT;
    }
}

void ProcessAllDisplayLists(void) {
    Vehicle *vehicle;
    s32 i, lod, k;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    for (i = 0; i < g_LoadedTrack.instanceCount; i++) {
        ProcessTrackNodeDisplayList(&g_LoadedTrack.instances[i]);
    }
#else
    g_bDisplayListSnowTint = FALSE;
#endif
    /* The models the vehicles share are patched for good. */
    for (i = 0, vehicle = g_aVehicles; i < g_dwNumActiveVehicles; i++, vehicle++) {
        for (lod = 0; lod < VEHICLE_LODS; lod++) {
            if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
                for (k = 0; k < VEHICLE_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->displayLists[lod][k],
                                       g_aaDlGhostRenderModes);
                }
                for (k = 0; k < VEHICLE_WHEEL_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->wheelDisplayLists[lod][k],
                                       g_aaDlGhostWheelRenderModes);
                }
            } else {
                for (k = 0; k < VEHICLE_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->displayLists[lod][k],
                                       g_aaDlTrackRenderModes);
                }
                for (k = 0; k < VEHICLE_WHEEL_DISPLAY_LISTS; k++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->wheelDisplayLists[lod][k],
                                       g_aaDlTrackRenderModes);
                }
            }
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_pfnRenderBackendUpdateFog();
#endif
}

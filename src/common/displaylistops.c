#include "common/displaylistops.h"

#include <stddef.h>

#include "common/track.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY
namespace {
    /**
     * Whether ProcessDisplayList() tints the colour combines of the snow. It clears this.
     *
     * @ghidraAddress USA: 0x8028ade8
     * @ghidraAddress Asia: 0x8028A938
     * @ghidraAddress EUR: 0x8028d098
     * @ghidraAddress JPN: 0x8028cac8
     * @ghidraAddress BRD3D: 0x106c666c
     * @ghidraAddress BRGlide: 0x106ed6fc
     */
    s32 g_bDisplayListSnowTint;
} // namespace
#endif /* TGR_BOSS_RALLY */

#define ARRAY_COUNT(array) (s32)(sizeof(array) / sizeof((array)[0]))
#define SNOW_PRIM_COLOR 0x60789000
#define SNOW_ENV_COLOR 0x8C9CA800

s32 ProcessDisplayList(DisplayListCommand *displayList, DisplayListCommand table[][4]) {
    s32 clear, translucent, column, snowTint, i;

    translucent = FALSE;
    clear = !g_dwNightMode && !g_dwRainMode;
    column = (s32)g_dwWeatherActive + !clear + 1;
    snowTint = FALSE;
    if (displayList != NULL) {
        for (;; displayList++) {
            switch ((displayList->words.w0 >> 24) & 0xFF) {
            case G_ENDDL:
                goto done;
            case G_SETOTHERMODE_L:
                for (i = 0; i < RENDER_MODE_ROWS; i++) {
                    if (displayList->words.w0 == table[i][0].words.w0 &&
                        displayList->words.w1 == table[i][0].words.w1) {
                        displayList->words.w0 = table[i][column].words.w0;
                        displayList->words.w1 = table[i][column].words.w1;
                        if (i >= RENDER_MODE_TRANSLUCENT_ROW) {
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
                            displayList->words.w0 = g_aDlClearCombinePatch[i][1].words.w0;
                            displayList->words.w1 = g_aDlClearCombinePatch[i][1].words.w1;
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
                    if (i < ARRAY_COUNT(g_aDlSnowTintCombines)) {
                        snowTint = TRUE;
                    } else {
                        snowTint = FALSE;
                    }
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
done:
    g_bDisplayListSnowTint = FALSE;
    return translucent;
}

#if TGR_BOSS_RALLY
void ProcessTrackNodeDisplayList(TrackInstance *instance) {
    s32 snowTint;

    g_bDisplayListSnowTint = FALSE;
    snowTint = g_dwSelectedTrackId != TRACK_COASTLINE &&
               g_dwSelectedTrackId != TRACK_COASTLINE + TRACK_MIRRORED;
    if (!(instance->flags & TRACK_INSTANCE_TWO_SIDED)) {
        g_bDisplayListSnowTint = snowTint;
    }
    if (ProcessDisplayList(instance->displayList, g_aaDlTrackRenderModes)) {
        instance->flags |= TRACK_INSTANCE_TRANSLUCENT;
    }
}
#endif /* TGR_BOSS_RALLY */

void ProcessAllDisplayLists(void) {
    s32 i, vehicleIndex, lod;

#if TGR_PLATFORM == TGR_PLATFORM_N64
    s32 snowTint;
    snowTint = (s32)g_dwSelectedTrackId != TRACK_COASTLINE &&
               (s32)g_dwSelectedTrackId != TRACK_COASTLINE + TRACK_MIRRORED;
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
    for (vehicleIndex = 0; vehicleIndex < g_dwNumActiveVehicles; vehicleIndex++) {
        Vehicle *vehicle = &g_aVehicles[vehicleIndex];

        for (lod = 0; lod < VEHICLE_LODS; lod++) {
            if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
                for (i = 0; i < VEHICLE_DISPLAY_LISTS; i++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->displayLists[lod][i],
                                       g_aaDlGhostRenderModes);
                }
                for (i = 0; i < VEHICLE_WHEEL_DISPLAY_LISTS; i++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->wheelDisplayLists[lod][i],
                                       g_aaDlGhostWheelRenderModes);
                }
            } else {
                for (i = 0; i < VEHICLE_DISPLAY_LISTS; i++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->displayLists[lod][i],
                                       g_aaDlTrackRenderModes);
                }
                for (i = 0; i < VEHICLE_WHEEL_DISPLAY_LISTS; i++) {
                    ProcessDisplayList(VEHICLE_GET(vehicle, Model)->wheelDisplayLists[lod][i],
                                       g_aaDlTrackRenderModes);
                }
            }
        }
    }
#if TGR_BOSS_RALLY
    g_pfnRenderBackendUpdateFog();
#endif
}

#pragma once

#include <render.h>

#include "vehiclefwd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether to draw the network race standings.
 * @ghidraAddress BRD3D: 0x100bd3fc
 */
extern int g_bHudShowNetworkStandings;

/**
 * Whether the network standings include connection diagnostics.
 * @ghidraAddress BRD3D: 0x118abdc8
 */
extern int g_bHideNetStandingsHud;

/**
 * Draw network race positions, player colours, time gaps, and optional connection diagnostics.
 * @param viewport The view whose origin positions the standings.
 * @param vehicles The active vehicle array.
 * @ghidraAddress BRD3D: 0x10017890
 */
void DrawNetworkRaceStandingsHud(const ViewportRect *viewport, const Vehicle *vehicles);

/**
 * @file
 * Race HUD helpers and debug-overlay messages.
 */

/**
 * The message the debug overlay shows, or null for none.
 *
 * @ghidraAddress BRD3D: 0x104b0338
 * @ghidraAddress BRGlide: 0x104abb20
 */
extern const char *g_pszDebugOverlayMessage;

/**
 * The time g_pszDebugOverlayMessage remains on the debug overlay, in seconds, or 0 for always.
 *
 * @ghidraAddress BRD3D: 0x104b033c
 * @ghidraAddress BRGlide: 0x104abb24
 */
extern float g_fDebugOverlayMessageTimer;

/**
 * Draw the current debug message inside the selected viewport unless fading out.
 *
 * @param viewports The viewport array.
 * @ghidraAddress BRD3D: 0x10017690
 */
void DrawDebugOverlayBanner(const ViewportRect *viewports);

/**
 * Count down the time of the message of the debug overlay by the time of the last frame, and clear
 * the message when the time runs out.
 *
 * @ghidraAddress BRD3D: 0x10017740
 * @ghidraAddress BRGlide: 0x10014cb0
 */
void TickDebugOverlayMessageTimer(void);

/**
 * Estimate the signed time gap to the best race position below 255.
 *
 * @param vehicles The active vehicle array.
 * @param index The vehicle whose distance and speed are measured.
 * @return The distance gap divided by speed, zero for an equal distance, or 1000 at zero speed.
 * @ghidraAddress BRD3D: 0x10017cd0
 */
long double ComputeTimeGapToLeader(const Vehicle *vehicles, int index);

/**
 * Format a positive time gap with a plus prefix, or clear the buffer otherwise.
 *
 * @param vehicles The active vehicle array.
 * @param index The vehicle whose gap is measured.
 * @return The shared gap text buffer.
 * @ghidraAddress BRD3D: 0x10017c80
 */
char *FormatGapToLeaderString(const Vehicle *vehicles, int index);
#ifdef __cplusplus
}
#endif

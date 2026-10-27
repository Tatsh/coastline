#pragma once

#include "vehicle.h"

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
extern "C" long double ComputeTimeGapToLeader(const Vehicle *vehicles, int index);

/**
 * The shared buffer for the gap prefix and formatted time.
 *
 * @ghidraAddress BRD3D: 0x104aff20
 */
extern char g_szGapToLeaderText[];

/**
 * Format a positive time gap with a plus prefix, or clear the buffer otherwise.
 *
 * @param vehicles The active vehicle array.
 * @param index The vehicle whose gap is measured.
 * @return The shared gap text buffer.
 * @ghidraAddress BRD3D: 0x10017c80
 */
extern "C" char *FormatGapToLeaderString(const Vehicle *vehicles, int index);

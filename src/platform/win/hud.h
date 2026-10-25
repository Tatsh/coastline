#pragma once

/**
 * @file
 * The display of a race of the PC port over the scene: the message of the debug overlay.
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

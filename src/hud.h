#ifndef COASTLINE_HUD_H
#define COASTLINE_HUD_H

/**
 * @file
 * The display over a race: the dashboard, the times, the lap and place, the messages and the
 * speed.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "texrect.h"

/** The views that keep their own dashboard. */
#define DASHBOARD_VIEWS 2

/** The colors of a palette of the dashboard. */
#define DASHBOARD_PALETTE_COLORS 256

/**
 * Whether speeds show in miles an hour rather than kilometers.
 *
 * @ghidraAddress USA: 0x802723d8
 */
extern s32 g_bSpeedUnitsMph;

/**
 * The car and the night flag of the dashboard each view has loaded, the car in the low byte.
 *
 * @ghidraAddress USA: 0x8028c790
 */
extern s32 g_adwDashboardLoadedKey[DASHBOARD_VIEWS];

/**
 * The gear cell each view has loaded.
 *
 * @ghidraAddress USA: 0x8028c798
 */
extern s32 g_adwDashboardLoadedGear[DASHBOARD_VIEWS];

/**
 * The face of the dashboard of each view.
 *
 * @ghidraAddress USA: 0x8028c7a8
 */
extern Texture g_aDashboardTextures[DASHBOARD_VIEWS];

/**
 * The cell of the dashboard of each view that shows the gear.
 *
 * @ghidraAddress USA: 0x8028c7d0
 */
extern Texture g_aDashboardGearTextures[DASHBOARD_VIEWS];

/**
 * The palette the dashboard of each view draws with.
 *
 * @ghidraAddress USA: 0x80361530
 */
extern u16 g_aawDashboardPalettes[DASHBOARD_VIEWS][DASHBOARD_PALETTE_COLORS];

/**
 * The palette of the dashboard as loaded, which the segments of a bar take their colors from.
 *
 * @ghidraAddress USA: 0x80361930
 */
extern u16 g_awDashboardBarPalette[DASHBOARD_PALETTE_COLORS];

/**
 * The needle of each view, for each frame buffer so a frame does not change one being drawn.
 *
 * @ghidraAddress USA: 0x80361b30
 */
extern Vtx g_aGaugeNeedleVerts[2][DASHBOARD_VIEWS][4];

/**
 * The text of the speed.
 *
 * @ghidraAddress USA: 0x80361c30
 */
extern char g_szHudSpeedTextBuf[16];

/**
 * Draw the dashboard of the active vehicle: its face, its gear and its tachometer.
 *
 * @ghidraAddress USA: 0x80237980
 */
void RenderDashboardGauges(void);

/**
 * Does nothing.
 *
 * @ghidraAddress USA: 0x8023870c
 */
void StubDashboard(void);

/**
 * Draw a label with a time below it in minutes, seconds and hundredths.
 *
 * @param color The color code the time starts with.
 * @ghidraAddress USA: 0x80238714
 */
void DrawLabelWithTime(const char *label, const char *color, f32 seconds, s32 x, s32 y);

/**
 * Draw the times the mode of play shows at the top right of a view.
 *
 * @ghidraAddress USA: 0x8023880c
 */
void DrawRaceTimerHud(void);

/**
 * Draw the lap at the top left of a view and the place at its bottom left.
 *
 * @ghidraAddress USA: 0x80238ab8
 */
void DrawLapCounterHud(void);

/**
 * Draw the message of the active vehicle in the middle of a view.
 *
 * @ghidraAddress USA: 0x80238dd4
 */
void DrawHudMessage(void);

/**
 * Draw the whole display of a view: the dashboard and times unless the camera is on the bumper,
 * the lap, place and message, and the speed.
 *
 * @ghidraAddress USA: 0x80238f30
 */
void DrawSpeedAndHudOverlay(void);

#endif

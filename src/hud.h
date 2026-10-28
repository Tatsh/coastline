#ifndef COASTLINE_HUD_H
#define COASTLINE_HUD_H

/**
 * @file
 * The display over a race: the dashboard, the times, the lap and place, the messages and the
 * speed.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "render.h"
#include "texrect.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "vehiclefwd.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** The views that keep their own dashboard. */
#define DASHBOARD_VIEWS 2

/** The colors of a palette of the dashboard. */
#define DASHBOARD_PALETTE_COLORS 256

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The gear cell each view has loaded.
 * @ghidraAddress BRD3D: 0x100a73a8
 * @ghidraAddress BRGlide: 0x100a6b68
 */
extern s32 g_adwDashboardLoadedGear[DASHBOARD_VIEWS];
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * The car and the night flag of the dashboard each view has loaded, the car in the low byte.
 *
 * @ghidraAddress USA: 0x8028c790
 * @ghidraAddress EUR: 0x8028eef0
 * @ghidraAddress JPN: 0x8028e940
 */
extern s32 g_adwDashboardLoadedKey[DASHBOARD_VIEWS];

/**
 * The gear cell each view has loaded.
 *
 * @ghidraAddress USA: 0x8028c798
 * @ghidraAddress EUR: 0x8028eef8
 * @ghidraAddress JPN: 0x8028e948
 */
extern s32 g_adwDashboardLoadedGear[DASHBOARD_VIEWS];

/**
 * The face of the dashboard of each view.
 *
 * @ghidraAddress USA: 0x8028c7a8
 * @ghidraAddress EUR: 0x8028ef08
 * @ghidraAddress JPN: 0x8028e958
 */
extern Texture g_aDashboardTextures[DASHBOARD_VIEWS];

/**
 * The cell of the dashboard of each view that shows the gear.
 *
 * @ghidraAddress USA: 0x8028c7d0
 * @ghidraAddress EUR: 0x8028ef30
 * @ghidraAddress JPN: 0x8028e980
 */
extern Texture g_aDashboardGearTextures[DASHBOARD_VIEWS];

/**
 * The palette the dashboard of each view draws with.
 *
 * @ghidraAddress USA: 0x80361530
 * @ghidraAddress EUR: 0x80363a30
 * @ghidraAddress JPN: 0x803626a0
 */
extern u16 g_aawDashboardPalettes[DASHBOARD_VIEWS][DASHBOARD_PALETTE_COLORS];

/**
 * The palette of the dashboard as loaded, which the segments of a bar take their colors from.
 *
 * @ghidraAddress USA: 0x80361930
 * @ghidraAddress EUR: 0x80363e30
 * @ghidraAddress JPN: 0x80362aa0
 */
extern u16 g_awDashboardBarPalette[DASHBOARD_PALETTE_COLORS];
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * The needle of each view, for each frame buffer so a frame does not change one being drawn.
 *
 * @ghidraAddress USA: 0x80361b30
 * @ghidraAddress EUR: 0x80364030
 * @ghidraAddress JPN: 0x80362ca0
 */
extern Vtx g_aGaugeNeedleVerts[2][DASHBOARD_VIEWS][4];
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

/**
 * The text of the speed.
 *
 * @ghidraAddress USA: 0x80361c30
 * @ghidraAddress EUR: 0x80364130
 * @ghidraAddress JPN: 0x80362da0
 * @ghidraAddress BRD3D: 0x104b0320
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern char g_szHudSpeedTextBuf[];
#else
extern char g_szHudSpeedTextBuf[16];
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Whether to display the dashboard and speedometer.
 * @ghidraAddress BRD3D: 0x100bd3f4
 * @ghidraAddress BRGlide: 0x100bcbfc
 */
extern s32 g_bHudShowSpeedometer;
/**
 * Draw the current viewport's dashboard, gear, and tachometer.
 * @param viewports The records of the views.
 * @ghidraAddress BRD3D: 0x10016b40
 * @ghidraAddress BRGlide: 0x100140b0
 */
void RenderDashboardGauges(const ViewportRect *viewports);
#else
/**
 * Draw the dashboard of the active vehicle: its face, its gear and its tachometer.
 *
 * @ghidraAddress USA: 0x80237980
 * @ghidraAddress EUR: 0x80238e90
 * @ghidraAddress JPN: 0x80238990
 */
void RenderDashboardGauges(void);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/**
 * Does nothing.
 *
 * @ghidraAddress USA: 0x8023870c
 * @ghidraAddress EUR: 0x80239c1c
 * @ghidraAddress JPN: 0x8023971c
 */
void StubDashboard(void);

/**
 * Draw a label with a time below it in minutes, seconds and hundredths.
 *
 * @param label The label.
 * @param color The color code the time starts with.
 * @param seconds The time in seconds.
 * @param x The X coordinate of the label and the time.
 * @param y The Y coordinate of the label.
 * @ghidraAddress USA: 0x80238714
 * @ghidraAddress EUR: 0x80239c24
 * @ghidraAddress JPN: 0x80239724
 * @ghidraAddress BRD3D: 0x100171f0
 * @ghidraAddress BRGlide: 0x10014760
 */
void DrawLabelWithTime(const char *label, const char *color, f32 seconds, s32 x, s32 y);

#if TGR_PLATFORM == TGR_PLATFORM_WIN

/**
 * Whether to display completed lap times.
 *
 * @ghidraAddress BRD3D: 0x100bd3f0
 * @ghidraAddress BRGlide: 0x100bcbf8
 */
extern s32 g_bHudShowLapTimes;

/**
 * Draw the active vehicle's completed lap times in the current viewport.
 *
 * @ghidraAddress BRD3D: 0x10017f30
 * @ghidraAddress BRGlide: 0x100154a0
 */
void DrawLapTimeListHud(const ViewportRect *viewports);

/**
 * Draw a numbered lap time in minutes, seconds, and hundredths.
 *
 * @ghidraAddress BRD3D: 0x10017fe0
 * @ghidraAddress BRGlide: 0x10015550
 */
void DrawFormattedLapTimeText(const char *color, s32 lap, f32 seconds, s32 x, s32 y);

#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Whether to display the lap counter.
 * @ghidraAddress BRD3D: 0x100bd3e8
 */
extern s32 g_bHudShowLapCounter;
/**
 * Whether to display the race timer.
 * @ghidraAddress BRD3D: 0x100bd3ec
 * @ghidraAddress BRGlide: 0x100bcbf4
 */
extern s32 g_bHudShowRaceTimer;
/**
 * Whether to display race position.
 * @ghidraAddress BRD3D: 0x100bd3f8
 */
extern s32 g_bHudShowPosition;
/**
 * Whether to display the frame rate.
 * @ghidraAddress BRD3D: 0x10b501d8
 */
extern s32 g_bShowFpsCounter;
/**
 * Refresh game and renderer frame rates and draw the renderer rate.
 * @ghidraAddress BRD3D: 0x10014930
 * @ghidraAddress BRGlide: 0x10011ea0
 */
void DrawFpsCounterOverlay(void);
/**
 * Draw the active vehicle's total and lap times for the current game mode.
 * @param viewports The records of the views.
 * @ghidraAddress BRD3D: 0x10017290
 * @ghidraAddress BRGlide: 0x10014800
 */
void DrawRaceTimerHud(const ViewportRect *viewports);
/**
 * Draw the current lap and race position according to the HUD display flags.
 * @param viewports The records of the views.
 * @ghidraAddress BRD3D: 0x100173f0
 * @ghidraAddress BRGlide: 0x10014960
 */
void DrawLapCounterHud(const ViewportRect *viewports);
#else
/**
 * Draw the times the mode of play shows at the top right of a view.
 *
 * @ghidraAddress USA: 0x8023880c
 * @ghidraAddress EUR: 0x80239d1c
 * @ghidraAddress JPN: 0x8023981c
 */
void DrawRaceTimerHud(void);

/**
 * Draw the lap at the top left of a view and the place at its bottom left.
 *
 * @ghidraAddress USA: 0x80238ab8
 * @ghidraAddress EUR: 0x80239fc8
 * @ghidraAddress JPN: 0x80239ac8
 */
void DrawLapCounterHud(void);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Draw the active vehicle's message unless paused or fading out.
 * @param viewports The records of the views.
 * @ghidraAddress BRD3D: 0x10017790
 * @ghidraAddress BRGlide: 0x10014d00
 */
void DrawHudMessage(const ViewportRect *viewports);
#else
/**
 * Draw the message of the active vehicle in the middle of a view.
 *
 * @ghidraAddress USA: 0x80238dd4
 * @ghidraAddress EUR: 0x8023a2e4
 * @ghidraAddress JPN: 0x80239de4
 */
void DrawHudMessage(void);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Draw the dashboard, times, laps, messages, and speed for a viewport.
 * @param viewports The records of the views.
 * @param vehicles The vehicle array. The original lap counter ignores the forwarded argument.
 * @ghidraAddress BRD3D: 0x10017d90
 * @ghidraAddress BRGlide: 0x10015300
 */
void DrawSpeedAndHudOverlay(const ViewportRect *viewports, const Vehicle *vehicles);
#else
/**
 * Draw the whole display of a view: the dashboard and times unless the camera is on the bumper,
 * the lap, place and message, and the speed.
 *
 * @ghidraAddress USA: 0x80238f30
 * @ghidraAddress EUR: 0x8023a440
 * @ghidraAddress JPN: 0x80239f40
 */
void DrawSpeedAndHudOverlay(void);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/**
 * Whether the views draw the HUD and the help of the instant replay.
 *
 * @ghidraAddress USA: 0x8028aa6c
 * @ghidraAddress EUR: 0x8028cc74
 * @ghidraAddress JPN: 0x8028c744
 */
extern s32 g_bShowHudHelpText;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_HUD_H */

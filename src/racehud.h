#ifndef COASTLINE_RACEHUD_H
#define COASTLINE_RACEHUD_H

/**
 * @file
 * The screen of a race and its display.
 */

#include <PR/ultratypes.h>

/** Whether the race is paused. @ghidraAddress USA: 0x8026ff10 */
extern u32 g_dwRacePauseState;

/** The laps of the race. @ghidraAddress USA: 0x8028b304 */
extern s32 g_dwTotalLaps;

/**
 * Set once the loop of the race screen ends, and cleared when a race starts.
 *
 * @ghidraAddress USA: 0x802a4c04
 */
extern s16 g_wRaceScreenLoopExited;

/**
 * Choose the track of the next race of the championship.
 *
 * @ghidraAddress USA: 0x80206304
 */
void SetTrackFromSeason(void);

/**
 * Run one frame of a race.
 *
 * @ghidraAddress USA: 0x8020082c
 */
void ProcessRaceHud(void);

/**
 * Score the season of the championship: advance to the next season when its points allow, unlock
 * its bonuses, and write the lines of the message of its result.
 *
 * @ghidraAddress USA: 0x802063a4
 */
void ProcessSeasonResults(void);

/**
 * Whether a time attack left something to save, so that its results open the save menu.
 *
 * @ghidraAddress USA: 0x802707a8
 */
extern s32 g_dwSaveAvailable;

/** The points of each place of a race of the championship. @ghidraAddress USA: 0x802707ac */
extern u8 g_abRacePositionPoints[];

/** The lines of the message of the result of a season. @ghidraAddress USA: 0x80315db0 */
extern const char *g_apszSeasonResultMsgLines[];

/** @ghidraAddress USA: 0x802707c0 */
extern s32 g_dwSeasonResultMsgLineCount;

/** The names of the places in a race: "1st" and on. @ghidraAddress USA: 0x80271fe0 */
extern const char *g_apszPlaceNames[];

#endif

#ifndef COASTLINE_RACEHUD_H
#define COASTLINE_RACEHUD_H

/**
 * @file
 * The screen of a race and its display.
 */

#include <PR/ultratypes.h>

/** Whether the race is paused. @ghidraAddress USA: 0x8026ff10 */
extern u32 g_dwRacePauseState;

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

#endif

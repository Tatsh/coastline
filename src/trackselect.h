#ifndef COASTLINE_TRACKSELECT_H
#define COASTLINE_TRACKSELECT_H

/**
 * @file
 * The track selection screen.
 */

#include <PR/ultratypes.h>

/** The track chosen for the next race. @ghidraAddress USA: 0x8028b940 */
extern u32 g_dwSelectedTrackId;

/** The number of tracks. @ghidraAddress USA: 0x8028ae04 */
extern s32 g_dwNumTracks;

/**
 * Run one frame of the track selection screen.
 *
 * @ghidraAddress USA: 0x8020d004
 */
void ProcessTrackSelectScreen(void);

#endif

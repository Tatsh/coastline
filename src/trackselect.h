#ifndef COASTLINE_TRACKSELECT_H
#define COASTLINE_TRACKSELECT_H

/**
 * @file
 * The tracks: how many there are, and the one chosen for the next race.
 */

#include <PR/ultratypes.h>

/** The track chosen for the next race. @ghidraAddress USA: 0x8028b940 */
extern u32 g_dwSelectedTrackId;

/** The number of tracks. @ghidraAddress USA: 0x8028ae04 */
extern s32 g_dwNumTracks;

/**
 * Whether a track exists on the cartridge.
 *
 * @param track The index of the track.
 * @return Whether it is below g_dwNumTracks and has data.
 * @ghidraAddress USA: 0x8021e180
 */
s32 IsTrackIndexValid(s32 track);

#endif

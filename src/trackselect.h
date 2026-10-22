#ifndef COASTLINE_TRACKSELECT_H
#define COASTLINE_TRACKSELECT_H

/**
 * @file
 * The tracks: how many there are, and the one chosen for the next race.
 */

#include <PR/ultratypes.h>

/** The tracks, the mirrored ones TRACK_MIRRORED places after the others. */
enum {
    TRACK_DESERT,            /*!< The desert. */
    TRACK_MOUNTAIN,          /*!< The mountain. */
    TRACK_COASTLINE,         /*!< The coastline. */
    TRACK_STRIP_MINE,        /*!< The strip mine. */
    TRACK_JUNGLE,            /*!< The jungle. */
    TRACK_MIRROR_DESERT,     /*!< The desert, mirrored. */
    TRACK_MIRROR_MOUNTAIN,   /*!< The mountain, mirrored. */
    TRACK_MIRROR_COASTLINE,  /*!< The coastline, mirrored. */
    TRACK_MIRROR_STRIP_MINE, /*!< The strip mine, mirrored. */
    TRACK_MIRROR_JUNGLE,     /*!< The jungle, mirrored. */
};

/** The places between a track and its mirrored version. */
#define TRACK_MIRRORED 5

/**
 * The track chosen for the next race.
 *
 * @ghidraAddress USA: 0x8028b940
 * @ghidraAddress EUR: 0x8028dbf0
 * @ghidraAddress JPN: 0x8028d620
 */
extern u32 g_dwSelectedTrackId;

/**
 * The number of tracks.
 *
 * @ghidraAddress USA: 0x8028ae04
 * @ghidraAddress EUR: 0x8028d0b4
 * @ghidraAddress JPN: 0x8028cae4
 */
extern s32 g_dwNumTracks;

/**
 * Whether a track exists on the cartridge.
 *
 * @param track The index of the track.
 * @return Whether it is below g_dwNumTracks and has data.
 * @ghidraAddress USA: 0x8021e180
 * @ghidraAddress EUR: 0x8021f380
 * @ghidraAddress JPN: 0x8021ec50
 */
s32 IsTrackIndexValid(s32 track);

#endif

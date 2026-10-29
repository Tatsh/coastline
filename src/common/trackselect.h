#ifndef COASTLINE_TRACKSELECT_H
#define COASTLINE_TRACKSELECT_H

/**
 * @file
 * The tracks: how many there are, and the one chosen for the next race.
 */

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The tracks, the mirrored ones #TRACK_MIRRORED places after the others. */
enum {
    TRACK_DESERT,     /*!< The desert. */
    TRACK_MOUNTAIN,   /*!< The mountain. */
    TRACK_COASTLINE,  /*!< The coastline. */
    TRACK_STRIP_MINE, /*!< The strip mine. */
    TRACK_JUNGLE,     /*!< The jungle. */
#if TGR_BOSS_RALLY
    TRACK_MIRROR_DESERT = 6, /*!< The desert, mirrored. */
#else
    TRACK_MIRROR_DESERT, /*!< The desert, mirrored. */
#endif
    TRACK_MIRROR_MOUNTAIN,   /*!< The mountain, mirrored. */
    TRACK_MIRROR_COASTLINE,  /*!< The coastline, mirrored. */
    TRACK_MIRROR_STRIP_MINE, /*!< The strip mine, mirrored. */
    TRACK_MIRROR_JUNGLE,     /*!< The jungle, mirrored. */
};

/** The places between a track and its mirrored version. */
#if TGR_BOSS_RALLY
#define TRACK_MIRRORED 6
#else
#define TRACK_MIRRORED 5
#endif

/**
 * The track chosen for the next race.
 *
 * @ghidraAddress USA: 0x8028b940
 * @ghidraAddress EUR: 0x8028dbf0
 * @ghidraAddress JPN: 0x8028d620
 * @ghidraAddress BRD3D: 0x100b380c
 * @ghidraAddress BRGlide: 0x100b3014
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
 * @return Whether the track has data on N64, or 1 on Windows.
 * @ghidraAddress USA: 0x8021e180
 * @ghidraAddress EUR: 0x8021f380
 * @ghidraAddress JPN: 0x8021ec50
 * @ghidraAddress BRD3D: 0x10035b87
 * @ghidraAddress BRGlide: 0x1002f238
 */
s32 IsTrackIndexValid(s32 track);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TRACKSELECT_H */

#ifndef COASTLINE_PROFILE_H
#define COASTLINE_PROFILE_H

/**
 * @file
 * The saved settings and progress of a player, and the size of the championship.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The size of the championship. */
#define SEASONS 6      /*!< The seasons of the championship. */
#define SEASON_RACES 4 /*!< The most races of a season. */

/** The tracks a profile keeps records for: the five tracks and their mirrored versions. */
#define PROFILE_RECORD_TRACKS 10

/**
 * The saved settings and progress of a player. Its fields join this structure as the routines that
 * use them are reconstructed.
 */
typedef struct {
    s32 championshipFlags; /*!< The progress of the championship across its years, negative when
                                never set, with bit 0 set while the seasons race the tracks
                                mirrored. */
    u8 season;             /*!< The season of the championship being raced, from 0. */
    u8 round;              /*!< The race of the season being raced, from 0. */
    u8 seasonPlaces[SEASONS][SEASON_RACES]; /*!< The place of each race of each season, from 0 for
                                                 the winner. */
    u16 seasonPoints[SEASONS];              /*!< The points scored in each season. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u8 unreconstructed2A[0x50 - 0x2A]; /*!< Not read by the routines reconstructed so far. */
#else
    u8 unreconstructed2A[0x2C - 0x2A]; /*!< Padding before seasonTimes. */
#endif
    f32 seasonTimes[SEASONS][SEASON_RACES]; /*!< The time of each race of each season, in
                                                 seconds. */
    f32 lapRecords[PROFILE_RECORD_TRACKS];  /*!< The best lap on each track, in seconds, or 0 when
                                                 none was set. */
    u8 unreconstructedB4[0xCC - 0xB4];      /*!< Not read by the routines reconstructed so far. */
    u16 unlocks;          /*!< The unlocked cars, a bit for each from bit 0, and further unlocks
                               above them. */
    u16 unlockedTracks;   /*!< The unlocked tracks, a bit for each. */
    u16 unlockedWeathers; /*!< The weathers the championship has raced in, a bit for each. */
    u8 unreconstructedD2[0xD4 - 0xD2]; /*!< Padding before handling. */
    s32 handling;         /*!< The handling option the player chose last, the steering mode of
                               the vehicle from 0. */
    s32 transmission;     /*!< The gearbox option the player chose last, nonzero for the
                               automatic. */
    s32 tires;            /*!< The tyre option the player chose last; the grip falls by 0.002 for
                               each option up. */
    s32 suspension;       /*!< The suspension option the player chose last, from 0 for the
                               softest springs. */
    s32 controllerLayout; /*!< The button layout the player chose on the controller screen. */
    f32 raceRecords[PROFILE_RECORD_TRACKS]; /*!< The best race on each track, in seconds, or 0 when
                                                 none was set. */
    u8 unreconstructed110[0x128 - 0x110];   /*!< Not read by the routines reconstructed so far. */
} PlayerProfile;

/**
 * The profile of the player being played.
 *
 * @ghidraAddress USA: 0x8031c5bc
 * @ghidraAddress EUR: 0x8031eabc
 * @ghidraAddress JPN: 0x8031d6ec
 */
extern PlayerProfile *g_pActiveProfile;

/**
 * The profiles of the two players a controller pak saves.
 *
 * @ghidraAddress USA: 0x80324300
 * @ghidraAddress EUR: 0x80326840
 * @ghidraAddress JPN: 0x80325470
 */
extern PlayerProfile g_aSeasonSaveBuffers[2];

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * The profile of the player being played.
 *
 * @ghidraAddress BRD3D: 0x10aa26f0
 * @ghidraAddress BRGlide: 0x10ac5a48
 */
extern PlayerProfile g_SeasonProgress;
#endif

#ifdef __cplusplus
}
#endif

#endif

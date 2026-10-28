#ifndef COASTLINE_GAME_H
#define COASTLINE_GAME_H

/**
 * @file
 * The main thread of the game, the weather of a race, and the player slots.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Number of local players.
 *
 * @ghidraAddress USA: 0x8026ff08
 * @ghidraAddress EUR: 0x80271ed8
 * @ghidraAddress JPN: 0x80271988
 * @ghidraAddress BRD3D: 0x100b4050
 */
extern u32 g_dwNumLocalPlayers;

/** The modes of play, in the order of the main menu. */
enum {
    GAME_MODE_CHAMPIONSHIP,     /*!< The seasons of the championship. */
    GAME_MODE_ARCADE,           /*!< A race of two vehicles with checkpoints that add time. */
    GAME_MODE_TIME_ATTACK,      /*!< Laps against the clock and a ghost car. */
    GAME_MODE_PRACTICE,         /*!< Free driving on a track, counting no laps. */
    GAME_MODE_DEMO,             /*!< The race the computer drives after the splash screens. */
    GAME_MODE_AFTER_SEASON = 5, /*!< The race that follows the results of a finished season. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    GAME_MODE_NETWORK = 6, /*!< A race of network play. */
#endif
};

/** The races the computer drives in GAME_MODE_DEMO. */
enum {
    RACE_SESSION_ATTRACT,           /*!< The race after the splash screens. */
    RACE_SESSION_CREDIT_CAPTIONS,   /*!< The race under the captions of the credits, from the
                                         options menu. */
    RACE_SESSION_SCROLLING_CREDITS, /*!< The race under the scrolling credits, after the
                                         championship or from the options menu. */
};

/**
 * The mode of play the main menu chose.
 *
 * @ghidraAddress USA: 0x8026ff18
 * @ghidraAddress EUR: 0x80271ee8
 * @ghidraAddress JPN: 0x80271998
 * @ghidraAddress BRD3D: 0x100aa010
 * @ghidraAddress BRGlide: 0x100a9360
 */
extern s32 g_dwGameMode;

/**
 * The race the computer drives, by the RACE_SESSION_ values.
 *
 * @ghidraAddress USA: 0x8026ff1c
 * @ghidraAddress EUR: 0x80271eec
 * @ghidraAddress JPN: 0x8027199c
 * @ghidraAddress BRD3D: 0x106805b8
 * @ghidraAddress BRGlide: 0x105bc760
 */
extern s32 g_dwRaceSessionType;

/**
 * Set weather flags and, on N64, patch display lists to match.
 *
 * @param weather The weather, a `WEATHER_*` value.
 * @ghidraAddress USA: 0x80200050
 * @ghidraAddress EUR: 0x80200050
 * @ghidraAddress JPN: 0x80200050
 * @ghidraAddress BRD3D: 0x10069740
 * @ghidraAddress BRGlide: 0x100627b0
 */
void ApplyWeather(u32 weather);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Set weather flags and refresh vehicle materials.
 *
 * @param weather Weather option.
 * @ghidraAddress BRD3D: 0x100697c0
 */
void SetWeatherAndRefreshVehicleMaterials(u32 weather);
#endif

/**
 * The screen routine of the main menu.
 *
 * @ghidraAddress USA: 0x80200108
 * @ghidraAddress EUR: 0x80200108
 * @ghidraAddress JPN: 0x80200108
 */
void MainMenuEntry(void);

/**
 * Set the number of local players and select the initial race callback.
 *
 * @param players The number of local players.
 * @ghidraAddress USA: 0x80200128
 * @ghidraAddress EUR: 0x80200128
 * @ghidraAddress JPN: 0x80200128
 * @ghidraAddress BRD3D: 0x100697e0
 * @ghidraAddress BRGlide: 0x10062850
 */
void StartRaceWithPlayerCount(u32 players);

/**
 * Reset the vehicle and the input of every player.
 *
 * @ghidraAddress USA: 0x80200154
 * @ghidraAddress EUR: 0x80200154
 * @ghidraAddress JPN: 0x80200154
 * @ghidraAddress BRD3D: 0x10069800
 * @ghidraAddress BRGlide: 0x10062870
 */
void ResetAllPlayers(void);

/**
 * The entry of the main thread. Set up the system, run the opening menus, start the music, and
 * enter the main loop. The routine takes no parameter, and the argument of the thread is ignored.
 *
 * @ghidraAddress USA: 0x802001b4
 * @ghidraAddress EUR: 0x802001b4
 * @ghidraAddress JPN: 0x802001b4
 * @ghidraAddress BRD3D: 0x10069840
 * @ghidraAddress BRGlide: 0x100628b0
 */
void InitGameAndShowMenus(void);

/**
 * Start the race the computer drives after the splash screens.
 *
 * @ghidraAddress USA: 0x802005fc
 * @ghidraAddress EUR: 0x8020062c
 * @ghidraAddress JPN: 0x802005ec
 */
void StartAttractDemo(void);

/**
 * Start the race the computer drives under the credits, which the options menu offers: the
 * scrolling credits in two-player mode and the captions otherwise.
 *
 * @ghidraAddress USA: 0x80200634
 * @ghidraAddress EUR: 0x80200664
 * @ghidraAddress JPN: 0x80200624
 */
void StartCreditsRace(void);

/**
 * Start the race under the scrolling credits, after the last season of the championship.
 *
 * @ghidraAddress USA: 0x8020068c
 * @ghidraAddress EUR: 0x802006bc
 * @ghidraAddress JPN: 0x8020067c
 * @ghidraAddress BRD3D: 0x1002c390
 * @ghidraAddress BRGlide: 0x10019900
 */
void StartEndingCredits(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_GAME_H */

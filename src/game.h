#ifndef COASTLINE_GAME_H
#define COASTLINE_GAME_H

/**
 * @file
 * The main thread of the game, the weather of a race, and the player slots.
 */

#include <PR/ultratypes.h>

/** The modes of play, in the order of the main menu. */
enum {
    GAME_MODE_CHAMPIONSHIP,     /*!< The seasons of the championship. */
    GAME_MODE_ARCADE,           /*!< A race of two vehicles with checkpoints that add time. */
    GAME_MODE_TIME_ATTACK,      /*!< Laps against the clock and a ghost car. */
    GAME_MODE_PRACTICE,         /*!< Free driving on a track, counting no laps. */
    GAME_MODE_DEMO,             /*!< The race the computer drives after the splash screens. */
    GAME_MODE_AFTER_SEASON = 5, /*!< The race that follows the results of a finished season. */
};

/** The races the computer drives in GAME_MODE_DEMO. */
enum {
    RACE_SESSION_ATTRACT,           /*!< The race after the splash screens. */
    RACE_SESSION_CREDIT_CAPTIONS,   /*!< The race under the captions of the credits, from the
                                         options menu. */
    RACE_SESSION_SCROLLING_CREDITS, /*!< The race under the scrolling credits, after the
                                         championship or from the options menu. */
};

/** The mode of play the main menu chose. @ghidraAddress USA: 0x8026ff18 */
extern u32 g_dwGameMode;

/** The race the computer drives, by the RACE_SESSION_ values. @ghidraAddress USA: 0x8026ff1c */
extern s32 g_dwRaceSessionType;

/**
 * Set the weather flags for a weather of the weather menu, and patch the display lists to match.
 *
 * @param weather The weather, a `WEATHER_*` value.
 * @ghidraAddress USA: 0x80200050
 */
void ApplyWeather(u32 weather);

/**
 * The screen routine of the main menu.
 *
 * @ghidraAddress USA: 0x80200108
 */
void MainMenuEntry(void);

/**
 * Set the number of local players and start the race screen. Nothing calls it.
 *
 * @param players The number of local players.
 * @ghidraAddress USA: 0x80200128
 */
void StartRaceWithPlayerCount(u32 players);

/**
 * Reset the vehicle and the input of every player.
 *
 * @ghidraAddress USA: 0x80200154
 */
void ResetAllPlayers(void);

/**
 * The entry of the main thread. Set up the system, run the opening menus, start the music, and
 * enter the main loop. The routine takes no parameter, and the argument of the thread is ignored.
 *
 * @ghidraAddress USA: 0x802001b4
 */
void InitGameAndShowMenus(void);

/**
 * Start the race the computer drives after the splash screens.
 *
 * @ghidraAddress USA: 0x802005fc
 */
void StartAttractDemo(void);

/**
 * Start the race the computer drives under the credits, which the options menu offers: the
 * scrolling credits in two-player mode and the captions otherwise.
 *
 * @ghidraAddress USA: 0x80200634
 */
void StartCreditsRace(void);

/**
 * Start the race under the scrolling credits, after the last season of the championship.
 *
 * @ghidraAddress USA: 0x8020068c
 */
void StartEndingCredits(void);

#endif

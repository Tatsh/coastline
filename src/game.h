#ifndef COASTLINE_GAME_H
#define COASTLINE_GAME_H

/**
 * @file
 * The main thread of the game, the weather of a race, and the player slots.
 */

#include <PR/ultratypes.h>

/** The modes of play, in the order of the main menu. */
enum {
    GAME_MODE_CHAMPIONSHIP,
    GAME_MODE_ARCADE,
    GAME_MODE_TIME_ATTACK,
    GAME_MODE_PRACTICE,
    /** The race the computer drives after the splash screens. */
    GAME_MODE_DEMO,
    /** The race that follows the results of a finished season. */
    GAME_MODE_AFTER_SEASON = 5,
};

/** The mode of play the main menu chose. @ghidraAddress USA: 0x8026ff18 */
extern u32 g_dwGameMode;

/**
 * Set the weather flags for a weather of the weather menu, and patch the display lists to match.
 *
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
 * enter the main loop.
 *
 * @ghidraAddress USA: 0x802001b4
 */
void InitGameAndShowMenus(void *arg);

/**
 * Start the race the computer drives after the splash screens.
 *
 * @ghidraAddress USA: 0x802005fc
 */
void StartAttractDemo(void);

#endif

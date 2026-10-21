#ifndef COASTLINE_GAME_H
#define COASTLINE_GAME_H

/**
 * @file
 * The main thread of the game, the weather of a race, and the player slots.
 */

#include <PR/ultratypes.h>

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

#endif

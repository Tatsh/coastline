#ifndef COASTLINE_MAINMENU_H
#define COASTLINE_MAINMENU_H

/**
 * @file
 * The main menu, and the starts of a championship or a time attack that the save menu also uses
 * after loading.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The items of the main menu, ending with NULL.
 *
 * @ghidraAddress USA: 0x80272360
 * @ghidraAddress Asia: 0x80271EB0
 * @ghidraAddress EUR: 0x80274340
 * @ghidraAddress JPN: 0x80273df0
 */
extern MenuItem *g_apMainMenuItems[];

/**
 * The picture of the title of the main menu.
 *
 * @ghidraAddress USA: 0x802722c0
 * @ghidraAddress Asia: 0x80271E10
 * @ghidraAddress EUR: 0x802742a0
 */
extern Texture g_MainMenuTitleTextureDesc;

/**
 * The start that led back to the main menu: 1 for a championship, 2 for a time attack, and 0
 * otherwise. The main menu selects the matching item and clears it.
 *
 * @ghidraAddress USA: 0x80272380
 * @ghidraAddress Asia: 0x80271ED0
 * @ghidraAddress EUR: 0x80274360
 * @ghidraAddress JPN: 0x80273e10
 */
extern s32 g_dwMainMenuEntrySource;

/**
 * @ghidraAddress USA: 0x80316380
 * @ghidraAddress Asia: 0x80315E30
 * @ghidraAddress EUR: 0x80318060
 * @ghidraAddress JPN: 0x80316c90
 */
extern s32 g_dwMainMenuItemCount;

/**
 * Whether the track selection leads to the paint shop.
 *
 * @ghidraAddress USA: 0x80272070
 * @ghidraAddress Asia: 0x80271BC0
 * @ghidraAddress EUR: 0x80274050
 * @ghidraAddress JPN: 0x80273afc
 */
extern s32 g_dwIsPaintMode;

/**
 * Start the next race of the championship for one player.
 *
 * @ghidraAddress USA: 0x80211150
 * @ghidraAddress Asia: 0x80210CE0
 * @ghidraAddress EUR: 0x80210fb0
 * @ghidraAddress JPN: 0x80210ea0
 */
void StartChampionshipRace(void);

/**
 * Start a time attack for one player.
 *
 * @ghidraAddress USA: 0x80211194
 * @ghidraAddress Asia: 0x80210D24
 * @ghidraAddress EUR: 0x80210ff4
 * @ghidraAddress JPN: 0x80210ee4
 */
void StartTimeAttack(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_MAINMENU_H */

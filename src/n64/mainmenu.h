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
 * @ghidraAddress EUR: 0x80274340
 * @ghidraAddress JPN: 0x80273df0
 */
extern MenuItem *g_apMainMenuItems[];

/**
 * The picture of the title of the main menu.
 *
 * @ghidraAddress USA: 0x802722c0
 * @ghidraAddress EUR: 0x802742a0
 */
extern Texture g_MainMenuTitleTextureDesc;

/**
 * The start that led back to the main menu: 1 for a championship, 2 for a time attack, and 0
 * otherwise. The main menu selects the matching item and clears it.
 *
 * @ghidraAddress USA: 0x80272380
 * @ghidraAddress EUR: 0x80274360
 * @ghidraAddress JPN: 0x80273e10
 */
extern s32 g_dwMainMenuEntrySource;

/**
 * The state of the main menu: 0 before it starts, 1 while it runs, and -1 once it is left.
 *
 * @ghidraAddress USA: 0x80272384
 * @ghidraAddress EUR: 0x80274364
 * @ghidraAddress JPN: 0x80273e14
 */
extern s32 g_dwMainMenuInitState;

/**
 * @ghidraAddress USA: 0x80316380
 * @ghidraAddress EUR: 0x80318060
 * @ghidraAddress JPN: 0x80316c90
 */
extern s32 g_dwMainMenuItemCount;

/**
 * @ghidraAddress USA: 0x80316384
 * @ghidraAddress EUR: 0x80318064
 * @ghidraAddress JPN: 0x80316c94
 */
extern s32 g_eMainMenuSelection;

/**
 * Whether the track selection leads to the paint shop.
 *
 * @ghidraAddress USA: 0x80272070
 * @ghidraAddress EUR: 0x80274050
 * @ghidraAddress JPN: 0x80273afc
 */
extern s32 g_dwIsPaintMode;

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
/**
 * The presses of the right C button so far with L and R held on the main menu. The third unlocks
 * every car, track, and weather.
 *
 * @ghidraAddress EUR: 0x80274368
 * @ghidraAddress JPN: 0x80273e18
 */
extern s32 g_dwUnlockAllCheatPressCount;
#endif /* TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP */

/**
 * Start the next race of the championship for one player.
 *
 * @ghidraAddress USA: 0x80211150
 * @ghidraAddress EUR: 0x80210fb0
 * @ghidraAddress JPN: 0x80210ea0
 */
void StartChampionshipRace(void);

/**
 * Start a time attack for one player.
 *
 * @ghidraAddress USA: 0x80211194
 * @ghidraAddress EUR: 0x80210ff4
 * @ghidraAddress JPN: 0x80210ee4
 */
void StartTimeAttack(void);

/**
 * Run one frame of the main menu.
 *
 * @ghidraAddress USA: 0x802111e0
 * @ghidraAddress EUR: 0x80211040
 * @ghidraAddress JPN: 0x80210f30
 */
void ProcessMainMenu(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_MAINMENU_H */

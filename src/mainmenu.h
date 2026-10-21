#ifndef COASTLINE_MAINMENU_H
#define COASTLINE_MAINMENU_H

/**
 * @file
 * The main menu, and the starts of a championship or a time attack that the save menu also uses
 * after loading.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"

/** The items of the main menu, ending with NULL. @ghidraAddress USA: 0x80272360 */
extern MenuItem *g_apMainMenuItems[];

/** The picture of the title of the main menu. @ghidraAddress USA: 0x802722c0 */
extern Texture g_MainMenuTitleTextureDesc;

/**
 * The start that led back to the main menu: 1 for a championship, 2 for a time attack, and 0
 * otherwise. The main menu selects the matching item and clears it.
 *
 * @ghidraAddress USA: 0x80272380
 */
extern s32 g_dwMainMenuEntrySource;

/**
 * The state of the main menu: 0 before it starts, 1 while it runs, and -1 once it is left.
 *
 * @ghidraAddress USA: 0x80272384
 */
extern s32 g_dwMainMenuInitState;

/** @ghidraAddress USA: 0x80316380 */
extern s32 g_dwMainMenuItemCount;

/** @ghidraAddress USA: 0x80316384 */
extern s32 g_eMainMenuSelection;

/** Whether the track selection leads to the paint shop. @ghidraAddress USA: 0x80272070 */
extern s32 g_dwIsPaintMode;

/**
 * Start the next race of the championship for one player.
 *
 * @ghidraAddress USA: 0x80211150
 */
void StartChampionshipRace(void);

/**
 * Start a time attack for one player.
 *
 * @ghidraAddress USA: 0x80211194
 */
void StartTimeAttack(void);

/**
 * Run one frame of the main menu.
 *
 * @ghidraAddress USA: 0x802111e0
 */
void ProcessMainMenu(void);

#endif

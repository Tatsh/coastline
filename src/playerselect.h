#ifndef COASTLINE_PLAYERSELECT_H
#define COASTLINE_PLAYERSELECT_H

/**
 * @file
 * The screen that chooses one or two players.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"

/** The items of the screen, ending with NULL. @ghidraAddress USA: 0x802723b8 */
extern MenuItem *g_apPlayerCountMenuItems[];

/**
 * The state of the screen: 0 before it starts, 1 while it runs, and -1 once it is left.
 *
 * @ghidraAddress USA: 0x802723c4
 */
extern s32 g_dwPlayerCountSelectState;

/** @ghidraAddress USA: 0x80316390 */
extern s32 g_dwPlayerCountMenuItemCount;

/** @ghidraAddress USA: 0x80316394 */
extern s32 g_dwPlayerCountMenuSelection;

/**
 * Explain below the menu that two players need two controllers, while that item is selected and
 * unavailable.
 *
 * @ghidraAddress USA: 0x802114e0
 */
void TwoControllersPostDrawCallback(s32 item);

/**
 * Run one frame of the player count screen.
 *
 * @ghidraAddress USA: 0x802116e0
 */
void ProcessPlayerCountSelect(void);

#endif

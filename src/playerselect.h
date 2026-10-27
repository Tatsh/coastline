#ifndef COASTLINE_PLAYERSELECT_H
#define COASTLINE_PLAYERSELECT_H

/**
 * @file
 * The screen that chooses one or two players.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"

/**
 * The items of the screen, ending with NULL.
 *
 * @ghidraAddress USA: 0x802723b8
 * @ghidraAddress EUR: 0x80274398
 * @ghidraAddress JPN: 0x80273e48
 */
extern MenuItem *g_apPlayerCountMenuItems[];

/**
 * @ghidraAddress USA: 0x80316390
 * @ghidraAddress EUR: 0x80318070
 * @ghidraAddress JPN: 0x80316ca0
 */
extern s32 g_dwPlayerCountMenuItemCount;

/**
 * @ghidraAddress USA: 0x80316394
 * @ghidraAddress EUR: 0x80318074
 * @ghidraAddress JPN: 0x80316ca4
 */
extern s32 g_dwPlayerCountMenuSelection;

/**
 * Explain below the menu that two players need two controllers, while that item is selected and
 * unavailable.
 *
 * @param item The index of the selected item of the player count menu.
 * @ghidraAddress USA: 0x802114e0
 * @ghidraAddress EUR: 0x802113d0
 * @ghidraAddress JPN: 0x802112c0
 */
void TwoControllersPostDrawCallback(s32 item);

/**
 * Run one frame of the player count screen.
 *
 * @ghidraAddress USA: 0x802116e0
 * @ghidraAddress EUR: 0x802115d0
 * @ghidraAddress JPN: 0x802114c0
 */
void ProcessPlayerCountSelect(void);

#endif /* COASTLINE_PLAYERSELECT_H */

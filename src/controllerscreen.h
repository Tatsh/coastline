#ifndef COASTLINE_CONTROLLERSCREEN_H
#define COASTLINE_CONTROLLERSCREEN_H

/**
 * @file
 * The options screen where a player chooses the button layout of their controller.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"
#include "texrect.h"

/** The pictures of the five button layouts. @ghidraAddress USA: 0x80272590 */
extern Texture g_texControllerLayout0;

/** @ghidraAddress USA: 0x802725a4 */
extern Texture g_texControllerLayout1;

/** @ghidraAddress USA: 0x802725b8 */
extern Texture g_texControllerLayout2;

/** @ghidraAddress USA: 0x802725cc */
extern Texture g_texControllerLayout3;

/** @ghidraAddress USA: 0x802725e0 */
extern Texture g_texControllerLayout4;

/** The title picture of the options screens. @ghidraAddress USA: 0x8027205c */
extern Texture g_TitleBarTextureDesc;

/** The items of the screen, one per layout. @ghidraAddress USA: 0x80272658 */
extern MenuItem *g_apControllerScreenItems[];

/**
 * The state of the screen: 0 before it starts, 1 while it runs.
 *
 * @ghidraAddress USA: 0x80272670
 */
extern s32 g_dwControllerScreenInitState;

/** @ghidraAddress USA: 0x80316440 */
extern s32 g_dwControllerScreenItemCount;

/** The layout under the cursor. @ghidraAddress USA: 0x80316444 */
extern s32 g_eControllerScreenSelection;

/**
 * Draw the picture of a button layout with the action of each button.
 *
 * @param layout The index of the layout, from 0 to 4.
 * @ghidraAddress USA: 0x802167e0
 */
void DrawControllerLayoutDiagram(s32 layout);

/**
 * Run one frame of the controller screen of the player who opened the options.
 *
 * @ghidraAddress USA: 0x802170c8
 */
void ProcessControllerConfigScreen(void);

#endif

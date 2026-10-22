#ifndef COASTLINE_CONTROLLERSCREEN_H
#define COASTLINE_CONTROLLERSCREEN_H

/**
 * @file
 * The options screen where a player chooses the button layout of their controller.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"
#include "texrect.h"

/**
 * The pictures of the five button layouts.
 *
 * @ghidraAddress USA: 0x80272590
 * @ghidraAddress JPN: 0x802740b0
 * @ghidraAddress EUR: 0x80274600
 */
extern Texture g_texControllerLayout0;

/**
 * @ghidraAddress USA: 0x802725a4
 * @ghidraAddress JPN: 0x802740c4
 * @ghidraAddress EUR: 0x80274614
 */
extern Texture g_texControllerLayout1;

/**
 * @ghidraAddress USA: 0x802725b8
 * @ghidraAddress JPN: 0x802740d8
 * @ghidraAddress EUR: 0x80274628
 */
extern Texture g_texControllerLayout2;

/**
 * @ghidraAddress USA: 0x802725cc
 * @ghidraAddress JPN: 0x802740ec
 * @ghidraAddress EUR: 0x8027463c
 */
extern Texture g_texControllerLayout3;

/**
 * @ghidraAddress USA: 0x802725e0
 * @ghidraAddress JPN: 0x80274100
 * @ghidraAddress EUR: 0x80274650
 */
extern Texture g_texControllerLayout4;

/**
 * The title picture of the options screens.
 *
 * @ghidraAddress USA: 0x8027205c
 * @ghidraAddress JPN: 0x80273d50
 * @ghidraAddress EUR: 0x8027403c
 */
extern Texture g_TitleBarTextureDesc;

/**
 * The items of the screen, one per layout.
 *
 * @ghidraAddress USA: 0x80272658
 * @ghidraAddress JPN: 0x80274178
 * @ghidraAddress EUR: 0x802746b4
 */
extern MenuItem *g_apControllerScreenItems[];

/**
 * Draw the picture of a button layout with the action of each button.
 *
 * @param layout The index of the layout, from 0 to 4.
 * @ghidraAddress USA: 0x802167e0
 * @ghidraAddress JPN: 0x80217150
 * @ghidraAddress EUR: 0x80217800
 */
void DrawControllerLayoutDiagram(s32 layout);

/**
 * Run one frame of the controller screen of the player who opened the options.
 *
 * @ghidraAddress USA: 0x802170c8
 * @ghidraAddress JPN: 0x80217a38
 * @ghidraAddress EUR: 0x802180e8
 * @stub{EUR JPN}
 */
void ProcessControllerConfigScreen(void);

#endif

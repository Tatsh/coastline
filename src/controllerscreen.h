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
 * @ghidraAddress EUR: 0x80274600
 * @ghidraAddress JPN: 0x802740b0
 */
extern Texture g_texControllerLayout0;

/**
 * @ghidraAddress USA: 0x802725a4
 * @ghidraAddress EUR: 0x80274614
 * @ghidraAddress JPN: 0x802740c4
 */
extern Texture g_texControllerLayout1;

/**
 * @ghidraAddress USA: 0x802725b8
 * @ghidraAddress EUR: 0x80274628
 * @ghidraAddress JPN: 0x802740d8
 */
extern Texture g_texControllerLayout2;

/**
 * @ghidraAddress USA: 0x802725cc
 * @ghidraAddress EUR: 0x8027463c
 * @ghidraAddress JPN: 0x802740ec
 */
extern Texture g_texControllerLayout3;

/**
 * @ghidraAddress USA: 0x802725e0
 * @ghidraAddress EUR: 0x80274650
 * @ghidraAddress JPN: 0x80274100
 */
extern Texture g_texControllerLayout4;

/**
 * The title picture of the options screens.
 *
 * @ghidraAddress USA: 0x8027205c
 * @ghidraAddress EUR: 0x8027403c
 * @ghidraAddress JPN: 0x80273d50
 */
extern Texture g_TitleBarTextureDesc;

/**
 * The items of the screen, one per layout.
 *
 * @ghidraAddress USA: 0x80272658
 * @ghidraAddress EUR: 0x802746b4
 * @ghidraAddress JPN: 0x80274178
 */
extern MenuItem *g_apControllerScreenItems[];

/**
 * Draw the picture of a button layout with the action of each button.
 *
 * @param layout The index of the layout, from 0 to 4.
 * @ghidraAddress USA: 0x802167e0
 * @ghidraAddress EUR: 0x80217800
 * @ghidraAddress JPN: 0x80217150
 */
void DrawControllerLayoutDiagram(s32 layout);

/**
 * Run one frame of the controller screen of the player who opened the options.
 *
 * @ghidraAddress USA: 0x802170c8
 * @ghidraAddress EUR: 0x802180e8
 * @ghidraAddress JPN: 0x80217a38
 */
void ProcessControllerConfigScreen(void);

#endif

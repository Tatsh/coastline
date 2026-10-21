#ifndef COASTLINE_PAINTSHOP_H
#define COASTLINE_PAINTSHOP_H

/**
 * @file
 * The paint shop, where the players paint decals, and the saving and loading of decals on the
 * controller pak.
 */

#include <PR/ultratypes.h>

#include "decaleditor.h"
#include "sprite.h"
#include "vecmath.h"
#include "vehicle.h"

/** The result of the dialog of the controller pak when the player cancels it. */
#define PAK_DIALOG_CANCELLED 999

/** The error of the dialog of the controller pak while it waits for the rumble pak to go. */
#define PAK_ERROR_RUMBLE_PAK 9999

/** The error of a transfer of decals that the controller pak rejected as invalid. */
#define PAK_ERROR_TRANSFER 99999

/** The bytes of the texels of a decal, at 4 bits a texel. */
#define DECAL_BUFFER_SIZE 0x800

/** The picture of a whole car on the menu of the clear tool. @ghidraAddress USA: 0x8028cba0 */
extern Sprite g_texPaintWholeCar;

/** The buttons of the decal panels. @ghidraAddress USA: 0x8028cbd0 */
extern Sprite g_texPanelLeftDoor;

/** @ghidraAddress USA: 0x8028cc00 */
extern Sprite g_texPanelRightDoor;

/** @ghidraAddress USA: 0x8028cc30 */
extern Sprite g_texPanelLeftFrontFender;

/** @ghidraAddress USA: 0x8028cc60 */
extern Sprite g_texPanelRightFrontFender;

/** @ghidraAddress USA: 0x8028cc90 */
extern Sprite g_texPanel4;

/** @ghidraAddress USA: 0x8028ccc0 */
extern Sprite g_texPanel5;

/** @ghidraAddress USA: 0x8028ccf0 */
extern Sprite g_texPanelLeftRearFender;

/** @ghidraAddress USA: 0x8028cd20 */
extern Sprite g_texPanelRightRearFender;

/** @ghidraAddress USA: 0x8028cd50 */
extern Sprite g_texPanel8;

/** @ghidraAddress USA: 0x8028cd80 */
extern Sprite g_texPanel9;

/** The lights drawn over the buttons of panels 8 and 9. @ghidraAddress USA: 0x8028cdb0 */
extern Sprite g_texPanel8Overlay;

/** @ghidraAddress USA: 0x8028cde0 */
extern Sprite g_texPanel9Overlay;

/** The brush sizes of the round brush, top to bottom. @ghidraAddress USA: 0x8028ce10 */
extern Sprite g_texBrushSizeRound;

/** The brush sizes of the square brush. @ghidraAddress USA: 0x8028ce40 */
extern Sprite g_texBrushSizeSquare;

/**
 * The button of the mirror; its style is the choice of its menu, 0 to mirror, 1 to flip
 * horizontally, and 2 to flip vertically.
 *
 * @ghidraAddress USA: 0x8028cf90
 */
extern Sprite g_texPaintToolMirror;

/** @ghidraAddress USA: 0x8028d140 */
extern Sprite g_texPaintFillCursor;

/** @ghidraAddress USA: 0x8028d170 */
extern Sprite g_texPaintBrushCursor;

/** @ghidraAddress USA: 0x8028d1a0 */
extern Sprite g_texPaintCrosshairCursor;

/** @ghidraAddress USA: 0x8028d1d0 */
extern Sprite g_texPaintTextCursor;

/** @ghidraAddress USA: 0x8028d200 */
extern Sprite g_texPaintEyedropperCursor;

/** The keyboard of the text tool. @ghidraAddress USA: 0x8028d230 */
extern Sprite g_texKeyboardPanel;

/** The backing of the keys of the keyboard. @ghidraAddress USA: 0x8028d260 */
extern Sprite g_texKeyboardBackground;

/**
 * The pictures of the buttons of the paint shop, by their PAINT_TOOL_ values.
 *
 * @ghidraAddress USA: 0x8028dad4
 * @ghidraAddress JPN: 0x8028fca4
 * @ghidraAddress EUR: 0x80290234
 */
extern Sprite *g_apPaintToolIcons[PAINT_TOOLS];

/** The texels of the decal before the last edit. @ghidraAddress USA: 0x8028db80 */
extern u8 *g_pDecalUndoBuffer;

/** The key under the cursor. @ghidraAddress USA: 0x8028dba4 */
extern u8 g_bKeyboardHoverKey;

/** Whether the car is drawn beside the decal. @ghidraAddress USA: 0x8028dbd8 */
extern u8 g_bShowPreview3D;

/** Whether the undo buffer has an edit. @ghidraAddress USA: 0x8028dbdc */
extern u8 g_bUndoAvailable;

/** Whether the B button picks the color under the cursor. @ghidraAddress USA: 0x8028dbec */
extern u8 g_bEyedropperActive;

/** The forward axis of the car that shows each decal slot. @ghidraAddress USA: 0x8028dc08 */
extern Vec3f s_aPanelViewDirections[DECAL_SLOTS];

/** @ghidraAddress USA: 0x8028dc80 */
extern u8 g_bPaintShopInitialized;

/** The texels of the decal a mirror copies from. @ghidraAddress USA: 0x8028dca4 */
extern u8 *s_pMirrorSource;

/** @ghidraAddress USA: 0x8028dcbc */
extern u8 g_bPakDialogActive;

/** Whether the file of the decals is created first. @ghidraAddress USA: 0x8028dcc0 */
extern u8 g_bPakFileNeedsCreate;

/** Where the next chunk of the file of the decals is. @ghidraAddress USA: 0x8028dcc4 */
extern s32 g_dwPakFileOffset;

/** The slot a mirror copies the decal to. @ghidraAddress USA: 0x80369dab */
extern u8 g_bMirrorTargetPanel;

/** Whether the rumble pak was swapped for the controller pak. @ghidraAddress USA: 0x80369e50 */
extern u8 g_bPakRumbleSwapped;

/** @ghidraAddress USA: 0x80369e51 */
extern u8 g_bPakRetryNewPak;

/** @ghidraAddress USA: 0x80369e54 */
extern s32 g_dwPakError;

/** @ghidraAddress USA: 0x80369e58 */
extern s32 g_dwPakFileNo;

/** The bytes of the chunk last moved. @ghidraAddress USA: 0x80369e5c */
extern s32 g_dwPakChunkSize;

/** @ghidraAddress USA: 0x80369e60 */
extern u8 g_bPakDialogState;

/** The decal being moved. @ghidraAddress USA: 0x80369e61 */
extern u8 g_bPakDecalIndex;

/** @ghidraAddress USA: 0x80369e62 */
extern u8 g_bPakWaitFrames;

/** The text typed. @ghidraAddress USA: 0x80369ea8 */
extern char g_szDecalText[24];

/**
 * Draw the car beside the decal, turning to the decal slot when it changes.
 *
 * @ghidraAddress USA: 0x80242bdc
 */
void RenderDecalPreview3D(void);

/**
 * Run one frame of the paint shop.
 *
 * @ghidraAddress USA: 0x80243260
 */
void ProcessPaintShopScreen(void);

/**
 * Set the tools, the colors, and the cursor back to their state on entering the paint shop.
 *
 * @ghidraAddress USA: 0x80244ba8
 */
void ResetPaintShopState(void);

/**
 * Draw the message of an error of the controller pak in a box.
 *
 * @param error The error, a PFS_ERR_ value or PAK_ERROR_TRANSFER.
 * @param operation PAINT_TOOL_LOAD or PAINT_TOOL_SAVE.
 * @param inCarSelect 1 when the dialog runs on the car select screen.
 * @ghidraAddress USA: 0x80244d84
 */
void DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect);

/**
 * Run one frame of the menu of the clear tool.
 *
 * @ghidraAddress USA: 0x80247b0c
 */
void ProcessFillClearMenu(void);

/**
 * Run one frame of the menu of the mirror tool.
 *
 * @ghidraAddress USA: 0x8024843c
 */
void ProcessMirrorFlipMenu(void);

/**
 * Copy the decal being painted mirrored to the slot on the other side of the car, and paint that
 * slot next.
 *
 * @ghidraAddress USA: 0x80248b80
 */
void MirrorDecalToOppositePanel(void);

/**
 * Flip the decal being painted by the choice of the menu of the mirror tool.
 *
 * @ghidraAddress USA: 0x80248db4
 */
void FlipDecalPixels(void);

/**
 * Allocate the buffer of the decals and load the textures of the A and B buttons.
 *
 * @ghidraAddress USA: 0x80248f38
 */
void AllocateEditorBuffers(void);

/**
 * Run one frame of the dialog that loads the decals of a model from the controller pak or saves
 * them to it.
 *
 * @param model The model the decals are loaded onto or saved from.
 * @param player The controller of the pak.
 * @param operation PAINT_TOOL_LOAD or PAINT_TOOL_SAVE.
 * @param inCarSelect 1 when the dialog runs on the car select screen.
 * @param done Set once the dialog is done.
 * @return 0 while it runs and once the decals are moved, PAK_DIALOG_CANCELLED when the player
 *         cancels the swap of the rumble pak on the car select screen, or an error.
 * @ghidraAddress USA: 0x80248f88
 */
s32 ProcessDecalSaveLoad(VehicleModel *model, u8 player, u8 operation, u8 inCarSelect, u8 *done);

#endif

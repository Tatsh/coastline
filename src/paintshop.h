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

/**
 * The picture of a whole car on the menu of the clear tool.
 *
 * @ghidraAddress USA: 0x8028cba0
 * @ghidraAddress EUR: 0x8028f300
 * @ghidraAddress JPN: 0x8028ed50
 */
extern Sprite g_texPaintWholeCar;

/**
 * The buttons of the decal panels.
 *
 * @ghidraAddress USA: 0x8028cbd0
 * @ghidraAddress EUR: 0x8028f330
 * @ghidraAddress JPN: 0x8028ed80
 */
extern Sprite g_texPanelLeftDoor;

/**
 * @ghidraAddress USA: 0x8028cc00
 * @ghidraAddress EUR: 0x8028f360
 * @ghidraAddress JPN: 0x8028edb0
 */
extern Sprite g_texPanelRightDoor;

/**
 * @ghidraAddress USA: 0x8028cc30
 * @ghidraAddress EUR: 0x8028f390
 * @ghidraAddress JPN: 0x8028ede0
 */
extern Sprite g_texPanelLeftFrontFender;

/**
 * @ghidraAddress USA: 0x8028cc60
 * @ghidraAddress EUR: 0x8028f3c0
 * @ghidraAddress JPN: 0x8028ee10
 */
extern Sprite g_texPanelRightFrontFender;

/**
 * @ghidraAddress USA: 0x8028cc90
 * @ghidraAddress EUR: 0x8028f3f0
 * @ghidraAddress JPN: 0x8028ee40
 */
extern Sprite g_texPanel4;

/**
 * @ghidraAddress USA: 0x8028ccc0
 * @ghidraAddress EUR: 0x8028f420
 * @ghidraAddress JPN: 0x8028ee70
 */
extern Sprite g_texPanel5;

/**
 * @ghidraAddress USA: 0x8028ccf0
 * @ghidraAddress EUR: 0x8028f450
 * @ghidraAddress JPN: 0x8028eea0
 */
extern Sprite g_texPanelLeftRearFender;

/**
 * @ghidraAddress USA: 0x8028cd20
 * @ghidraAddress EUR: 0x8028f480
 * @ghidraAddress JPN: 0x8028eed0
 */
extern Sprite g_texPanelRightRearFender;

/**
 * @ghidraAddress USA: 0x8028cd50
 * @ghidraAddress EUR: 0x8028f4b0
 * @ghidraAddress JPN: 0x8028ef00
 */
extern Sprite g_texPanel8;

/**
 * @ghidraAddress USA: 0x8028cd80
 * @ghidraAddress EUR: 0x8028f4e0
 * @ghidraAddress JPN: 0x8028ef30
 */
extern Sprite g_texPanel9;

/**
 * The lights drawn over the buttons of panels 8 and 9.
 *
 * @ghidraAddress USA: 0x8028cdb0
 * @ghidraAddress EUR: 0x8028f510
 * @ghidraAddress JPN: 0x8028ef60
 */
extern Sprite g_texPanel8Overlay;

/**
 * @ghidraAddress USA: 0x8028cde0
 * @ghidraAddress EUR: 0x8028f540
 * @ghidraAddress JPN: 0x8028ef90
 */
extern Sprite g_texPanel9Overlay;

/**
 * The brush sizes of the round brush, top to bottom.
 *
 * @ghidraAddress USA: 0x8028ce10
 * @ghidraAddress EUR: 0x8028f570
 * @ghidraAddress JPN: 0x8028efc0
 */
extern Sprite g_texBrushSizeRound;

/**
 * The brush sizes of the square brush.
 *
 * @ghidraAddress USA: 0x8028ce40
 * @ghidraAddress EUR: 0x8028f5a0
 * @ghidraAddress JPN: 0x8028eff0
 */
extern Sprite g_texBrushSizeSquare;

/**
 * The button of the mirror; its style is the choice of its menu, 0 to mirror, 1 to flip
 * horizontally, and 2 to flip vertically.
 *
 * @ghidraAddress USA: 0x8028cf90
 * @ghidraAddress EUR: 0x8028f6f0
 * @ghidraAddress JPN: 0x8028f160
 */
extern Sprite g_texPaintToolMirror;

/**
 * @ghidraAddress USA: 0x8028d140
 * @ghidraAddress EUR: 0x8028f8a0
 * @ghidraAddress JPN: 0x8028f310
 */
extern Sprite g_texPaintFillCursor;

/**
 * @ghidraAddress USA: 0x8028d170
 * @ghidraAddress EUR: 0x8028f8d0
 * @ghidraAddress JPN: 0x8028f340
 */
extern Sprite g_texPaintBrushCursor;

/**
 * @ghidraAddress USA: 0x8028d1a0
 * @ghidraAddress EUR: 0x8028f900
 * @ghidraAddress JPN: 0x8028f370
 */
extern Sprite g_texPaintCrosshairCursor;

/**
 * @ghidraAddress USA: 0x8028d1d0
 * @ghidraAddress EUR: 0x8028f930
 * @ghidraAddress JPN: 0x8028f3a0
 */
extern Sprite g_texPaintTextCursor;

/**
 * @ghidraAddress USA: 0x8028d200
 * @ghidraAddress EUR: 0x8028f960
 * @ghidraAddress JPN: 0x8028f3d0
 */
extern Sprite g_texPaintEyedropperCursor;

/**
 * The keyboard of the text tool.
 *
 * @ghidraAddress USA: 0x8028d230
 * @ghidraAddress EUR: 0x8028f990
 * @ghidraAddress JPN: 0x8028f400
 */
extern Sprite g_texKeyboardPanel;

/**
 * The backing of the keys of the keyboard.
 *
 * @ghidraAddress USA: 0x8028d260
 * @ghidraAddress EUR: 0x8028f9c0
 * @ghidraAddress JPN: 0x8028f430
 */
extern Sprite g_texKeyboardBackground;

/**
 * The pictures of the buttons of the paint shop, by their PAINT_TOOL_ values.
 *
 * @ghidraAddress USA: 0x8028dad4
 * @ghidraAddress EUR: 0x80290234
 * @ghidraAddress JPN: 0x8028fca4
 */
extern Sprite *g_apPaintToolIcons[PAINT_TOOLS];

#if TGR_REGION == TGR_REGION_JP
/**
 * The palette of the pictures of the buttons of the paint shop, read from the cartridge.
 *
 * @ghidraAddress JPN: 0x8028f020
 */
extern u16 g_awPaintShopButtonPalette[16];

/** The palette of the buttons on the cartridge. The link script places it. */
extern u8 _paintShopPaletteSegmentRomStart[];

/** The end of the palette of the buttons on the cartridge. The link script places it. */
extern u8 _paintShopPaletteSegmentRomEnd[];
#endif /* TGR_REGION == TGR_REGION_JP */

/**
 * The texels of the decal before the last edit.
 *
 * @ghidraAddress USA: 0x8028db80
 * @ghidraAddress EUR: 0x802902e0
 * @ghidraAddress JPN: 0x8028fd50
 */
extern u8 *g_pDecalUndoBuffer;

/**
 * The key under the cursor.
 *
 * @ghidraAddress USA: 0x8028dba4
 * @ghidraAddress EUR: 0x80290304
 * @ghidraAddress JPN: 0x8028fd74
 */
extern u8 g_bKeyboardHoverKey;

/**
 * Whether the car is drawn beside the decal.
 *
 * @ghidraAddress USA: 0x8028dbd8
 * @ghidraAddress EUR: 0x80290338
 * @ghidraAddress JPN: 0x8028fda8
 */
extern u8 g_bShowPreview3D;

/**
 * Whether the undo buffer has an edit.
 *
 * @ghidraAddress USA: 0x8028dbdc
 * @ghidraAddress EUR: 0x8029033c
 * @ghidraAddress JPN: 0x8028fdac
 */
extern u8 g_bUndoAvailable;

/**
 * Whether the B button picks the color under the cursor.
 *
 * @ghidraAddress USA: 0x8028dbec
 * @ghidraAddress EUR: 0x8029034c
 * @ghidraAddress JPN: 0x8028fdbc
 */
extern u8 g_bEyedropperActive;

/**
 * The forward axis of the car that shows each decal slot.
 *
 * @ghidraAddress USA: 0x8028dc08
 * @ghidraAddress EUR: 0x80290368
 * @ghidraAddress JPN: 0x8028fdd8
 */
extern Vec3f s_aPanelViewDirections[DECAL_SLOTS];

/**
 * @ghidraAddress USA: 0x8028dc80
 * @ghidraAddress EUR: 0x802903e0
 * @ghidraAddress JPN: 0x8028fe50
 */
extern u8 g_bPaintShopInitialized;

/**
 * The texels of the decal a mirror copies from.
 *
 * @ghidraAddress USA: 0x8028dca4
 * @ghidraAddress EUR: 0x80290444
 * @ghidraAddress JPN: 0x8028fec4
 */
extern u8 *s_pMirrorSource;

/**
 * @ghidraAddress USA: 0x8028dcbc
 * @ghidraAddress EUR: 0x8029045c
 * @ghidraAddress JPN: 0x8028fedc
 */
extern u8 g_bPakDialogActive;

/**
 * Whether the file of the decals is created first.
 *
 * @ghidraAddress USA: 0x8028dcc0
 * @ghidraAddress EUR: 0x80290460
 * @ghidraAddress JPN: 0x8028fee0
 */
extern u8 g_bPakFileNeedsCreate;

/**
 * Where the next chunk of the file of the decals is.
 *
 * @ghidraAddress USA: 0x8028dcc4
 * @ghidraAddress EUR: 0x80290464
 * @ghidraAddress JPN: 0x8028fee4
 */
extern s32 g_dwPakFileOffset;

/**
 * The slot a mirror copies the decal to.
 *
 * @ghidraAddress USA: 0x80369dab
 * @ghidraAddress EUR: 0x8036c2ab
 * @ghidraAddress JPN: 0x8036af0b
 */
extern u8 g_bMirrorTargetPanel;

/**
 * Whether the rumble pak was swapped for the controller pak.
 *
 * @ghidraAddress USA: 0x80369e50
 * @ghidraAddress EUR: 0x8036c350
 * @ghidraAddress JPN: 0x8036afb0
 */
extern u8 g_bPakRumbleSwapped;

/**
 * @ghidraAddress USA: 0x80369e51
 * @ghidraAddress EUR: 0x8036c351
 * @ghidraAddress JPN: 0x8036afb1
 */
extern u8 g_bPakRetryNewPak;

/**
 * @ghidraAddress USA: 0x80369e54
 * @ghidraAddress EUR: 0x8036c354
 * @ghidraAddress JPN: 0x8036afb4
 */
extern s32 g_dwPakError;

/**
 * @ghidraAddress USA: 0x80369e58
 * @ghidraAddress EUR: 0x8036c358
 * @ghidraAddress JPN: 0x8036afb8
 */
extern s32 g_dwPakFileNo;

/**
 * The bytes of the chunk last moved.
 *
 * @ghidraAddress USA: 0x80369e5c
 * @ghidraAddress EUR: 0x8036c35c
 * @ghidraAddress JPN: 0x8036afbc
 */
extern s32 g_dwPakChunkSize;

/**
 * @ghidraAddress USA: 0x80369e60
 * @ghidraAddress EUR: 0x8036c360
 * @ghidraAddress JPN: 0x8036afc0
 */
extern u8 g_bPakDialogState;

/**
 * The decal being moved.
 *
 * @ghidraAddress USA: 0x80369e61
 * @ghidraAddress EUR: 0x8036c361
 * @ghidraAddress JPN: 0x8036afc1
 */
extern u8 g_bPakDecalIndex;

/**
 * @ghidraAddress USA: 0x80369e62
 * @ghidraAddress EUR: 0x8036c362
 * @ghidraAddress JPN: 0x8036afc2
 */
extern u8 g_bPakWaitFrames;

/**
 * The text typed.
 *
 * @ghidraAddress USA: 0x80369ea8
 * @ghidraAddress EUR: 0x8036c3a8
 * @ghidraAddress JPN: 0x8036b008
 */
extern char g_szDecalText[24];

/**
 * Draw the car beside the decal, turning to the decal slot when it changes.
 *
 * @ghidraAddress USA: 0x80242bdc
 * @ghidraAddress EUR: 0x802440ec
 * @ghidraAddress JPN: 0x80243bcc
 */
void RenderDecalPreview3D(void);

/**
 * Run one frame of the paint shop.
 *
 * @ghidraAddress USA: 0x80243260
 * @ghidraAddress EUR: 0x80244770
 * @ghidraAddress JPN: 0x80244250
 */
void ProcessPaintShopScreen(void);

/**
 * Set the tools, the colors, and the cursor back to their state on entering the paint shop.
 *
 * @ghidraAddress USA: 0x80244ba8
 * @ghidraAddress EUR: 0x802460cc
 * @ghidraAddress JPN: 0x80245bc0
 */
void ResetPaintShopState(void);

/**
 * Draw the message of an error of the controller pak in a box.
 *
 * @param error The error, a PFS_ERR_ value or #PAK_ERROR_TRANSFER.
 * @param operation PAINT_TOOL_LOAD or PAINT_TOOL_SAVE.
 * @param inCarSelect 1 when the dialog runs on the car select screen.
 * @ghidraAddress USA: 0x80244d84
 * @ghidraAddress EUR: 0x802462a8
 * @ghidraAddress JPN: 0x80245d9c
 */
void DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect);

/**
 * Run one frame of the menu of the clear tool.
 *
 * @ghidraAddress USA: 0x80247b0c
 * @ghidraAddress EUR: 0x802491bc
 * @ghidraAddress JPN: 0x80248cf4
 */
void ProcessFillClearMenu(void);

/**
 * Run one frame of the menu of the mirror tool.
 *
 * @ghidraAddress USA: 0x8024843c
 * @ghidraAddress EUR: 0x80249aec
 * @ghidraAddress JPN: 0x80249630
 */
void ProcessMirrorFlipMenu(void);

/**
 * Copy the decal being painted mirrored to the slot on the other side of the car, and paint that
 * slot next.
 *
 * @ghidraAddress USA: 0x80248b80
 * @ghidraAddress EUR: 0x8024a164
 * @ghidraAddress JPN: 0x80249c9c
 */
void MirrorDecalToOppositePanel(void);

/**
 * Flip the decal being painted by the choice of the menu of the mirror tool.
 *
 * @ghidraAddress USA: 0x80248db4
 * @ghidraAddress EUR: 0x8024a398
 * @ghidraAddress JPN: 0x80249ed0
 */
void FlipDecalPixels(void);

/**
 * Allocate the buffer of the decals and load the textures of the A and B buttons.
 *
 * @ghidraAddress USA: 0x80248f38
 * @ghidraAddress EUR: 0x8024a51c
 * @ghidraAddress JPN: 0x8024a054
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
 * @return 0 while it runs and once the decals are moved, #PAK_DIALOG_CANCELLED when the player
 *         cancels the swap of the rumble pak on the car select screen, or an error.
 * @ghidraAddress USA: 0x80248f88
 * @ghidraAddress EUR: 0x8024a56c
 * @ghidraAddress JPN: 0x8024a094
 */
s32 ProcessDecalSaveLoad(VehicleModel *model, u8 player, u8 operation, u8 inCarSelect, u8 *done);

#endif /* COASTLINE_PAINTSHOP_H */

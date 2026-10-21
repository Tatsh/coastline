#ifndef COASTLINE_PAINTSHOP_H
#define COASTLINE_PAINTSHOP_H

/**
 * @file
 * The paint shop, where the players paint decals, and the saving and loading of decals on the
 * controller pak.
 */

#include <PR/ultratypes.h>

#include "vehicle.h"

/** The result of the dialog of the controller pak when it is left. */
#define PAK_DIALOG_CANCELLED 999

/**
 * Run one frame of the paint shop.
 *
 * @ghidraAddress USA: 0x80243260
 */
void ProcessPaintShopScreen(void);

/**
 * Draw the message of an error of the controller pak in a box.
 *
 * @param error The error.
 * @param unused1 Not used.
 * @param unused2 Not used.
 * @ghidraAddress USA: 0x80244d84
 */
void DrawPakErrorMessage(s32 error, s32 unused1, s32 unused2);

/**
 * Allocate the buffer of the decals and load the textures of the A and B buttons.
 *
 * @ghidraAddress USA: 0x80248f38
 */
void AllocateEditorBuffers(void);

/**
 * Run one frame of the dialog that loads decals from the controller pak onto a model.
 *
 * @param model The model the decals are loaded onto.
 * @param player Not used.
 * @param unused1 Not used.
 * @param unused2 Not used.
 * @param done Set once the dialog is done.
 * @return 0 while it runs and once the decals are loaded, PAK_DIALOG_CANCELLED when the dialog is
 *         left, or an error.
 * @ghidraAddress USA: 0x80248f88
 */
s32 ProcessDecalSaveLoad(VehicleModel *model, u8 player, s32 unused1, s32 unused2, u8 *done);

#endif

#ifndef COASTLINE_RACEMENU_H
#define COASTLINE_RACEMENU_H

/**
 * @file
 * Track selection, the race state machine, and the menu screens around a race.
 */

#include <PR/ultratypes.h>

/** The flag of an item that cannot be chosen, whose screen explains why. */
#define MENU_ITEM_UNAVAILABLE 0x08

/** The results of ProcessScreenState(). */
#define MENU_RESULT_CHOSEN 1
#define MENU_RESULT_CANCELLED 2

/** An item of a menu screen. */
typedef struct {
    const char *name;
    u32 flags;
    u32 unreconstructed8;
    /** The cartridge range of the item's picture. */
    u32 romStart;
    u32 romEnd;
} MenuItem;

/** A routine that tells whether a menu item may be chosen. */
typedef s32 (*MenuItemFilter)(s32 item);

/** A routine a menu screen calls after drawing, with the selected item. */
typedef void (*MenuPostDrawCallback)(s32 item);

/** @ghidraAddress USA: 0x80271fc8 */
extern MenuPostDrawCallback g_pfnScreenPostDrawCallback;

/** The player whose press chose the last menu item. @ghidraAddress USA: 0x80271fa8 */
extern s32 g_dwMenuChoosingPlayer;

/**
 * The player a screen belongs to, such as the player whose car the paint shop paints or whose
 * controller the controller screen sets up, or -1.
 *
 * @ghidraAddress USA: 0x80271fa0
 */
extern s32 g_dwScreenPlayerIdx;

/**
 * A bit for each player whose input the menu screens read, or -1 for all.
 *
 * @ghidraAddress USA: 0x80271fa4
 */
extern s32 g_dwMenuInputPlayerMask;

/** The picture of the title of a menu screen, and its rectangle. @ghidraAddress USA: 0x80271fb0 */
extern void *g_pMenuTitleImage;

/** @ghidraAddress USA: 0x80271fb4 */
extern s32 g_dwMenuTitleImageX;

/** @ghidraAddress USA: 0x80271fb8 */
extern s32 g_dwMenuTitleImageY;

/** @ghidraAddress USA: 0x80271fbc */
extern s32 g_dwMenuTitleImageWidth;

/** @ghidraAddress USA: 0x80271fc0 */
extern s32 g_dwMenuTitleImageHeight;

/** @ghidraAddress USA: 0x80271fc4 */
extern float g_flScreenRotationPeriod;

/**
 * Run one frame of the race.
 *
 * @ghidraAddress USA: 0x80209434
 */
void ProcessRaceState(void);

/**
 * Reset the heap and the callbacks of the menu screen, and load the pictures of its items.
 *
 * @param items The items of the menu, ended by a null pointer.
 * @param itemCount Receives the number of items.
 * @ghidraAddress USA: 0x8020c408
 */
void ResetAndInitDisplay(MenuItem **items, s32 *itemCount);

/**
 * Run one frame of a menu screen over a background gradient.
 *
 * @param title The title of the screen.
 * @param itemCount The number of items.
 * @param items The items of the menu.
 * @param selection The index of the item under the cursor, updated as the cursor moves.
 * @param isSelectable Tells whether an item may be chosen, or NULL to allow every item.
 * @param topRed The red component of the top of the gradient.
 * @param topGreen The green component of the top of the gradient.
 * @param topBlue The blue component of the top of the gradient.
 * @param bottomRed The red component of the bottom of the gradient.
 * @param bottomGreen The green component of the bottom of the gradient.
 * @param bottomBlue The blue component of the bottom of the gradient.
 * @return MENU_RESULT_CHOSEN once an item is chosen, MENU_RESULT_CANCELLED once the screen is
 *         left, and another value while the screen runs.
 * @ghidraAddress USA: 0x8020ad5c
 */
s32 ProcessScreenState(const char *title,
                       s32 itemCount,
                       MenuItem **items,
                       s32 *selection,
                       MenuItemFilter isSelectable,
                       s32 topRed,
                       s32 topGreen,
                       s32 topBlue,
                       s32 bottomRed,
                       s32 bottomGreen,
                       s32 bottomBlue);

#endif

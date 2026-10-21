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

/**
 * Run one frame of the race.
 *
 * @ghidraAddress USA: 0x80209434
 */
void ProcessRaceState(void);

/**
 * Reset the heap and the callbacks of the menu screen, and load the pictures of its items.
 *
 * @param itemCount Receives the number of items.
 * @ghidraAddress USA: 0x8020c408
 */
void ResetAndInitDisplay(MenuItem **items, s32 *itemCount);

/**
 * Run one frame of a menu screen over a background gradient.
 *
 * @param isSelectable Tells whether an item may be chosen, or NULL to allow every item.
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

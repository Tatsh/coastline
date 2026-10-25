#ifndef COASTLINE_PAUSE_H
#define COASTLINE_PAUSE_H

/**
 * @file
 * Whether the race is paused, and the items of the pause menu.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The items of the pause menu. */
enum {
    PAUSE_CONTINUE, /*!< Return to the race. */
    PAUSE_RESTART,  /*!< Restart the race. */
    PAUSE_MUSIC,    /*!< The volume of the music, changed with left and right. */
    PAUSE_SOUND,    /*!< The volume of the sound effects, changed with left and right. */
    PAUSE_EXIT,     /*!< Exit to the main menu. */
    PAUSE_ITEMS,    /*!< The number of items. */
};

/**
 * Whether the race is paused.
 *
 * @ghidraAddress USA: 0x8026ff10
 * @ghidraAddress EUR: 0x80271ee0
 * @ghidraAddress JPN: 0x80271990
 * @ghidraAddress BRD3D: 0x106909b4
 * @ghidraAddress BRGlide: 0x105ccb5c
 */
extern u32 g_dwRacePauseState;

/**
 * The item of the pause menu, by the PAUSE_ values.
 *
 * @ghidraAddress USA: 0x8026ff14
 * @ghidraAddress EUR: 0x80271ee4
 * @ghidraAddress JPN: 0x80271994
 * @ghidraAddress BRD3D: 0x10680734
 * @ghidraAddress BRGlide: 0x105bc8dc
 */
extern s32 g_dwPauseMenuSelection;

#ifdef __cplusplus
}
#endif

#endif

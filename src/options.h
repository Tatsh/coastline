#ifndef COASTLINE_OPTIONS_H
#define COASTLINE_OPTIONS_H

/**
 * @file
 * The options and audio menus, saving and loading, and the menu screens at start-up.
 */

#include <PR/ultratypes.h>

/**
 * Which screen opened the save and load menu, and so where it returns. The main menu sets 0.
 *
 * @ghidraAddress USA: 0x802724f4
 */
extern u8 g_bSaveLoadEntryPoint;

/**
 * Run one frame of the options menu.
 *
 * @ghidraAddress USA: 0x80211a3c
 */
void ProcessOptionsMenu(void);

/**
 * Run one frame of the save and load menu.
 *
 * @ghidraAddress USA: 0x80211d70
 */
void ProcessDataSaveLoadMenu(void);

/**
 * Apply the music and effect volumes of the options.
 *
 * @ghidraAddress USA: 0x802118b4
 */
void ApplyVolumeSettings(void);

/**
 * Load the textures of the menus into the heap.
 *
 * @ghidraAddress USA: 0x80214a3c
 */
void LoadMenuTextures(void);

/* The data ProcessPakDataScreen() loads or saves. */
enum {
    PAK_DATA_SEASON,
    PAK_DATA_GHOST,
    PAK_DATA_CONFIGURATION,
    PAK_DATA_CAR_SETTINGS,
};

/**
 * Run one frame of the screen that loads or saves data on a Controller Pak.
 *
 * @param dataType The data to load or save, a `PAK_DATA_*` value.
 * @param skipFade Start at once rather than fading the screen in first.
 * @return Nonzero once the screen is finished.
 * @ghidraAddress USA: 0x80214e0c
 */
s32 ProcessPakDataScreen(s32 dataType, u8 skipFade);

#endif

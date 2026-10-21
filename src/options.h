#ifndef COASTLINE_OPTIONS_H
#define COASTLINE_OPTIONS_H

/**
 * @file
 * The options menu, the save and load menu, and the screen that loads or saves data on a
 * Controller Pak.
 */

#include <PR/ultratypes.h>

#include "racemenu.h"

/** The levels of each volume, from 0 for silence. */
#define VOLUME_LEVELS 11

/** The items of the options menu. */
enum {
    OPTIONS_ITEM_BGM,
    OPTIONS_ITEM_SFX,
    OPTIONS_ITEM_UNITS,
    OPTIONS_ITEM_CONTROLLER,
    OPTIONS_ITEM_SAVE_CONFIGURATION,
    OPTIONS_ITEM_LOAD_CONFIGURATION,
    OPTIONS_ITEM_CREDITS,
    OPTIONS_ITEMS,
};

/** The screens that open the save and load menu, deciding what it offers and where it returns. */
enum {
    /** The main menu, for which the menu offers every choice. */
    SAVE_LOAD_FROM_MAIN_MENU,
    /** The results of a championship race, after which the menu asks to save the season. */
    SAVE_LOAD_FROM_SEASON,
    /** The results of a time attack, after which the menu asks to save the ghost car. */
    SAVE_LOAD_FROM_TIME_ATTACK,
};

/** The choices of the save and load menu. */
enum {
    SAVE_LOAD_LOAD_SEASON,
    SAVE_LOAD_SAVE_SEASON,
    SAVE_LOAD_LOAD_GHOST,
    SAVE_LOAD_SAVE_GHOST,
    SAVE_LOAD_LOAD_OPTIONS,
    SAVE_LOAD_SAVE_OPTIONS,
    SAVE_LOAD_CHOICES,
};

/** The data ProcessPakDataScreen() loads or saves. */
enum {
    PAK_DATA_LOAD_SEASON,
    PAK_DATA_LOAD_GHOST,
    PAK_DATA_LOAD_CONFIGURATION,
    PAK_DATA_SAVE_CONFIGURATION,
};

/** The settings of the options menu, which the configuration on a Controller Pak keeps. */
typedef struct {
    /** The levels of the music and the effects, from 0 to VOLUME_LEVELS - 1. */
    s32 musicVolume;
    s32 sfxVolume;
    /** Whether speeds show in miles an hour rather than kilometers. */
    s32 speedInMph;
} OptionSettings;

/** @ghidraAddress USA: 0x802723d0 */
extern OptionSettings g_OptionSettings;

/** The items of the options menu, by the OPTIONS_ITEM_ values. @ghidraAddress USA: 0x802723dc */
extern MenuItem g_aOptionsMenuItems[OPTIONS_ITEMS];

/** The items of the options menu, ending with NULL. @ghidraAddress USA: 0x80272468 */
extern MenuItem *g_apOptionsMenuItems[];

/**
 * The volume of the music and of the effects at each level.
 *
 * @ghidraAddress USA: 0x80272488
 */
extern s32 g_adwBgmVolumeLevels[VOLUME_LEVELS];

/** @ghidraAddress USA: 0x802724b4 */
extern s32 g_adwSfxVolumeLevels[VOLUME_LEVELS];

/**
 * The state of the options menu: 0 to set it up with the first item selected, -1 to set it up
 * keeping the selection, and 1 once it is set up.
 *
 * @ghidraAddress USA: 0x802724e0
 */
extern s32 g_dwOptionsMenuState;

/**
 * Which screen opened the save and load menu, by the SAVE_LOAD_FROM_ values.
 *
 * @ghidraAddress USA: 0x802724f4
 */
extern u8 g_bSaveLoadEntryPoint;

/**
 * Whether the save and load menu, or the screen of the Controller Pak data, is set up.
 *
 * @ghidraAddress USA: 0x802724f8
 */
extern u8 g_bSaveLoadMenuInitialized;

/**
 * Whether the player swapped the Rumble Pak for the Controller Pak, and is asked to swap it back
 * afterwards.
 *
 * @ghidraAddress USA: 0x802724fc
 */
extern u8 g_bSaveLoadRumbleSwapPending;

/** The buffer the data is read into and written from. @ghidraAddress USA: 0x80272500 */
extern u8 *g_pPakDataBuffer;

/**
 * The names of the notes of the save and load menu in the characters of the Controller Pak:
 * "TOP GEAR SEA", "TOP GEAR GHO" and "TOP GEAR OPTIONS".
 *
 * @ghidraAddress USA: 0x80272504
 */
extern u8 g_abSaveLoadSeasonNoteName[16];

/** @ghidraAddress USA: 0x80272514 */
extern u8 g_abSaveLoadGhostNoteName[16];

/** @ghidraAddress USA: 0x80272524 */
extern u8 g_abSaveLoadOptionsNoteName[16];

/** The extension of the notes, which is empty. @ghidraAddress USA: 0x80272534 */
extern u8 g_abSaveLoadNoteExtension[4];

/** The names of the choices of the save and load menu. @ghidraAddress USA: 0x80272538 */
extern const char *g_apszSaveLoadChoices[SAVE_LOAD_CHOICES];

/**
 * The choice of the save and load menu, by the SAVE_LOAD_ values.
 *
 * @ghidraAddress USA: 0x80272550
 */
extern u8 g_bSaveLoadChoice;

/** The step of the save and load menu. @ghidraAddress USA: 0x80272554 */
extern u8 g_bSaveLoadStep;

/**
 * The same names and extension for the screen of the Controller Pak data, whose options note is
 * the configuration.
 *
 * @ghidraAddress USA: 0x80272558
 */
extern u8 g_abPakDataSeasonNoteName[16];

/** @ghidraAddress USA: 0x80272568 */
extern u8 g_abPakDataGhostNoteName[16];

/** @ghidraAddress USA: 0x80272578 */
extern u8 g_abPakDataConfigurationNoteName[16];

/** @ghidraAddress USA: 0x80272588 */
extern u8 g_abPakDataNoteExtension[4];

/**
 * Whether the options menu opens the screen that loads or saves the configuration.
 *
 * @ghidraAddress USA: 0x803163a0
 */
extern s32 g_bLoadConfigurationPending;

/** @ghidraAddress USA: 0x803163a4 */
extern s32 g_bSaveConfigurationPending;

/** The number of items of the options menu, and the selected one. @ghidraAddress USA: 0x803163a8 */
extern s32 g_dwOptionsMenuItemCount;

/** @ghidraAddress USA: 0x803163ac */
extern s32 g_dwOptionsMenuSelection;

/** The names of the volume items. @ghidraAddress USA: 0x803163b0 */
extern char g_szBgmVolumeTextBuf[0x18];

/** @ghidraAddress USA: 0x803163c8 */
extern char g_szSfxVolumeTextBuf[0x18];

/**
 * Whether g_aabSavePakIds holds the identity of the Pak of each of the first two players.
 *
 * @ghidraAddress USA: 0x80316420
 */
extern u8 g_abSavePakIdKnown[2];

/** Whether the save and load menu loaded a season or a ghost car. @ghidraAddress USA: 0x80316422 */
extern u8 g_bSaveLoadDataWasLoaded;

/** Whether the save and load menu takes the input of its choices. @ghidraAddress USA: 0x80316423 */
extern u8 g_bSaveLoadInputEnabled;

/** The note the save and load menu found or made. @ghidraAddress USA: 0x80316424 */
extern s32 g_dwSaveLoadNoteIndex;

/**
 * The frames the save and load menu waited once the sounds faded out.
 *
 * @ghidraAddress USA: 0x80316428
 */
extern u8 g_bSaveLoadFadeDelayFrames;

/** The note the screen of the Controller Pak data found or made. @ghidraAddress USA: 0x8031642c */
extern s32 g_dwPakDataNoteIndex;

/** The step of the screen of the Controller Pak data. @ghidraAddress USA: 0x80316430 */
extern u8 g_bPakDataStep;

/**
 * The frames the screen of the Controller Pak data waited once the sounds faded out.
 *
 * @ghidraAddress USA: 0x80316431
 */
extern u8 g_bPakDataWaitFrames;

/**
 * Raise the music a level, and apply it.
 *
 * @ghidraAddress USA: 0x80211840
 */
void IncreaseBgmVolume(void);

/**
 * Lower the music a level, and apply it.
 *
 * @ghidraAddress USA: 0x8021187c
 */
void DecreaseBgmVolume(void);

/**
 * Apply the music and effect volumes of the options.
 *
 * @ghidraAddress USA: 0x802118b4
 */
void ApplyVolumeSettings(void);

/**
 * Raise the effects a level, and apply it.
 *
 * @ghidraAddress USA: 0x802118f8
 */
void IncreaseSfxVolume(void);

/**
 * Lower the effects a level, and apply it.
 *
 * @ghidraAddress USA: 0x80211934
 */
void DecreaseSfxVolume(void);

/**
 * Draw the hint of the volume items, and run the screen that loads or saves the configuration
 * while the options menu asks for it.
 *
 * @param item The selected item, by the OPTIONS_ITEM_ values.
 * @ghidraAddress USA: 0x8021196c
 */
void OptionsMenuPostDraw(s32 item);

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
 * Load the pictures of the A and B buttons, and allocate the buffer of the Controller Pak data.
 *
 * @ghidraAddress USA: 0x80214a3c
 */
void PreparePakDataScreen(void);

/**
 * Ask the player to put the Rumble Pak back, and wait for A.
 *
 * @param anyPlayer Take A from either of the first two players rather than the choosing one.
 * @return Whether A was pressed.
 * @ghidraAddress USA: 0x80214a88
 */
s32 DrawRumblePakPrompt(s32 anyPlayer);

/**
 * Look for a Rumble Pak in the controller of each player at this console, and stop its motor.
 *
 * @ghidraAddress USA: 0x80214bec
 */
void InitAllRumblePaks(void);

/**
 * Whether a player at this console has no working Rumble Pak in the controller. Nothing calls it.
 *
 * @return Whether a controller lacks a Rumble Pak.
 * @ghidraAddress USA: 0x80214cb0
 */
s32 IsRumblePakMissing(void);

/**
 * Run one frame of the screen that loads or saves data on a Controller Pak.
 *
 * @param dataType The data to load or save, a `PAK_DATA_*` value.
 * @param quiet Work without fading the sounds or showing any message, and finish when the data is
 * missing.
 * @return Nonzero once the screen is finished.
 * @ghidraAddress USA: 0x80214e0c
 */
s32 ProcessPakDataScreen(s32 dataType, u8 quiet);

#endif

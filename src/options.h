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
    OPTIONS_ITEM_BGM,        /*!< The volume of the music. */
    OPTIONS_ITEM_SFX,        /*!< The volume of the sound effects. */
    OPTIONS_ITEM_UNITS,      /*!< The units of speed, miles or kilometres an hour. */
    OPTIONS_ITEM_CONTROLLER, /*!< Opens the controller configuration screen. */
#if TGR_REGION == TGR_REGION_US
    OPTIONS_ITEM_SAVE_CONFIGURATION, /*!< Saves the settings to a Controller Pak. */
    OPTIONS_ITEM_LOAD_CONFIGURATION, /*!< Loads the settings from a Controller Pak. */
#endif
    OPTIONS_ITEM_CREDITS, /*!< Starts the credits race. */
    OPTIONS_ITEMS,        /*!< The number of items. */
};

/** The screens that open the save and load menu, deciding what it offers and where it returns. */
enum {
    SAVE_LOAD_FROM_MAIN_MENU,   /*!< The main menu; the menu offers every choice. */
    SAVE_LOAD_FROM_SEASON,      /*!< The results of a championship race; the menu offers to save
                                     the season. */
    SAVE_LOAD_FROM_TIME_ATTACK, /*!< The results of a time attack; the menu offers to save the
                                     ghost car. */
};

/** The choices of the save and load menu. */
enum {
    SAVE_LOAD_LOAD_SEASON,  /*!< Load a championship season. */
    SAVE_LOAD_SAVE_SEASON,  /*!< Save the championship season. */
    SAVE_LOAD_LOAD_GHOST,   /*!< Load a ghost car of time attack. */
    SAVE_LOAD_SAVE_GHOST,   /*!< Save the ghost car of time attack. */
    SAVE_LOAD_LOAD_OPTIONS, /*!< Load the settings of the options menu. */
    SAVE_LOAD_SAVE_OPTIONS, /*!< Save the settings of the options menu. */
    SAVE_LOAD_CHOICES,      /*!< The number of choices. */
};

/** The data ProcessPakDataScreen() loads or saves. */
enum {
    PAK_DATA_LOAD_SEASON,        /*!< Load a championship season. */
    PAK_DATA_LOAD_GHOST,         /*!< Load a ghost car of time attack. */
    PAK_DATA_LOAD_CONFIGURATION, /*!< Load the settings of the options menu. */
    PAK_DATA_SAVE_CONFIGURATION, /*!< Save the settings of the options menu. */
};

/** The settings of the options menu, which the configuration on a Controller Pak keeps. */
typedef struct {
    s32 musicVolume; /*!< The level of the music, from 0 to VOLUME_LEVELS - 1. */
    s32 sfxVolume;   /*!< The level of the sound effects, from 0 to VOLUME_LEVELS - 1. */
    s32 speedInMph;  /*!< Whether speeds show in miles an hour rather than kilometres. */
} OptionSettings;

/**
 * @ghidraAddress USA: 0x802723d0
 * @ghidraAddress JPN: 0x80273e60
 * @ghidraAddress EUR: 0x802743b0
 */
extern OptionSettings g_OptionSettings;

/**
 * The items of the options menu, by the OPTIONS_ITEM_ values.
 *
 * @ghidraAddress USA: 0x802723dc
 * @ghidraAddress JPN: 0x80273e6c
 * @ghidraAddress EUR: 0x802743bc
 */
extern MenuItem g_aOptionsMenuItems[OPTIONS_ITEMS];

/**
 * The items of the options menu, ending with NULL.
 *
 * @ghidraAddress USA: 0x80272468
 * @ghidraAddress JPN: 0x80273ed0
 * @ghidraAddress EUR: 0x80274420
 */
extern MenuItem *g_apOptionsMenuItems[];

/**
 * The volume of the music and of the effects at each level.
 *
 * @ghidraAddress USA: 0x80272488
 * @ghidraAddress JPN: 0x80273ee8
 * @ghidraAddress EUR: 0x8027443c
 */
extern s32 g_adwBgmVolumeLevels[VOLUME_LEVELS];

/**
 * @ghidraAddress USA: 0x802724b4
 * @ghidraAddress JPN: 0x80273f14
 * @ghidraAddress EUR: 0x80274464
 */
extern s32 g_adwSfxVolumeLevels[VOLUME_LEVELS];

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
/**
 * Whether a setting of the options menu changed since the menu opened, and the game data needs
 * saving to the EEPROM.
 *
 * @ghidraAddress JPN: 0x80273f40
 * @ghidraAddress EUR: 0x80274490
 */
extern s32 g_dwOptionsChangedNeedsSave;
#endif

/**
 * Which screen opened the save and load menu, by the SAVE_LOAD_FROM_ values.
 *
 * @ghidraAddress USA: 0x802724f4
 * @ghidraAddress JPN: 0x80273f54
 * @ghidraAddress EUR: 0x802744a4
 */
extern u8 g_bSaveLoadEntryPoint;

/**
 * Whether the save and load menu, or the screen of the Controller Pak data, is set up.
 *
 * @ghidraAddress USA: 0x802724f8
 * @ghidraAddress JPN: 0x80273f58
 * @ghidraAddress EUR: 0x802744a8
 */
extern u8 g_bSaveLoadMenuInitialized;

/**
 * Whether the player swapped the Rumble Pak for the Controller Pak, and is asked to swap it back
 * afterwards.
 *
 * @ghidraAddress USA: 0x802724fc
 * @ghidraAddress JPN: 0x80273f5c
 * @ghidraAddress EUR: 0x802744ac
 */
extern u8 g_bSaveLoadRumbleSwapPending;

/**
 * The buffer the data is read into and written from.
 *
 * @ghidraAddress USA: 0x80272500
 * @ghidraAddress JPN: 0x80273f60
 * @ghidraAddress EUR: 0x802744b0
 */
extern u8 *g_pPakDataBuffer;

/**
 * The names of the notes of the save and load menu in the characters of the Controller Pak:
 * "TOP GEAR SEA", "TOP GEAR GHO" and "TOP GEAR OPTIONS".
 *
 * @ghidraAddress USA: 0x80272504
 * @ghidraAddress JPN: 0x80273f64
 * @ghidraAddress EUR: 0x802744b4
 */
extern u8 g_abSaveLoadSeasonNoteName[16];

/**
 * @ghidraAddress USA: 0x80272514
 * @ghidraAddress JPN: 0x80273f74
 * @ghidraAddress EUR: 0x802744c4
 */
extern u8 g_abSaveLoadGhostNoteName[16];

/**
 * @ghidraAddress USA: 0x80272524
 * @ghidraAddress JPN: 0x80273f84
 * @ghidraAddress EUR: 0x802744d4
 */
extern u8 g_abSaveLoadOptionsNoteName[16];

/**
 * The extension of the notes, which is empty.
 *
 * @ghidraAddress USA: 0x80272534
 * @ghidraAddress JPN: 0x80273f94
 * @ghidraAddress EUR: 0x802744e4
 */
extern u8 g_abSaveLoadNoteExtension[4];

/** The names of the choices of the save and load menu. @ghidraAddress USA: 0x80272538 */
extern const char *g_apszSaveLoadChoices[SAVE_LOAD_CHOICES];

/**
 * The choice of the save and load menu, by the SAVE_LOAD_ values.
 *
 * @ghidraAddress USA: 0x80272550
 */
extern u8 g_bSaveLoadChoice;

/**
 * The step of the save and load menu.
 *
 * @ghidraAddress USA: 0x80272554
 * @ghidraAddress JPN: 0x8027406c
 * @ghidraAddress EUR: 0x802745bc
 */
extern u8 g_bSaveLoadStep;

/**
 * The same names and extension for the screen of the Controller Pak data, whose options note is
 * the configuration.
 *
 * @ghidraAddress USA: 0x80272558
 * @ghidraAddress JPN: 0x80274070
 * @ghidraAddress EUR: 0x802745c0
 */
extern u8 g_abPakDataSeasonNoteName[16];

/**
 * @ghidraAddress USA: 0x80272568
 * @ghidraAddress JPN: 0x80274080
 * @ghidraAddress EUR: 0x802745d0
 */
extern u8 g_abPakDataGhostNoteName[16];

/**
 * @ghidraAddress USA: 0x80272578
 * @ghidraAddress JPN: 0x80274090
 * @ghidraAddress EUR: 0x802745e0
 */
extern u8 g_abPakDataConfigurationNoteName[16];

/**
 * @ghidraAddress USA: 0x80272588
 * @ghidraAddress JPN: 0x802740a0
 * @ghidraAddress EUR: 0x802745f0
 */
extern u8 g_abPakDataNoteExtension[4];

/**
 * Whether the options menu opens the screen that loads or saves the configuration.
 *
 * @ghidraAddress USA: 0x803163a0
 * @ghidraAddress JPN: 0x80316cb0
 * @ghidraAddress EUR: 0x80318080
 */
extern s32 g_bLoadConfigurationPending;

/**
 * @ghidraAddress USA: 0x803163a4
 * @ghidraAddress JPN: 0x80316cb4
 * @ghidraAddress EUR: 0x80318084
 */
extern s32 g_bSaveConfigurationPending;

/**
 * The names of the volume items.
 *
 * @ghidraAddress USA: 0x803163b0
 * @ghidraAddress JPN: 0x80316cc0
 * @ghidraAddress EUR: 0x80318090
 */
extern char g_szBgmVolumeTextBuf[0x18];

/**
 * @ghidraAddress USA: 0x803163c8
 * @ghidraAddress JPN: 0x80316cd8
 * @ghidraAddress EUR: 0x803180a8
 */
extern char g_szSfxVolumeTextBuf[0x18];

/**
 * Whether g_aabSavePakIds holds the identity of the Pak of each of the first two players.
 *
 * @ghidraAddress USA: 0x80316420
 * @ghidraAddress JPN: 0x80316d30
 * @ghidraAddress EUR: 0x80318100
 */
extern u8 g_abSavePakIdKnown[2];

/** Whether the save and load menu loaded a season or a ghost car. @ghidraAddress USA: 0x80316422 */
extern u8 g_bSaveLoadDataWasLoaded;

/**
 * Whether the save and load menu takes the input of its choices.
 *
 * @ghidraAddress USA: 0x80316423
 * @ghidraAddress JPN: 0x80316d33
 * @ghidraAddress EUR: 0x8031810b
 */
extern u8 g_bSaveLoadInputEnabled;

/**
 * The note the save and load menu found or made.
 *
 * @ghidraAddress USA: 0x80316424
 * @ghidraAddress JPN: 0x80316d34
 * @ghidraAddress EUR: 0x8031810c
 */
extern s32 g_dwSaveLoadNoteIndex;

/**
 * The frames the save and load menu waited once the sounds faded out.
 *
 * @ghidraAddress USA: 0x80316428
 * @ghidraAddress JPN: 0x80316d39
 * @ghidraAddress EUR: 0x80318111
 */
extern u8 g_bSaveLoadFadeDelayFrames;

/**
 * The note the screen of the Controller Pak data found or made.
 *
 * @ghidraAddress USA: 0x8031642c
 * @ghidraAddress JPN: 0x80316d3c
 * @ghidraAddress EUR: 0x80318104
 */
extern s32 g_dwPakDataNoteIndex;

/**
 * The step of the screen of the Controller Pak data.
 *
 * @ghidraAddress USA: 0x80316430
 * @ghidraAddress JPN: 0x80316d40
 * @ghidraAddress EUR: 0x80318108
 */
extern u8 g_bPakDataStep;

/**
 * The frames the screen of the Controller Pak data waited once the sounds faded out.
 *
 * @ghidraAddress USA: 0x80316431
 * @ghidraAddress JPN: 0x80316d41
 * @ghidraAddress EUR: 0x80318109
 */
extern u8 g_bPakDataWaitFrames;

/**
 * Raise the music a level, and apply it.
 *
 * @ghidraAddress USA: 0x80211840
 * @ghidraAddress JPN: 0x80211620
 * @ghidraAddress EUR: 0x80211730
 */
void IncreaseBgmVolume(void);

/**
 * Lower the music a level, and apply it.
 *
 * @ghidraAddress USA: 0x8021187c
 * @ghidraAddress JPN: 0x8021165c
 * @ghidraAddress EUR: 0x8021176c
 */
void DecreaseBgmVolume(void);

/**
 * Apply the music and effect volumes of the options.
 *
 * @ghidraAddress USA: 0x802118b4
 * @ghidraAddress JPN: 0x80211694
 * @ghidraAddress EUR: 0x802117a4
 */
void ApplyVolumeSettings(void);

/**
 * Raise the effects a level, and apply it.
 *
 * @ghidraAddress USA: 0x802118f8
 * @ghidraAddress JPN: 0x802116d8
 * @ghidraAddress EUR: 0x802117e8
 */
void IncreaseSfxVolume(void);

/**
 * Lower the effects a level, and apply it.
 *
 * @ghidraAddress USA: 0x80211934
 * @ghidraAddress JPN: 0x80211714
 * @ghidraAddress EUR: 0x80211824
 */
void DecreaseSfxVolume(void);

/**
 * Draw the hint of the volume items, and run the screen that loads or saves the configuration
 * while the options menu asks for it.
 *
 * @param item The selected item, by the OPTIONS_ITEM_ values.
 * @ghidraAddress USA: 0x8021196c
 * @ghidraAddress JPN: 0x8021174c
 * @ghidraAddress EUR: 0x8021185c
 */
void OptionsMenuPostDraw(s32 item);

/**
 * Run one frame of the options menu.
 *
 * @ghidraAddress USA: 0x80211a3c
 * @ghidraAddress JPN: 0x802117e8
 * @ghidraAddress EUR: 0x8021192c
 */
void ProcessOptionsMenu(void);

/**
 * Run one frame of the save and load menu.
 *
 * @ghidraAddress USA: 0x80211d70
 * @ghidraAddress JPN: 0x80212058
 * @ghidraAddress EUR: 0x80212208
 * @stub{EUR JPN}
 */
void ProcessDataSaveLoadMenu(void);

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * Load the profile of both players, the equipment of the second player, and the options from the
 * EEPROM of the cartridge, when the EEPROM responds and passes its checksum.
 *
 * @return TRUE when the data was loaded, FALSE when the EEPROM is missing, unreadable, or fails
 *     its checksum.
 * @ghidraAddress JPN: 0x80211b60
 * @ghidraAddress EUR: 0x80211ca0
 * @stub{EUR JPN}
 */
s32 LoadSaveDataFromEeprom(void);

/**
 * Write the equipment of both players and the options to the EEPROM of the cartridge, with the
 * whole profile of the first player when the EEPROM fails its checksum.
 *
 * @ghidraAddress JPN: 0x80211d38
 * @ghidraAddress EUR: 0x80211e78
 * @stub{EUR JPN}
 */
void SaveGameDataToEeprom(void);
#endif

/**
 * Load the pictures of the A and B buttons, and allocate the buffer of the Controller Pak data.
 *
 * @ghidraAddress USA: 0x80214a3c
 * @ghidraAddress JPN: 0x802155cc
 * @ghidraAddress EUR: 0x80215b08
 */
void PreparePakDataScreen(void);

/**
 * Ask the player to put the Rumble Pak back, and wait for A.
 *
 * @param anyPlayer Take A from either of the first two players rather than the choosing one.
 * @return Whether A was pressed.
 * @ghidraAddress USA: 0x80214a88
 * @ghidraAddress JPN: 0x80215608
 * @ghidraAddress EUR: 0x80215b54
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
 * @ghidraAddress JPN: 0x80215744
 * @ghidraAddress EUR: 0x80215cb8
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
 * @ghidraAddress JPN: 0x80215964
 * @ghidraAddress EUR: 0x80215ed8
 * @stub{EUR JPN}
 */
s32 ProcessPakDataScreen(s32 dataType, u8 quiet);

#endif

#ifndef COASTLINE_FRONTENDSCREENS_H
#define COASTLINE_FRONTENDSCREENS_H

/**
 * @file
 * The screens of the front end of the Nintendo 64 releases that the game hands over to, and the
 * routines of the Controller Pak, the EEPROM, and the Rumble Pak the game calls.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The screens that open the save and load menu, deciding what it offers and where it returns. */
enum {
    SAVE_LOAD_FROM_MAIN_MENU,   /*!< The main menu; the menu offers every choice. */
    SAVE_LOAD_FROM_SEASON,      /*!< The results of a championship race; the menu offers to save
                                     the season. */
    SAVE_LOAD_FROM_TIME_ATTACK, /*!< The results of a time attack; the menu offers to save the
                                     ghost car. */
};

/** The data ProcessPakDataScreen() loads or saves. */
enum {
    PAK_DATA_LOAD_SEASON,        /*!< Load a championship season. */
    PAK_DATA_LOAD_GHOST,         /*!< Load a ghost car of time attack. */
    PAK_DATA_LOAD_CONFIGURATION, /*!< Load the settings of the options menu. */
    PAK_DATA_SAVE_CONFIGURATION, /*!< Save the settings of the options menu. */
};

/**
 * Which screen opened the save and load menu, by the SAVE_LOAD_FROM_ values.
 *
 * @ghidraAddress USA: 0x802724f4
 * @ghidraAddress EUR: 0x802744a4
 * @ghidraAddress JPN: 0x80273f54
 */
extern u8 g_bSaveLoadEntryPoint;

/**
 * Run one frame of the main menu.
 *
 * @ghidraAddress USA: 0x802111e0
 * @ghidraAddress EUR: 0x80211040
 * @ghidraAddress JPN: 0x80210f30
 */
void ProcessMainMenu(void);

/**
 * Run one frame of the track menu: set it up on its first frame, then act on its items.
 *
 * @ghidraAddress USA: 0x80209434
 * @ghidraAddress EUR: 0x80209214
 * @ghidraAddress JPN: 0x80208e20
 */
void ProcessTrackMenu(void);

/**
 * Run one frame of the results of a race, on the car selection screen.
 *
 * @ghidraAddress USA: 0x80210f4c
 * @ghidraAddress EUR: 0x80210da8
 * @ghidraAddress JPN: 0x80210ca0
 */
void ProcessRaceResultsScreen(void);

/**
 * Run one frame of the options menu.
 *
 * @ghidraAddress USA: 0x80211a3c
 * @ghidraAddress EUR: 0x8021192c
 * @ghidraAddress JPN: 0x802117e8
 */
void ProcessOptionsMenu(void);

/**
 * Run one frame of the save and load menu.
 *
 * @ghidraAddress USA: 0x80211d70
 * @ghidraAddress EUR: 0x80212208
 * @ghidraAddress JPN: 0x80212058
 */
void ProcessDataSaveLoadMenu(void);

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * Load the profile of both players, the equipment of the second player, and the options from the
 * EEPROM of the cartridge, when the EEPROM responds and passes its checksum.
 *
 * @return #TRUE when the data was loaded, #FALSE when the EEPROM is missing, unreadable, or fails
 *     its checksum.
 * @ghidraAddress EUR: 0x80211ca0
 * @ghidraAddress JPN: 0x80211b60
 */
s32 LoadSaveDataFromEeprom(void);
#endif /* TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU */

/**
 * Load the pictures of the A and B buttons, and allocate the buffer of the Controller Pak data.
 *
 * @ghidraAddress USA: 0x80214a3c
 * @ghidraAddress EUR: 0x80215b08
 * @ghidraAddress JPN: 0x802155cc
 */
void PreparePakDataScreen(void);

/**
 * Look for a Rumble Pak in the controller of each player at this console, and stop its motor.
 *
 * @ghidraAddress USA: 0x80214bec
 * @ghidraAddress EUR: 0x80215cb8
 * @ghidraAddress JPN: 0x80215744
 */
void InitAllRumblePaks(void);

/**
 * Run one frame of the screen that loads or saves data on a Controller Pak.
 *
 * @param dataType The data to load or save, a `PAK_DATA_*` value.
 * @param quiet Work without fading the sounds or showing any message, and finish when the data is
 * missing.
 * @return Nonzero once the screen is finished.
 * @ghidraAddress USA: 0x80214e0c
 * @ghidraAddress EUR: 0x80215ed8
 * @ghidraAddress JPN: 0x80215964
 */
s32 ProcessPakDataScreen(s32 dataType, u8 quiet);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_FRONTENDSCREENS_H */

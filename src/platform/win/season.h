#pragma once

/**
 * @file
 * The progress of a season of the championship on the PC port: its round, its year, and its class,
 * and their saving and restoring.
 */

#include "profile.h"

/** The size of g_szSeasonRoundLabel. */
#define SEASON_ROUND_LABEL_SIZE 32

/** The value of g_SeasonResumeState that starts a season again at its first round. */
#define SEASON_RESUME_STATE_RESTART 5

/** The bytes of profile memory written to a season file. */
#define SEASON_SAVE_PAYLOAD_SIZE 512

/** The bytes reserved for the saved slot label. */
#define SEASON_SAVE_SLOT_NAME_SIZE 128

/** Apply g_SeasonProgress without reading a file. */
#define SEASON_LOAD_CURRENT 0

/** Read the saved profile and its trailing menu settings. */
#define SEASON_LOAD_FILE 4

/** The menu settings appended to a saved season. */
struct SeasonSaveSettings {
    int transmission; /*!< The chosen transmission. */
    int tires;        /*!< The chosen tyres. */
    int suspension;   /*!< The chosen suspension. */
    int car;          /*!< The chosen car. */
    int handling;     /*!< The chosen handling. */
};

/**
 * The settings read from the end of a season file.
 *
 * @ghidraAddress BRD3D: 0x10ad0978
 */
extern SeasonSaveSettings g_LoadedSeasonSettings;

/**
 * The slot label copied for the second player after loading.
 *
 * @ghidraAddress BRD3D: 0x10ad34f8
 */
extern char g_szSecondarySeasonSaveSlotName[SEASON_SAVE_SLOT_NAME_SIZE];

/**
 * The player whose profile is being loaded, 0 or 1.
 *
 * @ghidraAddress BRD3D: 0x10690a18
 */
extern int g_dwSaveSlotPlayerIndex;

/**
 * The shared buffer for profile and configuration file reads.
 *
 * @ghidraAddress BRD3D: 0x11782e28
 */
extern u8 g_abSaveFileReadBuffer[SEASON_SAVE_PAYLOAD_SIZE];

/**
 * Apply g_SeasonProgress for #SEASON_LOAD_CURRENT or validate a saved file for other modes.
 * #SEASON_LOAD_FILE also restores trailing menu settings. Other modes preserve the other player's
 * controls while replacing both profiles.
 *
 * @param nMode The profile source and restoration mode.
 * @param bAllowFailure Whether open, header, payload, and checksum failures return success.
 * @return Whether loading succeeded or an allowed file failure occurred.
 * @ghidraAddress BRD3D: 0x10070610
 */
bool LoadSeasonSaveFile(int nMode, bool bAllowFailure);

/**
 * The four-byte signature of a saved season.
 *
 * @ghidraAddress BRD3D: 0x100b5d94
 */
extern const char g_szSeasonSaveMagic[4];

/**
 * The path used for automatic season saves.
 *
 * @ghidraAddress BRD3D: 0x100ad310
 */
extern const char g_szAutoSaveBrf[];

/**
 * The label appended to a saved season.
 *
 * @ghidraAddress BRD3D: 0x10ad0990
 */
extern char g_szSeasonSaveSlotName[SEASON_SAVE_SLOT_NAME_SIZE];

/**
 * Write the first player's profile, checksum, settings, and slot label to g_szSeasonSaveFileName.
 *
 * @return Whether the file opened and all checked writes completed. Settings writes and close
 * errors are ignored.
 * @ghidraAddress BRD3D: 0x100709a0
 */
bool SaveSeasonSaveFile(void);

/**
 * The profile used to save progress for the first player.
 *
 * @ghidraAddress BRD3D: 0x10aced34
 */
extern PlayerProfile *g_pPrimarySeasonSave;

/**
 * The profile used to save progress for the second player.
 *
 * @ghidraAddress BRD3D: 0x10ad189c
 */
extern PlayerProfile *g_pSecondarySeasonSave;

/**
 * Clear both players' save profiles, using static buffers for unassigned profiles.
 *
 * @return Always 1.
 * @ghidraAddress BRD3D: 0x1003e1d0
 */
int ResetSeasonSaveBuffers(void);

/**
 * How the season resumes as the menus show. Its values are not recovered past
 * #SEASON_RESUME_STATE_RESTART.
 *
 * @ghidraAddress BRD3D: 0x10220b20
 * @ghidraAddress BRGlide: 0x1021c650
 */
extern int g_SeasonResumeState;

/**
 * The round of the season the menus show.
 *
 * @ghidraAddress BRD3D: 0x10aa28a4
 * @ghidraAddress BRGlide: 0x10ac5bfc
 */
extern int g_dwSeasonRound;

/**
 * The label of the round of the season.
 *
 * @ghidraAddress BRD3D: 0x10a9d618
 * @ghidraAddress BRGlide: 0x10ac46a0
 */
extern char g_szSeasonRoundLabel[SEASON_ROUND_LABEL_SIZE];

/**
 * Whether a season is loaded.
 *
 * @ghidraAddress BRD3D: 0x10aa289c
 * @ghidraAddress BRGlide: 0x10ac5bf4
 */
extern int g_dwSeasonLoaded;

/**
 * The progress of the championship across its years that the menus show.
 *
 * @ghidraAddress BRD3D: 0x10aa28a0
 * @ghidraAddress BRGlide: 0x10ac5bf8
 */
extern int g_dwSeasonYear;

/**
 * The season of the championship the menus show, from 0.
 *
 * @ghidraAddress BRD3D: 0x10aa28b8
 * @ghidraAddress BRGlide: 0x10ac5c10
 */
extern int g_bSeasonClass;

/**
 * Copy the season, the round, and the settings of the car and the input device into the progress of
 * the season, choose the track and the weather of the round in the championship, and add the cars
 * and tracks the progress unlocked.
 *
 * @ghidraAddress BRD3D: 0x1005fcf0
 * @ghidraAddress BRGlide: 0x10058a30
 */
void SyncSeasonProgress(void);

/**
 * Restore the progress of the season from the race just run.
 *
 * @return Whether the progress was restored.
 * @ghidraAddress BRD3D: 0x1005f940
 * @ghidraAddress BRGlide: 0x10058680
 */
bool RestoreSeasonProgressState(void);

/**
 * Save the progress of the season to the file of the automatic save.
 *
 * @ghidraAddress BRD3D: 0x10041b50
 * @ghidraAddress BRGlide: 0x1003b0b0
 */
void SaveAutoSaveSeason(void);

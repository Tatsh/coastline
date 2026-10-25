#pragma once

/** The size of g_szSeasonRoundLabel. */
#define SEASON_ROUND_LABEL_SIZE 32

/** The value of g_SeasonResumeState that starts a season again at its first round. */
#define SEASON_RESUME_STATE_RESTART 5

/**
 * How the season resumes as the menus show. Its values are not recovered past
 * SEASON_RESUME_STATE_RESTART.
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

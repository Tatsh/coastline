#ifndef COASTLINE_FRONTENDSTATE_H
#define COASTLINE_FRONTENDSTATE_H

/**
 * @file
 * The selections of the menus that the game reads, which the menus of the PC release and
 * those of the SDL game set.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** The number of controls displayed by key configuration. */
#define KEY_CONFIG_ENTRY_COUNT 21

/** The size of the path of a saved season or ghost, MAX_PATH of Windows. */
#define FRONT_END_SAVE_PATH_SIZE 260

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** Front-end choices and host paths for one active game session. */
typedef struct SdlFrontEndState {
    char configPath[4096]; /*!< The null-terminated path of the active configuration file. */
    char seasonPath[FRONT_END_SAVE_PATH_SIZE];     /*!< The path of the saved season file. */
    char timeAttackPath[FRONT_END_SAVE_PATH_SIZE]; /*!< The path of the saved time-attack ghost. */
    int chosenCar; /*!< The selected car index transferred from the front end into race setup. */
    int networkCarSetup; /*!< Whether the network car setup screen is active. */
    int rootMenu;        /*!< Whether the root front-end screen is active. */
    int transmission;    /*!< The selected transmission mode. */
    int tires;           /*!< The selected tyre option index. */
    int suspension;      /*!< The selected suspension option index. */
    int car;             /*!< The car option index displayed in car setup. */
    int laps;            /*!< The configured race lap count. */
    int unlockedCars;    /*!< The accumulated car unlock mask, one bit per car. */
    int unlockedTracks;  /*!< The accumulated track unlock mask, one bit per track. */
    int handling;        /*!< The selected handling option index. */
    int benchmark;       /*!< Whether benchmark mode is requested. */
    int networkPlay;     /*!< Whether the selected race uses network play. */
    int frontEndEnabled; /*!< Whether PC front-end-dependent features are enabled. */
} SdlFrontEndState;

/**
 * @pre An EngineRuntime is active.
 * @return Front-end storage, valid until the runtime is destroyed.
 */
SdlFrontEndState *GetSdlFrontEndState(void);

/** @copydoc SdlFrontEndState::configPath */
#define g_szConfigFilePath (GetSdlFrontEndState()->configPath)
/** @copydoc SdlFrontEndState::seasonPath */
#define g_szSeasonSaveFileName (GetSdlFrontEndState()->seasonPath)
/** @copydoc SdlFrontEndState::timeAttackPath */
#define g_szTimeAttackSaveFileName (GetSdlFrontEndState()->timeAttackPath)
/** @copydoc SdlFrontEndState::chosenCar */
#define g_dwChosenCar (GetSdlFrontEndState()->chosenCar)
/** @copydoc SdlFrontEndState::networkCarSetup */
#define g_dwInNetworkCarSetup (GetSdlFrontEndState()->networkCarSetup)
/** @copydoc SdlFrontEndState::rootMenu */
#define g_dwFrontEndAtRootMenu (GetSdlFrontEndState()->rootMenu)
/** @copydoc SdlFrontEndState::transmission */
#define g_dwSelectedTransmission (GetSdlFrontEndState()->transmission)
/** @copydoc SdlFrontEndState::tires */
#define g_dwTireMenuIndex (GetSdlFrontEndState()->tires)
/** @copydoc SdlFrontEndState::suspension */
#define g_dwSuspensionMenuIndex (GetSdlFrontEndState()->suspension)
/** @copydoc SdlFrontEndState::car */
#define g_dwCarMenuIndex (GetSdlFrontEndState()->car)
/** @copydoc SdlFrontEndState::laps */
#define g_dwTotalLapsSetting (GetSdlFrontEndState()->laps)
/** @copydoc SdlFrontEndState::unlockedCars */
#define g_dwAccumUnlockedCarMask (GetSdlFrontEndState()->unlockedCars)
/** @copydoc SdlFrontEndState::unlockedTracks */
#define g_dwAccumUnlockedTrackMask (GetSdlFrontEndState()->unlockedTracks)
/** @copydoc SdlFrontEndState::handling */
#define g_dwHandlingTypeMenuIndex (GetSdlFrontEndState()->handling)
/** @copydoc SdlFrontEndState::benchmark */
#define g_dwRunBenchmark (GetSdlFrontEndState()->benchmark)
/** @copydoc SdlFrontEndState::networkPlay */
#define g_dwNetworkPlayMode (GetSdlFrontEndState()->networkPlay)
/** @copydoc SdlFrontEndState::frontEndEnabled */
#define g_dwNullStubEnable (GetSdlFrontEndState()->frontEndEnabled)
#else
/**
 * The path of the file of the saved season.
 *
 * @ghidraAddress BRD3D: 0x11782cd0
 * @ghidraAddress BRGlide: 0x117a6030
 */
extern char g_szSeasonSaveFileName[FRONT_END_SAVE_PATH_SIZE];

/**
 * The path of the file of the saved ghost of a time attack.
 *
 * @ghidraAddress BRD3D: 0x11782bc8
 * @ghidraAddress BRGlide: 0x117a5f28
 */
extern char g_szTimeAttackSaveFileName[FRONT_END_SAVE_PATH_SIZE];

/**
 * Whether the root screen is the screen that shows.
 *
 * @ghidraAddress BRD3D: 0x10aa2868
 * @ghidraAddress BRGlide: 0x10ac5bc0
 */
extern int g_dwFrontEndAtRootMenu;

/**
 * The chosen transmission.
 *
 * @ghidraAddress BRD3D: 0x10aa2a08
 * @ghidraAddress BRGlide: 0x10ac5d60
 */
extern int g_dwSelectedTransmission;

/**
 * The chosen tyres.
 *
 * @ghidraAddress BRD3D: 0x100ac64c
 * @ghidraAddress BRGlide: 0x100abdec
 */
extern int g_dwTireMenuIndex;

/**
 * The chosen suspension.
 *
 * @ghidraAddress BRD3D: 0x100ac650
 * @ghidraAddress BRGlide: 0x100abdf0
 */
extern int g_dwSuspensionMenuIndex;

/**
 * The chosen car.
 *
 * @ghidraAddress BRD3D: 0x100ac654
 * @ghidraAddress BRGlide: 0x100abdf4
 */
extern int g_dwCarMenuIndex;

/**
 * The chosen number of laps.
 *
 * @ghidraAddress BRD3D: 0x100ac658
 * @ghidraAddress BRGlide: 0x100abdf8
 */
extern int g_dwTotalLapsSetting;

/**
 * The cars unlocked so far, one bit to a car.
 *
 * @ghidraAddress BRD3D: 0x10aa2a10
 * @ghidraAddress BRGlide: 0x10ac5d68
 */
extern int g_dwAccumUnlockedCarMask;

/**
 * The tracks unlocked so far, one bit to a track.
 *
 * @ghidraAddress BRD3D: 0x10aa2a14
 * @ghidraAddress BRGlide: 0x10ac5d6c
 */
extern int g_dwAccumUnlockedTrackMask;

/**
 * The chosen handling.
 *
 * @ghidraAddress BRD3D: 0x100ac65c
 * @ghidraAddress BRGlide: 0x100abdfc
 */
extern int g_dwHandlingTypeMenuIndex;

/**
 * Whether the screen of the setup of the car of network play shows.
 *
 * @ghidraAddress BRD3D: 0x10aa285c
 * @ghidraAddress BRGlide: 0x10ac5bb4
 */
extern int g_dwInNetworkCarSetup;
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Copy the choices of the menus to adwMenuSelectionBackup of g_GlobalRallyConfig.
 * @ghidraAddress BRD3D: 0x1003e310
 * @ghidraAddress BRGlide: 0x10037920
 */
void BackupMenuSelections(void);

/**
 * Show an error of the game in a message box from its second character, and exit when the error is
 * fatal. Codes above the error-table bound show nothing.
 * @param nErrorCode The error code.
 * @ghidraAddress BRD3D: 0x1003e260
 * @ghidraAddress BRGlide: 0x100378c0
 */
void ShowErrorMessage(int nErrorCode);

/**
 * Restore menu choices, advance unavailable selections, and apply race settings.
 * @ghidraAddress BRD3D: 0x1003e510
 * @ghidraAddress BRGlide: 0x10037b20
 */
void ResolveRaceSelections(void);

/**
 * Reset menu selections, championship profiles, and result display values.
 * @ghidraAddress BRD3D: 0x1003e680
 * @ghidraAddress BRGlide: 0x10037c90
 */
void ResetMenuSelectionsToDefaults(void);

/** Stop and shut down the active menu screen after a season transition. */
void StopActiveFrontEndForSeasonTransition(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_FRONTENDSTATE_H */

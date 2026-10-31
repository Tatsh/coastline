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
    char configPath[4096];
    char seasonPath[FRONT_END_SAVE_PATH_SIZE];
    char timeAttackPath[FRONT_END_SAVE_PATH_SIZE];
    int chosenCar;
    int networkCarSetup;
    int rootMenu;
    int transmission;
    int tires;
    int suspension;
    int car;
    int laps;
    int unlockedCars;
    int unlockedTracks;
    int handling;
    int benchmark;
    int networkPlay;
    int frontEndEnabled;
} SdlFrontEndState;

/** @return Front-end storage for the active engine runtime. */
SdlFrontEndState *GetSdlFrontEndState(void);

#define g_szConfigFilePath (GetSdlFrontEndState()->configPath)
#define g_szSeasonSaveFileName (GetSdlFrontEndState()->seasonPath)
#define g_szTimeAttackSaveFileName (GetSdlFrontEndState()->timeAttackPath)
#define g_dwChosenCar (GetSdlFrontEndState()->chosenCar)
#define g_dwInNetworkCarSetup (GetSdlFrontEndState()->networkCarSetup)
#define g_dwFrontEndAtRootMenu (GetSdlFrontEndState()->rootMenu)
#define g_dwSelectedTransmission (GetSdlFrontEndState()->transmission)
#define g_dwTireMenuIndex (GetSdlFrontEndState()->tires)
#define g_dwSuspensionMenuIndex (GetSdlFrontEndState()->suspension)
#define g_dwCarMenuIndex (GetSdlFrontEndState()->car)
#define g_dwTotalLapsSetting (GetSdlFrontEndState()->laps)
#define g_dwAccumUnlockedCarMask (GetSdlFrontEndState()->unlockedCars)
#define g_dwAccumUnlockedTrackMask (GetSdlFrontEndState()->unlockedTracks)
#define g_dwHandlingTypeMenuIndex (GetSdlFrontEndState()->handling)
#define g_dwRunBenchmark (GetSdlFrontEndState()->benchmark)
#define g_dwNetworkPlayMode (GetSdlFrontEndState()->networkPlay)
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

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_FRONTENDSTATE_H */

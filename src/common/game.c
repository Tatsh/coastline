#include "common/game.h"

#include "common/fade.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/playerinput.h"
#include "common/system.h"
#include "common/vehicle.h"
#include "common/weather.h"

#if !TGR_BOSS_RALLY
#include <PR/os.h>

#include "common/audio.h"
#include "common/frontendscreens.h"
#include "common/optionsettings.h"
#include "common/replay.h"
#include "common/splash.h"
#else
#include "common/configfile.h"
#include "common/displaylistops.h"
#include "common/gameinit.h"
#include "common/ghost.h"
#include "common/hud.h"
#include "common/inputpoll.h"
#include "common/race.h"
#include "common/racephases.h"
#include "common/rallyconfig.h"
#include "common/screen.h"
#include "common/settings.h"
#include "common/soundsystem.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/directplay.h"
#include "win32/input.h"
#include "win32/sound.h"
#endif
#endif /* !TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x106805b8 */
s32 g_dwRaceSessionType;
#endif /* TGR_BOSS_RALLY */

#if !TGR_BOSS_RALLY

/* The cartridge offset of the music of the menus. */
#define ROM_MENU_MUSIC ((u32)_menuMusicSegmentRomStart)
#else

u32 g_dwWeatherActive;
u32 g_dwNightMode;
u32 g_dwSnowMode;
u32 g_dwRainMode;
u32 g_dwNumLocalPlayers = 1;
#endif /* !TGR_BOSS_RALLY */

void ApplyWeather(u32 weather) {
    g_dwNightMode = g_dwRainMode = g_dwSnowMode = 0;
    switch (weather) {
    case WEATHER_SUNNY:
        g_dwWeatherActive = 0;
        break;
    case WEATHER_FOG:
        g_dwWeatherActive = 1;
        break;
    case WEATHER_RAIN:
        g_dwWeatherActive = 1;
        g_dwRainMode = 1;
        break;
    case WEATHER_SNOW:
        g_dwWeatherActive = 1;
        g_dwSnowMode = 1;
        break;
    case WEATHER_NIGHT:
        g_dwWeatherActive = 1;
        g_dwNightMode = 1;
        break;
    }
#if !TGR_BOSS_RALLY
    ProcessAllDisplayLists();
#endif
}

#if !TGR_BOSS_RALLY
void MainMenuEntry(void) {
    ProcessMainMenu();
}
#endif

void StartRaceWithPlayerCount(u32 players) {
    g_dwNumLocalPlayers = players;
#if TGR_BOSS_RALLY
    SetCurrentScreenCallback(NullStub);
#else
    SetCurrentScreenCallback(ProcessTrackMenu);
#endif
}

void ResetAllPlayers(void) {
    int i;

    for (i = 0; i < MAX_PLAYERS; i++) {
#if TGR_BOSS_RALLY
        g_aVehicles[i].InitPlayerVehicleSlot();
        g_aPlayerInputState[i].InitPlayerInputState();
#else
        InitPlayerVehicleSlot(&g_aVehicles[i]);
        InitPlayerInputState(&g_aPlayerInputState[i]);
#endif
    }
}

void InitGameAndShowMenus(void) {
#if TGR_BOSS_RALLY
    g_dwSavedSfxVolume = g_GlobalRallyConfig.dwSfxVolume;
    g_dwSavedMusicVolume = g_GlobalRallyConfig.dwMusicVolume;
    ResetMouseState();
    g_bRaceStartReached = g_dwGameMode == GAME_MODE_DEMO;
    g_bNetReadyToStartSent = false;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_dwNetworkDataReceived = false;
#endif
#endif
    InitRenderingSubsystem();
    StubUpdateCallback();
    StubRenderCallback();
    ResetAllPlayers();
    InitControllersAndPaks();
    SnapScreenFade(0.0f);
#if TGR_BOSS_RALLY
    if (g_dwGameMode == GAME_MODE_ARCADE || g_dwGameMode == GAME_MODE_NETWORK) {
        VEHICLE_GET(&(g_aVehicles[0]), Profile)->unlockedTracks = 0xffff;
        VEHICLE_GET(&(g_aVehicles[0]), Profile)->unlocks = 0xffff;
        VEHICLE_GET(&(g_aVehicles[0]), Profile)->unlockedWeathers = 0xffff;
    }
    StartRaceWithPlayerCount(1);
    SetCurrentScreenCallback(NullStub);
    g_dwUnk_InitGameAndShowMenus_106c664c = 0;
    g_dwUnk_InitGameAndShowMenus_10b4df28 = 0;
    g_dwNumActiveVehicles = 2;
    g_dwNumLocalPlayers = 1;
    g_dwCurrentWeather = 1;
    g_pszDebugOverlayMessage = NULL;
    g_fDebugOverlayMessageTimer = 0.0f;
    SetCurrentScreenCallback(RunRaceGameScreen);
    g_dwGhostRecordingSize = 8;
#else
    SetCurrentScreenCallback(ProcessSplashScreens);

    /* The cartridge holds a ghost recording. The size the game sets afterwards marks the buffer
     * as holding no recording yet. */
    g_dwGhostRecordingSize = DmaReadAssetSize(ROM_GHOST_RECORDING);
    DecompressFromRom(g_abGhostRecording, ROM_GHOST_RECORDING, NULL);
    g_dwGhostRecordingSize = 8;
#endif /* TGR_BOSS_RALLY */

    ResetHeap();
#if TGR_BOSS_RALLY
    NullStub();
    while (!LoadConfigData(0, true)) {
    }
    ResetHeap();
    NullStub();
    while (!LoadConfigData(2, true)) {
    }
    ApplyAudioVolumeSettings();
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        SetTrackFromSeason();
    }
    if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP || g_dwGameMode == GAME_MODE_TIME_ATTACK) {
        g_dwNumLocalPlayers = 1;
    }
    g_dwCurrentWeather = g_dwChosenWeather;
    VEHICLE_GET(&(g_aVehicles[0]), Profile)->handling = g_dwHandlingType;
    VEHICLE_GET(&(g_aVehicles[0]), Profile)->transmission = g_dwTransmissionType;
    VEHICLE_GET(&(g_aVehicles[0]), Profile)->suspension = g_dwSuspensionType;
    VEHICLE_GET(&(g_aVehicles[0]), Profile)->tires = g_dwTireType;
    /* The Windows build retains the disabled audio and debug calls. */
    NullStub();
    NullStub();
    NullStub();
    NullStub();
#else
    /* Load the season data and the configuration from a Controller Pak, if one holds them. The
     * European and Japanese releases read the EEPROM of the cartridge first and use a Controller
     * Pak only when the EEPROM holds no data. */
    PreparePakDataScreen();
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    if (!LoadSaveDataFromEeprom()) {
        ResetHeap();
        PreparePakDataScreen();
#endif
        while (!ProcessPakDataScreen(PAK_DATA_LOAD_SEASON, TRUE)) {
        }
        ResetHeap();
        PreparePakDataScreen();
        while (!ProcessPakDataScreen(PAK_DATA_LOAD_CONFIGURATION, TRUE)) {
        }
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    }
#endif

    ApplyVolumeSettings();
    DecompressFromRom(g_abMenuMusicModule, ROM_MENU_MUSIC, NULL);
    InitAudioSystem(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);

    /* The structure sizes of the original, which the debug output reports. The Japanese release
     * reports none. */
#if TGR_REGION == TGR_REGION_EU
    osSyncPrintf("sizeof(OBJECT)=%d\n", 0x208);
#endif
#if TGR_REGION != TGR_REGION_JP
    osSyncPrintf("sizeof(UltraCarHeader)=%d\n", 0xDF88);
    osSyncPrintf("sizeof(Vehicle)=%d\n", (int)sizeof(Vehicle));
    osSyncPrintf("sizeof(Enemy)=%d\n", 0x78);
#endif
#endif /* TGR_BOSS_RALLY */

    GameModeMainLoop();
}
#if TGR_BOSS_RALLY
void SetWeatherAndRefreshVehicleMaterials(u32 weather) {
    ApplyWeather(weather);
    ProcessAllDisplayLists();
}
#endif

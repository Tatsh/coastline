#include "game.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/os.h>

#include "audio.h"
#include "fade.h"
#include "heap.h"
#include "input.h"
#include "mainmenu.h"
#include "options.h"
#include "racemenu.h"
#include "replay.h"
#include "splash.h"
#include "system.h"
#include "vehicle.h"
#include "weather.h"

/* The cartridge offset of the music of the menus. */
#define ROM_MENU_MUSIC ((u32)_menuMusicSegmentRomStart)
#else
#include "displaylistops.h"
#include "fade.h"
#include "heap.h"
#include "input.h"
#include "platform/win/gameinit.h"
#include "screen.h"
#include "system.h"
#include "weather.h"

u32 g_dwWeatherActive;
u32 g_dwNightMode;
u32 g_dwSnowMode;
u32 g_dwRainMode;
u32 g_dwNumLocalPlayers = 1;
#endif

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
#if TGR_PLATFORM == TGR_PLATFORM_N64
    ProcessAllDisplayLists();
#endif
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void MainMenuEntry(void) {
    ProcessMainMenu();
}
#endif

void StartRaceWithPlayerCount(u32 players) {
    g_dwNumLocalPlayers = players;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    SetCurrentScreenCallback(NullStub);
#else
    SetCurrentScreenCallback(ProcessTrackMenu);
#endif
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void ResetAllPlayers(void) {
    int i;

    for (i = 0; i < MAX_PLAYERS; i++) {
        InitPlayerVehicleSlot(&g_aVehicles[i]);
        InitPlayerInputState(&g_aPlayerInputState[i]);
    }
}
#endif

void InitGameAndShowMenus(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    PrepareWindowsGameSession();
#endif
    InitRenderingSubsystem();
    StubUpdateCallback();
    StubRenderCallback();
    ResetAllPlayers();
    InitControllersAndPaks();
    SnapScreenFade(0.0f);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    PrepareWindowsGameMenus();
#else
    SetCurrentScreenCallback(ProcessSplashScreens);

    /* The cartridge holds a ghost recording. The size the game sets afterwards marks the buffer
     * as holding no recording yet. */
    g_dwGhostRecordingSize = DmaReadAssetSize(ROM_GHOST_RECORDING);
    DecompressFromRom(g_abGhostRecording, ROM_GHOST_RECORDING, NULL);
    g_dwGhostRecordingSize = 8;
#endif

    ResetHeap();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    NullStub();
    while (!LoadWindowsMenuData(0)) {
    }
    ResetHeap();
    NullStub();
    while (!LoadWindowsMenuData(2)) {
    }
    ApplyWindowsMenuVolumeSettings();
    FinishWindowsGameSessionSetup();
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
#endif

    GameModeMainLoop();
}
#if TGR_PLATFORM == TGR_PLATFORM_WIN
void SetWeatherAndRefreshVehicleMaterials(u32 weather) {
    ApplyWeather(weather);
    ProcessAllDisplayLists();
}
#endif

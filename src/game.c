#include "game.h"

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

/* Assets at fixed cartridge offsets. */
#define ROM_GHOST_RECORDING 0xAD400
#define ROM_MENU_MUSIC 0xEBC00

void ApplyWeather(u32 weather) {
    g_dwSnowMode = 0;
    g_dwRainMode = 0;
    g_dwNightMode = 0;
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
    ProcessAllDisplayLists();
}

void MainMenuEntry(void) {
    ProcessMainMenu();
}

void StartRaceWithPlayerCount(u32 players) {
    g_dwNumLocalPlayers = players;
    SetCurrentScreenCallback(ProcessRaceState);
}

void ResetAllPlayers(void) {
    int i;

    for (i = 0; i < MAX_PLAYERS; i++) {
        InitPlayerVehicleSlot(&g_aVehicles[i]);
        InitPlayerInputState(&g_aPlayerInputState[i]);
    }
}

void InitGameAndShowMenus(void *arg) {
    InitRenderingSubsystem();
    StubUpdateCallback();
    StubRenderCallback();
    ResetAllPlayers();
    InitControllersAndPaks();
    SnapScreenFade(0.0f);
    SetCurrentScreenCallback(ProcessSplashScreens);

    /* The cartridge holds a ghost recording. The size the game sets afterwards marks the buffer
     * as holding no recording yet. */
    g_dwGhostRecordingSize = DmaReadAssetSize(ROM_GHOST_RECORDING);
    DecompressFromRom(g_abGhostRecording, ROM_GHOST_RECORDING, NULL);
    g_dwGhostRecordingSize = 8;

    /* Load the season data and the configuration from a Controller Pak, if one holds them. */
    ResetHeap();
    LoadMenuTextures();
    while (!ProcessPakDataScreen(PAK_DATA_SEASON, TRUE)) {
    }
    ResetHeap();
    LoadMenuTextures();
    while (!ProcessPakDataScreen(PAK_DATA_CONFIGURATION, TRUE)) {
    }

    ApplyVolumeSettings();
    DecompressFromRom(g_abMenuMusicModule, ROM_MENU_MUSIC, NULL);
    InitAudioSystem(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);

    /* The structure sizes of the original, which the debug output reports. */
    osSyncPrintf("sizeof(UltraCarHeader)=%d\n", 0xDF88);
    osSyncPrintf("sizeof(Vehicle)=%d\n", (int)sizeof(Vehicle));
    osSyncPrintf("sizeof(Enemy)=%d\n", 0x78);

    GameModeMainLoop();
}

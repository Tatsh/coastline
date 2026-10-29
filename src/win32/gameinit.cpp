#include "win32/gameinit.h"

#include "common/game.h"
#include "common/race.h"
#include "common/screen.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/weather.h"
#include "input.h"
#include "vehicle.h"
#include "win32/boot/rallyconfig.h"
#include "win32/configfile.h"
#include "win32/directplay.h"
#include "win32/ghost.h"
#include "win32/hud.h"
#include "win32/input.h"
#include "win32/renderinit.h"
#include "win32/settings.h"
#include "win32/sound.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/win32api.h"
#endif

s32 g_dwSavedSfxVolume;
s32 g_dwSavedMusicVolume;
s32 g_dwUnk_InitGameAndShowMenus_106c664c;
s32 g_dwUnk_InitGameAndShowMenus_10b4df28;
s32 g_dwNumActiveVehicles = 2;
u32 g_dwCurrentWeather;
HANDLE g_hRenderMutex;

void CreateWindowsRenderMutex(void) {
    g_hRenderMutex = CreateMutexA(nullptr, false, nullptr);
}

s32 ReturnZero(void) {
    return 0;
}

void InitControllersAndPaks(void) {
}

void PrepareWindowsGameSession(void) {
    g_dwSavedSfxVolume = g_GlobalRallyConfig.dwSfxVolume;
    g_dwSavedMusicVolume = g_GlobalRallyConfig.dwMusicVolume;
    ResetMouseState();
    g_bNetRaceStartReached = g_dwGameMode == GAME_MODE_DEMO;
    g_bNetReadyToStartSent = false;
    g_dwNetworkDataReceived = false;
}

void PrepareWindowsGameMenus(void) {
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
    g_pszDebugOverlayMessage = nullptr;
    g_fDebugOverlayMessageTimer = 0.0f;
    SetCurrentScreenCallback(RunRaceGameScreen);
    g_dwGhostRecordingSize = 8;
}

s32 LoadWindowsMenuData(s32 dataType) {
    return LoadConfigData(dataType, true);
}

void ApplyWindowsMenuVolumeSettings(void) {
    ApplyAudioVolumeSettings();
}

void FinishWindowsGameSessionSetup(void) {
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
}

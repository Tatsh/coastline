#include "platform/win/gameinit.h"

#include "game.h"
#include "input.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/configfile.h"
#include "platform/win/directplay.h"
#include "platform/win/ghost.h"
#include "platform/win/hud.h"
#include "platform/win/input.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#include "race.h"
#include "screen.h"
#include "vehicle.h"
#include "vehiclecount.h"
#include "weather.h"

s32 g_dwSavedSfxVolume;
s32 g_dwSavedMusicVolume;
s32 g_dwUnk_InitGameAndShowMenus_106c664c;
s32 g_dwUnk_InitGameAndShowMenus_10b4df28;
s32 g_dwNumActiveVehicles = 2;
u32 g_dwCurrentWeather;

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
        g_aVehicles[0].profile->unlockedTracks = 0xffff;
        g_aVehicles[0].profile->unlocks = 0xffff;
        g_aVehicles[0].profile->unlockedWeathers = 0xffff;
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
    g_aVehicles[0].profile->handling = g_dwHandlingType;
    g_aVehicles[0].profile->transmission = g_dwTransmissionType;
    g_aVehicles[0].profile->suspension = g_dwSuspensionType;
    g_aVehicles[0].profile->tires = g_dwTireType;
    /* The Windows build retains the disabled audio and debug calls. */
    NullStub();
    NullStub();
    NullStub();
    NullStub();
}

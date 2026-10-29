#include "mainmenu.h"

#include <PR/os.h>

#include "carselect.h"
#include "common/audio.h"
#include "common/game.h"
#include "common/input.h"
#include "common/race.h"
#include "common/splash.h"
#include "common/system.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "common/weather.h"
#include "controllerscreen.h"
#include "options.h"
#include "playerselect.h"
#include "racemenu.h"

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "common/romsegments.h"
#endif

/* Assets at fixed cartridge offsets. */
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#define ROM_MENU_MUSIC USA_MENU_MUSIC_ROM
#else
#define ROM_MENU_MUSIC ((u32)_menuMusicSegmentRomStart)
#endif

/* The items of the main menu. */
enum {
    MAIN_MENU_CHAMPIONSHIP, /*!< A race of the championship. */
    MAIN_MENU_ARCADE,       /*!< An arcade race, after the choice of the number of players. */
    MAIN_MENU_TIME_ATTACK,  /*!< A time attack. */
    MAIN_MENU_PRACTICE,     /*!< A practice race, after the choice of the number of players. */
    MAIN_MENU_PAINT_SHOP,   /*!< The paint shop, for the player who chose it. */
    MAIN_MENU_LOAD_SAVE,    /*!< The menu that loads and saves the game data. */
    MAIN_MENU_OPTIONS,      /*!< The options menu. */
};

/* What g_dwMainMenuEntrySource records. */
#define ENTRY_FROM_CHAMPIONSHIP 1
#define ENTRY_FROM_TIME_ATTACK 2

/* The channel count below which the main menu loads its music again. */
#define MENU_AUDIO_CHANNELS 8

void StartChampionshipRace(void) {
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    SetTrackFromSeason();
    g_dwNumLocalPlayers = 1;
    SetCurrentScreenCallback(ProcessTrackMenu);
    g_dwMainMenuEntrySource = ENTRY_FROM_CHAMPIONSHIP;
}

void StartTimeAttack(void) {
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    g_dwNumLocalPlayers = 1;
    g_dwCurrentWeather = WEATHER_FOG;
    SetCurrentScreenCallback(ProcessTrackMenu);
    g_dwMainMenuEntrySource = ENTRY_FROM_TIME_ATTACK;
}

/* The title picture of the main menu. The Japanese release shows the plain title bar. */
#if TGR_REGION == TGR_REGION_JP
#define MAIN_MENU_TITLE g_TitleBarTextureDesc
#else
#define MAIN_MENU_TITLE g_MainMenuTitleTextureDesc
#endif

/* The cheat that unlocks everything: three presses of the right C button with L and R held. */
#define UNLOCK_ALL_HOLD (INPUT_L | INPUT_R)
#define UNLOCK_ALL_PRESS INPUT_C_RIGHT
#define UNLOCK_ALL_PRESSES 3

void ProcessMainMenu(void) {
    if (g_dwMainMenuInitState <= 0) {
        g_dwMenuInputPlayerMask = -1;
        g_dwScreenPlayerIdx = -1;
        g_dwNumLocalPlayers = 2;
        ResetAndInitDisplay(g_apMainMenuItems, &g_dwMainMenuItemCount);

        if (g_nAudioChannelCount < MENU_AUDIO_CHANNELS) {
            ResetMusicPlaybackState();
            DecompressFromRom(g_abMenuMusicModule, ROM_MENU_MUSIC, NULL);
            InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
        }

        g_pMenuTitleImage = &MAIN_MENU_TITLE;
        g_dwMenuTitleImageX = 25;
        g_dwMenuTitleImageY = 14;
        g_dwMenuTitleImageWidth = 280;
        g_dwMenuTitleImageHeight = 32;
        g_fScreenIdleTimeout = 20.0f;

        /* Return to the item of the mode that led back here. */
        if (g_dwMainMenuEntrySource == ENTRY_FROM_CHAMPIONSHIP) {
            g_eMainMenuSelection = MAIN_MENU_CHAMPIONSHIP;
        } else if (g_dwMainMenuEntrySource == ENTRY_FROM_TIME_ATTACK) {
            g_eMainMenuSelection = MAIN_MENU_TIME_ATTACK;
        }
        g_dwMainMenuEntrySource = 0;
        if (g_dwMainMenuInitState == 0) {
            g_eMainMenuSelection = MAIN_MENU_CHAMPIONSHIP;
        }
        g_dwMainMenuInitState = 1;
    }

    switch (ProcessScreenState("TOP GEAR RALLY",
                               g_dwMainMenuItemCount,
                               g_apMainMenuItems,
                               &g_eMainMenuSelection,
                               NULL,
                               0x00,
                               0x00,
                               0x00,
                               0x40,
                               0x40,
                               0x40)) {
    case MENU_RESULT_CHOSEN:
        g_dwIsPaintMode = FALSE;
        switch (g_eMainMenuSelection) {
        case MAIN_MENU_CHAMPIONSHIP:
            StartChampionshipRace();
            break;
        case MAIN_MENU_ARCADE:
            g_dwNumLocalPlayers = 2;
            g_dwGameMode = GAME_MODE_ARCADE;
            SetCurrentScreenCallback(ProcessPlayerCountSelect);
            break;
        case MAIN_MENU_TIME_ATTACK:
            StartTimeAttack();
            break;
        case MAIN_MENU_PRACTICE:
            g_dwNumLocalPlayers = 2;
            g_dwGameMode = GAME_MODE_PRACTICE;
            SetCurrentScreenCallback(ProcessPlayerCountSelect);
            break;
        case MAIN_MENU_PAINT_SHOP:
            /* The paint shop takes the input of the player who chose it, and no other. */
            g_dwIsPaintMode = TRUE;
            g_dwNumLocalPlayers = 1;
            g_dwScreenPlayerIdx = g_dwMenuChoosingPlayer;
            g_dwMenuInputPlayerMask = 1 << g_dwMenuChoosingPlayer;
            SetCurrentScreenCallback(ProcessCarSelectScreen);
            break;
        case MAIN_MENU_LOAD_SAVE:
            g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_MAIN_MENU;
            SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
            break;
        case MAIN_MENU_OPTIONS:
            SetCurrentScreenCallback(ProcessOptionsMenu);
            break;
        }
        g_dwMainMenuInitState = -1;
        break;
    case MENU_RESULT_CANCELLED:
    case 5: /* The main menu treats this result as it treats leaving the screen. */
        SetCurrentScreenCallback(ProcessSplashScreens);
        g_dwMainMenuInitState = 0;
        break;
    }

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    if ((g_aPlayerInputState[0].buttons & UNLOCK_ALL_HOLD) == UNLOCK_ALL_HOLD) {
        if (g_dwUnlockAllCheatPressCount < UNLOCK_ALL_PRESSES &&
            (g_aPlayerInputState[0].buttons & UNLOCK_ALL_PRESS)) {
            osSyncPrintf("BANG!\n");
            TransferFlags(&g_aPlayerInputState[0], UNLOCK_ALL_PRESS);
            if (++g_dwUnlockAllCheatPressCount == UNLOCK_ALL_PRESSES) {
                g_aVehicles[0].profile->unlockedTracks = 0xFFFF;
                g_aVehicles[0].profile->unlocks = 0xFFFF;
                g_aVehicles[0].profile->unlockedWeathers = 0xFFFF;
            }
        }
    } else {
        g_dwUnlockAllCheatPressCount = 0;
    }
#endif /* TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP */
}

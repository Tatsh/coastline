#include "mainmenu.h"

#include "audio.h"
#include "game.h"
#include "input.h"
#include "options.h"
#include "playerselect.h"
#include "racehud.h"
#include "racemenu.h"
#include "splash.h"
#include "system.h"
#include "trackselect.h"
#include "weather.h"

/* Assets at fixed cartridge offsets. */
#define ROM_MENU_MUSIC 0xEBC00

/* The items of the main menu. */
enum {
    MAIN_MENU_CHAMPIONSHIP,
    MAIN_MENU_ARCADE,
    MAIN_MENU_TIME_ATTACK,
    MAIN_MENU_PRACTICE,
    MAIN_MENU_PAINT_SHOP,
    MAIN_MENU_LOAD_SAVE,
    MAIN_MENU_OPTIONS,
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
    SetCurrentScreenCallback(ProcessRaceState);
    g_dwMainMenuEntrySource = ENTRY_FROM_CHAMPIONSHIP;
}

void StartTimeAttack(void) {
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    g_dwNumLocalPlayers = 1;
    g_dwCurrentWeather = WEATHER_FOG;
    SetCurrentScreenCallback(ProcessRaceState);
    g_dwMainMenuEntrySource = ENTRY_FROM_TIME_ATTACK;
}

void ProcessMainMenu(void) {
    if (g_dwMainMenuInitState <= 0) {
        g_dwMenuInputPlayerMask = -1;
        g_dwPaintSourcePlayerIdx = -1;
        g_dwNumLocalPlayers = 2;
        ResetAndInitDisplay(g_apMainMenuItems, &g_dwMainMenuItemCount);

        if (g_nAudioChannelCount < MENU_AUDIO_CHANNELS) {
            ResetMusicPlaybackState();
            DecompressFromRom(g_abMenuMusicModule, ROM_MENU_MUSIC, NULL);
            InitMusicPlayback(g_abMenuMusicModule, g_abMusicModuleWorkBuffer);
        }

        g_pMenuTitleImage = g_MainMenuLayoutImage;
        g_dwMenuTitleImageX = 25;
        g_dwMenuTitleImageY = 14;
        g_dwMenuTitleImageWidth = 280;
        g_dwMenuTitleImageHeight = 32;
        g_flScreenRotationPeriod = 20.0f;

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
            g_dwPaintSourcePlayerIdx = g_dwMenuChoosingPlayer;
            g_dwMenuInputPlayerMask = 1 << g_dwMenuChoosingPlayer;
            SetCurrentScreenCallback(ProcessTrackSelectScreen);
            break;
        case MAIN_MENU_LOAD_SAVE:
            g_bSaveLoadEntryPoint = 0;
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
}

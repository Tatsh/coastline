#include "options.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "audio.h"
#include "controllerscreen.h"
#include "fade.h"
#include "game.h"
#include "heap.h"
#include "input.h"
#include "mainmenu.h"
#include "pakmenu.h"
#include "render.h"
#include "replay.h"
#include "rumble.h"
#include "sprite.h"
#include "system.h"
#include "text.h"
#include "trackselect.h"
#include "vehicle.h"

/* The time the sounds and the screen take to fade, in seconds. */
#define FADE_SECONDS 0.2f

/* The size of the buffer of the Controller Pak data, which a compressed ghost car fills. */
#define PAK_DATA_BUFFER_SIZE 0x3A00

/* The capacity of the ghost recording the data is expanded into. */
#define GHOST_RECORDING_CAPACITY 0xDE5C

/* The channels of a ghost recording, interleaved. */
#define GHOST_CHANNELS 2

/* The sizes of the notes of a season, a ghost car, and the options or the configuration. */
#define SEASON_NOTE_SIZE 0x200
#define OPTIONS_NOTE_SIZE 0x100

/* The options are kept in two halves of a note: the settings, then the equipment of the car. */
#define OPTIONS_HALF_SIZE 0x80

/* The values gControllerPakState takes besides the results of the Controller Pak routines: the
 * controller holds a Rumble Pak, or the ghost car does not fit in the buffer. */
#define PAK_STATE_RUMBLE_PAK 9999
#define PAK_STATE_GHOST_TOO_LARGE -99999

/* The buttons that close a message. */
#define DISMISS_BUTTONS (INPUT_A | INPUT_B | INPUT_Z)

/* The equipment of a car the configuration keeps: handling, gearbox, tyres, suspension, and the
 * button layout. */
#define EQUIPMENT_FIELDS 5

/* The steps of the save and load menu. */
enum {
    SAVE_LOAD_STEP_FADE_IN,           /*!< Wait for the screen to fade in. */
    SAVE_LOAD_STEP_CHOOSE,            /*!< Choose what to load or save. */
    SAVE_LOAD_STEP_QUIET,             /*!< Start fading out the sound and the music. */
    SAVE_LOAD_STEP_WAIT_QUIET,        /*!< Wait for the sound to fade out, then 4 more frames. */
    SAVE_LOAD_STEP_MOUNT,             /*!< Open the Controller Pak of the choosing player. */
    SAVE_LOAD_STEP_REPAIR,            /*!< Repair the identification of the Controller Pak. */
    SAVE_LOAD_STEP_NEW_PAK,           /*!< Report that a different Controller Pak is inserted. */
    SAVE_LOAD_STEP_FIND,              /*!< Look for the note of the chosen data. */
    SAVE_LOAD_STEP_LOAD,              /*!< Read the chosen data from its note. */
    SAVE_LOAD_STEP_ALLOCATE,          /*!< Create the note of the chosen data. */
    SAVE_LOAD_STEP_CONFIRM_OVERWRITE, /*!< Request confirmation to overwrite the note. */
    SAVE_LOAD_STEP_SAVE,              /*!< Write the chosen data to its note. */
    SAVE_LOAD_STEP_ERROR,             /*!< Show the message of a Controller Pak error. */
    SAVE_LOAD_STEP_DONE,              /*!< Show the message of a successful load or save. */
    SAVE_LOAD_STEP_RUMBLE_PAK,        /*!< Prompt the player to swap in the Rumble Pak. */
    SAVE_LOAD_STEP_FADE_OUT,          /*!< Start fading the screen out. */
    SAVE_LOAD_STEP_LEAVE,             /*!< Fade the sound back in until the screen is black. */
};

/* The steps of the screen of the Controller Pak data. */
enum {
    PAK_DATA_STEP_START,      /*!< Start fading out the sound and the music unless quiet. */
    PAK_DATA_STEP_WAIT_QUIET, /*!< Wait for the sound to fade out. */
    PAK_DATA_STEP_MOUNT,      /*!< Open the Controller Pak of the choosing player. */
    PAK_DATA_STEP_REPAIR,     /*!< Repair the identification of the Controller Pak. */
    PAK_DATA_STEP_NEW_PAK,    /*!< Report that a different Controller Pak is inserted. */
    PAK_DATA_STEP_FIND,       /*!< Look for the note of the data. */
    PAK_DATA_STEP_LOAD,       /*!< Read the data from its note. */
    PAK_DATA_STEP_ALLOCATE,   /*!< Create the note of the data. */
    PAK_DATA_STEP_SAVE,       /*!< Write the data to its note. */
    PAK_DATA_STEP_ERROR,      /*!< Show the message of a Controller Pak error unless quiet. */
    PAK_DATA_STEP_RUMBLE_PAK, /*!< Prompt the player to swap in the Rumble Pak. */
    PAK_DATA_STEP_DONE,       /*!< Show the message of a successful load or save unless quiet. */
    PAK_DATA_STEP_FINISH,     /*!< Restore the sound and report the screen finished. */
};

/* The text of the messages: white, with a light gray shadow. */
#define SET_MESSAGE_TEXT()                                                                         \
    {                                                                                              \
        SetTextStyleNormal();                                                                      \
        SetTextAlignLeft();                                                                        \
        SetTextSize(12);                                                                           \
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);                                   \
    }

/* The frame of a message. */
#define DRAW_MESSAGE_BOX(x, y, width, height)                                                      \
    DrawRectBorder((x), (y), (width), (height), 3, 0, 0, 0x80, 0x80, 0x80)

/* Copy the equipment of the car of a profile out. */
#define GET_EQUIPMENT(equipment, profile)                                                          \
    {                                                                                              \
        (equipment)[0] = (profile)->handling;                                                      \
        (equipment)[1] = (profile)->transmission;                                                  \
        (equipment)[2] = (profile)->tires;                                                         \
        (equipment)[3] = (profile)->suspension;                                                    \
        (equipment)[4] = (profile)->controllerLayout;                                              \
    }

/* The input of the player choosing in the menus, which the original looks up again at each use. */
#define CHOOSING_INPUT (&g_aPlayerInputState[g_dwMenuChoosingPlayer])

/* The original looks the profile of the vehicle up again for every field. */
#define SET_EQUIPMENT(vehicle, equipment)                                                          \
    do {                                                                                           \
        g_aVehicles[vehicle].profile->handling = (equipment)[0];                                   \
        g_aVehicles[vehicle].profile->transmission = (equipment)[1];                               \
        g_aVehicles[vehicle].profile->tires = (equipment)[2];                                      \
        g_aVehicles[vehicle].profile->suspension = (equipment)[3];                                 \
        g_aVehicles[vehicle].profile->controllerLayout = (equipment)[4];                           \
    } while (0)

void IncreaseBgmVolume(void) {
    if (g_OptionSettings.musicVolume < VOLUME_LEVELS - 1) {
        g_OptionSettings.musicVolume++;
    }
    g_bBgmVolume = g_adwBgmVolumeLevels[g_OptionSettings.musicVolume];
}

void DecreaseBgmVolume(void) {
    if (g_OptionSettings.musicVolume > 0) {
        g_OptionSettings.musicVolume--;
    }
    g_bBgmVolume = g_adwBgmVolumeLevels[g_OptionSettings.musicVolume];
}

void ApplyVolumeSettings(void) {
    g_bBgmVolume = g_adwBgmVolumeLevels[g_OptionSettings.musicVolume];
    g_bSfxMasterVolume = g_adwSfxVolumeLevels[g_OptionSettings.sfxVolume];
}

void IncreaseSfxVolume(void) {
    if (g_OptionSettings.sfxVolume < VOLUME_LEVELS - 1) {
        g_OptionSettings.sfxVolume++;
    }
    g_bSfxMasterVolume = g_adwSfxVolumeLevels[g_OptionSettings.sfxVolume];
}

void DecreaseSfxVolume(void) {
    if (g_OptionSettings.sfxVolume > 0) {
        g_OptionSettings.sfxVolume--;
    }
    g_bSfxMasterVolume = g_adwSfxVolumeLevels[g_OptionSettings.sfxVolume];
}

void OptionsMenuPostDraw(s32 item) {
    if (item < OPTIONS_ITEM_UNITS) {
        SetTextSize(10);
        SetTextStyleNormal();
        SetTextAlignCenter();
        DrawText("%wwPush up or down to adjust", 160, 90);
    }
    if (g_bLoadConfigurationPending) {
        if (ProcessPakDataScreen(PAK_DATA_LOAD_CONFIGURATION, FALSE)) {
            g_bLoadConfigurationPending = FALSE;
        } else {
            /* The menu takes no input while the screen of the Controller Pak runs. */
            g_aPlayerInputState[0].buttons = 0;
            g_aPlayerInputState[0].stickX = 0.0f;
        }
    } else if (g_bSaveConfigurationPending) {
        if (ProcessPakDataScreen(PAK_DATA_SAVE_CONFIGURATION, FALSE)) {
            g_bSaveConfigurationPending = FALSE;
        } else {
            g_aPlayerInputState[0].buttons = 0;
            g_aPlayerInputState[0].stickX = 0.0f;
        }
    }
}

void ProcessOptionsMenu(void) {
    /* The state of the options menu: 0 to set it up with the first item selected, -1 to set it up
     * keeping the selection, and 1 once it is set up. */
    /** @ghidraAddress USA: 0x802724e0 */
    static s32 g_dwOptionsMenuState = 0;
    /** @ghidraAddress USA: 0x803163a8 */
    static s32 g_dwOptionsMenuItemCount;
    /** @ghidraAddress USA: 0x803163ac */
    static s32 g_dwOptionsMenuSelection;

    if (g_dwOptionsMenuState <= 0) {
        g_dwMenuInputPlayerMask = -1;
        g_dwScreenPlayerIdx = -1;
        ResetAndInitDisplay(g_apOptionsMenuItems, &g_dwOptionsMenuItemCount);
        PreparePakDataScreen();
        g_pfnScreenPostDrawCallback = OptionsMenuPostDraw;
        g_pMenuTitleImage = &g_TitleBarTextureDesc;
        g_dwMenuTitleImageX = 22;
        g_dwMenuTitleImageY = 9;
        g_dwMenuTitleImageWidth = 280;
        g_dwMenuTitleImageHeight = 42;
        if (g_dwOptionsMenuState == 0) {
            g_dwOptionsMenuSelection = OPTIONS_ITEM_BGM;
        }
        g_dwOptionsMenuState = 1;
    }

    sprintf(g_szBgmVolumeTextBuf, "BGM Volume: %d", g_OptionSettings.musicVolume);
    sprintf(g_szSfxVolumeTextBuf, "SFX Volume: %d", g_OptionSettings.sfxVolume);
    g_aOptionsMenuItems[OPTIONS_ITEM_BGM].name = g_szBgmVolumeTextBuf;
    g_aOptionsMenuItems[OPTIONS_ITEM_SFX].name = g_szSfxVolumeTextBuf;
    if (g_OptionSettings.speedInMph) {
        g_aOptionsMenuItems[OPTIONS_ITEM_UNITS].name = "Units: mph";
    } else {
        g_aOptionsMenuItems[OPTIONS_ITEM_UNITS].name = "Units: kph";
    }

    switch (ProcessScreenState("OPTIONS",
                               g_dwOptionsMenuItemCount,
                               g_apOptionsMenuItems,
                               &g_dwOptionsMenuSelection,
                               NULL,
                               0,
                               0,
                               0,
                               0x80,
                               0x40,
                               0x20)) {
    case MENU_RESULT_DOWN:
        switch (g_dwOptionsMenuSelection) {
        case OPTIONS_ITEM_BGM:
            DecreaseBgmVolume();
            break;
        case OPTIONS_ITEM_SFX:
            DecreaseSfxVolume();
            break;
        case OPTIONS_ITEM_UNITS:
            g_OptionSettings.speedInMph = !g_OptionSettings.speedInMph;
            break;
        }
        break;
    case MENU_RESULT_UP:
        switch (g_dwOptionsMenuSelection) {
        case OPTIONS_ITEM_BGM:
            IncreaseBgmVolume();
            break;
        case OPTIONS_ITEM_SFX:
            IncreaseSfxVolume();
            break;
        case OPTIONS_ITEM_UNITS:
            g_OptionSettings.speedInMph = !g_OptionSettings.speedInMph;
            break;
        }
        break;
    case MENU_RESULT_CHOSEN:
        switch (g_dwOptionsMenuSelection) {
        case OPTIONS_ITEM_UNITS:
            g_OptionSettings.speedInMph = !g_OptionSettings.speedInMph;
            break;
        case OPTIONS_ITEM_CONTROLLER:
            g_dwOptionsMenuState = -1;
            g_dwScreenPlayerIdx = g_dwMenuChoosingPlayer;
            g_dwMenuInputPlayerMask = 1 << g_dwScreenPlayerIdx;
            SetCurrentScreenCallback(ProcessControllerConfigScreen);
            break;
        case OPTIONS_ITEM_SAVE_CONFIGURATION:
            g_bSaveConfigurationPending = TRUE;
            break;
        case OPTIONS_ITEM_LOAD_CONFIGURATION:
            g_bLoadConfigurationPending = TRUE;
            break;
        case OPTIONS_ITEM_CREDITS:
            g_dwOptionsMenuState = -1;
            StartCreditsRace();
            break;
        }
        break;
    case MENU_RESULT_CANCELLED:
        SetCurrentScreenCallback(ProcessMainMenu);
        g_dwOptionsMenuState = 0;
        break;
    }
}

/* The message that the Controller Pak is not the one the player used before. */
#define DRAW_NEW_PAK_MESSAGE(boxY, textY)                                                          \
    {                                                                                              \
        DRAW_MESSAGE_BOX(0xAF, (boxY), 0x122, 0x89);                                               \
        DrawText("A NEW CONTROLLER PAK", 0x5F, (textY));                                           \
        DrawText("WAS INSERTED.", 0x5F, (textY) + 0xD);                                            \
        DrawText("THIS CONTROLLER PAK", 0x5F, (textY) + 0x20);                                     \
        DrawText("WILL BE USED.", 0x5F, (textY) + 0x2D);                                           \
    }

/* The message that asks the player to swap the Rumble Pak for the Controller Pak, with its OK and
 * CANCEL buttons. */
#define DRAW_SWAP_FOR_CONTROLLER_PAK_MESSAGE(boxY, textY, buttonY, okText, cancelText)             \
    {                                                                                              \
        DRAW_MESSAGE_BOX(0x79, (boxY), 0x18D, 0xA0);                                               \
        DrawText("PLEASE REMOVE THE RUMBLE PAK", 0x44, (textY));                                   \
        DrawText("AND INSERT THE CONTROLLER PAK", 0x44, (textY) + 0xD);                            \
        DrawText("INTO THE CONTROLLER.  PRESS", 0x44, (textY) + 0x1A);                             \
        DrawText("THE A BUTTON WHEN READY.", 0x44, (textY) + 0x27);                                \
        SetTextAlignLeft();                                                                        \
        SetTextSize(10);                                                                           \
        DrawText((okText), (u32)(g_texButtonA.width + 0xE3) >> 1, ((buttonY) + 0x12) >> 1);        \
        DrawText((cancelText), (u32)(g_texButtonB.width + 0x144) >> 1, ((buttonY) + 0x12) >> 1);   \
        DrawTexturedSprite(&g_texButtonA, 0xDD, (buttonY));                                        \
        DrawTexturedSprite(&g_texButtonB, 0x13E, (buttonY));                                       \
    }

/* The message that asks the player to put the Rumble Pak back. */
#define DRAW_SWAP_FOR_RUMBLE_PAK_MESSAGE(boxX, boxY, boxWidth, textY)                              \
    {                                                                                              \
        DRAW_MESSAGE_BOX((boxX), (boxY), (boxWidth), 0x98);                                        \
        DrawText("IF THE RUMBLE PAK IS TO BE USED,", 0x41, (textY));                               \
        DrawText("REMOVE THE CONTROLLER PAK AND", 0x41, (textY) + 0xD);                            \
        DrawText("INSERT THE RUMBLE PAK INTO THE", 0x41, (textY) + 0x1A);                          \
        DrawText("CONTROLLER.  PRESS THE A BUTTON", 0x41, (textY) + 0x27);                         \
        DrawText("TO CONTINUE.", 0x41, (textY) + 0x34);                                            \
    }

/* The message that the Controller Pak lacks the room for a note. */
#define DRAW_INSUFFICIENT_PAGES_MESSAGE(textY, pages, data)                                        \
    {                                                                                              \
        DrawText("INSUFFICIENT FREE PAGES", 0x5A, (textY));                                        \
        DrawText("OR FREE NOTES IN THE", 0x5A, (textY) + 0xD);                                     \
        DrawText("CONTROLLER PAK.", 0x5A, (textY) + 0x1A);                                         \
        DrawText((pages), 0x5A, (textY) + 0x2D);                                                   \
        DrawText("ARE NEEDED TO SAVE THE", 0x5A, (textY) + 0x3A);                                  \
        DrawText((data), 0x5A, (textY) + 0x47);                                                    \
    }

/* A message of two lines in a box of the usual height. */
#define DRAW_SHORT_MESSAGE(boxX, boxWidth, textX, line1, line2)                                    \
    {                                                                                              \
        DRAW_MESSAGE_BOX((boxX), 0xD9, (boxWidth), 0x4A);                                          \
        DrawText((line1), (textX), 0x7C);                                                          \
        DrawText((line2), (textX), 0x8A);                                                          \
    }

/* The message that the data was loaded or saved, in a box whose top depends on the screen that
 * opened the save and load menu. */
#define DRAW_DONE_MESSAGE(boxX, boxWidth, top, line1, line2)                                       \
    {                                                                                              \
        DRAW_MESSAGE_BOX((boxX), (top), (boxWidth), 0x4A);                                         \
        DrawText((line1), 0x9F, ((top) + 0x20) >> 1);                                              \
        DrawText((line2), 0x9F, (((top) + 0x20) >> 1) + 0xE);                                      \
    }

/* A choice out of range sets no note. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
void ProcessDataSaveLoadMenu(void) {
    /* TOP GEAR SEASON, TOP GEAR GHOST and TOP GEAR OPTIONS in the font of the Controller Pak. */
    u8 seasonName[16] = {0x2D,
                         0x28,
                         0x29,
                         0x0F,
                         0x20,
                         0x1E,
                         0x1A,
                         0x2B,
                         0x0F,
                         0x2C,
                         0x1E,
                         0x1A,
                         0x2C,
                         0x28,
                         0x27,
                         0x00};
    u8 ghostName[16] = {0x2D,
                        0x28,
                        0x29,
                        0x0F,
                        0x20,
                        0x1E,
                        0x1A,
                        0x2B,
                        0x0F,
                        0x20,
                        0x21,
                        0x28,
                        0x2C,
                        0x2D,
                        0x00,
                        0x00};
    u8 optionsName[16] = {0x2D,
                          0x28,
                          0x29,
                          0x0F,
                          0x20,
                          0x1E,
                          0x1A,
                          0x2B,
                          0x0F,
                          0x28,
                          0x29,
                          0x2D,
                          0x22,
                          0x28,
                          0x27,
                          0x2C};
    u8 extension[4] = {0};
    const char *choices[SAVE_LOAD_CHOICES] = {"LOAD SEASON DATA",
                                              "SAVE SEASON DATA",
                                              "LOAD GHOST CAR DATA",
                                              "SAVE GHOST CAR DATA",
                                              "LOAD OPTIONS",
                                              "SAVE OPTIONS"};
    PlayerProfile *other;
    u8 *noteName;
    s32 noteSize;
    s32 equipment[EQUIPMENT_FIELDS];
    s32 freeBlocks;
    s32 compressedSize;
    s32 choice;
    s32 textY;
    s32 top;
    s32 buttonY;
    u8 plugged;

    g_wSerialInterfaceIdle = FALSE;
    if (!g_bSaveLoadMenuInitialized) {
        g_bSaveLoadMenuInitialized = TRUE;
        g_bSaveLoadDataWasLoaded = FALSE;
        g_bSaveLoadInputEnabled = TRUE;
        gControllerPakState = 0;
        g_bSaveLoadFadeDelayFrames = 0;
        ResetHeap();
        osViBlack(TRUE);
        StartHighResFrame();
        FlushDisplayList();
        osViBlack(TRUE);
        StartHighResFrame();
        FlushDisplayList();
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texButtonA);
        LoadTextureFromRom(&g_texButtonB);
        g_pPakDataBuffer = AllocHeapAligned(PAK_DATA_BUFFER_SIZE);
        if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
            g_bSaveLoadChoice = SAVE_LOAD_LOAD_SEASON;
        } else if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_SEASON) {
            g_bSaveLoadChoice = SAVE_LOAD_SAVE_SEASON;
        } else if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_TIME_ATTACK) {
            g_bSaveLoadChoice = SAVE_LOAD_SAVE_GHOST;
        }
        g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_IN;
        SetFadeTarget(1.0f, FADE_SECONDS);
    }

    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 0x60, 0, 0x80);
    DrawTexturedSprite(&g_texLogo, g_texLogo.originX, g_texLogo.originY + 8);

    switch (g_bSaveLoadChoice) {
    case SAVE_LOAD_LOAD_SEASON:
    case SAVE_LOAD_SAVE_SEASON:
        noteName = seasonName;
        noteSize = SEASON_NOTE_SIZE;
        break;
    case SAVE_LOAD_LOAD_GHOST:
    case SAVE_LOAD_SAVE_GHOST:
        noteName = ghostName;
        noteSize = PAK_DATA_BUFFER_SIZE;
        break;
    case SAVE_LOAD_LOAD_OPTIONS:
    case SAVE_LOAD_SAVE_OPTIONS:
        noteName = optionsName;
        noteSize = OPTIONS_NOTE_SIZE;
        break;
    }

    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
        DrawText("%ryDATA LOAD/SAVE", 160, 41);
    } else {
        DrawText("%ryDATA SAVE", 160, 41);
    }

    if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
        /* The list of choices, the current one brighter, all dimmed while a message is shown. */
        SetTextSize(16);
        for (choice = 0; choice != SAVE_LOAD_CHOICES; choice++) {
            if (g_bSaveLoadInputEnabled) {
                if (choice == g_bSaveLoadChoice) {
                    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00);
                } else {
                    SetTextCustomColors(0xFF, 0xF5, 0x00, 0xC8, 0x00, 0x00);
                }
            } else if (choice == g_bSaveLoadChoice) {
                SetTextCustomColors(0x80, 0x80, 0x80, 0x84, 0x7C, 0x00);
            } else {
                SetTextCustomColors(0x80, 0x78, 0x00, 0x64, 0x00, 0x00);
            }
            DrawText(choices[choice], 0x9E, 0x57 + choice * 18);
        }
        DrawTexturedSprite(&g_texButtonA, 0xCA, 0x1B0);
        DrawTexturedSprite(&g_texButtonB, 0x142, 0x1B0);
        SetTextAlignLeft();
        SetTextSize(11);
        DrawText("%wwSelect", 0x74, g_dwScreenHeight * 19 / 20 - 3);
        DrawText("%wwGo Back", 0xB0, g_dwScreenHeight * 19 / 20 - 3);
    } else {
        /* After a race the menu only asks whether to save it. */
        SetTextSize(20);
        if (g_bSaveLoadInputEnabled) {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00);
        } else {
            SetTextCustomColors(0x80, 0x80, 0x80, 0x84, 0x7C, 0x00);
        }
        if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_SEASON) {
            DrawText("SAVE SEASON DATA?", 0x9E, 0x80);
            g_bSaveLoadChoice = SAVE_LOAD_SAVE_SEASON;
        } else if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_TIME_ATTACK) {
            DrawText("SAVE GHOST CAR DATA?", 0x9E, 0x80);
            g_bSaveLoadChoice = SAVE_LOAD_SAVE_GHOST;
        }
        DrawTexturedSprite(&g_texButtonA, 0xEC, 0x1B0);
        DrawTexturedSprite(&g_texButtonB, 0x154, 0x1B0);
        SetTextAlignLeft();
        SetTextSize(11);
        DrawText("%wwYES", 0x84, g_dwScreenHeight * 19 / 20 - 3);
        DrawText("%wwNO", 0xB9, g_dwScreenHeight * 19 / 20 - 3);
    }

    switch (g_bSaveLoadStep) {
    case SAVE_LOAD_STEP_FADE_IN:
        if (IsFadedToWhite()) {
            g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
        }
        break;

    case SAVE_LOAD_STEP_CHOOSE:
        g_bSaveLoadInputEnabled = TRUE;
        if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
            UpdateStickRepeat(CHOOSING_INPUT);
            if (CHOOSING_INPUT->buttons & INPUT_DOWN) {
                TransferFlags(CHOOSING_INPUT, INPUT_DOWN);
                if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
                    g_bSaveLoadChoice = SAVE_LOAD_LOAD_SEASON;
                } else {
                    g_bSaveLoadChoice++;
                }
            } else if (CHOOSING_INPUT->buttons & INPUT_UP) {
                TransferFlags(CHOOSING_INPUT, INPUT_UP);
                if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON) {
                    g_bSaveLoadChoice = SAVE_LOAD_SAVE_OPTIONS;
                } else {
                    g_bSaveLoadChoice--;
                }
            }
        }
        if (CHOOSING_INPUT->buttons & INPUT_A) {
            TransferFlags(CHOOSING_INPUT, INPUT_A);
            g_bSaveLoadStep = SAVE_LOAD_STEP_QUIET;
        } else if (CHOOSING_INPUT->buttons & INPUT_B) {
            TransferFlags(CHOOSING_INPUT, INPUT_B);
            g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
        }
        break;

    case SAVE_LOAD_STEP_QUIET:
        FadeSfxTo(0.0f, FADE_SECONDS);
        FadeMusicTo(0.0f, FADE_SECONDS);
        g_bSaveLoadStep = SAVE_LOAD_STEP_WAIT_QUIET;
        break;

    case SAVE_LOAD_STEP_WAIT_QUIET:
        if (HasSfxFadeFinished() && g_bSaveLoadFadeDelayFrames++ == 4) {
            g_bSaveLoadStep = SAVE_LOAD_STEP_MOUNT;
            g_bSaveLoadFadeDelayFrames = 0;
        }
        break;

    case SAVE_LOAD_STEP_MOUNT:
        osPfsIsPlug(&g_SiMesgQueue, &plugged);
        if (!(plugged & (1 << g_dwMenuChoosingPlayer))) {
            gControllerPakState = PFS_ERR_NOPACK;
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            break;
        }
        osSyncPrintf("\nInitializing controller pak...\n");
        gControllerPakState = osPfsInitPak(
            &g_SiMesgQueue, &g_aPakPfs[g_dwMenuChoosingPlayer], g_dwMenuChoosingPlayer);
        if (gControllerPakState == 0) {
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            if (!g_abSavePakIdKnown[g_dwMenuChoosingPlayer]) {
                g_abSavePakIdKnown[g_dwMenuChoosingPlayer] = TRUE;
                memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                       g_aPakPfs[g_dwMenuChoosingPlayer].id,
                       sizeof(g_aPakPfs[0].id));
                g_bSaveLoadStep = SAVE_LOAD_STEP_FIND;
            } else if (bcmp(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                            g_aPakPfs[g_dwMenuChoosingPlayer].id,
                            sizeof(g_aPakPfs[0].id)) != 0) {
                memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                       g_aPakPfs[g_dwMenuChoosingPlayer].id,
                       sizeof(g_aPakPfs[0].id));
                g_bSaveLoadStep = SAVE_LOAD_STEP_NEW_PAK;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_FIND;
            }
        } else if (gControllerPakState == PFS_ERR_ID_FATAL) {
            /* A Pak without a file system may be a Rumble Pak, which the player must swap. */
            if (osMotorInit(&g_SiMesgQueue,
                            &g_aContPfs[g_dwMenuChoosingPlayer],
                            g_dwMenuChoosingPlayer) == 0) {
                osMotorStop(&g_aContPfs[g_dwMenuChoosingPlayer]);
                gControllerPakState = PAK_STATE_RUMBLE_PAK;
                g_bSaveLoadRumbleSwapPending = TRUE;
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            } else {
                gControllerPakState = 0;
                g_bSaveLoadStep = SAVE_LOAD_STEP_REPAIR;
            }
        } else {
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
        }
        break;

    case SAVE_LOAD_STEP_REPAIR:
        gControllerPakState = osPfsRepairId(&g_aPakPfs[g_dwMenuChoosingPlayer]);
        if (gControllerPakState != 0) {
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
        } else {
            g_bSaveLoadStep = SAVE_LOAD_STEP_MOUNT;
        }
        break;

    case SAVE_LOAD_STEP_NEW_PAK:
        g_bSaveLoadInputEnabled = FALSE;
        SET_MESSAGE_TEXT();
        DRAW_NEW_PAK_MESSAGE(0xB9, 0x6C);
        if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
            TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
            g_bSaveLoadStep = SAVE_LOAD_STEP_FIND;
        }
        break;

    case SAVE_LOAD_STEP_FIND:
        gControllerPakState = osPfsFindFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                            COMPANY_CODE,
                                            GAME_CODE,
                                            noteName,
                                            extension,
                                            &g_dwSaveLoadNoteIndex);
        if (gControllerPakState == PFS_ERR_INCONSISTENT) {
            osSyncPrintf("File system is corrupted, attempting repair...\n");
            osPfsChecker(&g_aPakPfs[g_dwMenuChoosingPlayer]);
            gControllerPakState = osPfsFindFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                COMPANY_CODE,
                                                GAME_CODE,
                                                noteName,
                                                extension,
                                                &g_dwSaveLoadNoteIndex);
        }
        if (gControllerPakState == 0) {
            switch (g_bSaveLoadChoice) {
            case SAVE_LOAD_LOAD_SEASON:
            case SAVE_LOAD_LOAD_GHOST:
            case SAVE_LOAD_LOAD_OPTIONS:
                g_bSaveLoadStep = SAVE_LOAD_STEP_LOAD;
                break;
            case SAVE_LOAD_SAVE_SEASON:
            case SAVE_LOAD_SAVE_GHOST:
                g_bSaveLoadStep = SAVE_LOAD_STEP_CONFIRM_OVERWRITE;
                break;
            case SAVE_LOAD_SAVE_OPTIONS:
                /* The options are overwritten without asking. */
                g_bSaveLoadStep = SAVE_LOAD_STEP_SAVE;
                break;
            default:
                g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
                break;
            }
        } else if (gControllerPakState == PFS_ERR_INVALID) {
            /* The note is missing: an error to load it, and made to save it. */
            if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON ||
                g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST ||
                g_bSaveLoadChoice == SAVE_LOAD_LOAD_OPTIONS) {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ALLOCATE;
            }
        } else {
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
        }
        break;

    case SAVE_LOAD_STEP_LOAD:
        if (gControllerPakState != 0) {
            break;
        }
        if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON) {
            osSyncPrintf("Loading season data...\n");
        } else if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST) {
            osSyncPrintf("Loading compressed ghost data...\n");
        } else if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_OPTIONS) {
            osSyncPrintf("Loading Options...\n");
        }
        if (g_bSaveLoadChoice != SAVE_LOAD_LOAD_OPTIONS) {
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_READ,
                                                     0,
                                                     noteSize,
                                                     g_pPakDataBuffer);
        } else {
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_READ,
                                                     0,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
        }
        if (gControllerPakState != 0) {
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            break;
        }
        if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON) {
            /* Both players take the season, but the other player keeps the equipment of the car
             * when the main menu opened the menu. */
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                other = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile;
                GET_EQUIPMENT(equipment, other);
            }
            memcpy(g_aVehicles[0].profile, g_pPakDataBuffer, sizeof(PlayerProfile));
            memcpy(g_aVehicles[1].profile, g_pPakDataBuffer, sizeof(PlayerProfile));
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                SET_EQUIPMENT(g_dwMenuChoosingPlayer ^ 1, equipment);
            }
            osSyncPrintf("Done!\n");
            /* A loaded season or ghost car starts its race at once, without a message, unless the
             * player must put the Rumble Pak back. */
            if (!g_bSaveLoadRumbleSwapPending) {
                g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_DONE;
            }
            g_bSaveLoadDataWasLoaded = TRUE;
        } else if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST) {
            osSyncPrintf("Extracting ghost data...\n");
            g_dwGhostRecordingSize = DecompressInterleavedRle(
                g_abGhostRecording, GHOST_RECORDING_CAPACITY, g_pPakDataBuffer, GHOST_CHANNELS);
            /* The recording starts with its track and its car, which the loading unlocks. */
            g_dwSelectedTrackId = g_abGhostRecording[0];
            g_aVehicles[0].profile->unlockedTracks |= 1 << g_abGhostRecording[0];
            g_aVehicles[0].profile->unlocks |= 1 << g_abGhostRecording[1];
            osSyncPrintf("Done!\n");
            if (!g_bSaveLoadRumbleSwapPending) {
                g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_DONE;
            }
            g_bSaveLoadDataWasLoaded = TRUE;
        } else if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_OPTIONS) {
            memcpy(&g_OptionSettings, g_pPakDataBuffer, sizeof(OptionSettings));
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_READ,
                                                     OPTIONS_HALF_SIZE,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState != 0) {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
                break;
            }
            memcpy(equipment, g_pPakDataBuffer, sizeof(equipment));
            SET_EQUIPMENT(g_dwMenuChoosingPlayer, equipment);
            osSyncPrintf("Done!\n");
            ApplyVolumeSettings();
            g_bSaveLoadStep = SAVE_LOAD_STEP_DONE;
        }
        break;

    case SAVE_LOAD_STEP_ALLOCATE:
        gControllerPakState = osPfsFreeBlocks(&g_aPakPfs[g_dwMenuChoosingPlayer], &freeBlocks);
        if (gControllerPakState != 0) {
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            break;
        }
        if (freeBlocks < noteSize) {
            gControllerPakState = PFS_DATA_FULL;
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            break;
        }
        if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_SEASON) {
            osSyncPrintf("Allocating %d bytes for season data...\n", noteSize);
        } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_GHOST) {
            osSyncPrintf("Allocating %d bytes for ghost data...\n", noteSize);
        } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
            osSyncPrintf("Allocating %d bytes for options...\n", noteSize);
        }
        gControllerPakState = osPfsAllocateFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                COMPANY_CODE,
                                                GAME_CODE,
                                                noteName,
                                                extension,
                                                noteSize,
                                                &g_dwSaveLoadNoteIndex);
        if (gControllerPakState == PFS_ERR_INCONSISTENT) {
            osPfsChecker(&g_aPakPfs[g_dwMenuChoosingPlayer]);
            gControllerPakState = osPfsAllocateFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                    COMPANY_CODE,
                                                    GAME_CODE,
                                                    noteName,
                                                    extension,
                                                    noteSize,
                                                    &g_dwSaveLoadNoteIndex);
        }
        if (gControllerPakState == 0) {
            g_bSaveLoadStep = SAVE_LOAD_STEP_SAVE;
        } else {
            g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
        }
        break;

    case SAVE_LOAD_STEP_CONFIRM_OVERWRITE:
        g_bSaveLoadInputEnabled = FALSE;
        top = g_bSaveLoadEntryPoint != SAVE_LOAD_FROM_MAIN_MENU ? 0xBF : 0xC9;
        /* The buttons are placed by the width of the picture of A, not its height. */
        buttonY = top - g_texButtonA.width + 0x62;
        DRAW_MESSAGE_BOX(0xD5, top, 0xD6, 0x6A);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextSize(13);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
        textY = (top + 0x1E) >> 1;
        DrawText("OK TO OVERWRITE", 0x72, textY);
        DrawText("SAVED DATA?", 0x72, textY + 0xE);
        SetTextSize(10);
        textY = (buttonY + 0x12) >> 1;
        DrawText("%wwOK", (u32)(g_texButtonA.width + 0xED) >> 1, textY);
        /* CANCEL is placed by the height of the picture of B, unlike the other labels. */
        DrawText("%wwCANCEL", (u32)(g_texButtonB.height + 0x13B) >> 1, textY);
        DrawTexturedSprite(&g_texButtonA, 0xE7, buttonY);
        DrawTexturedSprite(&g_texButtonB, 0x135, buttonY);
        if (CHOOSING_INPUT->buttons & INPUT_A) {
            TransferFlags(CHOOSING_INPUT, INPUT_A);
            g_bSaveLoadStep = SAVE_LOAD_STEP_SAVE;
        } else if (CHOOSING_INPUT->buttons & INPUT_B) {
            TransferFlags(CHOOSING_INPUT, INPUT_B);
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                FadeSfxTo(1.0f, FADE_SECONDS);
                FadeMusicTo(1.0f, FADE_SECONDS);
                g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
            }
        }
        break;

    case SAVE_LOAD_STEP_SAVE:
        if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_SEASON) {
            osSyncPrintf("Saving season data...\n");
            memcpy(g_pPakDataBuffer, g_aVehicles[0].profile, sizeof(PlayerProfile));
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_WRITE,
                                                     0,
                                                     SEASON_NOTE_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                osSyncPrintf("Done!\n");
                g_bSaveLoadStep = SAVE_LOAD_STEP_DONE;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            }
        } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_GHOST) {
            osSyncPrintf("Compressing ghost data...\n");
            compressedSize = CompressRleChannels(g_pPakDataBuffer,
                                                 PAK_DATA_BUFFER_SIZE,
                                                 g_abGhostRecording,
                                                 g_dwGhostRecordingSize,
                                                 GHOST_CHANNELS);
            osSyncPrintf("new size = %d\n", compressedSize);
            if (compressedSize == -1) {
                gControllerPakState = PAK_STATE_GHOST_TOO_LARGE;
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
                break;
            }
            osSyncPrintf("Saving ghost data to controller pak...\n");
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_WRITE,
                                                     0,
                                                     PAK_DATA_BUFFER_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                osSyncPrintf("Done!\n");
                g_bSaveLoadStep = SAVE_LOAD_STEP_DONE;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            }
        } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
            osSyncPrintf("Saving options...\n");
            memcpy(g_pPakDataBuffer, &g_OptionSettings, sizeof(OptionSettings));
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_WRITE,
                                                     0,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState != 0) {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
                break;
            }
            GET_EQUIPMENT(equipment, g_aVehicles[g_dwMenuChoosingPlayer].profile);
            memcpy(g_pPakDataBuffer, equipment, sizeof(equipment));
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwSaveLoadNoteIndex,
                                                     PFS_WRITE,
                                                     OPTIONS_HALF_SIZE,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState == 0) {
                osSyncPrintf("Done!\n");
                g_bSaveLoadStep = SAVE_LOAD_STEP_DONE;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_ERROR;
            }
        }
        break;

    case SAVE_LOAD_STEP_ERROR:
        g_bSaveLoadInputEnabled = FALSE;
        SET_MESSAGE_TEXT();
        switch (gControllerPakState) {
        case PFS_ERR_ID_FATAL:
            DRAW_SHORT_MESSAGE(0xBD, 0x106, 0x66, "THE CONTROLLER PAK", "IS NONFUNCTIONAL.");
            break;
        case PFS_ERR_NOPACK:
        case PFS_ERR_DEVICE:
            DRAW_MESSAGE_BOX(0x94, 0xD8, 0x158, 0x4C);
            DrawText("CONTROLLER PAK NOT FOUND.", 0x52, 0x7C);
            if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON ||
                g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST ||
                g_bSaveLoadChoice == SAVE_LOAD_LOAD_OPTIONS) {
                DrawText("DATA CANNOT BE LOADED.", 0x52, 0x8A);
            } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_SEASON ||
                       g_bSaveLoadChoice == SAVE_LOAD_SAVE_GHOST ||
                       g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
                DrawText("DATA CANNOT BE SAVED.", 0x52, 0x8A);
            }
            break;
        case PAK_STATE_RUMBLE_PAK:
            DRAW_SWAP_FOR_CONTROLLER_PAK_MESSAGE(
                0xAE, 0x67, 0x146 - g_texButtonA.width, "%wwOK", "%wwCANCEL");
            break;
        case PFS_DATA_FULL:
        case PFS_DIR_FULL:
            DRAW_MESSAGE_BOX(0xA5, 0x9F, 0x136, 0xBD);
            if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_SEASON) {
                DRAW_INSUFFICIENT_PAGES_MESSAGE(0x5F, "TWO PAGES AND ONE NOTE", "SEASON DATA.");
            } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_GHOST) {
                DRAW_INSUFFICIENT_PAGES_MESSAGE(0x5F, "58 PAGES AND ONE NOTE", "GHOST CAR DATA.");
            } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
                DRAW_INSUFFICIENT_PAGES_MESSAGE(0x5F, "ONE PAGE AND ONE NOTE", "OPTIONS.");
            }
            break;
        case PAK_STATE_GHOST_TOO_LARGE:
            DRAW_MESSAGE_BOX(0x75, 0xBD, 0x196, 0x82);
            DrawText("UNABLE TO SAVE GHOST CAR DATA.", 0x42, 0x6E);
            DrawText("GHOST CAR DATA IS LARGER THAN", 0x42, 0x7C);
            DrawText("THE MAXIMUM ALLOCATED SPACE", 0x42, 0x8A);
            DrawText("IN THE CONTROLLER PAK.", 0x42, 0x98);
            break;
        case PFS_ERR_CONTRFAIL:
            DRAW_SHORT_MESSAGE(0xBF, 0x102, 0x67, "CONTROLLER ERROR", "HAS BEEN DETECTED!");
            break;
        case PFS_ERR_INVALID:
            if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON ||
                g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST ||
                g_bSaveLoadChoice == SAVE_LOAD_LOAD_OPTIONS) {
                DRAW_SHORT_MESSAGE(0xAD, 0x125, 0x5E, "SAVED DATA NOT FOUND", "IN CONTROLLER PAK.");
            } else if (g_bSaveLoadChoice == SAVE_LOAD_SAVE_SEASON ||
                       g_bSaveLoadChoice == SAVE_LOAD_SAVE_GHOST ||
                       g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
                DRAW_SHORT_MESSAGE(0xB6, 0x114, 0x63, "UNABLE TO SAVE DATA", "TO CONTROLLER PAK.");
            }
            break;
        case PFS_ERR_NEW_PACK:
            DRAW_NEW_PAK_MESSAGE(0xBB, 0x6D);
            break;
        default:
            DRAW_SHORT_MESSAGE(0xB1, 0x11E, 0x60, "CONTROLLER PAK ERROR", "HAS BEEN DETECTED!");
            break;
        }
        if (gControllerPakState != PAK_STATE_RUMBLE_PAK) {
            if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
                TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
                gControllerPakState = 0;
                /* The state was just cleared, so the menu always returns to the choices. */
                if (gControllerPakState == PFS_ERR_NEW_PACK) {
                    g_bSaveLoadStep = SAVE_LOAD_STEP_MOUNT;
                } else {
                    g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
                }
            }
        } else if (CHOOSING_INPUT->buttons & INPUT_A) {
            TransferFlags(CHOOSING_INPUT, INPUT_A);
            gControllerPakState = 0;
            g_bSaveLoadStep = SAVE_LOAD_STEP_MOUNT;
        } else if (CHOOSING_INPUT->buttons & INPUT_B) {
            TransferFlags(CHOOSING_INPUT, INPUT_B);
            gControllerPakState = 0;
            FadeSfxTo(1.0f, FADE_SECONDS);
            FadeMusicTo(1.0f, FADE_SECONDS);
            g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
        }
        break;

    case SAVE_LOAD_STEP_DONE:
        g_bSaveLoadInputEnabled = FALSE;
        SetTextAlignCenter();
        SetTextStyleNormal();
        SetTextSize(12);
        top = g_bSaveLoadEntryPoint != SAVE_LOAD_FROM_MAIN_MENU ? 0xCE : 0xD9;
        switch (g_bSaveLoadChoice) {
        case SAVE_LOAD_LOAD_SEASON:
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywSEASON DATA", "%ywLOADED OK!");
            break;
        case SAVE_LOAD_SAVE_SEASON:
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywSEASON DATA", "%ywSAVED OK!");
            break;
        case SAVE_LOAD_LOAD_GHOST:
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywGHOST DATA", "%ywLOADED OK!");
            break;
        case SAVE_LOAD_SAVE_GHOST:
            DRAW_DONE_MESSAGE(0xD8, 0xD0, top, "%ywGHOST CAR DATA", "%ywSAVED OK!");
            break;
        case SAVE_LOAD_LOAD_OPTIONS:
            DRAW_MESSAGE_BOX(0xC3, 0xE7, 0xFA, 0x2E);
            DrawText("%ywOPTIONS LOADED OK!", 0x9F, 0x83);
            break;
        case SAVE_LOAD_SAVE_OPTIONS:
            DRAW_MESSAGE_BOX(0xC3, 0xE7, 0xFA, 0x2E);
            DrawText("%ywOPTIONS SAVED OK!", 0x9F, 0x83);
            break;
        default:
            g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
            break;
        }
        if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
            TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                if (g_bSaveLoadRumbleSwapPending) {
                    g_bSaveLoadStep = SAVE_LOAD_STEP_RUMBLE_PAK;
                } else {
                    FadeSfxTo(1.0f, FADE_SECONDS);
                    FadeMusicTo(1.0f, FADE_SECONDS);
                    g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
                }
            } else if (g_bSaveLoadRumbleSwapPending) {
                g_bSaveLoadStep = SAVE_LOAD_STEP_RUMBLE_PAK;
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
            }
        }
        break;

    case SAVE_LOAD_STEP_RUMBLE_PAK:
        g_bSaveLoadInputEnabled = FALSE;
        SET_MESSAGE_TEXT();
        DRAW_SWAP_FOR_RUMBLE_PAK_MESSAGE(0x72, 0xB2, 0x19C, 0x69);
        if (CHOOSING_INPUT->buttons & INPUT_A) {
            TransferFlags(CHOOSING_INPUT, INPUT_A);
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON ||
                    g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST) {
                    g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
                } else {
                    FadeSfxTo(1.0f, FADE_SECONDS);
                    FadeMusicTo(1.0f, FADE_SECONDS);
                    g_bSaveLoadStep = SAVE_LOAD_STEP_CHOOSE;
                }
            } else {
                g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
            }
            g_bSaveLoadRumbleSwapPending = FALSE;
            if (osMotorInit(&g_SiMesgQueue,
                            &g_aContPfs[g_dwMenuChoosingPlayer],
                            g_dwMenuChoosingPlayer) == 0) {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = TRUE;
            }
        }
        break;

    case SAVE_LOAD_STEP_FADE_OUT:
        SetFadeTarget(0.0f, FADE_SECONDS);
        g_bSaveLoadStep = SAVE_LOAD_STEP_LEAVE;
        break;

    case SAVE_LOAD_STEP_LEAVE:
        FadeSfxTo(1.0f, FADE_SECONDS);
        FadeMusicTo(1.0f, FADE_SECONDS);
        break;
    }

    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    FlushDisplayList();
    if (IsFadedToBlack()) {
        g_bSaveLoadMenuInitialized = FALSE;
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        gControllerPakState = 0;
        if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON && g_bSaveLoadDataWasLoaded) {
            g_bSaveLoadDataWasLoaded = FALSE;
            g_wSerialInterfaceIdle = TRUE;
            SetCurrentScreenCallback(StartChampionshipRace);
        } else if (g_bSaveLoadChoice == SAVE_LOAD_LOAD_GHOST && g_bSaveLoadDataWasLoaded) {
            g_bSaveLoadDataWasLoaded = FALSE;
            g_wSerialInterfaceIdle = TRUE;
            SetCurrentScreenCallback(StartTimeAttack);
        } else {
            g_bSaveLoadChoice = SAVE_LOAD_LOAD_SEASON;
            if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_MAIN_MENU) {
                g_wSerialInterfaceIdle = TRUE;
                SetCurrentScreenCallback(ProcessMainMenu);
            } else if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_SEASON) {
                g_wSerialInterfaceIdle = TRUE;
                SetCurrentScreenCallback(ProcessTrackMenu);
            } else if (g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_TIME_ATTACK) {
                g_wSerialInterfaceIdle = TRUE;
                SetCurrentScreenCallback(ProcessMainMenu);
            }
        }
    }
    /* This also clears the flag just set for the next screen, so the motors stay stopped until
     * another screen frees the serial interface. */
    g_wSerialInterfaceIdle = FALSE;
}
#pragma GCC diagnostic pop

void PreparePakDataScreen(void) {
    LoadTextureFromRom(&g_texButtonA);
    LoadTextureFromRom(&g_texButtonB);
    osSyncPrintf("Allocating %d bytes for data_buf...\n", PAK_DATA_BUFFER_SIZE);
    g_pPakDataBuffer = AllocHeapAligned(PAK_DATA_BUFFER_SIZE);
}

s32 DrawRumblePakPrompt(s32 anyPlayer) {
    s32 player;

    SetTextStyleNormal();
    SetTextAlignLeft();
    SetTextSize(12);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
    DRAW_SWAP_FOR_RUMBLE_PAK_MESSAGE(0x73, 0xB4, 0x19A, 0x6A);
    for (player = 0; player != 2; player++) {
        if ((anyPlayer || player == g_dwMenuChoosingPlayer) &&
            (g_aPlayerInputState[player].buttons & DISMISS_BUTTONS)) {
            TransferFlags(&g_aPlayerInputState[player], DISMISS_BUTTONS);
            return TRUE;
        }
    }
    return FALSE;
}

void InitAllRumblePaks(void) {
    s32 player;

    g_wSerialInterfaceIdle = FALSE;
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) == 0) {
            osMotorStop(&g_aContPfs[player]);
            g_abRumblePakPresent[player] = TRUE;
        }
    }
    g_wSerialInterfaceIdle = TRUE;
}

s32 IsRumblePakMissing(void) {
    u8 plugged;
    s32 player;

    g_wSerialInterfaceIdle = FALSE;
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        osPfsIsPlug(&g_SiMesgQueue, &plugged);
        if (!(plugged & (1 << player))) {
            goto missing;
        }
        gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[player], player);
        if (gControllerPakState != 0) {
            if (gControllerPakState != PFS_ERR_ID_FATAL) {
                goto missing;
            }
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) != 0) {
                goto missing;
            }
            osMotorStop(&g_aContPfs[player]);
        }
        /* A Controller Pak is not a Rumble Pak. */
        if (gControllerPakState == 0) {
            goto missing;
        }
    }
    g_wSerialInterfaceIdle = TRUE;
    return FALSE;

missing:
    g_wSerialInterfaceIdle = TRUE;
    return TRUE;
}

/* The original reads a byte of its stack that is never set, and other data types set no note. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
s32 ProcessPakDataScreen(s32 dataType, u8 quiet) {
    /* TOP GEAR SEASON, TOP GEAR GHOST and TOP GEAR OPTIONS in the font of the Controller Pak. */
    u8 seasonName[16] = {0x2D,
                         0x28,
                         0x29,
                         0x0F,
                         0x20,
                         0x1E,
                         0x1A,
                         0x2B,
                         0x0F,
                         0x2C,
                         0x1E,
                         0x1A,
                         0x2C,
                         0x28,
                         0x27,
                         0x00};
    u8 ghostName[16] = {0x2D,
                        0x28,
                        0x29,
                        0x0F,
                        0x20,
                        0x1E,
                        0x1A,
                        0x2B,
                        0x0F,
                        0x20,
                        0x21,
                        0x28,
                        0x2C,
                        0x2D,
                        0x00,
                        0x00};
    u8 configurationName[16] = {0x2D,
                                0x28,
                                0x29,
                                0x0F,
                                0x20,
                                0x1E,
                                0x1A,
                                0x2B,
                                0x0F,
                                0x28,
                                0x29,
                                0x2D,
                                0x22,
                                0x28,
                                0x27,
                                0x2C};
    u8 extension[4] = {0};
    PlayerProfile *other;
    u8 *noteName;
    s32 noteSize;
    s32 equipment[EQUIPMENT_FIELDS];
    s32 freeBlocks;
    s32 top;
    u8 plugged;
    u8 unsetOffset;

    g_wSerialInterfaceIdle = FALSE;
    if (!g_bSaveLoadMenuInitialized) {
        g_bSaveLoadMenuInitialized = TRUE;
        gControllerPakState = 0;
        g_bPakDataStep = PAK_DATA_STEP_START;
        g_bPakDataWaitFrames = 0;
    }
    if (dataType == PAK_DATA_LOAD_SEASON) {
        noteName = seasonName;
        noteSize = SEASON_NOTE_SIZE;
    } else if (dataType == PAK_DATA_LOAD_GHOST) {
        noteName = ghostName;
        noteSize = PAK_DATA_BUFFER_SIZE;
    } else if (dataType == PAK_DATA_LOAD_CONFIGURATION || dataType == PAK_DATA_SAVE_CONFIGURATION) {
        noteName = configurationName;
        noteSize = OPTIONS_NOTE_SIZE;
    }

    switch (g_bPakDataStep) {
    case PAK_DATA_STEP_START:
        if (!quiet) {
            FadeSfxTo(0.0f, FADE_SECONDS);
            FadeMusicTo(0.0f, FADE_SECONDS);
            g_bPakDataStep = PAK_DATA_STEP_WAIT_QUIET;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        }
        break;

    case PAK_DATA_STEP_WAIT_QUIET:
        /* The count of frames is not reset, so it waits only the first time. */
        if (HasSfxFadeFinished() && g_bPakDataWaitFrames++ == 3) {
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        }
        break;

    case PAK_DATA_STEP_MOUNT:
        osPfsIsPlug(&g_SiMesgQueue, &plugged);
        if (!(plugged & (1 << g_dwMenuChoosingPlayer))) {
            gControllerPakState = PFS_ERR_NOPACK;
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        osSyncPrintf("\nInitializing controller pak...\n");
        gControllerPakState = osPfsInitPak(
            &g_SiMesgQueue, &g_aPakPfs[g_dwMenuChoosingPlayer], g_dwMenuChoosingPlayer);
        if (gControllerPakState == 0) {
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            if (!g_abSavePakIdKnown[g_dwMenuChoosingPlayer]) {
                g_abSavePakIdKnown[g_dwMenuChoosingPlayer] = TRUE;
                memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                       g_aPakPfs[g_dwMenuChoosingPlayer].id,
                       sizeof(g_aPakPfs[0].id));
                g_bPakDataStep = PAK_DATA_STEP_FIND;
            } else if (bcmp(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                            g_aPakPfs[g_dwMenuChoosingPlayer].id,
                            sizeof(g_aPakPfs[0].id)) != 0) {
                memcpy(g_aabSavePakIds[g_dwMenuChoosingPlayer],
                       g_aPakPfs[g_dwMenuChoosingPlayer].id,
                       sizeof(g_aPakPfs[0].id));
                g_bPakDataStep = PAK_DATA_STEP_NEW_PAK;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_FIND;
            }
        } else if (gControllerPakState == PFS_ERR_ID_FATAL) {
            if (osMotorInit(&g_SiMesgQueue,
                            &g_aContPfs[g_dwMenuChoosingPlayer],
                            g_dwMenuChoosingPlayer) == 0) {
                osMotorStop(&g_aContPfs[g_dwMenuChoosingPlayer]);
                gControllerPakState = PAK_STATE_RUMBLE_PAK;
                g_bSaveLoadRumbleSwapPending = TRUE;
                g_bPakDataStep = PAK_DATA_STEP_ERROR;
            } else {
                gControllerPakState = 0;
                g_bPakDataStep = PAK_DATA_STEP_REPAIR;
            }
        } else {
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_REPAIR:
        gControllerPakState = osPfsRepairId(&g_aPakPfs[g_dwMenuChoosingPlayer]);
        if (gControllerPakState != 0) {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        }
        break;

    case PAK_DATA_STEP_NEW_PAK:
        if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FIND;
            break;
        }
        SET_MESSAGE_TEXT();
        DRAW_NEW_PAK_MESSAGE(0xC3, 0x71);
        if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
            TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
            g_bPakDataStep = PAK_DATA_STEP_FIND;
        }
        break;

    case PAK_DATA_STEP_FIND:
        osSyncPrintf("Finding file...\n");
        gControllerPakState = osPfsFindFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                            COMPANY_CODE,
                                            GAME_CODE,
                                            noteName,
                                            extension,
                                            &g_dwPakDataNoteIndex);
        if (gControllerPakState == PFS_ERR_INCONSISTENT) {
            osPfsChecker(&g_aPakPfs[g_dwMenuChoosingPlayer]);
            gControllerPakState = osPfsFindFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                COMPANY_CODE,
                                                GAME_CODE,
                                                noteName,
                                                extension,
                                                &g_dwPakDataNoteIndex);
        }
        if (gControllerPakState == 0) {
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                g_bPakDataStep = PAK_DATA_STEP_SAVE;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_LOAD;
            }
        } else if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
            g_bPakDataStep = PAK_DATA_STEP_ALLOCATE;
        } else if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_LOAD:
        if (dataType == PAK_DATA_LOAD_SEASON) {
            osSyncPrintf("Loading season data...\n");
        } else if (dataType == PAK_DATA_LOAD_GHOST) {
            osSyncPrintf("Loading ghost data...\n");
        } else if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
            osSyncPrintf("Loading configuration...\n");
        }
        if (dataType != PAK_DATA_LOAD_CONFIGURATION) {
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwPakDataNoteIndex,
                                                     PFS_READ,
                                                     0,
                                                     noteSize,
                                                     g_pPakDataBuffer);
        } else {
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwPakDataNoteIndex,
                                                     PFS_READ,
                                                     0,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
        }
        if (gControllerPakState != 0) {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        if (dataType == PAK_DATA_LOAD_SEASON) {
            /* Both players take the season, but the other player keeps the equipment of the car.
             */
            other = g_aVehicles[g_dwMenuChoosingPlayer ^ 1].profile;
            GET_EQUIPMENT(equipment, other);
            memcpy(g_aVehicles[0].profile, g_pPakDataBuffer, sizeof(PlayerProfile));
            memcpy(g_aVehicles[1].profile, g_pPakDataBuffer, sizeof(PlayerProfile));
            SET_EQUIPMENT(g_dwMenuChoosingPlayer ^ 1, equipment);
            osSyncPrintf("Done!\n");
        } else if (dataType == PAK_DATA_LOAD_GHOST) {
            osSyncPrintf("Extracting ghost data...\n");
            g_dwGhostRecordingSize = DecompressInterleavedRle(
                g_abGhostRecording, GHOST_RECORDING_CAPACITY, g_pPakDataBuffer, GHOST_CHANNELS);
            g_aVehicles[0].profile->unlockedTracks |= 1 << g_abGhostRecording[0];
            g_aVehicles[0].profile->unlocks |= 1 << g_abGhostRecording[1];
            osSyncPrintf("Done!\n");
        } else if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
            memcpy(&g_OptionSettings, g_pPakDataBuffer, sizeof(OptionSettings));
            osSyncPrintf("Loading car equipment settings...\n");
            gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                     g_dwPakDataNoteIndex,
                                                     PFS_READ,
                                                     OPTIONS_HALF_SIZE,
                                                     OPTIONS_HALF_SIZE,
                                                     g_pPakDataBuffer);
            if (gControllerPakState != 0) {
                g_bPakDataStep = PAK_DATA_STEP_ERROR;
            } else {
                memcpy(equipment, g_pPakDataBuffer, sizeof(equipment));
                SET_EQUIPMENT(g_dwMenuChoosingPlayer, equipment);
                osSyncPrintf("Done!\n");
            }
        }
        if (g_bPakDataStep == PAK_DATA_STEP_ERROR) {
            break;
        }
        /* Only the configuration says it was loaded, unless the player must put the Rumble Pak
         * back. */
        if (dataType == PAK_DATA_LOAD_CONFIGURATION || g_bSaveLoadRumbleSwapPending) {
            g_bPakDataStep = PAK_DATA_STEP_DONE;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    case PAK_DATA_STEP_ALLOCATE:
        gControllerPakState = osPfsFreeBlocks(&g_aPakPfs[g_dwMenuChoosingPlayer], &freeBlocks);
        if (gControllerPakState != 0) {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        if (freeBlocks < OPTIONS_NOTE_SIZE) {
            gControllerPakState = PFS_DATA_FULL;
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        osSyncPrintf("Allocating %d bytes for configuration...\n", OPTIONS_NOTE_SIZE);
        gControllerPakState = osPfsAllocateFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                COMPANY_CODE,
                                                GAME_CODE,
                                                noteName,
                                                extension,
                                                OPTIONS_NOTE_SIZE,
                                                &g_dwPakDataNoteIndex);
        if (gControllerPakState == PFS_ERR_INCONSISTENT) {
            osPfsChecker(&g_aPakPfs[g_dwMenuChoosingPlayer]);
            gControllerPakState = osPfsAllocateFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                    COMPANY_CODE,
                                                    GAME_CODE,
                                                    noteName,
                                                    extension,
                                                    OPTIONS_NOTE_SIZE,
                                                    &g_dwPakDataNoteIndex);
        }
        if (gControllerPakState == 0) {
            g_bPakDataStep = PAK_DATA_STEP_SAVE;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_SAVE:
        osSyncPrintf("Saving configuration...\n");
        memcpy(g_pPakDataBuffer, &g_OptionSettings, sizeof(OptionSettings));
        gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                 g_dwPakDataNoteIndex,
                                                 PFS_WRITE,
                                                 0,
                                                 OPTIONS_HALF_SIZE,
                                                 g_pPakDataBuffer);
        if (gControllerPakState != 0) {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        GET_EQUIPMENT(equipment, g_aVehicles[g_dwMenuChoosingPlayer].profile);
        memcpy(g_pPakDataBuffer, equipment, sizeof(equipment));
        gControllerPakState = osPfsReadWriteFile(&g_aPakPfs[g_dwMenuChoosingPlayer],
                                                 g_dwPakDataNoteIndex,
                                                 PFS_WRITE,
                                                 OPTIONS_HALF_SIZE,
                                                 OPTIONS_HALF_SIZE,
                                                 g_pPakDataBuffer);
        if (gControllerPakState == 0) {
            osSyncPrintf("Done!\n");
            g_bPakDataStep = PAK_DATA_STEP_DONE;
        } else {
            g_bPakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_ERROR:
        if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
            break;
        }
        SET_MESSAGE_TEXT();
        switch (gControllerPakState) {
        case PFS_ERR_ID_FATAL:
            DRAW_SHORT_MESSAGE(0xBD, 0x106, 0x66, "THE CONTROLLER PAK", "IS NONFUNCTIONAL!");
            break;
        case PFS_ERR_NOPACK:
        case PFS_ERR_DEVICE:
            DRAW_MESSAGE_BOX(0x94, 0xD8, 0x158, 0x4C);
            DrawText("CONTROLLER PAK NOT FOUND.", 0x52, 0x7C);
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                DrawText("DATA CANNOT BE SAVED.", 0x52, 0x8A);
            } else {
                DrawText("DATA CANNOT BE LOADED.", 0x52, 0x8A);
            }
            break;
        case PAK_STATE_RUMBLE_PAK:
            DRAW_SWAP_FOR_CONTROLLER_PAK_MESSAGE(
                0xB0, 0x68, 0x148 - g_texButtonA.width, "%wwOK", "%wwCANCEL");
            break;
        case PFS_DATA_FULL:
        case PFS_DIR_FULL:
            DRAW_MESSAGE_BOX(0xA5, 0xA5, 0x136, 0xBD);
            DRAW_INSUFFICIENT_PAGES_MESSAGE(0x62, "ONE PAGE AND ONE NOTE", "OPTIONS.");
            break;
        case PFS_ERR_CONTRFAIL:
            DRAW_SHORT_MESSAGE(0xBF, 0x102, 0x67, "CONTROLLER ERROR", "HAS BEEN DETECTED!");
            break;
        case PFS_ERR_INVALID:
            if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
                DRAW_SHORT_MESSAGE(0xAF, 0x122, 0x5F, "CONTROLLER PAK ERROR", "HAS BEEN DETECTED!");
            } else {
                DRAW_SHORT_MESSAGE(0xAF, 0x122, 0x5F, "SAVED DATA NOT FOUND", "IN CONTROLLER PAK.");
            }
            break;
        case PFS_ERR_NEW_PACK:
            DRAW_NEW_PAK_MESSAGE(0xB9, 0x6C);
            break;
        default:
            DRAW_SHORT_MESSAGE(0xAF, 0x122, 0x5F, "CONTROLLER PAK ERROR", "HAS BEEN DETECTED.");
            break;
        }
        if (gControllerPakState != PAK_STATE_RUMBLE_PAK) {
            if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
                TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
                gControllerPakState = 0;
                /* The state was just cleared, so the screen always finishes. */
                if (gControllerPakState == PFS_ERR_NEW_PACK) {
                    g_bPakDataStep = PAK_DATA_STEP_MOUNT;
                } else {
                    g_bPakDataStep = PAK_DATA_STEP_FINISH;
                }
            }
        } else if (CHOOSING_INPUT->buttons & INPUT_A) {
            TransferFlags(CHOOSING_INPUT, INPUT_A);
            gControllerPakState = 0;
            g_bPakDataStep = PAK_DATA_STEP_MOUNT;
        } else if (CHOOSING_INPUT->buttons & INPUT_B) {
            TransferFlags(CHOOSING_INPUT, INPUT_B);
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    case PAK_DATA_STEP_DONE:
        if (quiet) {
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
            break;
        }
        SetTextAlignCenter();
        SetTextStyleNormal();
        SetTextSize(12);
        if (dataType == PAK_DATA_LOAD_SEASON) {
            top = g_bSaveLoadEntryPoint != SAVE_LOAD_FROM_MAIN_MENU ? 0xCE : unsetOffset + 0xCB;
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywSEASON DATA", "%ywLOADED OK!");
        } else if (dataType == PAK_DATA_LOAD_GHOST) {
            top = g_bSaveLoadEntryPoint != SAVE_LOAD_FROM_MAIN_MENU ? 0xCE : unsetOffset + 0xCB;
            DRAW_DONE_MESSAGE(0xE2, 0xBC, top, "%ywGHOST DATA", "%ywLOADED OK!");
        } else if (dataType == PAK_DATA_LOAD_CONFIGURATION ||
                   dataType == PAK_DATA_SAVE_CONFIGURATION) {
            DRAW_MESSAGE_BOX(0xD8, 0xD9, 0xD0, 0x4A);
            DrawText("%ywCONFIGURATION", 0x9F, 0x7C);
            if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                DrawText("%ywLOADED OK!", 0x9F, 0x8A);
            } else {
                DrawText("%ywSAVED OK!", 0x9F, 0x8A);
            }
        }
        if (CHOOSING_INPUT->buttons & DISMISS_BUTTONS) {
            TransferFlags(CHOOSING_INPUT, DISMISS_BUTTONS);
            if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                ApplyVolumeSettings();
            }
            if (g_bSaveLoadRumbleSwapPending) {
                g_bPakDataStep = PAK_DATA_STEP_RUMBLE_PAK;
            } else {
                g_bPakDataStep = PAK_DATA_STEP_FINISH;
            }
        }
        break;

    case PAK_DATA_STEP_RUMBLE_PAK:
        if (DrawRumblePakPrompt(FALSE)) {
            g_bSaveLoadRumbleSwapPending = FALSE;
            if (osMotorInit(&g_SiMesgQueue,
                            &g_aContPfs[g_dwMenuChoosingPlayer],
                            g_dwMenuChoosingPlayer) == 0) {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = TRUE;
            }
            g_bPakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    case PAK_DATA_STEP_FINISH:
        gControllerPakState = 0;
        g_bSaveLoadMenuInitialized = FALSE;
        if (!quiet) {
            FadeSfxTo(1.0f, FADE_SECONDS);
            FadeMusicTo(1.0f, FADE_SECONDS);
        }
        g_wSerialInterfaceIdle = TRUE;
        return TRUE;
    }

    g_wSerialInterfaceIdle = TRUE;
    return FALSE;
}
#pragma GCC diagnostic pop

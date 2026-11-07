#ifndef COASTLINE_N64_OPTIONS_INTERNAL_H
#define COASTLINE_N64_OPTIONS_INTERNAL_H

#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "common/audio.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/rumble.h"
#include "common/system.h"
#include "common/text.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "controllerscreen.h"
#include "mainmenu.h"
#include "options-message.h"
#include "options.h"
#include "pakmenu.h"
#include "sprite.h"

#define OPTIONS_US_STYLE (TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA)

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
    SAVE_LOAD_STEP_FADE_IN,    /*!< Wait for the screen to fade in. */
    SAVE_LOAD_STEP_CHOOSE,     /*!< Choose what to load or save. */
    SAVE_LOAD_STEP_QUIET,      /*!< Start fading out the sound and the music. */
    SAVE_LOAD_STEP_WAIT_QUIET, /*!< Wait for the sound to fade out, then 4 more frames. */
#if !OPTIONS_US_STYLE
    SAVE_LOAD_STEP_GAME_PAK,      /*!< Load or save the chosen data on the EEPROM. */
    SAVE_LOAD_STEP_ERASE,         /*!< Reset both players and write the defaults to the EEPROM. */
    SAVE_LOAD_STEP_CONFIRM_ERASE, /*!< Request confirmation to erase the EEPROM. */
#endif
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
        SetTextSize(MESSAGE_TEXT_SIZE);                                                            \
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);                                   \
    }

/* The size of the text of the messages. */
#if TGR_REGION == TGR_REGION_JP
#define MESSAGE_TEXT_SIZE 14
#else
#define MESSAGE_TEXT_SIZE 12
#endif

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

/* The progress of a Controller Pak transfer, which the Japanese release does not print. The
 * arguments go in their own parentheses. */
#if TGR_REGION == TGR_REGION_JP
#define PAK_LOG(args)
#else
#define PAK_LOG(args) osSyncPrintf args
#endif

/* The choice of the save and load menu, which the other releases take from the row under the
 * cursor. */
#if OPTIONS_US_STYLE
#define SAVE_LOAD_ACTION g_bSaveLoadChoice
#else
#define SAVE_LOAD_ACTION (items[g_bSaveLoadMenuCursor].action)
#endif

/* The original looks the profile of the vehicle up again for every field. */
#define SET_EQUIPMENT(vehicle, equipment)                                                          \
    {                                                                                              \
        g_aVehicles[vehicle].profile->handling = (equipment)[0];                                   \
        g_aVehicles[vehicle].profile->transmission = (equipment)[1];                               \
        g_aVehicles[vehicle].profile->tires = (equipment)[2];                                      \
        g_aVehicles[vehicle].profile->suspension = (equipment)[3];                                 \
        g_aVehicles[vehicle].profile->controllerLayout = (equipment)[4];                           \
    }

#endif

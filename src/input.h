#ifndef COASTLINE_INPUT_H
#define COASTLINE_INPUT_H

/**
 * @file
 * The input of each player, the cheat codes, and the translation of controller data.
 */

#include <PR/os_cont.h>
#include <PR/ultratypes.h>

/** The number of players the input state holds. */
#define MAX_PLAYERS 4

/* The buttons of the input, which the menus read. The stick also presses the four directions. */
#define INPUT_RIGHT 0x0001
#define INPUT_DOWN 0x0002
#define INPUT_LEFT 0x0004
#define INPUT_UP 0x0008
#define INPUT_A 0x0010
#define INPUT_B 0x0020
#define INPUT_C_UP 0x0100
#define INPUT_C_RIGHT 0x0200
#define INPUT_C_DOWN 0x0400
#define INPUT_C_LEFT 0x0800
#define INPUT_L 0x1000
#define INPUT_R 0x2000
#define INPUT_START 0x4000
#define INPUT_Z 0x8000

/* The actions of a race, which the button layout maps the buttons to. */
#define INPUT_ACCELERATE 0x00010000
#define INPUT_REVERSE 0x00020000
#define INPUT_BRAKE 0x00040000
#define INPUT_EBRAKE 0x00080000
#define INPUT_GEAR_UP 0x00100000
#define INPUT_GEAR_DOWN 0x00200000
/* Three more actions the C buttons give, whose use is not reconstructed yet. */
#define INPUT_RACE_ACTION_24 0x01000000
#define INPUT_RACE_ACTION_26 0x04000000
#define INPUT_RACE_ACTION_27 0x08000000

/** The actions a recording keeps, with the steering. */
#define INPUT_RECORDED_ACTIONS 0x003F0000

/** The button layouts of the controller screen. */
#define CONTROLLER_LAYOUTS 5

/** The button changes a player's input remembers, for the cheat codes. */
#define INPUT_HISTORY_SIZE 128

/** The recordings of the input a player can make at once. */
#define INPUT_RECORDERS 2

/** The input of one player. */
typedef struct {
    /** The buttons pressed and the race actions, as the INPUT_ bits. */
    u32 buttons;
    /** The buttons TransferFlags() moved out of the input until they are released. */
    u32 heldButtons;
    /** The time left before the stick presses a direction again, in milliseconds. */
    s32 repeatUp;
    s32 repeatDown;
    s32 repeatLeft;
    s32 repeatRight;
    /** The stick and the steering, from -1 to 1. */
    f32 stickX;
    f32 stickY;
    f32 steering;
    /** The steering as the stick or the recording gives it. */
    s8 steeringInput;
    /** The button layout of the race, from 0 to CONTROLLER_LAYOUTS - 1. */
    u8 controllerLayout;
    /** Set when the controller of the player does not respond. */
    s32 controllerMissing;
    /** The buffers that record two bytes of input each frame, or NULL. */
    u8 *recordBuffers[INPUT_RECORDERS];
    s32 recordPositions[INPUT_RECORDERS];
    s32 recordLimits[INPUT_RECORDERS];
    /** The recording that replaces the input, or NULL. */
    u8 *playbackBuffer;
    s32 playbackPosition;
    s32 playbackLimit;
    /** The last button changes, the newest at historyIndex. */
    u16 history[INPUT_HISTORY_SIZE];
    s32 historyIndex;
    s32 playerIndex;
    /** The pad the controller reads fill. */
    OSContPad *pad;
} PlayerInputState;

/** A cheat: the sequence of button changes that applies it, newest first, ending in 0xFFFF. */
typedef struct {
    void (*apply)(void);
    u16 *sequence;
} CheatCode;

/** @ghidraAddress USA: 0x8036a8e0 */
extern PlayerInputState g_aPlayerInputState[MAX_PLAYERS];

/** The pads the controller reads fill, one per player. @ghidraAddress USA: 0x8031a3e0 */
extern OSContPad g_aContPadData[MAX_PLAYERS];

/** The number of players at this console. @ghidraAddress USA: 0x8026ff08 */
extern u32 g_dwNumLocalPlayers;

/** The cheats, ending with a NULL sequence. @ghidraAddress USA: 0x8028df48 */
extern CheatCode g_aCheatCodes[];

/** Whether each frame shows the start of memory in place of the picture. @ghidraAddress USA:
 * 0x8028aa94 */
extern s32 g_bShowRamOnScreen;

/** @ghidraAddress USA: 0x8026ff20 */
extern s32 g_dwMultiplayerMode;

/** Whether textures are filtered. @ghidraAddress USA: 0x8028aa68 */
extern s32 g_bBilinearFiltering;

/**
 * Toggle the view of memory.
 *
 * @ghidraAddress USA: 0x80254f40
 */
void CheatToggleRamView(void);

/**
 * Unlock the first five tracks.
 *
 * @ghidraAddress USA: 0x80254f58
 */
void CheatUnlockTracks0To4(void);

/**
 * Unlock the first nine cars.
 *
 * @ghidraAddress USA: 0x80254f70
 */
void CheatUnlockCars0To8(void);

/**
 * Set bit 9 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254f88
 */
void CheatSetUnlockBit200(void);

/**
 * Set bit 12 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254fa0
 */
void CheatSetUnlockBit1000(void);

/**
 * Set bit 11 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254fb8
 */
void CheatSetUnlockBit800(void);

/**
 * Set bit 10 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254fd0
 */
void CheatSetUnlockBit400(void);

/**
 * Unlock tracks 5 to 9.
 *
 * @ghidraAddress USA: 0x80254fe8
 */
void CheatUnlockTracks5To9(void);

/**
 * Set bit 15 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80255000
 */
void CheatSetUnlockBit8000(void);

/** @ghidraAddress USA: 0x80255018 */
void CheatToggleMultiplayerMode(void);

/** @ghidraAddress USA: 0x80255030 */
void CheatToggleBilinearFilter(void);

/**
 * Remember a change of the buttons and apply each cheat whose sequence the latest changes match.
 *
 * @ghidraAddress USA: 0x80255048
 */
void ProcessInputCommandBuffer(PlayerInputState *state);

/**
 * Read the pad of a player into buttons, race actions for the button layout, and the stick, record
 * the input or replay a recording in its place, and check the cheats.
 *
 * @ghidraAddress USA: 0x80255120
 */
void TranslateControllerInput(PlayerInputState *state);

/**
 * Move buttons out of the input until they are released, so that they act once.
 *
 * @ghidraAddress USA: 0x80255910
 */
void TransferFlags(PlayerInputState *state, u32 mask);

/**
 * Drop from the input the held buttons that are still pressed, and forget the held buttons that
 * are released.
 *
 * @ghidraAddress USA: 0x80255934
 */
void ConsumeInputFlags(PlayerInputState *state);

/**
 * Press a direction when a stick axis passes 0.6, then again after half a second and every 0.2
 * seconds while the axis stays above 0.3.
 *
 * @ghidraAddress USA: 0x80255954
 */
void CheckAnalogStickPositive(PlayerInputState *state, s32 *timer, f32 *axis, u32 button);

/**
 * The same as CheckAnalogStickPositive() for the negative side of an axis.
 *
 * @ghidraAddress USA: 0x80255a18
 */
void CheckAnalogStickNegative(PlayerInputState *state, s32 *timer, f32 *axis, u32 button);

/**
 * Press the directions of the stick with repeat, as the menus read them.
 *
 * @ghidraAddress USA: 0x80255adc
 */
void UpdateStickRepeat(PlayerInputState *state);

/**
 * Reset the input of a player.
 *
 * @ghidraAddress USA: 0x80255b54
 */
void InitPlayerInputState(PlayerInputState *state);

#endif

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
#define INPUT_RIGHT 0x0001   /*!< The right of the directional pad or the stick. */
#define INPUT_DOWN 0x0002    /*!< The down of the directional pad or the stick. */
#define INPUT_LEFT 0x0004    /*!< The left of the directional pad or the stick. */
#define INPUT_UP 0x0008      /*!< The up of the directional pad or the stick. */
#define INPUT_A 0x0010       /*!< The A button. */
#define INPUT_B 0x0020       /*!< The B button. */
#define INPUT_C_UP 0x0100    /*!< The up C button. */
#define INPUT_C_RIGHT 0x0200 /*!< The right C button. */
#define INPUT_C_DOWN 0x0400  /*!< The down C button. */
#define INPUT_C_LEFT 0x0800  /*!< The left C button. */
#define INPUT_L 0x1000       /*!< The L trigger. */
#define INPUT_R 0x2000       /*!< The R trigger. */
#define INPUT_START 0x4000   /*!< The Start button. */
#define INPUT_Z 0x8000       /*!< The Z trigger. */

/* The actions of a race, which the button layout maps the buttons to. */
#define INPUT_ACCELERATE 0x00010000 /*!< Accelerate. */
#define INPUT_REVERSE 0x00020000    /*!< Drive in reverse. */
#define INPUT_BRAKE 0x00040000      /*!< Brake. */
#define INPUT_EBRAKE 0x00080000     /*!< Pull the handbrake. */
#define INPUT_GEAR_UP 0x00100000    /*!< Shift up a gear. */
#define INPUT_GEAR_DOWN 0x00200000  /*!< Shift down a gear. */
/* The views the C buttons choose: the cockpit, the trackside camera, the chase camera and the
 * bumper. The computer's drivers request the trackside camera. */
#define INPUT_VIEW_COCKPIT 0x01000000   /*!< Choose the view from the cockpit. */
#define INPUT_VIEW_TRACKSIDE 0x02000000 /*!< Choose the trackside camera. */
#define INPUT_VIEW_CHASE 0x04000000     /*!< Choose the chase camera. */
#define INPUT_VIEW_BUMPER 0x08000000    /*!< Choose the view from the bumper. */
#define INPUT_VIEWS 0x0F000000          /*!< Every bit that chooses a view. */

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
    u32 buttons;           /*!< The buttons pressed and the race actions, as the INPUT_ bits. */
    u32 heldButtons;       /*!< The buttons TransferFlags() moved out of the input until they are
                                released. */
    s32 repeatUp;          /*!< The milliseconds before the stick presses up again. */
    s32 repeatDown;        /*!< The milliseconds before the stick presses down again. */
    s32 repeatLeft;        /*!< The milliseconds before the stick presses left again. */
    s32 repeatRight;       /*!< The milliseconds before the stick presses right again. */
    f32 stickX;            /*!< The horizontal position of the stick, from -1 to 1. */
    f32 stickY;            /*!< The vertical position of the stick, from -1 to 1. */
    f32 steering;          /*!< The steering, from -1 to 1. */
    s8 steeringInput;      /*!< The steering as the stick, the pad, or the recording gives it. */
    u8 controllerLayout;   /*!< The button layout of the race, from 0 to #CONTROLLER_LAYOUTS - 1. */
    s32 controllerMissing; /*!< Set when the controller of the player does not respond. */
    u8 *recordBuffers[INPUT_RECORDERS];   /*!< The buffers that record two bytes of input each
                                               frame, or NULL. */
    s32 recordPositions[INPUT_RECORDERS]; /*!< The byte offset of the next frame of each
                                               recording. */
    s32 recordLimits[INPUT_RECORDERS];    /*!< The size of each recording buffer, in bytes. */
    u8 *playbackBuffer;                   /*!< The recording that replaces the input, or NULL. */
    s32 playbackPosition; /*!< The byte offset of the next frame of the recording played. */
    s32 playbackLimit;    /*!< The size of the recording played, in bytes. */
    u16 history[INPUT_HISTORY_SIZE]; /*!< The last button changes, the newest at historyIndex. */
    s32 historyIndex; /*!< The index of the newest button change, moving down as changes come. */
    s32 playerIndex;  /*!< The player of the input, from 0 to #MAX_PLAYERS - 1. */
    OSContPad *pad;   /*!< The pad the controller reads fill. */
} PlayerInputState;

/** A cheat: the sequence of button changes that applies it, newest first, ending in 0xFFFF. */
typedef struct {
    void (*apply)(void); /*!< The routine called when the sequence is entered. */
    u16 *sequence;       /*!< The button changes, or NULL to end the list of cheats. */
} CheatCode;

/**
 * @ghidraAddress USA: 0x8036a8e0
 * @ghidraAddress EUR: 0x8036cde0
 * @ghidraAddress JPN: 0x8036ba50
 */
extern PlayerInputState g_aPlayerInputState[MAX_PLAYERS];

/**
 * The pads the controller reads fill, one per player.
 *
 * @ghidraAddress USA: 0x8031a3e0
 * @ghidraAddress EUR: 0x8031c8e0
 * @ghidraAddress JPN: 0x8031b510
 */
extern OSContPad g_aContPadData[MAX_PLAYERS];

/**
 * The number of players at this console.
 *
 * @ghidraAddress USA: 0x8026ff08
 * @ghidraAddress EUR: 0x80271ed8
 * @ghidraAddress JPN: 0x80271988
 */
extern u32 g_dwNumLocalPlayers;

/**
 * The cheats, ending with a NULL sequence.
 *
 * @ghidraAddress USA: 0x8028df48
 * @ghidraAddress EUR: 0x80290704
 * @ghidraAddress JPN: 0x802901a4
 */
extern CheatCode g_aCheatCodes[];

/**
 * Whether each frame shows the depth buffer, at the start of memory, in place of the picture.
 *
 * @ghidraAddress USA: 0x8028aa94
 * @ghidraAddress EUR: 0x8028cc9c
 * @ghidraAddress JPN: 0x8028c76c
 */
extern s32 g_bShowDepthBuffer;

/**
 * @ghidraAddress USA: 0x8026ff20
 * @ghidraAddress EUR: 0x80271ef0
 * @ghidraAddress JPN: 0x802719a0
 */
extern s32 g_dwMultiplayerMode;

/**
 * Whether textures are filtered.
 *
 * @ghidraAddress USA: 0x8028aa68
 * @ghidraAddress EUR: 0x8028cc70
 * @ghidraAddress JPN: 0x8028c740
 */
extern s32 g_bBilinearFiltering;

/**
 * Toggle the view of memory.
 *
 * @ghidraAddress USA: 0x80254f40
 * @ghidraAddress EUR: 0x802564e0
 * @ghidraAddress JPN: 0x80255e70
 */
void CheatToggleRamView(void);

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * Set by CheatFinishRace() to finish the race.
 *
 * @ghidraAddress EUR: 0x80272798
 * @ghidraAddress JPN: 0x80272248
 */
extern s32 g_dwCheatFinishRacePending;

/**
 * Request that the race finish.
 *
 * @ghidraAddress EUR: 0x802564f8
 * @ghidraAddress JPN: 0x80255e88
 */
void CheatFinishRace(void);
#endif

/**
 * Unlock the first five tracks.
 *
 * @ghidraAddress USA: 0x80254f58
 * @ghidraAddress EUR: 0x80256528
 * @ghidraAddress JPN: 0x80255eb8
 */
void CheatUnlockTracks0To4(void);

/**
 * Unlock the first nine cars.
 *
 * @ghidraAddress USA: 0x80254f70
 * @ghidraAddress EUR: 0x80256540
 * @ghidraAddress JPN: 0x80255ed0
 */
void CheatUnlockCars0To8(void);

/**
 * Set bit 9 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254f88
 * @ghidraAddress EUR: 0x80256558
 * @ghidraAddress JPN: 0x80255ee8
 */
void CheatSetUnlockBit200(void);

/**
 * Set bit 12 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254fa0
 * @ghidraAddress EUR: 0x80256570
 * @ghidraAddress JPN: 0x80255f00
 */
void CheatSetUnlockBit1000(void);

/**
 * Set bit 11 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254fb8
 * @ghidraAddress EUR: 0x80256588
 * @ghidraAddress JPN: 0x80255f18
 */
void CheatSetUnlockBit800(void);

/**
 * Set bit 10 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80254fd0
 * @ghidraAddress EUR: 0x802565a0
 * @ghidraAddress JPN: 0x80255f30
 */
void CheatSetUnlockBit400(void);

/**
 * Unlock tracks 5 to 9.
 *
 * @ghidraAddress USA: 0x80254fe8
 * @ghidraAddress EUR: 0x802565b8
 * @ghidraAddress JPN: 0x80255f48
 */
void CheatUnlockTracks5To9(void);

/**
 * Set bit 15 of the unlocks of the profile.
 *
 * @ghidraAddress USA: 0x80255000
 * @ghidraAddress EUR: 0x802565d0
 * @ghidraAddress JPN: 0x80255f60
 */
void CheatSetUnlockBit8000(void);

/**
 * @ghidraAddress USA: 0x80255018
 * @ghidraAddress EUR: 0x802565e8
 * @ghidraAddress JPN: 0x80255f78
 */
void CheatToggleMultiplayerMode(void);

/**
 * @ghidraAddress USA: 0x80255030
 * @ghidraAddress EUR: 0x80256600
 * @ghidraAddress JPN: 0x80255f90
 */
void CheatToggleBilinearFilter(void);

/**
 * Remember a change of the buttons and apply each cheat whose sequence the latest changes match.
 *
 * @param state The input of the player.
 * @ghidraAddress USA: 0x80255048
 * @ghidraAddress EUR: 0x80256618
 * @ghidraAddress JPN: 0x80255fa8
 */
void ProcessInputCommandBuffer(PlayerInputState *state);

/**
 * Read the pad of a player into buttons, race actions for the button layout, and the stick, record
 * the input or replay a recording in its place, and check the cheats.
 *
 * @param state The input of the player.
 * @ghidraAddress USA: 0x80255120
 * @ghidraAddress EUR: 0x802566f0
 * @ghidraAddress JPN: 0x80256080
 */
void TranslateControllerInput(PlayerInputState *state);

/**
 * Move buttons out of the input until they are released, so that they act once.
 *
 * @param state The input of the player.
 * @param mask The buttons to move.
 * @ghidraAddress USA: 0x80255910
 * @ghidraAddress EUR: 0x80256ee0
 * @ghidraAddress JPN: 0x80256870
 */
void TransferFlags(PlayerInputState *state, u32 mask);

/**
 * Drop from the input the held buttons that are still pressed, and forget the held buttons that
 * are released.
 *
 * @param state The input of the player.
 * @ghidraAddress USA: 0x80255934
 * @ghidraAddress EUR: 0x80256f04
 * @ghidraAddress JPN: 0x80256894
 */
void ConsumeInputFlags(PlayerInputState *state);

/**
 * Press a direction when a stick axis passes 0.6, then again after half a second and every 0.2
 * seconds while the axis stays above 0.3.
 *
 * @param state The input of the player.
 * @param timer The time to the next repeat in milliseconds, 0 while the axis is released.
 * @param axis The position of the axis, from -1 to 1.
 * @param button The direction to press.
 * @ghidraAddress USA: 0x80255954
 * @ghidraAddress EUR: 0x80256f24
 * @ghidraAddress JPN: 0x802568b4
 */
void CheckAnalogStickPositive(PlayerInputState *state, s32 *timer, f32 *axis, u32 button);

/**
 * The same as CheckAnalogStickPositive() for the negative side of an axis.
 *
 * @param state The input of the player.
 * @param timer The time to the next repeat in milliseconds, 0 while the axis is released.
 * @param axis The position of the axis, from -1 to 1.
 * @param button The direction to press.
 * @ghidraAddress USA: 0x80255a18
 * @ghidraAddress EUR: 0x80256fe8
 * @ghidraAddress JPN: 0x80256978
 */
void CheckAnalogStickNegative(PlayerInputState *state, s32 *timer, f32 *axis, u32 button);

/**
 * Press the directions of the stick with repeat, as the menus read them.
 *
 * @param state The input of the player.
 * @ghidraAddress USA: 0x80255adc
 * @ghidraAddress EUR: 0x802570ac
 * @ghidraAddress JPN: 0x80256a3c
 */
void UpdateStickRepeat(PlayerInputState *state);

/**
 * Reset the input of a player.
 *
 * @param state The input to reset.
 * @ghidraAddress USA: 0x80255b54
 * @ghidraAddress EUR: 0x80257124
 * @ghidraAddress JPN: 0x80256ab4
 */
void InitPlayerInputState(PlayerInputState *state);

/**
 * Whether a read of the controllers is started and not yet received.
 *
 * @ghidraAddress USA: 0x8028ab6c
 * @ghidraAddress EUR: 0x8028ce18
 * @ghidraAddress JPN: 0x8028c848
 */
extern s32 g_dwContReadPending;

/**
 * Whether InitControllersAndPaks() has mounted the Controller Pak of the first controller.
 *
 * @ghidraAddress USA: 0x8028adf0
 * @ghidraAddress EUR: 0x8028d0a0
 * @ghidraAddress JPN: 0x8028cad0
 */
extern u8 g_bPakPort0Initialized;

/**
 * Start a read of the controllers, unless one is already pending.
 *
 * @ghidraAddress USA: 0x8021a920
 * @ghidraAddress EUR: 0x8021ba38
 * @ghidraAddress JPN: 0x8021b314
 */
void StartControllerRead(void);

/**
 * Wait for a read of the controllers, starting one if none is pending, and fill the pads.
 *
 * @ghidraAddress USA: 0x8021a964
 * @ghidraAddress EUR: 0x8021ba7c
 * @ghidraAddress JPN: 0x8021b358
 */
void ReadControllers(void);

/**
 * Read the controllers and turn their state into the input of the players.
 *
 * @ghidraAddress USA: 0x8021a9b4
 * @ghidraAddress EUR: 0x8021bacc
 * @ghidraAddress JPN: 0x8021b3a8
 */
void UpdateControllerState(void);

/**
 * Probe the controllers and the Controller and Rumble Paks, running the controller check until each
 * warning is dismissed and the Controller Pak menu while the first player holds start.
 *
 * @ghidraAddress USA: 0x8021c188
 * @ghidraAddress EUR: 0x8021d37c
 * @ghidraAddress JPN: 0x8021cc4c
 */
void InitControllersAndPaks(void);

#endif

#ifndef COASTLINE_PLAYERINPUT_H
#define COASTLINE_PLAYERINPUT_H

#include "types.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(errno)
/* The C library macro would replace controller error fields. */
#pragma push_macro("errno")
#undef errno
#define COASTLINE_RESTORE_ERRNO_MACRO
#endif
#include <PR/os_cont.h>
#ifdef COASTLINE_RESTORE_ERRNO_MACRO
#pragma pop_macro("errno")
#undef COASTLINE_RESTORE_ERRNO_MACRO
#endif

/** Number of input slots. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define MAX_PLAYERS 16
#else
#define MAX_PLAYERS 4
#endif

/** The button changes a player's input remembers, for the cheat codes. */
#define INPUT_HISTORY_SIZE 128

/** The recordings of the input a player can make at once. */
#define INPUT_RECORDERS 2

/** The input of one player. */
typedef struct PlayerInputState {
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
    u8 controllerLayout;   /*!< The button layout of the race. */
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
    /**
     * @ghidraAddress BRD3D: 0x10035ce0
     */
    void TranslateControllerInput(void);

    /** @ghidraAddress BRD3D: 0x10035fa0 */
    void TransferFlags(u32 mask);

    /**
     * Remove held buttons from current input and discard released held buttons.
     *
     * @ghidraAddress BRD3D: 0x10035fc0
     * @ghidraAddress BRGlide: 0x1002f660
     */
    void ConsumeInputFlags(void);

    /**
     * Reset recording pointers and bind the player index and controller pad.
     *
     * @ghidraAddress BRD3D: 0x10035fe0
     */
    void InitPlayerInputState(void);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus) */
} PlayerInputState;

#ifdef __cplusplus
extern "C" {
#endif

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
 * Move buttons out of the input until they are released, for a single action per press.
 *
 * @param state The input of the player.
 * @param mask The buttons to move.
 * @ghidraAddress USA: 0x80255910
 * @ghidraAddress EUR: 0x80256ee0
 * @ghidraAddress JPN: 0x80256870
 */
void TransferFlags(PlayerInputState *state, u32 mask);

/**
 * Input state of every player.
 *
 * @ghidraAddress USA: 0x8036a8e0
 * @ghidraAddress EUR: 0x8036cde0
 * @ghidraAddress JPN: 0x8036ba50
 * @ghidraAddress BRD3D: 0x106c6678
 */
extern PlayerInputState g_aPlayerInputState[MAX_PLAYERS];

/**
 * Emulated controller pad storage for four ports.
 *
 * @ghidraAddress USA: 0x8031a3e0
 * @ghidraAddress EUR: 0x8031c8e0
 * @ghidraAddress JPN: 0x8031b510
 * @ghidraAddress BRD3D: 0x106c65a0
 */
extern OSContPad g_aContPadData[MAXCONTROLLERS];

/**
 * Reset the input of a player.
 *
 * @param state The input to reset.
 * @ghidraAddress USA: 0x80255b54
 * @ghidraAddress EUR: 0x80257124
 * @ghidraAddress JPN: 0x80256ab4
 */
void InitPlayerInputState(PlayerInputState *state);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLAYERINPUT_H */

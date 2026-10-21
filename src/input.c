#include "input.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "options.h"
#include "pakmenu.h"
#include "race.h"
#include "rumble.h"
#include "splash.h"
#include "system.h"
#include "vehicle.h"

/* The stick reading that counts as full deflection. */
#define STICK_RANGE 70.0f

/* The steering the directional pad gives in the third layout. */
#define DPAD_STEERING 0x50

/* How far down the stick must be held with the accelerator to reverse. */
#define REVERSE_STICK_Y (-0x40)

/* The stick positions of the fourth layout that accelerate and that reverse. */
#define ACCELERATE_STICK_Y 0x21
#define LAYOUT3_REVERSE_STICK_Y (-0x10)

/* The stick repeat: the axis to press, the axis to keep, and the delays in milliseconds. */
#define STICK_PRESS 0.6f
#define STICK_RELEASE 0.3f
#define STICK_FIRST_DELAY 500
#define STICK_REPEAT_DELAY 200

/* The end of a cheat sequence. */
#define CHEAT_SEQUENCE_END 0xFFFF

/* The unlocks of the profile of the first player that the cheats set. */
#define UNLOCKED_TRACKS_0_TO_4 0x001F
#define UNLOCKED_TRACKS_5_TO_9 0x03E0
#define UNLOCKED_CARS_0_TO_8 0x01FF

void CheatToggleRamView(void) {
    g_bShowDepthBuffer = !g_bShowDepthBuffer;
}

void CheatUnlockTracks0To4(void) {
    g_aVehicles[0].profile->unlockedTracks |= UNLOCKED_TRACKS_0_TO_4;
}

void CheatUnlockCars0To8(void) {
    g_aVehicles[0].profile->unlocks |= UNLOCKED_CARS_0_TO_8;
}

void CheatSetUnlockBit200(void) {
    g_aVehicles[0].profile->unlocks |= 0x0200;
}

void CheatSetUnlockBit1000(void) {
    g_aVehicles[0].profile->unlocks |= 0x1000;
}

void CheatSetUnlockBit800(void) {
    g_aVehicles[0].profile->unlocks |= 0x0800;
}

void CheatSetUnlockBit400(void) {
    g_aVehicles[0].profile->unlocks |= 0x0400;
}

void CheatUnlockTracks5To9(void) {
    g_aVehicles[0].profile->unlockedTracks |= UNLOCKED_TRACKS_5_TO_9;
}

void CheatSetUnlockBit8000(void) {
    g_aVehicles[0].profile->unlocks |= 0x8000;
}

void CheatToggleMultiplayerMode(void) {
    g_dwMultiplayerMode = !g_dwMultiplayerMode;
}

void CheatToggleBilinearFilter(void) {
    g_bBilinearFiltering = !g_bBilinearFiltering;
}

void ProcessInputCommandBuffer(PlayerInputState *state) {
    u16 buttons = (u16)state->buttons;
    u16 *code;
    u16 *history;
    u16 *sequence;
    s32 i;
    s32 j;

    if (buttons != state->history[state->historyIndex]) {
        state->historyIndex = (state->historyIndex - 1) & (INPUT_HISTORY_SIZE - 1);
        state->history[state->historyIndex] = buttons;

        for (j = 0; (sequence = g_aCheatCodes[j].sequence) != NULL; j++) {
            i = state->historyIndex;
            history = state->history;
            for (code = sequence; *code != CHEAT_SEQUENCE_END;
                 code++, i = (i + 1) & (INPUT_HISTORY_SIZE - 1)) {
                if (*code != history[i]) {
                    goto next;
                }
            }
            g_aCheatCodes[j].apply();
        next:;
        }
    }
}

/* Add the brake, which becomes the e-brake while the car accelerates. */
#define ADD_BRAKE_ACTION(state)                                                                    \
    if ((state)->buttons & INPUT_ACCELERATE) {                                                     \
        (state)->buttons |= INPUT_EBRAKE;                                                          \
    } else {                                                                                       \
        (state)->buttons |= INPUT_BRAKE;                                                           \
    }

void TranslateControllerInput(PlayerInputState *state) {
    OSContPad *pad = state->pad;
    u32 pressed;
    s32 i;

    if (pad->errno != 0) {
        state->controllerMissing = pad->errno == CONT_NO_RESPONSE_ERROR;
        pad->stick_x = 0;
        state->pad->stick_y = 0;
        state->pad->button = 0;
    } else {
        state->controllerMissing = FALSE;
    }

    pressed = state->pad->button;
    state->buttons = 0;
    if (pressed & U_JPAD) {
        state->buttons = INPUT_UP;
    }
    if (pressed & D_JPAD) {
        state->buttons |= INPUT_DOWN;
    }
    if (pressed & L_JPAD) {
        state->buttons |= INPUT_LEFT;
    }
    if (pressed & R_JPAD) {
        state->buttons |= INPUT_RIGHT;
    }
    if (pressed & A_BUTTON) {
        state->buttons |= INPUT_A;
    }
    if (pressed & B_BUTTON) {
        state->buttons |= INPUT_B;
    }
    if (pressed & L_TRIG) {
        state->buttons |= INPUT_L;
    }
    if (pressed & R_TRIG) {
        state->buttons |= INPUT_R;
    }
    if (pressed & Z_TRIG) {
        state->buttons |= INPUT_Z;
    }
    if (pressed & START_BUTTON) {
        state->buttons |= INPUT_START;
    }
    if (pressed & U_CBUTTONS) {
        state->buttons |= INPUT_C_UP;
    }
    if (pressed & R_CBUTTONS) {
        state->buttons |= INPUT_C_RIGHT;
    }
    if (pressed & D_CBUTTONS) {
        state->buttons |= INPUT_C_DOWN;
    }
    if (pressed & L_CBUTTONS) {
        state->buttons |= INPUT_C_LEFT;
    }

    /* Map the buttons to the actions of a race by the layout of the player. */
    if (IsCurrentScreen(ProcessRacingScreen)) {
        switch (state->controllerLayout) {
        case 4:
            if (state->buttons & INPUT_L) {
                state->buttons |= INPUT_GEAR_DOWN;
            }
            if (state->buttons & INPUT_R) {
                state->buttons |= INPUT_GEAR_UP;
            }
            /* The fifth layout differs from the first only in its gear down button. */
            goto firstLayout;
        case 0:
            if (state->buttons & INPUT_Z) {
                state->buttons |= INPUT_GEAR_DOWN;
            }
            if (state->buttons & INPUT_R) {
                state->buttons |= INPUT_GEAR_UP;
            }
        firstLayout:
            state->steeringInput = state->pad->stick_x;
            if (state->buttons & INPUT_A) {
                if (state->pad->stick_y < REVERSE_STICK_Y) {
                    state->buttons |= INPUT_REVERSE;
                }
                state->buttons |= INPUT_ACCELERATE;
            }
            if (state->buttons & INPUT_B) {
                ADD_BRAKE_ACTION(state);
            }
            if (state->buttons & INPUT_C_UP) {
                state->buttons |= INPUT_VIEW_COCKPIT;
            }
            if (state->buttons & INPUT_C_DOWN) {
                state->buttons |= INPUT_VIEW_CHASE;
            }
            if (state->buttons & INPUT_C_LEFT) {
                state->buttons |= INPUT_VIEW_BUMPER;
            }
            break;
        case 1:
            if (state->buttons & INPUT_Z) {
                state->buttons |= INPUT_GEAR_DOWN;
            }
            if (state->buttons & INPUT_R) {
                state->buttons |= INPUT_GEAR_UP;
            }
            state->steeringInput = state->pad->stick_x;
            if (state->buttons & INPUT_A) {
                if (state->pad->stick_y < REVERSE_STICK_Y) {
                    state->buttons |= INPUT_REVERSE;
                }
                state->buttons |= INPUT_ACCELERATE;
            }
            if (state->buttons & INPUT_B) {
                state->buttons |= INPUT_BRAKE;
            }
            if (state->buttons & INPUT_C_DOWN) {
                state->buttons |= INPUT_EBRAKE;
            }
            if (state->buttons & INPUT_C_UP) {
                state->buttons |= INPUT_VIEW_COCKPIT;
            }
            if (state->buttons & INPUT_C_RIGHT) {
                state->buttons |= INPUT_VIEW_CHASE;
            }
            if (state->buttons & INPUT_C_LEFT) {
                state->buttons |= INPUT_VIEW_BUMPER;
            }
            break;
        case 2:
            /* The directional pad steers, and both of its sides cancel out. */
            if (state->buttons & INPUT_LEFT) {
                if (state->buttons & INPUT_RIGHT) {
                    state->steeringInput = 0;
                } else {
                    state->steeringInput = -DPAD_STEERING;
                }
            } else if (state->buttons & INPUT_RIGHT) {
                state->steeringInput = DPAD_STEERING;
            } else {
                state->steeringInput = 0;
            }
            if (state->buttons & INPUT_A) {
                if (state->buttons & INPUT_DOWN) {
                    state->buttons |= INPUT_REVERSE;
                }
                state->buttons |= INPUT_ACCELERATE;
            }
            if (state->buttons & INPUT_B) {
                ADD_BRAKE_ACTION(state);
            }
            if (state->buttons & INPUT_L) {
                state->buttons |= INPUT_GEAR_DOWN;
            }
            if (state->buttons & INPUT_R) {
                state->buttons |= INPUT_GEAR_UP;
            }
            if (state->buttons & INPUT_C_UP) {
                state->buttons |= INPUT_VIEW_COCKPIT;
            }
            if (state->buttons & INPUT_C_DOWN) {
                state->buttons |= INPUT_VIEW_CHASE;
            }
            if (state->buttons & INPUT_C_LEFT) {
                state->buttons |= INPUT_VIEW_BUMPER;
            }
            break;
        case 3:
            /* The stick accelerates forward and reverses back. */
            if (state->pad->stick_y >= ACCELERATE_STICK_Y) {
                state->buttons |= INPUT_ACCELERATE;
            } else if (state->pad->stick_y < LAYOUT3_REVERSE_STICK_Y) {
                state->buttons |= INPUT_ACCELERATE | INPUT_REVERSE;
            }
            state->steeringInput = state->pad->stick_x;
            if (state->buttons & INPUT_Z) {
                ADD_BRAKE_ACTION(state);
            }
            if (state->buttons & (INPUT_A | INPUT_DOWN)) {
                state->buttons |= INPUT_GEAR_DOWN;
            }
            if (state->buttons & (INPUT_B | INPUT_UP)) {
                state->buttons |= INPUT_GEAR_UP;
            }
            if (state->buttons & INPUT_C_UP) {
                state->buttons |= INPUT_VIEW_COCKPIT;
            }
            if (state->buttons & INPUT_C_DOWN) {
                state->buttons |= INPUT_VIEW_CHASE;
            }
            if (state->buttons & INPUT_C_LEFT) {
                state->buttons |= INPUT_VIEW_BUMPER;
            }
            break;
        }
    }

    /* Record the steering and the actions, or replay a recording in their place. A pause holds
     * the position of each. */
    if (state->recordBuffers[0] != NULL || state->recordBuffers[1] != NULL) {
        for (i = 0; i < INPUT_RECORDERS; i++) {
            if (state->recordBuffers[i] != NULL &&
                state->recordPositions[i] < state->recordLimits[i]) {
                state->recordBuffers[i][state->recordPositions[i]] = state->steeringInput;
                state->recordBuffers[i][state->recordPositions[i] + 1] =
                    (state->buttons & INPUT_RECORDED_ACTIONS) >> 16;
                if (!g_dwRacePauseState) {
                    state->recordPositions[i] += 2;
                }
            }
        }
    } else if (state->playbackBuffer != NULL) {
        if (state->playbackPosition < state->playbackLimit) {
            state->steeringInput = state->playbackBuffer[state->playbackPosition];
            state->buttons = (state->buttons & ~INPUT_RECORDED_ACTIONS) |
                             (state->playbackBuffer[state->playbackPosition + 1] << 16);
            if (!g_dwRacePauseState) {
                state->playbackPosition += 2;
            }
        } else {
            state->steeringInput = 0;
            state->buttons &= ~INPUT_RECORDED_ACTIONS;
        }
    }

    state->stickX = state->pad->stick_x / STICK_RANGE;
    state->steering = state->steeringInput / STICK_RANGE;
    state->stickY = state->pad->stick_y / STICK_RANGE;
    if (state->stickX > 1.0f) {
        state->stickX = 1.0f;
    } else if (state->stickX < -1.0f) {
        state->stickX = -1.0f;
    }
    if (state->stickY > 1.0f) {
        state->stickY = 1.0f;
    } else if (state->stickY < -1.0f) {
        state->stickY = -1.0f;
    }
    if (state->steering > 1.0f) {
        state->steering = 1.0f;
    } else if (state->steering < -1.0f) {
        state->steering = -1.0f;
    }

    ProcessInputCommandBuffer(state);
}

void TransferFlags(PlayerInputState *state, u32 mask) {
    state->heldButtons |= state->buttons & mask;
    state->buttons &= ~mask;
}

void ConsumeInputFlags(PlayerInputState *state) {
    u32 buttons = state->buttons;

    state->buttons ^= buttons & state->heldButtons;
    state->heldButtons &= buttons;
}

void CheckAnalogStickPositive(PlayerInputState *state, s32 *timer, f32 *axis, u32 button) {
    s32 time = *timer;

    if (time != 0) {
        if (*axis < STICK_RELEASE) {
            *timer = 0;
        } else if (time > 0) {
            /* A positive time counts down to a press, and a negative one counts up to it. */
            *timer = time - g_dwFrameDeltaMs;
            if (*timer <= 0) {
                state->buttons |= button;
                *timer = -STICK_REPEAT_DELAY;
            }
        } else {
            *timer = time + g_dwFrameDeltaMs;
            if (*timer >= 0) {
                state->buttons |= button;
                *timer = -STICK_REPEAT_DELAY;
            }
        }
    } else if (*axis > STICK_PRESS) {
        state->buttons |= button;
        *timer = STICK_FIRST_DELAY;
    }
}

void CheckAnalogStickNegative(PlayerInputState *state, s32 *timer, f32 *axis, u32 button) {
    s32 time = *timer;

    if (time != 0) {
        if (*axis > -STICK_RELEASE) {
            *timer = 0;
        } else if (time > 0) {
            *timer = time - g_dwFrameDeltaMs;
            if (*timer <= 0) {
                state->buttons |= button;
                *timer = -STICK_REPEAT_DELAY;
            }
        } else {
            *timer = time + g_dwFrameDeltaMs;
            if (*timer >= 0) {
                state->buttons |= button;
                *timer = -STICK_REPEAT_DELAY;
            }
        }
    } else if (*axis < -STICK_PRESS) {
        state->buttons |= button;
        *timer = STICK_FIRST_DELAY;
    }
}

void UpdateStickRepeat(PlayerInputState *state) {
    CheckAnalogStickPositive(state, &state->repeatUp, &state->stickY, INPUT_UP);
    CheckAnalogStickNegative(state, &state->repeatDown, &state->stickY, INPUT_DOWN);
    CheckAnalogStickNegative(state, &state->repeatLeft, &state->stickX, INPUT_LEFT);
    CheckAnalogStickPositive(state, &state->repeatRight, &state->stickX, INPUT_RIGHT);
}

void InitPlayerInputState(PlayerInputState *state) {
    s32 player = state - g_aPlayerInputState;

    state->recordBuffers[1] = NULL;
    state->recordBuffers[0] = NULL;
    state->playbackBuffer = NULL;
    state->playerIndex = player;
    state->pad = &g_aContPadData[player];
}

void StartControllerRead(void) {
    if (!g_dwContReadPending) {
        g_dwContReadPending = TRUE;
        g_wSerialInterfaceIdle = FALSE;
        osContStartReadData(&g_SiMesgQueue);
    }
}

void ReadControllers(void) {
    StartControllerRead();
    osRecvMesg(&g_SiMesgQueue, NULL, OS_MESG_BLOCK);
    osContGetReadData(g_aContPadData);
    g_wSerialInterfaceIdle = TRUE;
    g_dwContReadPending = FALSE;
}

void UpdateControllerState(void) {
    s32 i;

    ReadControllers();
    for (i = 0; i < MAX_PLAYERS; i++) {
        TranslateControllerInput(&g_aPlayerInputState[i]);
        ConsumeInputFlags(&g_aPlayerInputState[i]);
    }
}

/* Run the controller check with a warning until the player dismisses it. */
static void WarnAboutController(u8 warning) {
    SetCurrentScreenCallback(ProcessControllerCheck);
    g_bControllerWarningType = warning;
    while (g_pfnCurrentScreen == ProcessControllerCheck) {
        ProcessControllerCheck();
    }
}

void InitControllersAndPaks(void) {
    u8 plugged;

    UpdateControllerState();
    g_wSerialInterfaceIdle = FALSE;
    if (g_aPlayerInputState[0].controllerMissing) {
        WarnAboutController(CONTROLLER_WARNING_NO_CONTROLLER);
    }

    osPfsIsPlug(&g_SiMesgQueue, &plugged);
    if (!(plugged & (1 << 0))) {
        WarnAboutController(CONTROLLER_WARNING_NO_PAK);
    } else {
        if (!g_bPakPort0Initialized) {
            g_bPakPort0Initialized = TRUE;
            osSyncPrintf("\nInitializing controller pak...\n");
            gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
        }
        if (gControllerPakState != 0) {
            if (gControllerPakState == PFS_ERR_ID_FATAL) {
                /* The result is discarded: the warning follows whether a Rumble Pak answers. */
                osMotorInit(&g_SiMesgQueue, &g_aContPfs[0], 0);
            }
            WarnAboutController(CONTROLLER_WARNING_NO_PAK);
        } else {
            memcpy(g_aabSavePakIds[0], g_aPakPfs[0].id, sizeof(g_aPakPfs[0].id));
            g_abSavePakIdKnown[0] = TRUE;
        }
    }

    /* The Pak of the second controller gets no warning. */
    if (plugged & (1 << 1)) {
        gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[1], 1);
        if (gControllerPakState == 0) {
            memcpy(g_aabSavePakIds[1], g_aPakPfs[1].id, sizeof(g_aPakPfs[1].id));
            g_abSavePakIdKnown[1] = TRUE;
        } else if (gControllerPakState == PFS_ERR_ID_FATAL) {
            osMotorInit(&g_SiMesgQueue, &g_aContPfs[1], 1);
        }
    }

    if (g_aPlayerInputState[0].buttons & INPUT_START) {
        TransferFlags(&g_aPlayerInputState[0], INPUT_START);
        SetCurrentScreenCallback(ProcessPakMenuScreen);
        while (g_pfnCurrentScreen == ProcessPakMenuScreen) {
            ProcessPakMenuScreen();
        }
    }
    if (!InitPakAndEnumerateFiles()) {
        WarnAboutController(CONTROLLER_WARNING_FOREIGN_PAK);
    }
    g_wSerialInterfaceIdle = TRUE;
}

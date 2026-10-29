#include "common/input.h"
#include "common/pause.h"
#include "common/race.h"
#include "common/screen.h"

#if TGR_PC
#include "win32/boot/rallyconfig.h"
#ifdef errno
#undef errno
#endif
#endif

/* The stick reading that counts as full deflection. */
#define STICK_RANGE 70.0f

/* The steering the directional pad supplies. */
#define DPAD_STEERING 0x50

/* How far down the stick must be held with the accelerator to reverse. */
#define REVERSE_STICK_Y (-0x40)

/* The stick positions of the fourth layout that accelerate and that reverse. */
#define ACCELERATE_STICK_Y 0x21
#define LAYOUT3_REVERSE_STICK_Y (-0x10)

/* Add the brake, which becomes the e-brake while the car accelerates. */
#define ADD_BRAKE_ACTION(state)                                                                    \
    if ((state)->buttons & INPUT_ACCELERATE) {                                                     \
        (state)->buttons |= INPUT_EBRAKE;                                                          \
    } else {                                                                                       \
        (state)->buttons |= INPUT_BRAKE;                                                           \
    }

#if TGR_PC
void PlayerInputState::TranslateControllerInput(void) {
    PlayerInputState *state = this;
#else
void TranslateControllerInput(PlayerInputState *state) {
#endif
    OSContPad *pad = state->pad;
    u32 pressed;
    s32 i;

    if (pad->CONT_ERRNO != 0) {
        state->controllerMissing = pad->CONT_ERRNO == CONT_NO_RESPONSE_ERROR;
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

#if TGR_PC
    if (IsCurrentScreen(RunRaceGameScreen)) {
        if (state->buttons & INPUT_L) {
            state->buttons |= INPUT_GEAR_DOWN;
        }
        if (state->buttons & INPUT_R) {
            state->buttons |= INPUT_GEAR_UP;
        }
        if ((g_GlobalRallyConfig.pActiveInputBindings[0].wControl & 0x8000) ||
            (g_GlobalRallyConfig.pActiveInputBindings[1].wControl & 0x8000)) {
            state->steeringInput = state->pad->stick_x;
        } else if ((state->buttons & INPUT_LEFT) && !(state->buttons & INPUT_RIGHT)) {
            state->steeringInput = -DPAD_STEERING;
        } else if ((state->buttons & INPUT_RIGHT) && !(state->buttons & INPUT_LEFT)) {
            state->steeringInput = DPAD_STEERING;
        } else {
            state->steeringInput = 0;
        }
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
#else
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
#endif /* TGR_PC */
    }

    if (state->recordBuffers[0] != NULL || state->recordBuffers[1] != NULL) {
        for (i = 0; i < INPUT_RECORDERS; i++) {
            if (state->recordBuffers[i] != NULL &&
                state->recordPositions[i] < state->recordLimits[i]) {
#if !TGR_PC
                state->recordBuffers[i][state->recordPositions[i]] = state->steeringInput;
                state->recordBuffers[i][state->recordPositions[i] + 1] =
                    (state->buttons & INPUT_RECORDED_ACTIONS) >> 16;
#endif
                if (!g_dwRacePauseState) {
#if TGR_PC
                    state->recordPositions[i] = (s32)((u32)state->recordPositions[i] + 2);
#else
                    state->recordPositions[i] += 2;
#endif
                }
            }
        }
    } else if (state->playbackBuffer != NULL) {
#if TGR_PC
        /* The playback buffer is read and nothing is played back. */
#else
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
#endif /* TGR_PC */
    }

#if TGR_PC
    state->stickX = state->pad->stick_x * (1.0f / STICK_RANGE);
    state->steering = state->steeringInput * (1.0f / STICK_RANGE);
    state->stickY = state->pad->stick_y * (1.0f / STICK_RANGE);
#else
    state->stickX = state->pad->stick_x / STICK_RANGE;
    state->steering = state->steeringInput / STICK_RANGE;
    state->stickY = state->pad->stick_y / STICK_RANGE;
#endif
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

#if !TGR_PC
    ProcessInputCommandBuffer(state);
#endif
}

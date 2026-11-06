#include "common/input.h"
#include "common/pause.h"
#include "common/race.h"
#include "common/screen.h"

void TranslateControllerInput(PlayerInputState *state) {
    unsigned int buttons;
    int recorderIndex;

    if ((buttons = state->pad->CONT_ERRNO) != 0) {
        state->controllerMissing = buttons == 8;
        state->pad->stick_x = 0;
        state->pad->stick_y = 0;
        state->pad->button = 0;
    } else {
        state->controllerMissing = 0;
    }
    buttons = state->pad->button;
    state->buttons = 0;
    if (buttons & 0x800) {
        state->buttons |= 8;
    }
    if (buttons & 0x400) {
        state->buttons |= 2;
    }
    if (buttons & 0x200) {
        state->buttons |= 4;
    }
    if (buttons & 0x100) {
        state->buttons |= 1;
    }
    if (buttons & 0x8000) {
        state->buttons |= 0x10;
    }
    if (buttons & 0x4000) {
        state->buttons |= 0x20;
    }
    if (buttons & 0x20) {
        state->buttons |= 0x1000;
    }
    if (buttons & 0x10) {
        state->buttons |= 0x2000;
    }
    if (buttons & 0x2000) {
        state->buttons |= 0x8000;
    }
    if (buttons & 0x1000) {
        state->buttons |= 0x4000;
    }
    if (buttons & 8) {
        state->buttons |= 0x100;
    }
    if (buttons & 1) {
        state->buttons |= 0x200;
    }
    if (buttons & 4) {
        state->buttons |= 0x400;
    }
    if (buttons & 2) {
        state->buttons |= 0x800;
    }
    if (IsCurrentScreen(ProcessRacingScreen)) {
        switch (state->controllerLayout) {
        case 4:
            if (state->buttons & 0x1000) {
                state->buttons |= 0x200000;
            }
            if (state->buttons & 0x2000) {
                state->buttons |= 0x100000;
            }
            goto stick;
        case 0:
            if (state->buttons & 0x8000) {
                state->buttons |= 0x200000;
            }
            if (state->buttons & 0x2000) {
                state->buttons |= 0x100000;
            }
        stick:
            state->steeringInput = state->pad->stick_x;
            if (state->buttons & 0x10) {
                if (state->pad->stick_y < -0x40) {
                    state->buttons |= 0x20000;
                }
                state->buttons |= 0x10000;
            }
            if (state->buttons & 0x20) {
                if (state->buttons & 0x10000) {
                    state->buttons |= 0x80000;
                } else {
                    state->buttons |= 0x40000;
                }
            }
            if (state->buttons & 0x100) {
                state->buttons |= 0x1000000;
            }
            if (state->buttons & 0x400) {
                state->buttons |= 0x4000000;
            }
            if (state->buttons & 0x800) {
                state->buttons |= 0x8000000;
            }
            break;
        case 1:
            if (state->buttons & 0x8000) {
                state->buttons |= 0x200000;
            }
            if (state->buttons & 0x2000) {
                state->buttons |= 0x100000;
            }
            state->steeringInput = state->pad->stick_x;
            if (state->buttons & 0x10) {
                if (state->pad->stick_y < -0x40) {
                    state->buttons |= 0x20000;
                }
                state->buttons |= 0x10000;
            }
            if (state->buttons & 0x20) {
                state->buttons |= 0x40000;
            }
            if (state->buttons & 0x400) {
                state->buttons |= 0x80000;
            }
            if (state->buttons & 0x100) {
                state->buttons |= 0x1000000;
            }
            if (state->buttons & 0x200) {
                state->buttons |= 0x4000000;
            }
            if (state->buttons & 0x800) {
                state->buttons |= 0x8000000;
            }
            break;
        case 2:
            if (state->buttons & 4) {
                if (!(state->buttons & 1)) {
                    state->steeringInput = -0x50;
                } else {
                    state->steeringInput = 0;
                }
            } else if (state->buttons & 1) {
                state->steeringInput = 0x50;
            } else {
                state->steeringInput = 0;
            }
            if (state->buttons & 0x10) {
                if (state->buttons & 2) {
                    state->buttons |= 0x20000;
                }
                state->buttons |= 0x10000;
            }
            if (state->buttons & 0x20) {
                if (state->buttons & 0x10000) {
                    state->buttons |= 0x80000;
                } else {
                    state->buttons |= 0x40000;
                }
            }
            if (state->buttons & 0x1000) {
                state->buttons |= 0x200000;
            }
            if (state->buttons & 0x2000) {
                state->buttons |= 0x100000;
            }
            if (state->buttons & 0x100) {
                state->buttons |= 0x1000000;
            }
            if (state->buttons & 0x400) {
                state->buttons |= 0x4000000;
            }
            if (state->buttons & 0x800) {
                state->buttons |= 0x8000000;
            }
            break;
        case 3:
            if (state->pad->stick_y > 0x20) {
                state->buttons |= 0x10000;
            } else if (state->pad->stick_y < -0x10) {
                state->buttons |= 0x30000;
            }
            state->steeringInput = state->pad->stick_x;
            if (state->buttons & 0x8000) {
                if (state->buttons & 0x10000) {
                    state->buttons |= 0x80000;
                } else {
                    state->buttons |= 0x40000;
                }
            }
            if (state->buttons & 0x12) {
                state->buttons |= 0x200000;
            }
            if (state->buttons & 0x28) {
                state->buttons |= 0x100000;
            }
            if (state->buttons & 0x100) {
                state->buttons |= 0x1000000;
            }
            if (state->buttons & 0x400) {
                state->buttons |= 0x4000000;
            }
            if (state->buttons & 0x800) {
                state->buttons |= 0x8000000;
            }
            break;
        }
    }
    if (state->recordBuffers[0] != 0 || state->recordBuffers[1] != 0) {
        for (recorderIndex = 0; recorderIndex < 2; recorderIndex++) {
            if (state->recordBuffers[recorderIndex] != 0 &&
                state->recordPositions[recorderIndex] < state->recordLimits[recorderIndex]) {
                state->recordBuffers[recorderIndex][state->recordPositions[recorderIndex]] =
                    state->steeringInput;
                state->recordBuffers[recorderIndex][state->recordPositions[recorderIndex] + 1] =
                    (state->buttons & 0x3f0000) >> 16;
                if (g_dwRacePauseState == 0) {
                    state->recordPositions[recorderIndex] += 2;
                }
            }
        }
    } else if (state->playbackBuffer != 0) {
        if (state->playbackPosition < state->playbackLimit) {
            state->steeringInput = state->playbackBuffer[state->playbackPosition];
            state->buttons = (state->playbackBuffer[state->playbackPosition + 1] << 16) |
                             (state->buttons & 0xffc0ffff);
            if (g_dwRacePauseState == 0) {
                state->playbackPosition += 2;
            }
        } else {
            state->steeringInput = 0;
            state->buttons &= 0xffc0ffff;
        }
    }
    state->stickX = (float)state->pad->stick_x / 70.0f;
    state->stickY = (float)state->pad->stick_y / 70.0f;
    state->steering = (float)state->steeringInput / 70.0f;
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

#include "common/input.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/race.h"
#include "common/rumble.h"
#include "common/splash.h"
#include "common/system.h"
#include "common/vehicle.h"
#include "options.h"
#include "pakmenu.h"

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

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void CheatFinishRace(void) {
    g_dwCheatFinishRacePending = TRUE;
    osSyncPrintf("FINISH!");
}
#endif

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
    u16 *history;
    u16 *sequence;
    s32 i;
    s32 j;
    s32 k;

    if (buttons != state->history[state->historyIndex]) {
        state->historyIndex = (state->historyIndex - 1) & (INPUT_HISTORY_SIZE - 1);
        state->history[state->historyIndex] = buttons;

        for (j = 0; (sequence = g_aCheatCodes[j].sequence) != NULL; j++) {
            i = state->historyIndex;
            history = state->history;
            for (k = 0; sequence[k] != CHEAT_SEQUENCE_END;
                 k++, i = (i + 1) & (INPUT_HISTORY_SIZE - 1)) {
                if (sequence[k] != history[i]) {
                    goto next;
                }
            }
            g_aCheatCodes[j].apply();
        next:;
        }
    }
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

/* Run the controller check with a warning until the player dismisses it. */

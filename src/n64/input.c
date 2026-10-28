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
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    /* The game saves to files, and the keyboard stands in for a missing controller. */
    g_wSerialInterfaceIdle = TRUE;
    return;
#endif
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

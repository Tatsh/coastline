#include "n64/peripheralinput.h"

#include <PR/os.h>

#include "common/input.h"
#include "common/race.h"
#include "common/rumble.h"
#include "common/screen.h"
#include "common/system.h"
#include "n64/libultra/io/controller.h"
#include "n64/libultra/io/siint.h"
#include "n64/paintinput.h"
#include "n64/paintshop.h"

#define RANDNET_KEYBOARD_TYPE 0x0200
#define RANDNET_READ_KEYS 0x13
#define RANDNET_POWER_LED 0x04
#define RANDNET_KEY_LEFT 0x0205
#define RANDNET_KEY_RIGHT 0x0405
#define RANDNET_KEY_UP 0x0204
#define RANDNET_KEY_DOWN 0x0305
#define RANDNET_KEY_ENTER 0x0D04
#define RANDNET_KEY_ESCAPE 0x0A08
#define RANDNET_KEY_SPACE 0x0602
#define RANDNET_KEY_A 0x0D07
#define RANDNET_KEY_F1 0x0B01
#define RANDNET_KEY_F2 0x0A01
#define RANDNET_KEY_F3 0x0B08
#define RANDNET_KEY_F4 0x0A07

/* Nonzero initial values keep the patch's key history out of the ROM's BSS. */
static u16 s_previousKeys[MAXCONTROLLERS][3] = {
    {0xFFFF, 0xFFFF, 0xFFFF},
    {0xFFFF, 0xFFFF, 0xFFFF},
    {0xFFFF, 0xFFFF, 0xFFFF},
    {0xFFFF, 0xFFFF, 0xFFFF},
};
static s32 s_statusPollFrames = 1;

s32 IsRandnetShiftDown(void) {
    s32 port;
    s32 i;

    for (port = 0; port < MAXCONTROLLERS; port++) {
        for (i = 0; i < 3; i++) {
            if (s_previousKeys[port][i] == RANDNET_KEY_LEFT_SHIFT ||
                s_previousKeys[port][i] == RANDNET_KEY_RIGHT_SHIFT) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void RefreshPeripheralStatus(void) {
    u64 packet[8];
    u8 *bytes = (u8 *)packet;
    s32 port;
    s32 i;

    for (i = 0; i < 8; i++) {
        packet[i] = 0;
    }
    packet[7] = 1;
    for (port = 0; port < MAXCONTROLLERS; port++) {
        i = port * 8;
        bytes[i] = CONT_CMD_NOP;
        bytes[i + 1] = CONT_CMD_REQUEST_STATUS_TX;
        bytes[i + 2] = CONT_CMD_REQUEST_STATUS_RX;
        bytes[i + 3] = CONT_CMD_REQUEST_STATUS;
        bytes[i + 4] = CONT_CMD_NOP;
        bytes[i + 5] = CONT_CMD_NOP;
        bytes[i + 6] = CONT_CMD_NOP;
        bytes[i + 7] = CONT_CMD_NOP;
    }
    bytes[MAXCONTROLLERS * 8] = CONT_CMD_END;

    g_wSerialInterfaceIdle = FALSE;
    __osSiGetAccess();
    if (__osSiRawStartDma(OS_WRITE, packet) == 0) {
        osRecvMesg(&g_SiMesgQueue, NULL, OS_MESG_BLOCK);
        if (__osSiRawStartDma(OS_READ, packet) == 0) {
            osRecvMesg(&g_SiMesgQueue, NULL, OS_MESG_BLOCK);
            for (port = 0; port < MAXCONTROLLERS; port++) {
                i = port * 8;
                g_aContStatus[port].CONT_ERRNO = (bytes[i + 2] & 0xC0) >> 4;
                if (g_aContStatus[port].CONT_ERRNO == 0) {
                    g_aContStatus[port].type = ((u16)bytes[i + 5] << 8) | bytes[i + 4];
                    g_aContStatus[port].status = bytes[i + 6];
                } else {
                    g_aContStatus[port].type = 0;
                }
            }
        }
    }
    __osContLastCmd = CONT_CMD_NOP;
    __osSiRelAccess();
    g_wSerialInterfaceIdle = TRUE;
}

void UpdateN64PeripheralInputForPlayer(s32 player) {
    if (player == 0 && --s_statusPollFrames <= 0) {
        RefreshPeripheralStatus();
        s_statusPollFrames = 16;
    }
    if (IsCurrentScreen(ProcessPaintShopScreen)) {
        if (player == g_bPaintShopPlayer) {
            UpdatePaintPeripheralInput(&g_aPlayerInputState[player]);
        }
    } else {
        if (g_aContStatus[player].CONT_ERRNO == 0 && g_aContStatus[player].type == CONT_RELATIVE) {
            g_aPlayerInputState[player].buttons = 0;
            g_aPlayerInputState[player].steeringInput = 0;
            g_aPlayerInputState[player].stickX = 0.0f;
            g_aPlayerInputState[player].stickY = 0.0f;
            g_aPlayerInputState[player].steering = 0.0f;
            g_aPlayerInputState[player].controllerMissing = TRUE;
        }
        if (player == 0) {
            UpdateGameKeyboardInput(&g_aPlayerInputState[player]);
        }
    }
}

static void ClearPreviousKeys(s32 port);

s32 ReadRandnetKeyboard(s32 port, u16 keys[3]) {
    u64 packet[8];
    u8 *bytes = (u8 *)packet;
    u8 *reply;
    s32 i;
    s32 ok = FALSE;

    if (g_aContStatus[port].CONT_ERRNO != 0 || g_aContStatus[port].type != RANDNET_KEYBOARD_TYPE) {
        ClearPreviousKeys(port);
        return FALSE;
    }

    for (i = 0; i < 7; i++) {
        packet[i] = 0;
    }
    packet[7] = 1;
    bytes[port] = 2;
    bytes[port + 1] = 7;
    bytes[port + 2] = RANDNET_READ_KEYS;
    bytes[port + 3] = RANDNET_POWER_LED;

    g_wSerialInterfaceIdle = FALSE;
    __osSiGetAccess();
    if (__osSiRawStartDma(OS_WRITE, packet) == 0) {
        osRecvMesg(&g_SiMesgQueue, NULL, OS_MESG_BLOCK);
        if (__osSiRawStartDma(OS_READ, packet) == 0) {
            osRecvMesg(&g_SiMesgQueue, NULL, OS_MESG_BLOCK);
            reply = bytes + port + 4;
            if (!(bytes[port + 1] & 0xC0) && !(reply[6] & 0x10)) {
                for (i = 0; i < 3; i++) {
                    keys[i] = ((u16)reply[i * 2] << 8) | reply[i * 2 + 1];
                }
                ok = TRUE;
            }
        }
    }
    __osContLastCmd = CONT_CMD_NOP;
    __osSiRelAccess();
    g_wSerialInterfaceIdle = TRUE;
    if (!ok) {
        ClearPreviousKeys(port);
    }
    return ok;
}

s32 WasRandnetKeyDown(s32 port, u16 code) {
    s32 i;

    for (i = 0; i < 3; i++) {
        if (s_previousKeys[port][i] == code) {
            return TRUE;
        }
    }
    return FALSE;
}

static void ClearPreviousKeys(s32 port) {
    s32 i;

    for (i = 0; i < 3; i++) {
        s_previousKeys[port][i] = 0xFFFF;
    }
}

static s32 KeyboardHasKey(const u16 keys[3], u16 code) {
    return keys[0] == code || keys[1] == code || keys[2] == code;
}

void RememberRandnetKeys(s32 port, const u16 keys[3]) {
    s32 i;

    for (i = 0; i < 3; i++) {
        s_previousKeys[port][i] = keys[i];
    }
}

void UpdateGameKeyboardInput(PlayerInputState *state) {
    u16 keys[3];
    s32 port;
    s32 leftDown = FALSE;
    s32 rightDown = FALSE;
    s32 keyboardFound = FALSE;

    for (port = 0; port < MAXCONTROLLERS; port++) {
        if (!ReadRandnetKeyboard(port, keys)) {
            continue;
        }
        keyboardFound = TRUE;
        leftDown |= KeyboardHasKey(keys, RANDNET_KEY_LEFT);
        rightDown |= KeyboardHasKey(keys, RANDNET_KEY_RIGHT);
        if (KeyboardHasKey(keys, RANDNET_KEY_LEFT)) {
            state->buttons |= INPUT_LEFT;
        }
        if (KeyboardHasKey(keys, RANDNET_KEY_RIGHT)) {
            state->buttons |= INPUT_RIGHT;
        }
        if (KeyboardHasKey(keys, RANDNET_KEY_UP)) {
            state->buttons |= INPUT_UP;
        }
        if (KeyboardHasKey(keys, RANDNET_KEY_DOWN)) {
            state->buttons |= INPUT_DOWN;
        }
        if (IsCurrentScreen(ProcessRacingScreen)) {
            if (KeyboardHasKey(keys, RANDNET_KEY_ENTER) ||
                KeyboardHasKey(keys, RANDNET_KEY_ESCAPE)) {
                state->buttons |= INPUT_START;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_UP)) {
                state->buttons |= INPUT_ACCELERATE;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_DOWN)) {
                state->buttons |= INPUT_ACCELERATE | INPUT_REVERSE;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_CONTROL)) {
                state->buttons |= INPUT_BRAKE;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_LEFT_SHIFT) ||
                KeyboardHasKey(keys, RANDNET_KEY_RIGHT_SHIFT)) {
                state->buttons |= INPUT_EBRAKE;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_A)) {
                state->buttons |= INPUT_GEAR_DOWN;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_Z)) {
                state->buttons |= INPUT_GEAR_UP;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_F1) && !WasRandnetKeyDown(port, RANDNET_KEY_F1)) {
                state->buttons |= INPUT_VIEW_COCKPIT;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_F2) && !WasRandnetKeyDown(port, RANDNET_KEY_F2)) {
                state->buttons |= INPUT_VIEW_TRACKSIDE;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_F3) && !WasRandnetKeyDown(port, RANDNET_KEY_F3)) {
                state->buttons |= INPUT_VIEW_CHASE;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_F4) && !WasRandnetKeyDown(port, RANDNET_KEY_F4)) {
                state->buttons |= INPUT_VIEW_BUMPER;
            }
        } else {
            if (KeyboardHasKey(keys, RANDNET_KEY_ENTER)) {
                state->buttons |= INPUT_A;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_ESCAPE)) {
                state->buttons |= INPUT_B;
            }
            if (KeyboardHasKey(keys, RANDNET_KEY_SPACE)) {
                state->buttons |= INPUT_START;
            }
        }
        RememberRandnetKeys(port, keys);
    }
    if (keyboardFound) {
        state->controllerMissing = FALSE;
    }
    if (IsCurrentScreen(ProcessRacingScreen) && (leftDown || rightDown)) {
        state->steeringInput = leftDown == rightDown ? 0 : leftDown ? -80 : 80;
    }
}

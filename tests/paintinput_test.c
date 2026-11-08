#include <assert.h>

#include "common/input.h"
#include "common/rumble.h"
#include "common/screen.h"
#include "common/system.h"
#include "n64/decaleditor.h"
#include "n64/libultra/io/controller.h"
#include "n64/paintinput.h"
#include "n64/peripheralinput.h"

OSContStatus g_aContStatus[MAXCONTROLLERS];
OSContPad g_aContPadData[MAXCONTROLLERS];
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
OSMesgQueue g_SiMesgQueue;
s16 g_wSerialInterfaceIdle;
u8 __osContLastCmd;
Sprite g_texPaintCursor, g_texKeyboardHighlight;
DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];
u8 g_bPaintTool, g_bDecalDragActive, g_bPaintToolStarted, g_bDecalTextLength;
u8 g_bPaintShopPlayer;

static u16 keyboardKey;
static u16 keyboardKey2, keyboardKey3;
static u16 otherKeyboardKeys[MAXCONTROLLERS][3];
static u16 connectedType[MAXCONTROLLERS];
static u8 keyboardStatus;
static s32 keyboardReads;
static s32 statusReads;
static s32 racing;
static s32 painting;
static s32 activePort;
static s32 statusCommand;

void ProcessRacingScreen(void) {
}

void ProcessPaintShopScreen(void) {
}

s32 IsCurrentScreen(ScreenCallback callback) {
    return (racing && callback == ProcessRacingScreen) ||
           (painting && callback == ProcessPaintShopScreen);
}

void __osSiGetAccess(void) {
}

void __osSiRelAccess(void) {
}

s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flag) {
    (void)queue;
    (void)message;
    (void)flag;
    return 0;
}

s32 __osSiRawStartDma(s32 direction, void *buffer) {
    u8 *bytes = buffer;
    s32 port;

    if (direction == OS_WRITE) {
        statusCommand = bytes[0] == CONT_CMD_NOP;
        if (statusCommand) {
            assert(bytes[MAXCONTROLLERS * 8] == CONT_CMD_END);
            for (port = 0; port < MAXCONTROLLERS; port++) {
                assert(bytes[port * 8 + 1] == CONT_CMD_REQUEST_STATUS_TX);
                assert(bytes[port * 8 + 2] == CONT_CMD_REQUEST_STATUS_RX);
                assert(bytes[port * 8 + 3] == CONT_CMD_REQUEST_STATUS);
            }
        } else {
            for (port = 0; port < MAXCONTROLLERS; port++) {
                if (bytes[port] == 2 && bytes[port + 1] == 7 && bytes[port + 2] == 0x13) {
                    break;
                }
            }
            assert(port < MAXCONTROLLERS);
            assert(bytes[port + 3] == 0x04);
            activePort = port;
        }
    } else {
        if (statusCommand) {
            for (port = 0; port < MAXCONTROLLERS; port++) {
                s32 offset = port * 8;
                if (connectedType[port] == 0) {
                    bytes[offset + 2] = 0x80;
                } else {
                    bytes[offset + 2] = CONT_CMD_REQUEST_STATUS_RX;
                    bytes[offset + 4] = connectedType[port];
                    bytes[offset + 5] = connectedType[port] >> 8;
                }
            }
            statusReads++;
        } else {
            u16 keys[3];
            keys[0] = activePort == 1 ? keyboardKey : otherKeyboardKeys[activePort][0];
            keys[1] = activePort == 1 ? keyboardKey2 : otherKeyboardKeys[activePort][1];
            keys[2] = activePort == 1 ? keyboardKey3 : otherKeyboardKeys[activePort][2];
            for (port = 0; port < 3; port++) {
                bytes[activePort + 4 + port * 2] = keys[port] >> 8;
                bytes[activePort + 5 + port * 2] = keys[port];
            }
            bytes[activePort + 10] = keyboardStatus;
            keyboardReads++;
        }
    }
    return 0;
}

int main(void) {
    PlayerInputState input = {0};

    g_aContStatus[1].type = 0x0200;
    g_bPaintTool = PAINT_TOOL_TEXT;
    g_bDecalDragActive = 1;
    g_aDecalGlyphs[0].character = 'A';
    g_aDecalGlyphs[0].x = 100;
    g_aDecalGlyphs[0].y = 150;
    g_texKeyboardHighlight.drawWidth = 12;
    g_texKeyboardHighlight.drawHeight = 10;
    keyboardKey = 0x0D07;

    UpdatePaintPeripheralInput(&input);
    assert(keyboardReads == 1);
    assert(input.buttons == INPUT_A);
    assert(g_texPaintCursor.originX == 106);
    assert(g_texPaintCursor.originY == 155);
    assert(__osContLastCmd == CONT_CMD_NOP);
    assert(g_wSerialInterfaceIdle == TRUE);

    input.buttons = 0;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == 0);

    keyboardKey = 0;
    UpdatePaintPeripheralInput(&input);
    keyboardKey = 0x0D06;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == 0);

    keyboardKey = 0;
    UpdatePaintPeripheralInput(&input);
    keyboardKey = 0x0D07;
    g_bDecalTextLength = DECAL_TEXT_LENGTH;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == 0);
    g_bDecalTextLength = 0;

    keyboardKey = 0;
    UpdatePaintPeripheralInput(&input);
    keyboardKey = 0x0D04;
    g_aDecalGlyphs[49].x = 200;
    g_aDecalGlyphs[49].y = 220;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == INPUT_A);
    assert(g_texPaintCursor.originX == 206);
    assert(g_texPaintCursor.originY == 225);

    keyboardStatus = 0x10;
    input.buttons = 0;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == 0);

    keyboardStatus = 0;
    keyboardKey = 0x0E01;
    UpdatePaintPeripheralInput(&input);
    assert(IsRandnetShiftDown());
    keyboardKey = 0;
    UpdatePaintPeripheralInput(&input);
    assert(!IsRandnetShiftDown());

    g_bPaintTool = PAINT_TOOL_BRUSH;
    g_bDecalDragActive = 0;
    input.buttons = 0;
    keyboardKey = 0x1107;
    keyboardKey2 = 0x0D08;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == INPUT_L);
    input.buttons = 0;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == 0);
    keyboardKey3 = 0x0E01;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == 0);
    keyboardKey2 = 0;
    UpdatePaintPeripheralInput(&input);
    keyboardKey2 = 0x0D08;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == (INPUT_L | INPUT_R));
    keyboardKey2 = 0;
    keyboardKey3 = 0;
    UpdatePaintPeripheralInput(&input);
    g_bPaintTool = PAINT_TOOL_TEXT;
    g_bDecalDragActive = 1;
    g_aDecalGlyphs[1].character = 'Z';
    g_aDecalGlyphs[1].x = 150;
    g_aDecalGlyphs[1].y = 160;
    input.buttons = 0;
    keyboardKey2 = 0x0D08;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == INPUT_L);
    assert(g_texPaintCursor.originX == 206 && g_texPaintCursor.originY == 225);
    keyboardKey2 = keyboardKey3 = 0;
    input.buttons = 0;

    g_aContStatus[1].type = 0;
    g_aContStatus[2].type = CONT_RELATIVE;
    g_aContPadData[2].stick_x = -120;
    g_aContPadData[2].stick_y = 80;
    g_aContPadData[2].button = CONT_A | CONT_B;
    UpdatePaintPeripheralInput(&input);
    assert(g_texPaintCursor.originX == 86);
    assert(g_texPaintCursor.originY == 145);
    assert(input.buttons == (INPUT_A | INPUT_B));

    g_aContPadData[2].stick_x = -127;
    g_aContPadData[2].stick_y = 127;
    UpdatePaintPeripheralInput(&input);
    assert(g_texPaintCursor.originX == 0x1F);
    assert(g_texPaintCursor.originY == 0x16);

    g_aContStatus[3].type = CONT_RELATIVE;
    g_aContPadData[2].stick_x = 5;
    g_aContPadData[2].stick_y = -5;
    g_aContPadData[2].button = 0;
    g_aContPadData[3].stick_x = 10;
    g_aContPadData[3].stick_y = -5;
    g_aContPadData[3].button = CONT_B;
    g_texPaintCursor.originX = 100;
    g_texPaintCursor.originY = 100;
    input.buttons = 0;
    UpdatePaintPeripheralInput(&input);
    assert(g_texPaintCursor.originX == 115);
    assert(g_texPaintCursor.originY == 110);
    assert(input.buttons == INPUT_B);
    g_aContStatus[3].CONT_ERRNO = CONT_NO_RESPONSE_ERROR;
    input.buttons = 0;
    UpdatePaintPeripheralInput(&input);
    assert(g_texPaintCursor.originX == 120);
    assert(g_texPaintCursor.originY == 115);
    assert(input.buttons == 0);
    g_aContStatus[3].CONT_ERRNO = 0;
    g_aContStatus[3].type = 0x0200;
    otherKeyboardKeys[3][0] = 0x0D04;
    input.buttons = 0;
    UpdatePaintPeripheralInput(&input);
    assert(input.buttons == INPUT_A);

    g_aContStatus[1].type = 0x0200;
    keyboardKey = 0x0205;
    keyboardKey2 = 0x0204;
    keyboardKey3 = 0;
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == (INPUT_LEFT | INPUT_UP | INPUT_A));
    assert(!input.controllerMissing);

    racing = 1;
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == (INPUT_LEFT | INPUT_UP | INPUT_ACCELERATE | INPUT_START));
    assert(input.steeringInput == -80);

    keyboardKey = 0x1107;
    keyboardKey2 = 0x0D07;
    keyboardKey3 = 0x0D08;
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == (INPUT_BRAKE | INPUT_GEAR_DOWN | INPUT_GEAR_UP | INPUT_START));

    keyboardKey = 0x0E01;
    keyboardKey2 = 0x0B01;
    keyboardKey3 = 0;
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == (INPUT_EBRAKE | INPUT_VIEW_COCKPIT | INPUT_START));
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == (INPUT_EBRAKE | INPUT_START));

    keyboardKey = 0x0D04;
    keyboardKey2 = 0x0A08;
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == INPUT_START);

    racing = 0;
    input.buttons = 0;
    UpdateGameKeyboardInput(&input);
    assert(input.buttons == (INPUT_A | INPUT_B));

    connectedType[0] = CONT_RELATIVE;
    connectedType[1] = 0x0200;
    connectedType[2] = 0x0200;
    connectedType[3] = 0x0200;
    keyboardKey = keyboardKey2 = keyboardKey3 = 0;
    otherKeyboardKeys[2][0] = 0x0602;
    otherKeyboardKeys[3][0] = 0;
    g_aPlayerInputState[0].buttons = INPUT_A;
    g_aPlayerInputState[0].steeringInput = 50;
    UpdateN64PeripheralInputForPlayer(0);
    assert(statusReads == 1);
    assert(g_aContStatus[2].type == 0x0200);
    assert(g_aPlayerInputState[0].buttons == INPUT_START);
    assert(g_aPlayerInputState[0].steeringInput == 0);

    connectedType[2] = 0;
    for (int frame = 0; frame < 16; frame++) {
        g_aPlayerInputState[0].buttons = 0;
        UpdateN64PeripheralInputForPlayer(0);
    }
    assert(statusReads == 2);
    assert(g_aContStatus[2].CONT_ERRNO == CONT_NO_RESPONSE_ERROR);
    assert(g_aPlayerInputState[0].buttons == 0);
    return 0;
}

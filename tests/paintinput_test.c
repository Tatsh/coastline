#include <assert.h>

#include "common/input.h"
#include "common/rumble.h"
#include "common/system.h"
#include "n64/decaleditor.h"
#include "n64/libultra/io/controller.h"
#include "n64/paintinput.h"

OSContStatus g_aContStatus[MAXCONTROLLERS];
OSContPad g_aContPadData[MAXCONTROLLERS];
OSMesgQueue g_SiMesgQueue;
s16 g_wSerialInterfaceIdle;
u8 __osContLastCmd;
Sprite g_texPaintCursor, g_texKeyboardHighlight;
DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];
u8 g_bPaintTool, g_bDecalDragActive, g_bPaintToolStarted, g_bDecalTextLength;

static u16 keyboardKey;
static u8 keyboardStatus;
static s32 keyboardReads;

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

    if (direction == OS_WRITE) {
        assert(bytes[1] == 2);
        assert(bytes[2] == 7);
        assert(bytes[3] == 0x13);
        assert(bytes[4] == 0x04);
    } else {
        bytes[5] = keyboardKey >> 8;
        bytes[6] = keyboardKey;
        bytes[11] = keyboardStatus;
        keyboardReads++;
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
    return 0;
}

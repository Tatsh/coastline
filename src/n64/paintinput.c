#include "n64/paintinput.h"

#include <PR/os.h>

#include "common/input.h"
#include "common/system.h"
#include "n64/decaleditor.h"
#include "n64/paintshop.h"
#include "n64/peripheralinput.h"

#define PAINT_CURSOR_MIN_X 0x1F
#define PAINT_CURSOR_MAX_X 0x261
#define PAINT_CURSOR_MIN_Y 0x16
#define PAINT_CURSOR_MAX_Y 0x1CA
#define PAINT_KEY_BACKSPACE 47
#define PAINT_KEY_ENTER 49

typedef struct {
    u16 code;
    char character;
} KeyboardGlyph;

static const KeyboardGlyph s_keyboardGlyphs[] = {
    {0x0D07, 'A'}, {0x0708, 'B'}, {0x0508, 'C'}, {0x0507, 'D'}, {0x0601, 'E'}, {0x0607, 'F'},
    {0x0707, 'G'}, {0x0807, 'H'}, {0x0804, 'I'}, {0x0907, 'J'}, {0x0903, 'K'}, {0x0803, 'L'},
    {0x0908, 'M'}, {0x0808, 'N'}, {0x0704, 'O'}, {0x0604, 'P'}, {0x0C01, 'Q'}, {0x0701, 'R'},
    {0x0C07, 'S'}, {0x0801, 'T'}, {0x0904, 'U'}, {0x0608, 'V'}, {0x0501, 'W'}, {0x0C08, 'X'},
    {0x0901, 'Y'}, {0x0D08, 'Z'}, {0x0606, '0'}, {0x0C05, '1'}, {0x0505, '2'}, {0x0605, '3'},
    {0x0705, '4'}, {0x0805, '5'}, {0x0905, '6'}, {0x0906, '7'}, {0x0806, '8'}, {0x0706, '9'},
    {0x0602, ' '}, {0x0902, ','}, {0x0802, '.'}, {0x0702, '?'}, {0x1004, '-'}, {0x0504, '\''},
    {0x1105, ';'},
};

static s32 ClampCursor(s32 value, s32 minimum, s32 maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static s32 KeyboardGlyphIndex(u16 code) {
    s32 i;
    s32 j;
    char character = 0;

    if (code == 0x0D06) {
        return PAINT_KEY_BACKSPACE;
    }
    if (code == 0x0D04) {
        return PAINT_KEY_ENTER;
    }
    for (i = 0; i < (s32)(sizeof(s_keyboardGlyphs) / sizeof(s_keyboardGlyphs[0])); i++) {
        if (s_keyboardGlyphs[i].code == code) {
            character = s_keyboardGlyphs[i].character;
            break;
        }
    }
    if (!character) {
        return -1;
    }
    for (j = 0; j < DECAL_GLYPHS; j++) {
        if (g_aDecalGlyphs[j].character == character ||
            (character >= 'A' && character <= 'Z' &&
             g_aDecalGlyphs[j].character == character + ('a' - 'A'))) {
            return j;
        }
    }
    return -1;
}

static void ApplyKeyboardKey(PlayerInputState *state, u16 code, s32 fresh) {
    s32 glyph;

    if (g_bPaintTool == PAINT_TOOL_TEXT && g_bDecalDragActive && !g_bPaintToolStarted) {
        if (!fresh) {
            return;
        }
        glyph = KeyboardGlyphIndex(code);
        if ((glyph == PAINT_KEY_BACKSPACE && g_bDecalTextLength == 0) ||
            (glyph >= 0 && glyph != PAINT_KEY_BACKSPACE && glyph != PAINT_KEY_ENTER &&
             g_bDecalTextLength >= DECAL_TEXT_LENGTH)) {
            return;
        }
        if (glyph >= 0) {
            g_texPaintCursor.originX =
                g_aDecalGlyphs[glyph].x + (g_texKeyboardHighlight.drawWidth >> 1);
            g_texPaintCursor.originY =
                g_aDecalGlyphs[glyph].y + (g_texKeyboardHighlight.drawHeight >> 1);
            state->buttons |= INPUT_A;
        } else if (code == 0x0A08) {
            state->buttons |= INPUT_B;
        }
        return;
    }
    switch (code) {
    case 0x0204:
        state->buttons |= INPUT_UP;
        break;
    case 0x0305:
        state->buttons |= INPUT_DOWN;
        break;
    case 0x0205:
        state->buttons |= INPUT_LEFT;
        break;
    case 0x0405:
        state->buttons |= INPUT_RIGHT;
        break;
    case 0x0D04:
        state->buttons |= INPUT_A;
        break;
    case 0x0A08:
    case 0x0D06:
        state->buttons |= INPUT_B;
        break;
    }
}

void UpdatePaintPeripheralInput(PlayerInputState *state) {
    s32 port;
    u16 keys[3];
    s32 i;

    for (port = 0; port < MAXCONTROLLERS; port++) {
        s32 keyboardPresent = ReadRandnetKeyboard(port, keys);

        if (g_aContStatus[port].CONT_ERRNO != 0) {
            continue;
        }
        if (g_aContStatus[port].type == CONT_RELATIVE && g_aContPadData[port].CONT_ERRNO == 0) {
            OSContPad *mouse = &g_aContPadData[port];

            g_texPaintCursor.originX = ClampCursor(
                g_texPaintCursor.originX + mouse->stick_x, PAINT_CURSOR_MIN_X, PAINT_CURSOR_MAX_X);
            g_texPaintCursor.originY = ClampCursor(
                g_texPaintCursor.originY - mouse->stick_y, PAINT_CURSOR_MIN_Y, PAINT_CURSOR_MAX_Y);
            if (mouse->button & CONT_A) {
                state->buttons |= INPUT_A;
            }
            if (mouse->button & CONT_B) {
                state->buttons |= INPUT_B;
            }
            if (port == state->playerIndex) {
                state->stickX = 0.0f;
                state->stickY = 0.0f;
            }
        }
        if (!keyboardPresent) {
            continue;
        }
        {
            s32 controlDown = FALSE;
            s32 shiftDown = FALSE;
            s32 zDown = FALSE;

            for (i = 0; i < 3; i++) {
                controlDown |= keys[i] == RANDNET_KEY_CONTROL;
                shiftDown |=
                    keys[i] == RANDNET_KEY_LEFT_SHIFT || keys[i] == RANDNET_KEY_RIGHT_SHIFT;
                zDown |= keys[i] == RANDNET_KEY_Z;
            }
            for (i = 0; i < 3; i++) {
                if (keys[i] != 0 && !(controlDown && keys[i] == RANDNET_KEY_Z)) {
                    ApplyKeyboardKey(state, keys[i], !WasRandnetKeyDown(port, keys[i]));
                }
            }
            if (controlDown && zDown && !WasRandnetKeyDown(port, RANDNET_KEY_Z)) {
                state->buttons |= shiftDown ? INPUT_L | INPUT_R : INPUT_L;
            }
        }
        RememberRandnetKeys(port, keys);
    }
}

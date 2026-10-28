/* Host check of src/n64/paintshop.c against the branches and command words of the disassembly. */
#include <stdio.h>
#include <string.h>

#include <PR/os_cont.h>

#include "fade.h"
#include "input.h"
#include "options.h"
#include "paintshop.h"
#include "pakmenu.h"
#include "racemenu.h"
#include "render.h"
#include "rumble.h"
#include "system.h"
#include "text.h"

Gfx *gDisplayListPtr;
s32 g_dwScreenScaleShift = 1;
u16 g_wScreenFrameCount;
s16 g_wSerialInterfaceIdle;
s32 g_dwMenuChoosingPlayer;
VehicleModel *g_pCurrentVehicleModelData;
PlayerInputState g_aPlayerInputState[MAXCONTROLLERS];
OSPfs g_aPakPfs[MAXCONTROLLERS], g_aContPfs[MAXCONTROLLERS];
OSMesgQueue g_SiMesgQueue;
u8 g_abRumblePakPresent[MAXCONTROLLERS], g_abSavePakIdKnown[2], g_aabSavePakIds[2][32];
Sprite g_texButtonA, g_texButtonB, g_texPaintCursor, g_texKeyboardPanel, g_texKeyboardBackground,
    g_texKeyboardHighlight, g_texPaintToolRect, g_texPaintToolOval, g_texPaintToolMirror;
Sprite *g_apDecalSlotIcons[DECAL_SLOTS], *g_pDecalSlotIcon;
u8 g_bDecalSlot, g_bPreviousDecalSlot, g_bUndoDecalSlot, g_bBrushSize, g_bSavedColor, g_bDecalColor,
    g_bPaintTool, g_bPreviousPaintTool, g_bDecalEditCount, g_bPaintToolStarted, g_bDecalDragActive,
    g_bPaletteEnabled, g_bDecalSlotHasMirror, g_bDecalSlotChanged, g_bColorEditorOpen,
    g_bShowPreview3D, g_bUndoAvailable, g_bStyleMenuOpen, g_bTextShadowPickerOpen,
    g_bEyedropperActive, g_bMirrorTargetPanel, g_bDecalWidth, g_bDecalHeight, g_bKeyboardHoverKey,
    g_bDecalTextLength, g_bDecalTextWidth, g_bPaintShopPlayer;
u8 *g_pDecalTexels, *g_pDecalMask, *g_pDecalUndoBuffer, *s_pMirrorSource;
u8 g_bPakDialogActive, g_bPakFileNeedsCreate, g_bPakRumbleSwapped, g_bPakRetryNewPak,
    g_bPakDialogState, g_bPakDecalIndex, g_bPakWaitFrames;
s32 g_dwPakFileOffset, g_dwPakError, g_dwPakFileNo, g_dwPakChunkSize;
PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];
DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];
DecalGlyph *g_apDecalTextGlyphs[DECAL_TEXT_LENGTH];
char g_szDecalText[24];

/* The size of the file of the decals. */
#define DECAL_FILE_SIZE 0x3E00

/* The keys that shorten and end the text. */
#define KEY_BACKSPACE 47
#define KEY_ENTER 49

/* The texts drawn, and where. */
static const char *texts[16];
static s32 textX[16], textY[16], textCount;

void DrawText(const char *text, s32 x, s32 y) {
    if (textCount < 16) {
        texts[textCount] = text;
        textX[textCount] = x;
        textY[textCount++] = y;
    }
}
void SetTextSize(s32 size) {
}
void SetTextAlignLeft(void) {
}
void SetTextAlignCenter(void) {
}
void SetTextStyleNormal(void) {
}
void SetTextCustomColors(s32 r, s32 g, s32 b, s32 r2, s32 g2, s32 b2) {
}
void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y) {
}
void DrawSpriteTinted(Sprite *sprite, u8 red, u8 green, u8 blue) {
}
void DrawSpriteAtOrigin(Sprite *sprite) {
}
void DrawSpriteRegion(
    Sprite *sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
}
void DrawRectOutline(
    s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue) {
}

void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
}

void SwapBytes(u8 *a, u8 *b) {
    u8 t = *a;

    *a = *b;
    *b = t;
}

void TransferFlags(PlayerInputState *state, u32 mask) {
    state->buttons &= ~mask;
}
void UpdateStickRepeat(PlayerInputState *state) {
}

/* The cursor is over the keyboard and over the key the test points it at. */
static s32 cursorKey = -1;
s32 IsCursorInButton(Sprite *sprite) {
    return sprite == &g_texKeyboardBackground || sprite->originX == cursorKey;
}
s32 IsCursorOnKeyboardKey(DecalGlyph *key) {
    return key->x == cursorKey;
}

/* A decal of 8 by 4 texels, one a byte. */
static u8 texels[4][8], undo[4][8];
void SetPixel4Bit(s32 x, s32 y, u8 color) {
    texels[y][x] = color;
}
u8 ReadPixel4Bit(u8 *buffer, s32 x, s32 y) {
    return buffer[y * 8 + x];
}

void ConvertPaletteToRgb(void) {
}

static s32 fadedIn;
void FadeSfxTo(float target, float seconds) {
    fadedIn = target > 0.5f;
}
void FadeMusicTo(float target, float seconds) {
}
s32 HasSfxFadeFinished(void) {
    return TRUE;
}

/* A controller pak on controller 1 holding the decals. */
static u8 pak[DECAL_FILE_SIZE];
static s32 readCalls, findCalls;
s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern) {
    *pattern = 2;
    return 0;
}
s32 osPfsInitPak(OSMesgQueue *queue, OSPfs *pfs, int channel) {
    memset(pfs->id, channel, sizeof(pfs->id));
    return 0;
}
s32 osPfsFindFile(
    OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name, s32 *file_no) {
    findCalls++;
    if (company_code != 0x3544 || game_code != 0x4E475245 || game_name[0] != 0x2D ||
        game_name[14] != 0x2C || ext_name[0] != 0) {
        return PFS_ERR_INVALID;
    }
    *file_no = 3;
    return 0;
}
s32 osPfsReadWriteFile(
    OSPfs *pfs, s32 file_no, u8 flag, int offset, int size_in_bytes, u8 *data_buffer) {
    readCalls++;
    if (flag == PFS_READ) {
        memcpy(data_buffer, pak + offset, size_in_bytes);
    } else {
        memcpy(pak + offset, data_buffer, size_in_bytes);
    }
    return 0;
}
s32 osPfsChecker(OSPfs *pfs) {
    return 0;
}
s32 osPfsRepairId(OSPfs *pfs) {
    return 0;
}
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *bytes_not_used) {
    *bytes_not_used = DECAL_FILE_SIZE;
    return 0;
}
s32 osPfsAllocateFile(OSPfs *pfs,
                      u16 company_code,
                      u32 game_code,
                      u8 *game_name,
                      u8 *ext_name,
                      int file_size_in_bytes,
                      s32 *file_no) {
    return 0;
}
s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel) {
    return 1;
}
s32 osMotorStop(OSPfs *pfs) {
    return 0;
}
void osSyncPrintf(const char *fmt, ...) {
}

/* The words of a box at (10, 20) of 30 by 40 with a border of 2, as the disassembly writes it. */
static const u32 border[][2] = {
    {0xE7000000, 0},          {0xBA001402, 0x00300000}, {0xB900031D, 0x0F0A4000},
    {0xF7000000, 0x84218421}, {0xF60A00F0, 0x00028050}, {0xE7000000, 0},
    {0xF7000000, 0xE739E739}, {0xF60280F4, 0x0002404C}, {0xF60240F8, 0x00020048},
    {0xF60A4050, 0x0002404C}, {0xF60A804C, 0x00020048}, {0xE7000000, 0},
    {0xF7000000, 0x318D318D}, {0xF60A40F4, 0x000A004C}, {0xF60A80F8, 0x000A4048},
    {0xF60A40F4, 0x000240F0}, {0xF60A80F8, 0x000200F4}, {0xE7000000, 0},
    {0xBA001402, 0},
};

static int CheckBorder(void) {
    static Gfx list[64];
    u32 i, n = sizeof(border) / sizeof(border[0]);

    gDisplayListPtr = list;
    DrawRectBorder(10, 20, 30, 40, 2, FALSE, 0, 0x80, 0x80, 0x80);
    if ((u32)(gDisplayListPtr - list) != n) {
        printf("border %d words\n", (int)(gDisplayListPtr - list));
        return 1;
    }
    for (i = 0; i < n; i++) {
        if (list[i].words.w0 != border[i][0] || list[i].words.w1 != border[i][1]) {
            printf("border %u: %08X %08X\n", i, list[i].words.w0, list[i].words.w1);
            return 1;
        }
    }
    /* Sunken swaps the shades, and a transparent box skips its fill. */
    gDisplayListPtr = list;
    DrawRectBorder(10, 20, 30, 40, 2, TRUE, 1, 0x80, 0x80, 0x80);
    if (gDisplayListPtr - list != (s32)n - 3 || list[3].words.w1 != 0x318D318D ||
        list[9].words.w1 != 0xE739E739) {
        printf("sunken %08X %08X\n", list[3].words.w1, list[9].words.w1);
        return 1;
    }
    return 0;
}

/* Start a frame with an empty display list and no texts. */
static void NewFrame(void) {
    static Gfx frame[256];

    gDisplayListPtr = frame;
    textCount = 0;
}

static int CheckErrorMessages(void) {
    NewFrame();
    DrawPakErrorMessage(PFS_ERR_INCONSISTENT, PAINT_TOOL_LOAD, 1);
    /* A box 0x116 wide is centred at 0xB5; its text sits 16 in, at half scale. */
    if (textCount != 2 || strcmp(texts[0], "UNABLE TO READ FROM") || textX[0] != 0x62 ||
        textY[0] != 0x80 || textY[1] != 0x8E) {
        printf("inconsistent %d %d %d\n", textCount, textX[0], textY[0]);
        return 1;
    }
    NewFrame();
    DrawPakErrorMessage(PAK_ERROR_TRANSFER, PAINT_TOOL_SAVE, 0);
    if (textCount != 2 || strcmp(texts[1], "WHILE SAVING DECALS") || textX[0] != 0x64 ||
        textY[0] != 0x75) {
        printf("transfer %d %d %d\n", textCount, textX[0], textY[0]);
        return 1;
    }
    NewFrame();
    DrawPakErrorMessage(PFS_ERR_NEW_PACK, PAINT_TOOL_LOAD, 0);
    DrawPakErrorMessage(12, PAINT_TOOL_LOAD, 0);
    DrawPakErrorMessage(0, PAINT_TOOL_LOAD, 0);
    if (textCount != 0) {
        return 1;
    }
    NewFrame();
    DrawPakErrorMessage(PFS_ERR_CONTRFAIL, PAINT_TOOL_LOAD, 0);
    if (textCount != 4 || strcmp(texts[3], "BE LOADED.") || textY[3] != 0x67 + 42) {
        return 1;
    }
    return 0;
}

static int CheckUndo(void) {
    static ModelTexture textures[4];
    static VehicleModel model;
    static u8 decal[DECAL_BUFFER_SIZE], buffer[DECAL_BUFFER_SIZE];

    model.textures = textures;
    model.materialTextures[5] = 3;
    textures[3].image = decal;
    g_pCurrentVehicleModelData = &model;
    g_pDecalUndoBuffer = buffer;
    g_pDecalTexels = decal;
    g_bDecalSlot = 5;
    g_bDecalEditCount = 0;
    decal[0] = 0x12;
    SaveDecalUndo();
    if (buffer[0] != 0x12 || g_bUndoDecalSlot != 5 || g_bDecalEditCount != 1 || !g_bUndoAvailable) {
        return 1;
    }
    decal[0] = 0x34;
    g_bDecalSlot = 2;
    UndoDecalEdit();
    if (decal[0] != 0x12 || g_bDecalEditCount != 0 || g_bUndoAvailable) {
        return 1;
    }
    /* Without an edit nothing is restored. */
    decal[0] = 0x56;
    UndoDecalEdit();
    return decal[0] != 0x56 || g_bDecalEditCount != 0;
}

static int CheckMirror(void) {
    static ModelTexture textures[4];
    static VehicleModel model;
    static u8 *masks[VEHICLE_MATERIALS];
    static u8 left[DECAL_BUFFER_SIZE], right[DECAL_BUFFER_SIZE], buffer[DECAL_BUFFER_SIZE];
    static const u8 pairs[][2] = {{0, 1}, {1, 0}, {2, 3}, {3, 2}, {6, 7}, {7, 6}};
    s32 i, x, y;

    model.textures = textures;
    model.decalMasks = masks;
    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        model.materialTextures[i] = 1;
    }
    textures[1].image = left;
    g_pCurrentVehicleModelData = &model;
    g_pDecalUndoBuffer = buffer;
    g_pDecalTexels = right;
    g_bDecalWidth = 8;
    g_bDecalHeight = 4;
    g_bDecalSlotHasMirror = TRUE;
    g_bPreviousPaintTool = PAINT_TOOL_BRUSH;
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 8; x++) {
            left[y * 8 + x] = y * 8 + x;
        }
    }
    for (i = 0; i < 6; i++) {
        g_bDecalSlot = pairs[i][0];
        g_bPaintTool = PAINT_TOOL_MIRROR;
        MirrorDecalToOppositePanel();
        if (g_bDecalSlot != pairs[i][1] || g_bPreviousDecalSlot != pairs[i][0] ||
            g_bPaintTool != PAINT_TOOL_BRUSH || !g_bDecalSlotChanged || s_pMirrorSource != NULL) {
            printf("mirror %d: %d\n", i, g_bDecalSlot);
            return 1;
        }
    }
    if (texels[2][0] != 2 * 8 + 7 || texels[3][7] != 3 * 8) {
        return 1;
    }
    /* A slot without a pair mirrors to the last target. */
    g_bDecalSlot = 4;
    MirrorDecalToOppositePanel();
    if (g_bDecalSlot != 6) {
        return 1;
    }
    /* A vertical flip reads the copy saved for undo. */
    g_pDecalTexels = left;
    g_texPaintToolMirror.style = 2;
    g_bPaintTool = PAINT_TOOL_MIRROR;
    FlipDecalPixels();
    if (texels[0][5] != 3 * 8 + 5 || texels[3][1] != 1 || g_texPaintToolMirror.style ||
        g_bPaintTool != PAINT_TOOL_BRUSH) {
        return 1;
    }
    return 0;
}

/* Press a key of the keyboard. */
static void PressKey(s32 key) {
    cursorKey = 1000 + key;
    g_aPlayerInputState[0].buttons = INPUT_A;
    ProcessTextInputKeyboard();
    cursorKey = -1;
}

static int CheckKeyboard(void) {
    s32 i;

    for (i = 0; i < DECAL_GLYPHS; i++) {
        g_aDecalGlyphs[i].x = 1000 + i;
        g_aDecalGlyphs[i].character = 'A' + i % 26;
        g_aDecalGlyphs[i].width = 4 + i % 3;
        g_aDecalGlyphs[i].kernLeft = i % 2;
        g_aDecalGlyphs[i].kernRight = 2 - i % 3;
    }
    g_bDecalWidth = 64;
    g_bPaintShopPlayer = 0;
    g_bDecalTextLength = 0;
    g_bDecalTextWidth = 0;
    /* The first key adds its width; the next take away the overlap and add 2. */
    PressKey(0);
    if (g_bDecalTextLength != 1 || g_bDecalTextWidth != 4 || strcmp(g_szDecalText, "A") ||
        g_wScreenFrameCount != 3 || g_bKeyboardHoverKey != 0) {
        printf("first %d %d\n", g_bDecalTextLength, g_bDecalTextWidth);
        return 1;
    }
    PressKey(1);
    /* Key 1 is 5 wide with 1 empty column at the left; key 0 has 2 at the right. */
    if (g_bDecalTextLength != 2 || g_bDecalTextWidth != 4 + 5 - 1 + 2 ||
        strcmp(g_szDecalText, "AB") || g_apDecalTextGlyphs[1] != &g_aDecalGlyphs[1]) {
        printf("second %d %d\n", g_bDecalTextLength, g_bDecalTextWidth);
        return 1;
    }
    PressKey(KEY_BACKSPACE);
    if (g_bDecalTextLength != 1 || g_bDecalTextWidth != 4 || strcmp(g_szDecalText, "A") ||
        g_apDecalTextGlyphs[1] != NULL) {
        printf("backspace %d %d\n", g_bDecalTextLength, g_bDecalTextWidth);
        return 1;
    }
    PressKey(KEY_BACKSPACE);
    if (g_bDecalTextLength != 0 || g_bDecalTextWidth != 0 || g_szDecalText[0]) {
        return 1;
    }
    /* Enter with no text closes the keyboard. */
    g_bDecalDragActive = TRUE;
    PressKey(KEY_ENTER);
    if (g_bDecalDragActive || g_bPaintToolStarted || !g_bShowPreview3D) {
        return 1;
    }
    /* A key that does not fit the decal is not typed. */
    g_bDecalWidth = 6;
    PressKey(0);
    return g_bDecalTextLength != 0;
}

/* Run the dialog for some frames, until it returns a result or is done. */
static s32 RunDialog(VehicleModel *model, u8 operation, u8 inCarSelect, u8 *done, s32 frames) {
    s32 result = 0;

    while (frames-- > 0 && !*done) {
        NewFrame();
        result = ProcessDecalSaveLoad(model, 1, operation, inCarSelect, done);
        if (result) {
            break;
        }
    }
    return result;
}

static int CheckSaveLoad(void) {
    static ModelTexture textures[VEHICLE_MATERIALS];
    static VehicleModel model;
    static u8 *masks[VEHICLE_MATERIALS];
    static u8 images[VEHICLE_MATERIALS][0x200], buffer[DECAL_BUFFER_SIZE], mask[2 * 8];
    static u16 palettes[VEHICLE_MATERIALS][16];
    s32 i, result;
    u8 done = FALSE;

    model.textures = textures;
    model.decalMasks = masks;
    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        model.materialTextures[i] = i;
        textures[i].image = images[i];
        textures[i].palette = palettes[i];
        textures[i].width = 16;
        textures[i].height = 8;
    }
    g_pDecalUndoBuffer = buffer;
    for (i = 0; i < DECAL_FILE_SIZE; i++) {
        pak[i] = i * 7;
    }
    /* Slot 3 protects the left half of its first row. */
    mask[0] = 0xFF;
    masks[3] = mask;
    images[3][0] = 0xEE;
    g_bPakDialogActive = FALSE;
    result = RunDialog(&model, PAINT_TOOL_LOAD, 1, &done, 40);
    if (result || !done || !g_wSerialInterfaceIdle || !fadedIn || findCalls != 1) {
        printf("load %d %d %d\n", (int)result, done, g_bPakDialogState);
        return 1;
    }
    /* The doors move a row more. The first decal is 72 bytes and the third starts at 144. */
    if (images[0][0] != 0 || images[0][71] != (u8)(71 * 7) || images[2][0] != (u8)(144 * 7)) {
        printf("decals %02X %02X\n", images[0][71], images[2][0]);
        return 1;
    }
    /* The masked texels of slot 3 stay; the rest of its row loads. */
    if (images[3][0] != 0xEE || images[3][4] != (u8)((144 + 64 + 4) * 7)) {
        printf("mask %02X %02X\n", images[3][0], images[3][4]);
        return 1;
    }
    /* The palette follows the decals, and the lights take its first two colors. */
    if (palettes[VEHICLE_MATERIAL_HEADLIGHTS][1] != palettes[VEHICLE_MATERIAL_PAINT][1] ||
        palettes[VEHICLE_MATERIAL_REAR_LIGHTS][0] != palettes[VEHICLE_MATERIAL_PAINT][0] ||
        memcmp(palettes[VEHICLE_MATERIAL_PAINT], pak + 144 + 64 * 8, 0x20) || g_dwPakFileOffset ||
        readCalls != 11 || !g_abSavePakIdKnown[1] || g_aabSavePakIds[1][0] != 1) {
        printf("palette %04X %d\n", palettes[VEHICLE_MATERIAL_PAINT][0], (int)readCalls);
        return 1;
    }
    /* A save from the paint shop asks first, then overwrites, and returns to the previous tool. */
    done = FALSE;
    g_bPreviousPaintTool = PAINT_TOOL_BRUSH;
    g_bPaintTool = PAINT_TOOL_SAVE;
    images[0][1] = 0x99;
    NewFrame();
    result = ProcessDecalSaveLoad(&model, 1, PAINT_TOOL_SAVE, 0, &done);
    if (g_bPakDialogState != 2 || result) {
        return 1;
    }
    g_aPlayerInputState[1].buttons = INPUT_A;
    RunDialog(&model, PAINT_TOOL_SAVE, 0, &done, 8);
    if (g_bPakDialogState != 5) {
        printf("overwrite %d\n", g_bPakDialogState);
        return 1;
    }
    g_aPlayerInputState[1].buttons = INPUT_A;
    result = RunDialog(&model, PAINT_TOOL_SAVE, 0, &done, 40);
    if (result || !done || pak[1] != 0x99 || g_bPaintTool != PAINT_TOOL_BRUSH ||
        g_bDecalEditCount) {
        printf("save %d %d %02X\n", (int)result, done, pak[1]);
        return 1;
    }
    return 0;
}

int main(void) {
    static Sprite icons[DECAL_SLOTS];
    s32 i;

    for (i = 0; i < DECAL_SLOTS; i++) {
        g_apDecalSlotIcons[i] = &icons[i];
    }
    ResetPaintShopState();
    if (g_bDecalSlot != 2 || g_pDecalSlotIcon != &icons[2] || g_bDecalColor != 3 ||
        g_bPaintTool != PAINT_TOOL_BRUSH || g_texPaintCursor.originX != 244 || !g_bShowPreview3D ||
        !g_bDecalSlotHasMirror) {
        return 1;
    }
    return CheckBorder() || CheckErrorMessages() || CheckUndo() || CheckMirror() ||
           CheckKeyboard() || CheckSaveLoad();
}

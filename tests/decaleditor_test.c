/* Host check of the input, the colors and the menus of src/n64/decaleditor.c. It paints through
 * src/n64/decaldraw.c, and the drawing to the screen is stubbed. */
#include <assert.h>
#include <string.h>

#include "common/fade.h"
#include "common/input.h"
#include "common/render.h"
#include "common/system.h"
#include "common/text.h"
#include "common/vehicle.h"
#include "common/vehiclerender.h"
#include "decaldraw.h"
#include "decaleditor.h"
#include "pakmenu.h"

Sprite g_texPaintToolBrush, g_texPaintToolText, g_texPaintToolRect, g_texPaintToolOval;
Sprite g_texPaintToolClear, g_texPaintCursor, g_texKeyboardHighlight, g_texDecalFont;
Sprite g_texPaintShapeFilledCircle, g_texButtonA, g_texButtonB;
ScreenRect g_rcDecalCanvasFrame = {280, 94, 328, 260};
ScreenRect g_rcPaletteFrameLeft = {20, 400, 48, 22};
ScreenRect g_rcPaletteFrameRight = {80, 400, 232, 22};
ScreenRect g_aBrushSizeRects[BRUSH_SIZES] = {
    {0, 0, 1, 1}, {0, 0, 2, 2}, {0, 0, 4, 4}, {0, 0, 6, 6}, {0, 0, 8, 8}};
ScreenRect g_aBrushSizeButtonRects[BRUSH_SIZES];
DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];
u8 g_bDashColorA, g_bDashColorB = 0xFF, g_bBrushSize;
ScreenRect g_rcBrush;
Sprite *g_pDecalSlotIcon;
Sprite *g_apDecalSlotIcons[DECAL_SLOTS];
Sprite *g_apRectStyleIcons[4], *g_apOvalStyleIcons[4];
u8 g_bSavedColor, g_bDecalColor, g_bTextShadowColor, g_bPaintTool, g_bPreviousPaintTool;
u8 g_bDecalSlot, g_bPreviousDecalSlot, g_bSavedDecalSlot, g_bUndoDecalSlot;
u8 *g_pDecalTexels, *g_pDecalMask, *g_pDecalScratchTexels;
u8 g_bDecalWidth, g_bDecalHeight;
u16 *g_pDecalPalette;
ScreenRect g_rcDecalCanvas;
u8 g_bDecalTextLength, g_bDecalTextWidth, g_bDecalEditCount, g_bStyleMenuCursor;
u8 g_bPaintShopPlayer, g_bPaintToolStarted, g_bDecalDragActive, g_bPaletteEnabled = 1;
u8 g_bDecalSlotHasMirror, g_bDecalSlotChanged, g_bColorEditorOpen, g_bStyleMenuOpen;
u8 g_bTextShadowPickerOpen, g_bPaintShopExiting, g_bColorEditorChannel;
u8 g_aabColorEditorBars[3][3] = {{0xFF, 0, 0}, {0, 0xFF, 0}, {0, 0, 0xFF}};
const char *g_apszColorChannelNames[3] = {"R", "G", "B"};
const char *g_apszBrushStyleNames[2] = {"ROUND BRUSH", "SQUARE BRUSH"};
const char *g_apszTextStyleNames[2] = {"NORMAL TEXT", "DROP-SHADOW TEXT"};
const char *g_apszRectStyleNames[4] = {"A", "B", "C", "D"};
const char *g_apszOvalStyleNames[4] = {"A", "B", "C", "D"};
s32 g_nPaintAnchorX, g_nPaintAnchorY, g_dwAnchorXInitialized, g_dwAnchorYInitialized;
u32 g_dwLastClickTime;
PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];
ScreenRect g_aPaintToolButtonRects[PAINT_TOOLS];
u8 g_abSavedSwatchColor[3];
ScreenRect g_aDecalSlotRects[DECAL_SLOTS];
DecalGlyph *g_apDecalTextGlyphs[DECAL_TEXT_LENGTH];
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
VehicleModel *g_pCurrentVehicleModelData;
u16 g_wScreenFrameCount;
Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode;
s32 g_dwScreenScaleShift = 1;
u64 osClockRate = 1000000;

static VehicleModel model;
static ModelTexture textures[12];
static u16 palettes[12][16];
static u8 texels[0x800], *masks[12];
static u16 palette[16];
static Sprite icons[4];
static u32 count;
static s32 fades, saves, undoCalls, keyboards, pixels, lastBorderWidth;
static f32 fadeTarget;

u32 osGetCount(void) {
    return count;
}
void osSyncPrintf(const char *format, ...) {
}
void TransferFlags(PlayerInputState *state, u32 mask) {
    state->heldButtons |= state->buttons & mask;
    state->buttons &= ~mask;
}
void CheckAnalogStickPositive(PlayerInputState *state, s32 *timer, f32 *axis, u32 button) {
}
void CheckAnalogStickNegative(PlayerInputState *state, s32 *timer, f32 *axis, u32 button) {
}
void UpdateStickRepeat(PlayerInputState *state) {
}
void SetFadeTarget(float target, float seconds) {
    fades++;
    fadeTarget = target;
}
void SetTextStyleNormal(void) {
}
void SetTextAlignCenter(void) {
}
void SetTextAlignLeft(void) {
}
void SetTextAlignRight(void) {
}
void SetTextSize(s32 size) {
}
void SetTextCustomColors(s32 red, s32 green, s32 blue, s32 red2, s32 green2, s32 blue2) {
}
void DrawText(const char *text, s32 x, s32 y) {
}
void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y) {
}
void DrawSpriteAtOrigin(Sprite *sprite) {
}
void DrawSpriteRegion(
    Sprite *sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
}
void DrawRectBorder(s32 x,
                    s32 y,
                    s32 width,
                    s32 height,
                    s32 border,
                    u8 transparent,
                    u8 sunken,
                    u8 red,
                    u8 green,
                    u8 blue) {
    lastBorderWidth = width;
}
void DrawRectOutline(
    s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue) {
}
void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
}
void DrawPixel(s32 x, s32 y, u8 shade) {
    pixels++;
}
void DrawScaledTexturedRect(Sprite *sprite,
                            s32 srcX,
                            s32 srcY,
                            s32 srcWidth,
                            s32 srcHeight,
                            s32 x,
                            s32 y,
                            s32 width,
                            s32 height,
                            u8 red,
                            u8 green,
                            u8 blue) {
}
void SaveDecalUndo(void) {
    saves++;
}
void UndoDecalEdit(void) {
    undoCalls++;
}
void ProcessTextInputKeyboard(void) {
    keyboards++;
}

static PlayerInputState *input = &g_aPlayerInputState[1];

static void Press(u32 buttons) {
    input->buttons = buttons;
    input->heldButtons = 0;
}

static void Point(s32 x, s32 y) {
    g_texPaintCursor.originX = x;
    g_texPaintCursor.originY = y;
}

static void TestHits(void) {
    ScreenRect rect = {10, 20, 30, 40};
    PaletteSwatch swatch = {{10, 20, 30, 40}};
    DecalGlyph key = {10, 20};
    Sprite sprite;

    Point(10, 20);
    assert(IsCursorInRect(&rect) && IsCursorInSwatch(&swatch));
    Point(40, 60);
    assert(IsCursorInRect(&rect) && IsCursorInSwatch(&swatch));
    Point(41, 60);
    assert(!IsCursorInRect(&rect) && !IsCursorInSwatch(&swatch));
    Point(40, 61);
    assert(!IsCursorInRect(&rect));
    Point(9, 30);
    assert(!IsCursorInRect(&rect));

    g_texKeyboardHighlight.drawWidth = 46;
    g_texKeyboardHighlight.drawHeight = 36;
    Point(56, 56);
    assert(IsCursorOnKeyboardKey(&key));
    Point(57, 56);
    assert(!IsCursorOnKeyboardKey(&key));

    /* A flipped sprite counts its width as positive. */
    memset(&sprite, 0, sizeof(sprite));
    sprite.originX = 100;
    sprite.originY = 50;
    sprite.drawWidth = -20;
    sprite.drawHeight = 10;
    Point(120, 60);
    assert(IsCursorInButton(&sprite));
    Point(121, 60);
    assert(!IsCursorInButton(&sprite));
    Point(110, 61);
    assert(!IsCursorInButton(&sprite));
}

static void TestPalette(void) {
    g_pDecalPalette = palette;
    palette[0] = 0xF801;
    palette[5] = 0x0842;
    palette[6] = 0xFFFF;
    ConvertPaletteToRgb();
    assert(g_aDecalSwatches[0].color[0] == 0xFF && g_aDecalSwatches[0].color[1] == 0 &&
           g_aDecalSwatches[0].color[2] == 0);
    assert(g_aDecalSwatches[5].color[0] == 8 && g_aDecalSwatches[5].color[1] == 8 &&
           g_aDecalSwatches[5].color[2] == 8);
    assert(g_aDecalSwatches[6].color[2] == 0xFF);
    DrawDecalColorPalette();
}

static void TestColorEditor(void) {
    s32 i;

    g_bPaintShopPlayer = 1;
    g_pCurrentVehicleModelData = &model;
    g_bDecalColor = 5;
    g_bColorEditorChannel = 0;
    g_bColorEditorOpen = TRUE;
    input->stickX = 0.0f;
    /* The D-pad steps by 4, and the alpha bit of the entry stays. */
    palette[5] = 0x0843;
    g_aDecalSwatches[5].color[0] = 0x80;
    Press(INPUT_RIGHT);
    ProcessColorEditorDialog();
    assert(g_aDecalSwatches[5].color[0] == 0x84 && palette[5] == 0x8043);
    /* The stick steps by a tenth of its push, truncated, on the channel moved to. */
    input->stickX = -0.35f;
    g_bColorEditorChannel = 2;
    Press(INPUT_DOWN);
    ProcessColorEditorDialog();
    assert(g_bColorEditorChannel == 0 && g_aDecalSwatches[5].color[0] == 0x81);
    input->stickX = 0.0f;
    Press(INPUT_C_UP);
    ProcessColorEditorDialog();
    assert(g_bColorEditorChannel == 2);
    g_aDecalSwatches[5].color[2] = 253;
    Press(INPUT_RIGHT);
    ProcessColorEditorDialog();
    assert(g_aDecalSwatches[5].color[2] == 0xFF);
    g_aDecalSwatches[5].color[2] = 2;
    Press(INPUT_LEFT);
    ProcessColorEditorDialog();
    assert(g_aDecalSwatches[5].color[2] == 0);

    /* The first color sets the second at half, and both go into the lights of the car. */
    for (i = 0; i < 12; i++) {
        textures[i].palette = palettes[i];
    }
    model.textures = textures;
    model.materialTextures[VEHICLE_MATERIAL_HEADLIGHTS] = 10;
    model.materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS] = 11;
    g_bDecalColor = 0;
    g_bColorEditorChannel = 1;
    g_aDecalSwatches[0].color[0] = 0xFF;
    g_aDecalSwatches[0].color[1] = 0x40;
    g_aDecalSwatches[0].color[2] = 0x80;
    Press(INPUT_C_RIGHT);
    ProcessColorEditorDialog();
    assert(g_aDecalSwatches[0].color[1] == 0x41);
    assert(g_aDecalSwatches[1].color[0] == 0x7F && g_aDecalSwatches[1].color[1] == 0x20 &&
           g_aDecalSwatches[1].color[2] == 0x40);
    assert(palette[1] == ((0x78 << 8) | (0x20 << 3) | (0x40 >> 2)));
    assert(palettes[10][0] == palette[0] && palettes[10][1] == palette[1]);
    assert(palettes[11][0] == palette[0] && palettes[11][1] == 0);

    /* The second runs to 127 and sets the first at double. */
    g_bDecalColor = 1;
    g_bColorEditorChannel = 0;
    g_aDecalSwatches[1].color[0] = 0x7D;
    Press(INPUT_RIGHT);
    ProcessColorEditorDialog();
    assert(g_aDecalSwatches[1].color[0] == 0x7F && g_aDecalSwatches[0].color[0] == 0xFF);
    assert(g_aDecalSwatches[0].color[1] == 0x40);

    /* B restores the saved color. A accepts the color and counts an edit. */
    g_bDecalColor = 0;
    g_abSavedSwatchColor[0] = 0x10;
    g_abSavedSwatchColor[1] = 0x20;
    g_abSavedSwatchColor[2] = 0x30;
    Press(INPUT_B);
    ProcessColorEditorDialog();
    assert(!g_bColorEditorOpen && g_aDecalSwatches[0].color[0] == 0x10);
    assert(g_aDecalSwatches[1].color[1] == 0x10 && g_aDecalSwatches[1].color[2] == 0x18);
    assert(palette[0] == (1 | (0x10 << 8) | (0x20 << 3) | (0x30 >> 2)));
    g_bColorEditorOpen = TRUE;
    g_bColorEditorChannel = 2;
    g_bDecalEditCount = 0;
    Press(INPUT_A);
    ProcessColorEditorDialog();
    assert(!g_bColorEditorOpen && g_bColorEditorChannel == 0 && g_bDecalEditCount == 1);
    assert(input->heldButtons == INPUT_A && input->buttons == 0);
}

static void TestCursor(void) {
    g_rcDecalCanvas.x = 400;
    g_rcDecalCanvas.y = 100;
    g_rcDecalCanvas.width = 64;
    g_rcDecalCanvas.height = 64;
    Press(0);
    input->stickX = 1.0f;
    input->stickY = -0.5f;
    Point(100, 100);
    UpdateCursorFromInput();
    assert(g_texPaintCursor.originX == 112 && g_texPaintCursor.originY == 106);
    /* Over the decal the cursor is slower. */
    Point(420, 120);
    UpdateCursorFromInput();
    assert(g_texPaintCursor.originX == 426 && g_texPaintCursor.originY == 123);
    input->stickX = 0.04f;
    input->stickY = 0.0f;
    Press(INPUT_C_LEFT | INPUT_DOWN);
    UpdateCursorFromInput();
    assert(g_texPaintCursor.originX == 425 && g_texPaintCursor.originY == 124);
    Press(0);
    Point(1000, -5);
    UpdateCursorFromInput();
    assert(g_texPaintCursor.originX == 0x261 && g_texPaintCursor.originY == 0x16);
    Point(0, 1000);
    UpdateCursorFromInput();
    assert(g_texPaintCursor.originX == 0x1F && g_texPaintCursor.originY == 0x1CA);
    g_bPaintShopExiting = TRUE;
    Press(INPUT_RIGHT);
    UpdateCursorFromInput();
    assert(g_texPaintCursor.originX == 0x1F);
    g_bPaintShopExiting = FALSE;
    input->stickX = 0.0f;
}

static void TestEditor(void) {
    s32 i;

    memset(texels, 0, sizeof(texels));
    for (i = 0; i < DECAL_SLOTS; i++) {
        g_aDecalSlotRects[i].x = 10 + i * 20;
        g_aDecalSlotRects[i].y = 10;
        g_aDecalSlotRects[i].width = 16;
        g_aDecalSlotRects[i].height = 16;
        g_apDecalSlotIcons[i] = &icons[i & 3];
    }
    for (i = 0; i < PAINT_TOOLS; i++) {
        g_aPaintToolButtonRects[i].x = 10 + i * 20;
        g_aPaintToolButtonRects[i].y = 50;
        g_aPaintToolButtonRects[i].width = 16;
        g_aPaintToolButtonRects[i].height = 16;
    }
    for (i = 0; i < DECAL_SWATCHES; i++) {
        g_aDecalSwatches[i].rect.x = 80 + i * 14;
        g_aDecalSwatches[i].rect.y = 403;
        g_aDecalSwatches[i].rect.width = 12;
        g_aDecalSwatches[i].rect.height = 12;
    }
    textures[4].image = texels;
    textures[4].width = 16;
    textures[4].height = 8;
    model.materialTextures[4] = 4;
    model.decalMasks = masks;
    g_bDecalSlot = 2;

    /* The anchor is taken from the cursor once. */
    Point(5, 5);
    Press(0);
    ProcessDecalEditorInput();
    assert(g_dwAnchorXInitialized && g_nPaintAnchorX == 5 && g_nPaintAnchorY == 5);

    /* A decal clicked becomes the one edited, centered in its frame. */
    Point(10 + 4 * 20 + 3, 12);
    Press(INPUT_Z);
    ProcessDecalEditorInput();
    assert(g_bDecalSlot == 4 && g_bPreviousDecalSlot == 2 && g_bDecalSlotChanged);
    assert(g_pDecalTexels == texels && g_bDecalWidth == 16 && g_bDecalHeight == 8);
    assert(g_rcDecalCanvas.x == 280 + (328 - 64) / 2 && g_rcDecalCanvas.y == 94 + (260 - 32) / 2);
    assert(g_rcDecalCanvas.width == 64 && g_rcDecalCanvas.height == 32);
    assert(!g_bDecalSlotHasMirror && g_pDecalSlotIcon == &icons[0]);
    assert(input->buttons == 0 && input->heldButtons == INPUT_Z);
    g_bDecalSlotChanged = FALSE;

    /* A second click on a color within 350 ms opens the color editor. */
    g_bDecalColor = 3;
    count = 1000000;
    Point(80 + 7 * 14 + 2, 405);
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(g_bDecalColor == 7 && g_bSavedColor == 3 && g_dwLastClickTime == 1000);
    count = 1500000;
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(!g_bColorEditorOpen && g_dwLastClickTime == 1500);
    count = 1800000;
    g_aDecalSwatches[7].color[1] = 0x55;
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(g_bColorEditorOpen && g_abSavedSwatchColor[1] == 0x55 && g_dwLastClickTime == 1500);
    g_bColorEditorOpen = FALSE;

    /* A tool clicked twice opens the menu of its styles. */
    g_bPaintTool = PAINT_TOOL_LINE;
    Point(10 + PAINT_TOOL_RECTANGLE * 20, 50);
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(g_bPaintTool == PAINT_TOOL_RECTANGLE && g_bPreviousPaintTool == PAINT_TOOL_LINE);
    assert(!g_bStyleMenuOpen && g_dwLastClickTime == 1800);
    count = 1900000;
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(g_bStyleMenuOpen && g_bPaintToolStarted);
    g_bStyleMenuOpen = FALSE;
    /* Undo restores the decal and the tool before. */
    Point(10 + PAINT_TOOL_UNDO * 20, 50);
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(undoCalls == 1 && g_bPaintTool == PAINT_TOOL_RECTANGLE && !g_bPaintToolStarted);

    /* The smallest brush paints the texel under the cursor while the button is pressed. */
    g_bPaintTool = PAINT_TOOL_BRUSH;
    g_bBrushSize = 0;
    g_bDecalColor = 6;
    Point(g_rcDecalCanvas.x + 4 * 3 + 1, g_rcDecalCanvas.y + 32 - 4 * 2 - 1);
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(GetPixel4Bit(3, 2) == 6 && saves == 1 && g_bPaintToolStarted);
    assert(input->buttons == INPUT_A);
    /* Letting go ends the stroke. */
    Press(0);
    ProcessDecalEditorInput();
    assert(!g_bPaintToolStarted);

    /* A line from its first click to its second, the dashes drawn in between. */
    g_bPaintTool = PAINT_TOOL_LINE;
    Point(g_rcDecalCanvas.x + 1, g_rcDecalCanvas.y + 1);
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(g_bPaintToolStarted && g_nPaintAnchorX == g_rcDecalCanvas.x + 1);
    Point(g_rcDecalCanvas.x + 40, g_rcDecalCanvas.y + 1);
    Press(0);
    pixels = 0;
    ProcessDecalEditorInput();
    assert(pixels == 20);
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(!g_bPaintToolStarted && saves == 2 && GetPixel4Bit(5, 7) == 6);
    assert(GetPixel4Bit(10, 7) == 6 && GetPixel4Bit(11, 7) == 0);

    /* A click on the decal with the text tool opens the keyboard on the key of O. */
    g_bPaintTool = PAINT_TOOL_TEXT;
    g_aDecalGlyphs[24].x = 300;
    g_aDecalGlyphs[24].y = 200;
    Press(INPUT_A);
    ProcessDecalEditorInput();
    assert(keyboards == 1 && g_bDecalDragActive && g_texPaintCursor.originX == 323);
    assert(g_texPaintCursor.originY == 218 && g_nPaintAnchorX == g_rcDecalCanvas.x + 40);
    /* With nothing typed, the text tool ends. */
    g_bPaintToolStarted = TRUE;
    Press(0);
    ProcessDecalEditorInput();
    assert(!g_bPaintToolStarted && !g_bDecalDragActive);
}

static void TestMenus(void) {
    s32 i;

    /* The brush menu takes the style on its first frame, left or right toggles, and A sets it. */
    g_texPaintToolBrush.style = 1;
    g_bPaintToolStarted = TRUE;
    g_bStyleMenuOpen = TRUE;
    Press(INPUT_LEFT);
    ProcessBrushStyleMenu();
    assert(!g_bPaintToolStarted && g_bStyleMenuCursor == 0 && g_bStyleMenuOpen);
    Press(INPUT_A);
    ProcessBrushStyleMenu();
    assert(g_texPaintToolBrush.style == 0 && !g_bStyleMenuOpen);

    /* In the rectangle menu left goes around from the first style to the last. B does not set it.
     */
    g_texPaintToolRect.style = 0;
    g_bPaintToolStarted = TRUE;
    g_bStyleMenuOpen = TRUE;
    for (i = 0; i < 4; i++) {
        g_apRectStyleIcons[i] = &icons[i];
        g_apOvalStyleIcons[i] = &icons[i];
        icons[i].drawWidth = 40 + i;
    }
    Press(INPUT_LEFT);
    ProcessRectStyleMenu();
    assert(g_bStyleMenuCursor == 3);
    Press(INPUT_B);
    ProcessRectStyleMenu();
    assert(g_texPaintToolRect.style == 0 && !g_bStyleMenuOpen);
    g_bStyleMenuOpen = TRUE;
    Press(INPUT_RIGHT);
    ProcessRectStyleMenu();
    assert(g_bStyleMenuCursor == 0);
    Press(INPUT_RIGHT | INPUT_A);
    ProcessRectStyleMenu();
    assert(g_bStyleMenuCursor == 1 && g_texPaintToolRect.style == 1 && !g_bStyleMenuOpen);

    /* The oval menu places the icons on its first frame, the circles from the second ellipse. */
    g_texPaintToolOval.style = 2;
    g_bPaintToolStarted = TRUE;
    g_bStyleMenuOpen = TRUE;
    Press(INPUT_A);
    ProcessOvalStyleMenu();
    assert(icons[0].originX == 0xC8 && icons[1].originX == 0x110 && icons[0].originY == 0xCB);
    assert(icons[2].originX == 40 + 0x110 && icons[3].originX == 41 + 0x110 + 0x34);
    assert(g_texPaintToolOval.style == 2 && !g_bStyleMenuOpen);

    /* In the text menu A on the shadow opens its colors, and A there sets the style. */
    g_texPaintToolText.style = 0;
    g_bTextShadowColor = 7;
    g_bPaintToolStarted = TRUE;
    g_bStyleMenuOpen = TRUE;
    Press(INPUT_RIGHT);
    ProcessTextStyleMenu();
    assert(g_bStyleMenuCursor == 1 && g_bSavedColor == 7 && !g_bTextShadowPickerOpen);
    Press(INPUT_A);
    ProcessTextStyleMenu();
    assert(g_bTextShadowPickerOpen && g_bStyleMenuOpen);
    Press(INPUT_RIGHT);
    ProcessTextStyleMenu();
    assert(g_bTextShadowColor == 0);
    Press(INPUT_LEFT);
    ProcessTextStyleMenu();
    assert(g_bTextShadowColor == 7);
    Press(INPUT_DOWN);
    ProcessTextStyleMenu();
    assert(g_bTextShadowColor == 15);
    Press(INPUT_B);
    ProcessTextStyleMenu();
    assert(g_bTextShadowColor == 7 && !g_bTextShadowPickerOpen && g_bStyleMenuOpen);
    Press(INPUT_A);
    ProcessTextStyleMenu();
    Press(INPUT_A);
    ProcessTextStyleMenu();
    assert(g_texPaintToolText.style == 1 && !g_bStyleMenuOpen && !g_bTextShadowPickerOpen);
    /* A on normal text closes the menu, and the shadow stays on. */
    g_bStyleMenuOpen = TRUE;
    g_bPaintToolStarted = TRUE;
    Press(INPUT_LEFT);
    ProcessTextStyleMenu();
    Press(INPUT_A);
    ProcessTextStyleMenu();
    assert(g_bStyleMenuCursor == 0 && !g_bStyleMenuOpen && g_texPaintToolText.style == 1);
}

static void TestExit(void) {
    g_bDecalEditCount = 0;
    ProcessPaintShopExitConfirm();
    assert(g_bPaintShopExiting && fades == 1 && fadeTarget == 0.0f);
    ProcessPaintShopExitConfirm();
    assert(fades == 1);

    g_bPaintShopExiting = FALSE;
    g_bDecalEditCount = 2;
    g_bPaintTool = PAINT_TOOL_EXIT;
    g_bPreviousPaintTool = PAINT_TOOL_FILL;
    Press(0);
    ProcessPaintShopExitConfirm();
    assert(!g_bPaintShopExiting && lastBorderWidth == 0x102);
    Press(INPUT_B);
    ProcessPaintShopExitConfirm();
    assert(!g_bPaintShopExiting && g_bPaintTool == PAINT_TOOL_FILL);
    Press(INPUT_A);
    ProcessPaintShopExitConfirm();
    assert(g_bPaintShopExiting && fades == 2);

    g_bBrushSize = 3;
    g_bDecalSlot = 6;
    ResetBrushAndSlotHistory();
    assert(g_rcBrush.width == 6 && g_rcBrush.height == 6 && g_bPreviousDecalSlot == 6);
    assert(g_bSavedDecalSlot == 6 && g_bUndoDecalSlot == 6);
}

int main(void) {
    TestHits();
    TestPalette();
    TestColorEditor();
    TestCursor();
    TestEditor();
    TestMenus();
    TestExit();
    return 0;
}

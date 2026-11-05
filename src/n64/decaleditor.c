#include "decaleditor.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/fade.h"
#include "common/input.h"
#include "common/render.h"
#include "common/system.h"
#include "common/text.h"
#include "common/vehicle.h"
#include "common/vehiclerender.h"
#include "decaldraw.h"
#include "pakmenu.h"

/* The buttons that pick and paint. */
#define INPUT_PICK (INPUT_Z | INPUT_A)

/* How far the stick must be pushed to move the cursor or a bar of the color editor. */
#define CURSOR_DEAD_ZONE 0.05f
#define COLOR_DEAD_ZONE 0.1f

/* The pixels the cursor moves for the whole push of the stick, slower over the decal. */
#define CURSOR_SPEED 12.0f
#define CURSOR_SPEED_FINE 6.0f

/* The steps a bar of the color editor moves for the whole push of the stick. */
#define COLOR_SPEED 10.0f

/* Where the cursor may go. */
#define CURSOR_MIN_X 0x1F
#define CURSOR_MAX_X 0x261
#define CURSOR_MIN_Y 0x16
#define CURSOR_MAX_Y 0x1CA

/* The second color of the palette is the first at half brightness, and runs to 127. */
#define DARK_SWATCH 1
#define DARK_SWATCH_MAX 0x7F

/* The cell of the letter A in the font. The text menu shows its styles with it. */
#define PREVIEW_GLYPH 10

/* The highlight of the paint shop. */
#define HIGHLIGHT_RED 0x20
#define HIGHLIGHT_GREEN 0xC8
#define HIGHLIGHT_BLUE 0xFF

#define PaintShopInput() (&g_aPlayerInputState[g_bPaintShopPlayer])

#define GetTimeMilliseconds() ((u32)((u64)osGetCount() * 1000000 / osClockRate / 1000))

/* A color of the palette in RGBA 5551 from 8-bit channels, with the alpha of the old entry. */
#define PackPaletteColor(old, red, green, blue)                                                    \
    ((u16)(((old) & 1) | (((red) & 0xF8) << 8) | (((green) & 0xF8) << 3) | (((blue) & 0xF8) >> 2)))

#define MaterialPalette(material)                                                                  \
    (g_pCurrentVehicleModelData->textures[g_pCurrentVehicleModelData->materialTextures[material]]  \
         .palette)

/* The colours of the bars and the names of the channels, as the colour editor copies them. */
typedef struct {
    u8 color[3][3]; /*!< The red, green, and blue of the bar of each channel. */
} ColorEditorBars;

typedef struct {
    const char *name[3]; /*!< The names of the red, green, and blue channels. */
} ColorChannelNames;

/* The names of the styles of a tool, as its menu copies them. */
typedef struct {
    const char *name[2]; /*!< The name of each of the two styles. */
} TwoStyleNames;

typedef struct {
    const char *name[4]; /*!< The name of each of the four styles. */
} FourStyleNames;

/* Copy the first two colors of the palette into the palettes of the lights of the car. */
#define CopyLightPalettes()                                                                        \
    MaterialPalette(VEHICLE_MATERIAL_HEADLIGHTS)[0] = g_pDecalPalette[0];                          \
    MaterialPalette(VEHICLE_MATERIAL_HEADLIGHTS)[1] = g_pDecalPalette[1];                          \
    MaterialPalette(VEHICLE_MATERIAL_REAR_LIGHTS)[0] = g_pDecalPalette[0];                         \
    /* The second color of the rear lights is never set; the first is set again. */                \
    MaterialPalette(VEHICLE_MATERIAL_REAR_LIGHTS)[0] = g_pDecalPalette[0]

void ProcessColorEditorDialog(void) {
    PlayerInputState *input;
    PaletteSwatch *swatch;
    ColorEditorBars bars;
    ColorChannelNames names;
    char text[40];
    s32 cancelX, buttonY, step, channel, i, y, level;
    u32 buttons;

    /* The bars and the names are read from copies on the stack. */
    bars = *(ColorEditorBars *)g_aabColorEditorBars;
    names = *(ColorChannelNames *)g_apszColorChannelNames;
    cancelX = 0x151 - g_texButtonB.width;
    buttonY = 0x122 - g_texButtonA.height;
    step = 0;
    DrawRectBorder(0xAD, 0xB6, 0x126, 0x74, 3, 0, 0, 0x80, 0x80, 0x80);
    swatch = &g_aDecalSwatches[g_bDecalColor];
    DrawRectBorder(
        0xBB, 0xC2, 0x3C, 0x3C, 1, 0, 1, swatch->color[0], swatch->color[1], swatch->color[2]);

    /* The dark color fills a bar at 127; the others are shown at half. */
    channel = g_bColorEditorChannel;
    for (i = 0, y = 0xC2; i < 3; i++, y += 0x16) {
        if (i == channel) {
            DrawRectBorder(0x11D, y, 0x80, 0x10, 1, 0, 1, 0xB4, 0xB4, 0xB4);
        } else {
            DrawRectBorder(0x11D, y, 0x80, 0x10, 1, 1, 1, 0x80, 0x80, 0x80);
        }
        level = g_aDecalSwatches[g_bDecalColor].color[i];
        if (level <= 0) {
            continue;
        }
        if (g_bDecalColor == DARK_SWATCH) {
            DrawFilledRectHiRes(
                0x11D, y, level, 0x10, bars.color[i][0], bars.color[i][1], bars.color[i][2]);
            if (i == channel) {
                DrawRectBorder(0x11D,
                               y,
                               g_aDecalSwatches[g_bDecalColor].color[i],
                               0x10,
                               1,
                               0,
                               0,
                               bars.color[i][0],
                               bars.color[i][1],
                               bars.color[i][2]);
            }
        } else {
            DrawFilledRectHiRes(
                0x11D, y, level >> 1, 0x10, bars.color[i][0], bars.color[i][1], bars.color[i][2]);
            if (i == channel) {
                DrawRectBorder(0x11D,
                               y,
                               g_aDecalSwatches[g_bDecalColor].color[i] >> 1,
                               0x10,
                               1,
                               0,
                               0,
                               bars.color[i][0],
                               bars.color[i][1],
                               bars.color[i][2]);
            }
        }
    }

    for (i = 0, y = 0x68; i < 3; i++, y += 11) {
        if (i == g_bColorEditorChannel) {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0);
        } else {
            SetTextCustomColors(0xC8, 0xC8, 0xC8, 0x80, 0x80, 0x80);
        }
        SetTextSize(11);
        SetTextAlignLeft();
        DrawText(names.name[i], 0x83, y);
        SetTextSize(10);
        SetTextAlignRight();
        sprintf(text, "%d", g_aDecalSwatches[g_bDecalColor].color[i]);
        DrawText(text, 0xE2, y);
    }

    DrawTexturedSprite(&g_texButtonA, 0xE5, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
    SetTextAlignLeft();
    SetTextSize(10);
    DrawText("%wwOK", (g_texButtonA.width + 0xEB) >> 1, (buttonY + 0x12) >> 1);
    DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, (buttonY + 0x12) >> 1);

    input = PaintShopInput();
    CheckAnalogStickPositive(input, &input->repeatUp, &input->stickY, INPUT_UP);
    input = PaintShopInput();
    CheckAnalogStickNegative(input, &input->repeatDown, &input->stickY, INPUT_DOWN);
    input = PaintShopInput();
    if ((input->stickX < 0.0 ? -input->stickX : input->stickX) >= COLOR_DEAD_ZONE) {
        step = (s32)(input->stickX * COLOR_SPEED);
    } else if (input->buttons & INPUT_RIGHT) {
        step = 4;
    } else if (input->buttons & INPUT_LEFT) {
        step = -4;
    } else if (input->buttons & INPUT_C_RIGHT) {
        step = 1;
    } else if (input->buttons & INPUT_C_LEFT) {
        step = -1;
    }
    buttons = input->buttons;
    if (buttons & (INPUT_DOWN | INPUT_C_DOWN)) {
        TransferFlags(input, INPUT_DOWN | INPUT_C_DOWN);
        if (g_bColorEditorChannel == 2) {
            g_bColorEditorChannel = 0;
        } else {
            g_bColorEditorChannel++;
        }
    } else if (buttons & (INPUT_UP | INPUT_C_UP)) {
        TransferFlags(input, INPUT_UP | INPUT_C_UP);
        if (g_bColorEditorChannel == 0) {
            g_bColorEditorChannel = 2;
        } else {
            g_bColorEditorChannel--;
        }
    }

    /* Each branch indexes the bar afresh. */
    if (g_bDecalColor == DARK_SWATCH) {
        level = g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] + step;
        if (level < 0) {
            g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] = 0;
        } else if (level > DARK_SWATCH_MAX) {
            g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] = DARK_SWATCH_MAX;
        } else {
            g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] = level;
        }
    } else {
        level = g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] + step;
        if (level < 0) {
            g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] = 0;
        } else if (level > 0xFF) {
            g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] = 0xFF;
        } else {
            g_aDecalSwatches[g_bDecalColor].color[g_bColorEditorChannel] = level;
        }
    }

    if (step != 0) {
        swatch = &g_aDecalSwatches[g_bDecalColor];
        g_pDecalPalette[g_bDecalColor] = PackPaletteColor(
            g_pDecalPalette[g_bDecalColor], swatch->color[0], swatch->color[1], swatch->color[2]);
        /* The first two colors follow each other, the second at half the first. */
        if (g_bDecalColor == 0) {
            g_pDecalPalette[1] = PackPaletteColor(g_pDecalPalette[1],
                                                  g_aDecalSwatches[0].color[0] >> 1,
                                                  g_aDecalSwatches[0].color[1] >> 1,
                                                  g_aDecalSwatches[0].color[2] >> 1);
            for (i = 0; i < 3; i++) {
                g_aDecalSwatches[1].color[i] = g_aDecalSwatches[0].color[i] == 0xFF ?
                                                   DARK_SWATCH_MAX :
                                                   g_aDecalSwatches[0].color[i] >> 1;
            }
        } else if (g_bDecalColor == DARK_SWATCH) {
            g_pDecalPalette[0] = PackPaletteColor(g_pDecalPalette[0],
                                                  g_aDecalSwatches[1].color[0] << 1,
                                                  g_aDecalSwatches[1].color[1] << 1,
                                                  g_aDecalSwatches[1].color[2] << 1);
            for (i = 0; i < 3; i++) {
                g_aDecalSwatches[0].color[i] = g_aDecalSwatches[1].color[i] == DARK_SWATCH_MAX ?
                                                   0xFF :
                                                   g_aDecalSwatches[1].color[i] << 1;
            }
        }
        if (g_bDecalColor == 0 || g_bDecalColor == DARK_SWATCH) {
            CopyLightPalettes();
        }
    }

    input = PaintShopInput();
    buttons = input->buttons;
    if (buttons & INPUT_A) {
        TransferFlags(input, INPUT_A);
        g_bColorEditorChannel = 0;
        g_bColorEditorOpen = FALSE;
        g_bDecalEditCount++;
    } else if (buttons & INPUT_B) {
        TransferFlags(input, INPUT_B);
        g_pDecalPalette[g_bDecalColor] = PackPaletteColor(g_pDecalPalette[g_bDecalColor],
                                                          g_abSavedSwatchColor[0],
                                                          g_abSavedSwatchColor[1],
                                                          g_abSavedSwatchColor[2]);
        for (i = 0; i < 3; i++) {
            g_aDecalSwatches[g_bDecalColor].color[i] = g_abSavedSwatchColor[i];
        }
        /* The test is on the restored color, the value from the saved one. */
        if (g_bDecalColor == 0) {
            g_pDecalPalette[1] = PackPaletteColor(g_pDecalPalette[1],
                                                  g_abSavedSwatchColor[0] >> 1,
                                                  g_abSavedSwatchColor[1] >> 1,
                                                  g_abSavedSwatchColor[2] >> 1);
            for (i = 0; i < 3; i++) {
                g_aDecalSwatches[1].color[i] = g_aDecalSwatches[0].color[i] == 0xFF ?
                                                   DARK_SWATCH_MAX :
                                                   g_abSavedSwatchColor[i] >> 1;
            }
        } else if (g_bDecalColor == DARK_SWATCH) {
            g_pDecalPalette[0] = PackPaletteColor(g_pDecalPalette[0],
                                                  g_abSavedSwatchColor[0] << 1,
                                                  g_abSavedSwatchColor[1] << 1,
                                                  g_abSavedSwatchColor[2] << 1);
            for (i = 0; i < 3; i++) {
                g_aDecalSwatches[0].color[i] = g_aDecalSwatches[1].color[i] == DARK_SWATCH_MAX ?
                                                   0xFF :
                                                   g_abSavedSwatchColor[i] << 1;
            }
        }
        if (g_bDecalColor == 0 || g_bDecalColor == DARK_SWATCH) {
            CopyLightPalettes();
        }
        g_bColorEditorChannel = 0;
        g_bColorEditorOpen = FALSE;
    }
}

void UpdateCursorFromInput(void) {
    PlayerInputState *input;

    if (g_bPaintShopExiting) {
        return;
    }
    input = PaintShopInput();
    if ((input->stickX < 0.0 ? -input->stickX : input->stickX) >= CURSOR_DEAD_ZONE) {
        if (IsCursorInRect(&g_rcDecalCanvas) && !g_bDecalDragActive) {
            input = PaintShopInput();
            g_texPaintCursor.originX += (s32)(input->stickX * CURSOR_SPEED_FINE);
        } else {
            input = PaintShopInput();
            g_texPaintCursor.originX += (s32)(input->stickX * CURSOR_SPEED);
        }
    } else if (input->buttons & (INPUT_RIGHT | INPUT_C_RIGHT)) {
        g_texPaintCursor.originX++;
    } else if (input->buttons & (INPUT_LEFT | INPUT_C_LEFT)) {
        g_texPaintCursor.originX--;
    }

    /* Up on the stick is up the screen, where Y goes down. */
    if ((input->stickY < 0.0 ? -input->stickY : input->stickY) >= CURSOR_DEAD_ZONE) {
        if (IsCursorInRect(&g_rcDecalCanvas) && !g_bDecalDragActive) {
            g_texPaintCursor.originY -= (s32)(PaintShopInput()->stickY * CURSOR_SPEED_FINE);
        } else {
            g_texPaintCursor.originY -= (s32)(PaintShopInput()->stickY * CURSOR_SPEED);
        }
    } else if (input->buttons & (INPUT_DOWN | INPUT_C_DOWN)) {
        g_texPaintCursor.originY++;
    } else if (input->buttons & (INPUT_UP | INPUT_C_UP)) {
        g_texPaintCursor.originY--;
    }

    if (g_texPaintCursor.originX > CURSOR_MAX_X) {
        g_texPaintCursor.originX = CURSOR_MAX_X;
    } else if (g_texPaintCursor.originX < CURSOR_MIN_X) {
        g_texPaintCursor.originX = CURSOR_MIN_X;
    }
    if (g_texPaintCursor.originY < CURSOR_MIN_Y) {
        g_texPaintCursor.originY = CURSOR_MIN_Y;
    } else if (g_texPaintCursor.originY > CURSOR_MAX_Y) {
        g_texPaintCursor.originY = CURSOR_MAX_Y;
    }
}

void ProcessDecalEditorInput(void) {
    static int g_nPaintAnchorX; /* the drag anchor */
    static int g_nPaintAnchorY;
    static unsigned int g_dwLastClickTime; /* the last click, in milliseconds */
    static int g_dwAnchorXInitialized;     /* the anchor has been set */
    static int g_dwAnchorYInitialized;
    int j;
    int i;
    int w;
    int h;
    int r;
    unsigned int t;
    /* Unused locals preserve the original IDO stack layout. */
    int unusedFrameWord;
    int x;
    int y;
    int unusedFrameWords[3];
    char clicked;

    /* clang-format off */
    if (g_dwAnchorXInitialized != 0) { } else { g_dwAnchorXInitialized = 1; g_nPaintAnchorX = g_texPaintCursor.originX; }
    /* clang-format on */
    if (g_dwAnchorYInitialized == 0) {
        g_dwAnchorYInitialized = 1;
        g_nPaintAnchorY = g_texPaintCursor.originY;
    }
    if (g_bPaintShopExiting != 0) {
        return;
    }
    if (g_bDecalDragActive == 0 && g_bDecalSlotChanged == 0 &&
        (PaintShopInput()->buttons & INPUT_PICK)) {
        for (i = 0; i < 10; i++) {
            if (IsCursorInRect(&g_aDecalSlotRects[i])) {
                TransferFlags(PaintShopInput(), 0x8010);
                g_bPreviousDecalSlot = g_bDecalSlot;
                g_pDecalSlotIcon = g_apDecalSlotIcons[i];
                g_bDecalSlot = i;
                if (g_bDecalSlot != g_bPreviousDecalSlot) {
                    g_bDecalSlotChanged = 1;
                    g_wScreenFrameCount = 0;
                    g_pDecalTexels = g_pCurrentVehicleModelData
                                         ->textures[g_pCurrentVehicleModelData->materialTextures[i]]
                                         .image;
                    g_pDecalMask = g_pCurrentVehicleModelData->decalMasks[g_bDecalSlot];
                    g_bDecalWidth = g_pCurrentVehicleModelData
                                        ->textures[g_pCurrentVehicleModelData->materialTextures[i]]
                                        .width;
                    g_bDecalHeight = g_pCurrentVehicleModelData
                                         ->textures[g_pCurrentVehicleModelData->materialTextures[i]]
                                         .height;
#if !(TGR_REGION == TGR_REGION_JP)
                    osSyncPrintf("decal_width = %d\n", g_bDecalWidth);
                    osSyncPrintf("decal_height = %d\n", g_bDecalHeight);
#endif
                    w = (g_rcDecalCanvasFrame[0].width - g_bDecalWidth * 4) >> 1;
                    h = (g_rcDecalCanvasFrame[0].height - g_bDecalHeight * 4) >> 1;
                    g_rcDecalCanvas.x = g_rcDecalCanvasFrame[0].x + w;
                    g_rcDecalCanvas.y = g_rcDecalCanvasFrame[0].y + h;
                    g_rcDecalCanvas.width = g_bDecalWidth * 4;
                    g_rcDecalCanvas.height = g_bDecalHeight * 4;
                }
                if (g_bDecalSlot == 4 || g_bDecalSlot == 5 || g_bDecalSlot == 8 ||
                    g_bDecalSlot == 9) {
                    g_bDecalSlotHasMirror = 0;
                } else {
                    g_bDecalSlotHasMirror = 1;
                }
                return;
            }
        }
    }
    if ((IsCursorInRect(&g_rcPaletteFrameLeft) || IsCursorInRect(&g_rcPaletteFrameRight)) &&
        g_bPaletteEnabled != 0 && (PaintShopInput()->buttons & INPUT_PICK)) {
        for (j = 0; j < 16; j++) {
            if (IsCursorInSwatch(&g_aDecalSwatches[j])) {
                TransferFlags(PaintShopInput(), 0x8010);
                g_bSavedColor = g_bDecalColor;
                g_bDecalColor = j;
                if (g_bSavedColor != j) {
                    g_dwLastClickTime = GetTimeMilliseconds();
                    return;
                }
                t = GetTimeMilliseconds();
                if (t - g_dwLastClickTime < 350) {
                    for (j = 0; j < 3; j++) {
                        g_abSavedSwatchColor[j] = g_aDecalSwatches[g_bDecalColor].color[j];
                    }
                    g_bColorEditorOpen = 1;
                    return;
                }
                g_dwLastClickTime = t;
                return;
            }
        }
    }
    if (g_bDecalDragActive == 0) {
        for (i = 0; i < 13; i++) {
            if (IsCursorInRect(&g_aPaintToolButtonRects[i]) &&
                (PaintShopInput()->buttons & INPUT_PICK)) {
                TransferFlags(PaintShopInput(), 0x8010);
                g_bPaintToolStarted = 0;
                if (i != 0) {
                    g_bPreviousPaintTool = g_bPaintTool;
                    g_bPaintTool = i;
                    if (g_bPaintTool != g_bPreviousPaintTool) {
                        if (g_bPaintTool == 5 || g_bPaintTool == 6 || g_bPaintTool == 1 ||
                            g_bPaintTool == 4) {
                            g_dwLastClickTime = GetTimeMilliseconds();
                        }
                    } else if (g_bPaintTool == 5 || g_bPaintTool == 6 || g_bPaintTool == 1 ||
                               g_bPaintTool == 4) {
                        t = GetTimeMilliseconds();
                        if (t - g_dwLastClickTime < 350) {
                            g_bPaintToolStarted = 1;
                            g_bStyleMenuOpen = 1;
                            return;
                        }
                        g_dwLastClickTime = t;
                    }
                }
                switch (i) {
                case 0:
                    for (j = 0; j < 5; j++) {
                        if (IsCursorInRect(&g_aBrushSizeButtonRects[j])) {
                            g_bBrushSize = j;
                            return;
                        }
                    }
                    return;
                case 7:
                    g_bPaintToolStarted = 1;
                    return;
                case 11:
                    UndoDecalEdit();
                    g_bPaintTool = g_bPreviousPaintTool;
                    return;
                case 8:
                    g_texPaintToolClear.style = 0;
                    return;
                case 9:
                case 10:
                    g_bPaintToolStarted = 0;
                    return;
                case 12:
                    return;
                }
                return;
            }
        }
    }
    if (g_bPaintTool == 4) {
        clicked = 0;
        if (IsCursorInRect(&g_rcDecalCanvas) && g_bDecalDragActive == 0 &&
            (PaintShopInput()->buttons & INPUT_PICK)) {
            TransferFlags(PaintShopInput(), 0x8010);
            g_nPaintAnchorX = g_texPaintCursor.originX;
            g_nPaintAnchorY = g_texPaintCursor.originY;
            clicked = 1;
            g_bDecalDragActive = 1;
        }
        if (g_bDecalDragActive != 0 && g_bPaintToolStarted == 0) {
            if (clicked) {
                g_texPaintCursor.originX =
                    g_aDecalGlyphs[24].x + (g_texKeyboardHighlight.drawWidth >> 1);
                g_texPaintCursor.originY =
                    g_aDecalGlyphs[24].y + (g_texKeyboardHighlight.drawHeight >> 1);
                g_wScreenFrameCount = 0;
            }
            ProcessTextInputKeyboard();
            return;
        }
        if (g_bPaintToolStarted != 0 && !IsCursorInRect(&g_rcPaletteFrameLeft) &&
            !IsCursorInRect(&g_rcPaletteFrameRight) && !(PaintShopInput()->buttons & INPUT_PICK)) {
            if (g_bDecalTextLength > 0) {
                if (g_bTextShadowPickerOpen != 0) {
                    g_texPaintCursor.originX = g_nPaintAnchorX;
                    g_texPaintCursor.originY = g_nPaintAnchorY;
                    g_bTextShadowPickerOpen = 0;
                }
                DrawDecalTextPreview(g_texPaintCursor.originX, g_texPaintCursor.originY + 1);
                return;
            }
            if (g_bDecalTextLength == 0) {
                g_bPaintToolStarted = 0;
                g_bDecalDragActive = 0;
            }
            return;
        }
        if (g_bPaintToolStarted != 0 && (PaintShopInput()->buttons & INPUT_PICK)) {
            TransferFlags(PaintShopInput(), 0x8010);
            SaveDecalUndo();
            StampTextOnDecal(g_texPaintCursor.originX, g_texPaintCursor.originY + 1);
            g_bDecalTextLength = 0;
            g_bDecalTextWidth = 0;
            g_bPaintToolStarted = 0;
            g_bDecalDragActive = 0;
        }
    } else if (g_bPaintTool != 4 && IsCursorInRect(&g_rcDecalCanvas) &&
               (PaintShopInput()->buttons & INPUT_PICK)) {
        switch (g_bPaintTool) {
        case 1:
            if (g_bPaintToolStarted == 0) {
                SaveDecalUndo();
                g_bPaintToolStarted = 1;
            }
            i = g_bBrushSize;
            if (i == 0) {
                x = g_texPaintCursor.originX;
                y = g_texPaintCursor.originY;
                /* clang-format off */
                x -= g_rcDecalCanvas.x;
                x >>= 2; y = g_rcDecalCanvas.y + g_rcDecalCanvas.height - y; y >>= 2;
                /* clang-format on */
                SetPixel4Bit(x, y, g_bDecalColor);
            } else if (g_texPaintToolBrush.style == 0) {
                FillCircle4Bit(g_texPaintCursor.originX,
                               g_texPaintCursor.originY,
                               g_aBrushSizeRects[i].width >> 1,
                               1);
            } else {
                r = g_aBrushSizeRects[i].width * 2;
                FillRect4Bit(g_texPaintCursor.originX - r,
                             g_texPaintCursor.originY - r,
                             g_texPaintCursor.originX + r,
                             g_texPaintCursor.originY + r);
            }
            return;
        case 3:
            if (g_bPaintToolStarted == 0) {
                g_nPaintAnchorX = g_texPaintCursor.originX;
                g_nPaintAnchorY = g_texPaintCursor.originY;
                g_bPaintToolStarted = 1;
            } else {
                SaveDecalUndo();
                DrawLine4Bit(g_nPaintAnchorX,
                             g_nPaintAnchorY,
                             g_texPaintCursor.originX,
                             g_texPaintCursor.originY);
                g_bPaintToolStarted = 0;
            }
            TransferFlags(PaintShopInput(), 0x8010);
            return;
        case 2:
            SaveDecalUndo();
            FloodFill4Bit(g_texPaintCursor.originX, g_texPaintCursor.originY);
            TransferFlags(PaintShopInput(), 0x8010);
            return;
        case 5:
            if (g_bPaintToolStarted == 0) {
                g_nPaintAnchorX = g_texPaintCursor.originX;
                g_nPaintAnchorY = g_texPaintCursor.originY;
                g_bPaintToolStarted = 1;
            } else {
                SaveDecalUndo();
                if (g_texPaintToolRect.style == 0) {
                    FillRect4Bit(g_nPaintAnchorX,
                                 g_nPaintAnchorY,
                                 g_texPaintCursor.originX,
                                 g_texPaintCursor.originY);
                } else if (g_texPaintToolRect.style == 1) {
                    StrokeDecalRectangle(g_nPaintAnchorX,
                                         g_nPaintAnchorY,
                                         g_texPaintCursor.originX,
                                         g_texPaintCursor.originY);
                } else if (g_texPaintToolRect.style == 2) {
                    FillDecalRoundRect(g_nPaintAnchorX,
                                       g_nPaintAnchorY,
                                       g_texPaintCursor.originX,
                                       g_texPaintCursor.originY);
                } else if (g_texPaintToolRect.style == 3) {
                    DrawDecalRoundedRectOutline(g_nPaintAnchorX,
                                                g_nPaintAnchorY,
                                                g_texPaintCursor.originX,
                                                g_texPaintCursor.originY);
                }
                g_bPaintToolStarted = 0;
            }
            TransferFlags(PaintShopInput(), 0x8010);
            return;
        case 6:
            if (0 == g_bPaintToolStarted) {
                g_nPaintAnchorX = g_texPaintCursor.originX;
                g_nPaintAnchorY = g_texPaintCursor.originY;
                g_bPaintToolStarted = 1;
            } else {
                SaveDecalUndo();
                if (g_texPaintToolOval.style == 2 || g_texPaintToolOval.style == 3) {
                    x = g_texPaintCursor.originX - g_nPaintAnchorX;
                    y = g_texPaintCursor.originY - g_nPaintAnchorY;
                    r = sqrtf(x * x + y * y);
                    r >>= 2;
                }
                if (g_texPaintToolOval.style == 0) {
                    DrawDecalFilledEllipse(g_nPaintAnchorX,
                                           g_nPaintAnchorY,
                                           g_texPaintCursor.originX,
                                           g_texPaintCursor.originY);
                } else if (g_texPaintToolOval.style == 1) {
                    DrawDecalEllipseOutline(g_nPaintAnchorX,
                                            g_nPaintAnchorY,
                                            g_texPaintCursor.originX,
                                            g_texPaintCursor.originY);
                } else if (g_texPaintToolOval.style == 2) {
                    FillCircle4Bit(g_nPaintAnchorX, g_nPaintAnchorY, r, 1);
                } else if (g_texPaintToolOval.style == 3) {
                    DrawDecalCircleOutline(g_nPaintAnchorX, g_nPaintAnchorY, r);
                }
                g_bPaintToolStarted = 0;
            }
            TransferFlags(PaintShopInput(), 0x8010);
            return;
        }
    } else if (g_bPaintTool != 4 && g_bPaintToolStarted != 0 && IsCursorInRect(&g_rcDecalCanvas) &&
               !(PaintShopInput()->buttons & INPUT_PICK)) {
        switch (g_bPaintTool) {
        case 1:
            g_bPaintToolStarted = 0;
            return;
        case 3:
            DrawDashedLine(g_nPaintAnchorX,
                           g_nPaintAnchorY,
                           g_texPaintCursor.originX,
                           g_texPaintCursor.originY);
            return;
        case 5:
            if (g_texPaintToolRect.style == 0 || g_texPaintToolRect.style == 1) {
                DrawDashedRectangle(g_nPaintAnchorX,
                                    g_nPaintAnchorY,
                                    g_texPaintCursor.originX,
                                    g_texPaintCursor.originY);
                return;
            }
            DrawDashedRoundedRect(g_nPaintAnchorX,
                                  g_nPaintAnchorY,
                                  g_texPaintCursor.originX,
                                  g_texPaintCursor.originY);
            return;
        case 6:
            if (g_texPaintToolOval.style == 0 || g_texPaintToolOval.style == 1) {
                DrawDashedEllipse(g_nPaintAnchorX,
                                  g_nPaintAnchorY,
                                  g_texPaintCursor.originX,
                                  g_texPaintCursor.originY);
                return;
            }
            x = g_texPaintCursor.originX - g_nPaintAnchorX;
            y = g_texPaintCursor.originY - g_nPaintAnchorY;
            r = sqrtf(x * x + y * y);
            DrawDashedCircle(g_nPaintAnchorX, g_nPaintAnchorY, r);
            return;
        }
    }
}

s32 IsCursorInButton(Sprite *sprite) {
    if (g_texPaintCursor.originX >= sprite->originX &&
        (sprite->drawWidth > 0 ? sprite->drawWidth : -sprite->drawWidth) + sprite->originX >=
            g_texPaintCursor.originX &&
        g_texPaintCursor.originY >= sprite->originY &&
        sprite->originY + sprite->drawHeight >= g_texPaintCursor.originY) {
        return TRUE;
    }
    return FALSE;
}

s32 IsCursorInRect(ScreenRect *rect) {
    if (g_texPaintCursor.originX >= rect->x && rect->x + rect->width >= g_texPaintCursor.originX &&
        g_texPaintCursor.originY >= rect->y && rect->y + rect->height >= g_texPaintCursor.originY) {
        return TRUE;
    }
    return FALSE;
}

s32 IsCursorInSwatch(PaletteSwatch *swatch) {
    if (g_texPaintCursor.originX >= swatch->rect.x &&
        swatch->rect.x + swatch->rect.width >= g_texPaintCursor.originX &&
        g_texPaintCursor.originY >= swatch->rect.y &&
        swatch->rect.y + swatch->rect.height >= g_texPaintCursor.originY) {
        return TRUE;
    }
    return FALSE;
}

s32 IsCursorOnKeyboardKey(DecalGlyph *key) {
    if (g_texPaintCursor.originX >= key->x &&
        key->x + g_texKeyboardHighlight.drawWidth >= g_texPaintCursor.originX &&
        g_texPaintCursor.originY >= key->y &&
        key->y + g_texKeyboardHighlight.drawHeight >= g_texPaintCursor.originY) {
        return TRUE;
    }
    return FALSE;
}

void ConvertPaletteToRgb(void) {
    s32 i;

    /* Each 5-bit channel fills the 8 bits by repeating its highest bits. */
    for (i = 0; i < DECAL_SWATCHES; i++) {
        g_aDecalSwatches[i].color[0] =
            ((g_pDecalPalette[i] >> 8) & 0xF8) | ((g_pDecalPalette[i] >> 13) & 7);
        g_aDecalSwatches[i].color[1] =
            ((g_pDecalPalette[i] >> 3) & 0xF8) | ((g_pDecalPalette[i] >> 8) & 7);
        g_aDecalSwatches[i].color[2] =
            ((g_pDecalPalette[i] << 2) & 0xF8) | ((g_pDecalPalette[i] >> 3) & 7);
    }
}

void DrawDecalColorPalette(void) {
    PaletteSwatch *swatch;
    s32 i;

    DrawFilledRectHiRes(g_rcPaletteFrameLeft.x,
                        g_rcPaletteFrameLeft.y,
                        g_rcPaletteFrameLeft.width,
                        g_rcPaletteFrameLeft.height,
                        0,
                        0,
                        0);
    DrawFilledRectHiRes(g_rcPaletteFrameRight.x,
                        g_rcPaletteFrameRight.y,
                        g_rcPaletteFrameRight.width,
                        g_rcPaletteFrameRight.height,
                        0,
                        0,
                        0);
    for (i = 0; i < DECAL_SWATCHES; i++) {
        swatch = &g_aDecalSwatches[i];
        DrawFilledRectHiRes(swatch->rect.x,
                            swatch->rect.y,
                            swatch->rect.width,
                            swatch->rect.height,
                            swatch->color[0],
                            swatch->color[1],
                            swatch->color[2]);
    }
    swatch = &g_aDecalSwatches[g_bDecalColor];
    DrawRectOutline(swatch->rect.x - 2,
                    swatch->rect.y - 2,
                    swatch->rect.width + 4,
                    swatch->rect.height + 4,
                    2,
                    HIGHLIGHT_RED,
                    HIGHLIGHT_GREEN,
                    HIGHLIGHT_BLUE);
}

/* The Japanese style menus show no name of the chosen style. */
#if TGR_REGION == TGR_REGION_JP
#define SHOWS_STYLE_NAMES 0
#else
#define SHOWS_STYLE_NAMES 1
#endif

void ProcessBrushStyleMenu(void) {
    /* The unused area preserves the original stack frame. */
    char spare[28];
    s32 buttonY, cancelX;
#if SHOWS_STYLE_NAMES
    TwoStyleNames names;
#endif

    (void)spare;
#if SHOWS_STYLE_NAMES
    /* The names are read from a copy on the stack. */
    names = *(TwoStyleNames *)g_apszBrushStyleNames;
#endif
    /* The button A is placed by its width. */
    buttonY = 0x142 - g_texButtonA.width;
    cancelX = 0x15D - g_texButtonB.width;
    DrawRectBorder(0xC8, 0x96, 0xF0, 0xB4, 3, 0, 0, 0x80, 0x80, 0x80);
    if (g_bPaintToolStarted) {
        g_bStyleMenuCursor = g_texPaintToolBrush.style;
        g_bPaintToolStarted = FALSE;
    }
    SetTextSize(15);
    SetTextAlignCenter();
    SetTextStyleNormal();
    DrawText("%rySELECT STYLE", 0x9F, 0x5B);
    if (g_bStyleMenuCursor == 0) {
        DrawSpriteRegion(&g_texPaintShapeFilledCircle,
                         0xFA,
                         0xC8,
                         g_texPaintShapeFilledCircle.drawWidth,
                         g_texPaintShapeFilledCircle.drawHeight,
                         HIGHLIGHT_RED,
                         HIGHLIGHT_GREEN,
                         HIGHLIGHT_BLUE);
    } else {
        DrawSpriteRegion(&g_texPaintShapeFilledCircle,
                         0xFA,
                         0xC8,
                         g_texPaintShapeFilledCircle.drawWidth,
                         g_texPaintShapeFilledCircle.drawHeight,
                         0,
                         0,
                         0);
    }
    if (g_bStyleMenuCursor == 1) {
        DrawFilledRectHiRes(g_texPaintShapeFilledCircle.drawWidth + 0x114,
                            0xCA,
                            0x26,
                            0x26,
                            HIGHLIGHT_RED,
                            HIGHLIGHT_GREEN,
                            HIGHLIGHT_BLUE);
    } else {
        DrawFilledRectHiRes(
            g_texPaintShapeFilledCircle.drawWidth + 0x114, 0xCA, 0x26, 0x26, 0, 0, 0);
    }
#if SHOWS_STYLE_NAMES
    DrawRectBorder(0xE0, 0xFF, 0xC0, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
#endif
    SetTextSize(11);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#if SHOWS_STYLE_NAMES
    DrawText(names.name[g_bStyleMenuCursor], 0x9F, 0x8A);
#endif
    DrawTexturedSprite(&g_texButtonA, 0xD8, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
    SetTextSize(10);
    SetTextAlignLeft();
    DrawText("%wwSELECT", (g_texButtonA.width + 0xDE) >> 1, (buttonY + 0x12) >> 1);
    DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, (buttonY + 0x12) >> 1);

    UpdateStickRepeat(PaintShopInput());
    if (PaintShopInput()->buttons & (INPUT_RIGHT | INPUT_LEFT)) {
        TransferFlags(PaintShopInput(), INPUT_RIGHT | INPUT_LEFT);
        g_bStyleMenuCursor ^= 1;
    }
    if (PaintShopInput()->buttons & INPUT_A) {
        TransferFlags(PaintShopInput(), INPUT_A);
        g_texPaintToolBrush.style = g_bStyleMenuCursor;
        g_bStyleMenuOpen = FALSE;
    } else if (PaintShopInput()->buttons & INPUT_B) {
        TransferFlags(PaintShopInput(), INPUT_B);
        g_bStyleMenuOpen = FALSE;
    }
}

void ProcessTextStyleMenu(void) {
    PaletteSwatch swatches[DECAL_SWATCHES];
    ScreenRect choices[2];
#if SHOWS_STYLE_NAMES
    TwoStyleNames names;
#endif
    PlayerInputState *input;
    PaletteSwatch *swatch;
    s32 i, buttonY, cancelX, textY, selectX;

#if SHOWS_STYLE_NAMES
    /* The names are read from a copy on the stack. */
    names = *(TwoStyleNames *)g_apszTextStyleNames;
#endif
    DrawRectBorder(0xB4, 0x82, 0x118, 0xD0, 3, 0, 0, 0x80, 0x80, 0x80);
    SetTextSize(16);
    SetTextAlignCenter();
    SetTextStyleNormal();
    DrawText("%rySELECT OPTION", 0x9F, 0x53);
    if (g_bPaintToolStarted) {
        g_bStyleMenuCursor = g_texPaintToolText.style;
        g_bSavedColor = g_bTextShadowColor;
        g_bPaintToolStarted = FALSE;
        g_bTextShadowPickerOpen = FALSE;
    }
    choices[0].x = 0xF0;
    choices[0].y = 0xB8;
    choices[0].width = 0x40;
    choices[0].height = 0x40;
    choices[1].x = 0x150;
    choices[1].y = 0xB8;
    choices[1].width = 0x40;
    choices[1].height = 0x40;
    for (i = 0; i < 2; i++) {
        /* The width is passed as the height too. */
        if (i == g_bStyleMenuCursor) {
            DrawRectOutline(choices[i].x,
                            choices[i].y,
                            choices[i].width,
                            choices[i].width,
                            1,
                            HIGHLIGHT_RED,
                            HIGHLIGHT_GREEN,
                            HIGHLIGHT_BLUE);
        } else {
            DrawRectOutline(choices[i].x,
                            choices[i].y,
                            choices[i].width,
                            choices[i].width,
                            1,
                            0x20,
                            0x20,
                            0x20);
        }
    }
    swatch = &g_aDecalSwatches[g_bDecalColor];
    DrawScaledTexturedRect(&g_texDecalFont,
                           PREVIEW_GLYPH * 16,
                           0,
                           16,
                           16,
                           choices[0].x + 7,
                           choices[0].y + 10,
                           0x30,
                           0x30,
                           swatch->color[0],
                           swatch->color[1],
                           swatch->color[2]);
    swatch = &g_aDecalSwatches[g_bTextShadowColor];
    DrawScaledTexturedRect(&g_texDecalFont,
                           PREVIEW_GLYPH * 16,
                           0,
                           16,
                           16,
                           choices[1].x + 9,
                           choices[1].y + 12,
                           0x30,
                           0x30,
                           swatch->color[0],
                           swatch->color[1],
                           swatch->color[2]);
    swatch = &g_aDecalSwatches[g_bDecalColor];
    DrawScaledTexturedRect(&g_texDecalFont,
                           PREVIEW_GLYPH * 16,
                           0,
                           16,
                           16,
                           choices[1].x + 5,
                           choices[1].y + 8,
                           0x30,
                           0x30,
                           swatch->color[0],
                           swatch->color[1],
                           swatch->color[2]);

    if (!g_bTextShadowPickerOpen) {
#if SHOWS_STYLE_NAMES
        DrawRectBorder(
            0xC6, choices[0].height + choices[0].y + 0x11, 0xF4, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
#endif
        SetTextSize(11);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#if SHOWS_STYLE_NAMES
        DrawText(
            names.name[g_bStyleMenuCursor], 0x9E, (choices[1].height + choices[0].y + 0x26) >> 1);
#endif
        SetTextSize(10);
        SetTextAlignLeft();
        /* The button A is placed by its width and the button B by its height. */
        buttonY = 0x14A - g_texButtonA.width;
        cancelX = 0x160 - g_texButtonB.height;
        textY = (buttonY + 0x12) >> 1;
        DrawText("%wwSELECT", (g_texButtonA.width + 0xDC) >> 1, textY);
        DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, textY);
        DrawTexturedSprite(&g_texButtonA, 0xD6, buttonY);
        DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
        UpdateStickRepeat(PaintShopInput());
        input = PaintShopInput();
        if (input->buttons & (INPUT_RIGHT | INPUT_LEFT)) {
            TransferFlags(input, INPUT_RIGHT | INPUT_LEFT);
            g_bStyleMenuCursor ^= 1;
        }
    } else {
        DrawRectBorder(0xC6, 0x106, 0x116, 0x84, 2, 0, 0, 0x80, 0x80, 0x80);
        SetTextSize(12);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        SetTextAlignCenter();
        DrawText("SELECT SHADOW COLOR", 0xA7, 0x92);
        /* Two rows of eight. */
        for (i = 0; i < DECAL_SWATCHES; i++) {
            swatches[i].rect.width = 0x14;
            swatches[i].rect.height = 0x10;
            swatches[i].rect.x = (i % 8) * 30 + 0xDE;
            swatches[i].rect.y = (i >> 3) * 26 + 0x134;
            DrawRectBorder(swatches[i].rect.x,
                           swatches[i].rect.y,
                           0x14,
                           0x10,
                           1,
                           0,
                           1,
                           g_aDecalSwatches[i].color[0],
                           g_aDecalSwatches[i].color[1],
                           g_aDecalSwatches[i].color[2]);
        }
        swatch = &swatches[g_bTextShadowColor];
        DrawRectOutline(swatch->rect.x - 3,
                        swatch->rect.y - 3,
                        swatch->rect.width + 6,
                        swatch->rect.height + 6,
                        1,
                        HIGHLIGHT_RED,
                        HIGHLIGHT_GREEN,
                        HIGHLIGHT_BLUE);
        buttonY = 0x182 - g_texButtonA.height;
        cancelX = 0x16E - g_texButtonB.height;
        /* The position of the button A is stored on the stack for its label. */
        selectX = 0xE6;
        DrawTexturedSprite(&g_texButtonA, selectX, buttonY);
        DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
        SetTextSize(10);
        SetTextAlignLeft();
        textY = (buttonY + 0x12) >> 1;
        DrawText("%wwSELECT", (selectX + g_texButtonA.width + 6) >> 1, textY);
        DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, textY);

        UpdateStickRepeat(PaintShopInput());
        input = PaintShopInput();
        if (input->buttons & INPUT_LEFT) {
            TransferFlags(input, INPUT_LEFT);
            if (g_bTextShadowColor == 0) {
                g_bTextShadowColor = 7;
            } else if (g_bTextShadowColor == 8) {
                g_bTextShadowColor = 15;
            } else {
                g_bTextShadowColor--;
            }
        } else if (input->buttons & INPUT_RIGHT) {
            TransferFlags(input, INPUT_RIGHT);
            if (g_bTextShadowColor == 7) {
                g_bTextShadowColor = 0;
            } else if (g_bTextShadowColor == 15) {
                g_bTextShadowColor = 8;
            } else {
                g_bTextShadowColor++;
            }
        } else if (input->buttons & (INPUT_DOWN | INPUT_UP)) {
            TransferFlags(input, INPUT_DOWN | INPUT_UP);
            if (g_bTextShadowColor < 8) {
                g_bTextShadowColor += 8;
            } else {
                g_bTextShadowColor -= 8;
            }
        }
        input = PaintShopInput();
        if (input->buttons & INPUT_A) {
            TransferFlags(input, INPUT_A);
            g_texPaintToolText.style = g_bStyleMenuCursor;
            g_bTextShadowPickerOpen = FALSE;
            g_bStyleMenuOpen = FALSE;
        } else if (input->buttons & INPUT_B) {
            TransferFlags(input, INPUT_B);
            g_bTextShadowColor = g_bSavedColor;
            g_bTextShadowPickerOpen = FALSE;
        }
    }

    /* A on the drop shadow opens its colors. A on normal text closes the menu without setting the
     * style, and a drop shadow stays on. Past the colors, the buttons are already taken. */
    input = PaintShopInput();
    if (input->buttons & INPUT_A) {
        TransferFlags(input, INPUT_A);
        if (g_bStyleMenuCursor) {
            g_bTextShadowPickerOpen = TRUE;
        } else {
            g_bStyleMenuOpen = FALSE;
        }
    } else if (input->buttons & INPUT_B) {
        TransferFlags(input, INPUT_B);
        g_bStyleMenuOpen = FALSE;
        g_bTextShadowPickerOpen = FALSE;
    }
}

void ProcessRectStyleMenu(void) {
    char spare[24];
    s32 x, i, buttonY, cancelX;
#if SHOWS_STYLE_NAMES
    FourStyleNames names;
#endif

    (void)spare;
    (void)x;

#if SHOWS_STYLE_NAMES
    /* The names are read from a copy on the stack. */
    names = *(FourStyleNames *)g_apszRectStyleNames;
#endif
    /* Both buttons are placed by their widths. */
    buttonY = 0x143 - g_texButtonA.width;
    cancelX = 0x161 - g_texButtonB.width;
    DrawRectBorder(0x9F, 0x95, 0x142, 0xB6, 3, 0, 0, 0x80, 0x80, 0x80);
    SetTextSize(16);
    SetTextAlignCenter();
    SetTextStyleNormal();
    DrawText("%rySELECT STYLE", 0x9F, 0x5C);
    if (g_bPaintToolStarted) {
        g_bStyleMenuCursor = g_texPaintToolRect.style;
        g_bPaintToolStarted = FALSE;
    }
    for (i = 0; i < 4; i++) {
        if (i == g_bStyleMenuCursor) {
            DrawSpriteRegion(g_apRectStyleIcons[i],
                             i * 76 + 175,
                             0xCB,
                             g_apRectStyleIcons[i]->drawWidth,
                             g_apRectStyleIcons[i]->drawHeight,
                             HIGHLIGHT_RED,
                             HIGHLIGHT_GREEN,
                             HIGHLIGHT_BLUE);
        } else {
            DrawTexturedSprite(g_apRectStyleIcons[i], i * 76 + 175, 0xCB);
        }
    }
#if SHOWS_STYLE_NAMES
    DrawRectBorder(0xB1, 0x103, 0x11E, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
#endif
    SetTextSize(11);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#if SHOWS_STYLE_NAMES
    DrawText(names.name[g_bStyleMenuCursor], 0x9F, 0x8C);
#endif
    SetTextSize(10);
    SetTextAlignLeft();
    DrawText("%wwSELECT", (g_texButtonA.width + 0xDB) >> 1, (buttonY + 0x12) >> 1);
    DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, (buttonY + 0x12) >> 1);
    DrawTexturedSprite(&g_texButtonA, 0xD5, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);

    UpdateStickRepeat(PaintShopInput());
    if (PaintShopInput()->buttons & INPUT_LEFT) {
        TransferFlags(PaintShopInput(), INPUT_LEFT);
        if (g_bStyleMenuCursor == 0) {
            g_bStyleMenuCursor = 3;
        } else {
            g_bStyleMenuCursor--;
        }
    } else if (PaintShopInput()->buttons & INPUT_RIGHT) {
        TransferFlags(PaintShopInput(), INPUT_RIGHT);
        if (g_bStyleMenuCursor == 3) {
            g_bStyleMenuCursor = 0;
        } else {
            g_bStyleMenuCursor++;
        }
    }
    if (PaintShopInput()->buttons & INPUT_A) {
        TransferFlags(PaintShopInput(), INPUT_A);
        g_texPaintToolRect.style = g_bStyleMenuCursor;
        g_bStyleMenuOpen = FALSE;
    } else if (PaintShopInput()->buttons & INPUT_B) {
        TransferFlags(PaintShopInput(), INPUT_B);
        g_bStyleMenuOpen = FALSE;
    }
}

void ProcessOvalStyleMenu(void) {
    char spare[24];
    s32 x, i, buttonY, cancelX;
#if SHOWS_STYLE_NAMES
    FourStyleNames names;
#endif

    (void)spare;
    (void)x;

#if SHOWS_STYLE_NAMES
    /* The names are read from a copy on the stack. */
    names = *(FourStyleNames *)g_apszOvalStyleNames;
#endif
    buttonY = 0x143 - g_texButtonA.width;
    cancelX = 0x160 - g_texButtonB.width;
    DrawRectBorder(0xB8, 0x95, 0x110, 0xB6, 3, 0, 0, 0x80, 0x80, 0x80);
    SetTextSize(16);
    SetTextAlignCenter();
    SetTextStyleNormal();
    DrawText("%rySELECT STYLE", 0xA0, 0x5B);
    if (g_bPaintToolStarted) {
        g_bStyleMenuCursor = g_texPaintToolOval.style;
        for (i = 0; i * 72 + 200 < 344; i++) {
            g_apOvalStyleIcons[i]->originX = i * 72 + 200;
            g_apOvalStyleIcons[i]->originY = 0xCB;
        }
        /* The circles follow the ellipses, each placed from the second ellipse whatever its
         * column. */
        for (i = 0; i < 2; i++) {
            g_apOvalStyleIcons[i + 2]->originX =
                g_apOvalStyleIcons[1]->originX + g_apOvalStyleIcons[i]->drawWidth + i * 0x34;
            g_apOvalStyleIcons[i + 2]->originY = 0xCB;
        }
        g_bPaintToolStarted = FALSE;
    }
    for (i = 0; i < 4; i++) {
        if (i == g_bStyleMenuCursor) {
            DrawSpriteRegion(g_apOvalStyleIcons[i],
                             g_apOvalStyleIcons[i]->originX,
                             g_apOvalStyleIcons[i]->originY,
                             g_apOvalStyleIcons[i]->drawWidth,
                             g_apOvalStyleIcons[i]->drawHeight,
                             HIGHLIGHT_RED,
                             HIGHLIGHT_GREEN,
                             HIGHLIGHT_BLUE);
        } else {
            DrawSpriteAtOrigin(WITH_PALETTE(g_apOvalStyleIcons[i], NULL));
        }
    }
#if SHOWS_STYLE_NAMES
    DrawRectBorder(0xCE, 0x101, 0xE4, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
#endif
    SetTextSize(11);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#if SHOWS_STYLE_NAMES
    DrawText(names.name[g_bStyleMenuCursor], 0x9F, 0x8B);
#endif
    SetTextSize(10);
    SetTextAlignLeft();
    DrawText("%wwSELECT", (g_texButtonA.width + 0xDC) >> 1, (buttonY + 0x12) >> 1);
    DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, (buttonY + 0x12) >> 1);
    DrawTexturedSprite(&g_texButtonA, 0xD6, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);

    UpdateStickRepeat(PaintShopInput());
    if (PaintShopInput()->buttons & INPUT_LEFT) {
        TransferFlags(PaintShopInput(), INPUT_LEFT);
        if (g_bStyleMenuCursor == 0) {
            g_bStyleMenuCursor = 3;
        } else {
            g_bStyleMenuCursor--;
        }
    } else if (PaintShopInput()->buttons & INPUT_RIGHT) {
        TransferFlags(PaintShopInput(), INPUT_RIGHT);
        if (g_bStyleMenuCursor == 3) {
            g_bStyleMenuCursor = 0;
        } else {
            g_bStyleMenuCursor++;
        }
    }
    if (PaintShopInput()->buttons & INPUT_A) {
        TransferFlags(PaintShopInput(), INPUT_A);
        g_texPaintToolOval.style = g_bStyleMenuCursor;
        g_bStyleMenuOpen = FALSE;
    } else if (PaintShopInput()->buttons & INPUT_B) {
        TransferFlags(PaintShopInput(), INPUT_B);
        g_bStyleMenuOpen = FALSE;
    }
}

void ProcessPaintShopExitConfirm(void) {
    /* The original reserves stack for 24 bytes of locals it does not use. */
    char unused[24];
    PlayerInputState *input;
    s32 textY, buttonY, cancelX;

    (void)unused;
    if (g_bPaintShopExiting) {
        return;
    }
    /* The unsigned count is tested as signed. */
    if (g_bDecalEditCount <= 0) {
        g_bPaintShopExiting = TRUE;
        SetFadeTarget(0.0f, 0.2f);
        return;
    }
    /* The button A is placed by its width. */
    buttonY = 0x11D - g_texButtonA.width;
    cancelX = 0x169 - g_texButtonB.width;
    DrawRectBorder(0xBF, 0xBB, 0x102, 0x6A, 3, 0, 0, 0x80, 0x80, 0x80);
#if TGR_REGION == TGR_REGION_US
    SetTextSize(12);
    SetTextAlignLeft();
    SetTextStyleNormal();
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
    DrawText("CHANGES NOT SAVED.", 0x67, 0x6D);
    DrawText("EXIT ANYWAY?", 0x67, 0x7B);
#elif TGR_REGION == TGR_REGION_EU
    SetTextStyleNormal();
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
    SetTextAlignLeft();
    SetTextSize(12);
    DrawText("CHANGES NOT SAVED.", 0x67, 0x6D);
    DrawText("EXIT ANYWAY?", 0x67, 0x7B);
#else
    SetTextStyleNormal();
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
    SetTextAlignCenter();
    SetTextSize(14);
    DrawText("%jセーブせずに", g_dwScreenWidth / 2, 0x6D);
    DrawText("EXIT%jしますか？", g_dwScreenWidth / 2, 0x7B);
    SetTextAlignLeft();
#endif /* TGR_REGION == TGR_REGION_US */
    SetTextSize(10);
    textY = (buttonY + 0x12) >> 1;
    DrawText("%wwYES", (g_texButtonA.drawWidth + 0xF9) >> 1, textY);
    DrawText("%wwNO", (cancelX + g_texButtonB.drawWidth + 8) >> 1, textY);
    DrawTexturedSprite(&g_texButtonA, 0xF3, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);

    input = PaintShopInput();
    if (input->buttons & INPUT_A) {
        TransferFlags(input, INPUT_A);
        g_bPaintShopExiting = TRUE;
        SetFadeTarget(0.0f, 0.2f);
    } else if (input->buttons & INPUT_B) {
        TransferFlags(input, INPUT_B);
        g_bPaintTool = g_bPreviousPaintTool;
    }
}

void ResetBrushAndSlotHistory(void) {
    g_rcBrush = g_aBrushSizeRects[g_bBrushSize];
    g_bPreviousDecalSlot = g_bDecalSlot;
    g_bSavedDecalSlot = g_bDecalSlot;
    g_bUndoDecalSlot = g_bDecalSlot;
}

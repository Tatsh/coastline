#ifndef COASTLINE_DECALEDITOR_H
#define COASTLINE_DECALEDITOR_H

/**
 * @file
 * The decal editor of the paint shop: the cursor, the palette and its color editor, the tools and
 * the menus of their styles, and the dialog shown on exit from the paint shop.
 */

#include <PR/ultratypes.h>

#include "sprite.h"

/** The colors of the palette of a decal. */
#define DECAL_SWATCHES 16

/** The decals of a car that the paint shop edits. */
#define DECAL_SLOTS 10

/** The characters of the font of the decals, also the keys of the keyboard. */
#define DECAL_GLYPHS 50

/** The characters a text painted on a decal may have. */
#define DECAL_TEXT_LENGTH 16

/** The sizes of the brush. */
#define BRUSH_SIZES 5

/** The tools of the paint shop, each a button. */
enum {
    PAINT_TOOL_BRUSH_SIZE, /*!< The buttons that pick the size of the brush. */
    PAINT_TOOL_BRUSH,      /*!< The brush, painting round or square dots. */
    PAINT_TOOL_FILL,       /*!< The flood fill. */
    PAINT_TOOL_LINE,       /*!< A straight line from an anchor to the cursor. */
    PAINT_TOOL_TEXT,       /*!< A text typed on the keyboard of the font of the decals. */
    PAINT_TOOL_RECTANGLE,  /*!< A rectangle in one of the RECT_STYLE_ styles. */
    PAINT_TOOL_OVAL,       /*!< An ellipse or a circle in one of the OVAL_STYLE_ styles. */
    PAINT_TOOL_MIRROR,     /*!< The menu that mirrors or flips the decal. */
    PAINT_TOOL_CLEAR,      /*!< The menu that fills or clears the decal. */
    PAINT_TOOL_LOAD,       /*!< The dialog that loads decals from a Controller Pak. */
    PAINT_TOOL_SAVE,       /*!< The dialog that saves decals to a Controller Pak. */
    PAINT_TOOL_UNDO,       /*!< Undo the last edit and return to the previous tool. */
    PAINT_TOOL_EXIT,       /*!< The dialog that confirms the exit from the paint shop. */
    PAINT_TOOLS,           /*!< The number of tools. */
};

/** The styles of the rectangle tool. */
enum {
    RECT_STYLE_FILLED,         /*!< A filled rectangle. */
    RECT_STYLE_OUTLINED,       /*!< The outline of a rectangle. */
    RECT_STYLE_FILLED_ROUND,   /*!< A filled rectangle with rounded corners. */
    RECT_STYLE_OUTLINED_ROUND, /*!< The outline of a rectangle with rounded corners. */
};

/** The styles of the oval tool. */
enum {
    OVAL_STYLE_FILLED_ELLIPSE,   /*!< A filled ellipse in the box from the anchor to the cursor. */
    OVAL_STYLE_OUTLINED_ELLIPSE, /*!< The outline of an ellipse in the same box. */
    OVAL_STYLE_FILLED_CIRCLE,    /*!< A filled circle about the anchor, out to the cursor. */
    OVAL_STYLE_OUTLINED_CIRCLE,  /*!< The outline of a circle about the anchor. */
};

/** A rectangle of the screen of the paint shop, 640 by 480. */
typedef struct {
    s32 x;      /*!< The left edge, in pixels. */
    s32 y;      /*!< The top edge, in pixels. */
    s32 width;  /*!< The width, in pixels. */
    s32 height; /*!< The height, in pixels. */
} ScreenRect;

/** A color of the palette of the decal, and where it is shown to be picked. */
typedef struct {
    ScreenRect rect; /*!< Where the swatch is shown on the screen. */
    u8 color[3];     /*!< The red, green and blue of the color, from 0 to 255. */
} PaletteSwatch;

/** A character of the font of the decals, and its key on the keyboard of the text tool. */
typedef struct {
    s32 x;         /*!< The x of the key on the screen, where its highlight is drawn. */
    s32 y;         /*!< The y of the key on the screen, where its highlight is drawn. */
    u8 character;  /*!< The ASCII of the character. */
    s32 cellX;     /*!< Where the character starts in its cell of 16 texels of the font. */
    s32 width;     /*!< The width of the character, in texels. */
    s32 kernLeft;  /*!< How far the character may be moved towards the one before it. */
    s32 kernRight; /*!< How far the character may be moved towards the one after it. */
} DecalGlyph;

/**
 * The picture of the brush tool, whose style is round or square.
 *
 * @ghidraAddress USA: 0x8028ce70
 * @ghidraAddress JPN: 0x8028f040
 * @ghidraAddress EUR: 0x8028f5d0
 */
extern Sprite g_texPaintToolBrush;

/**
 * The picture of the text tool, whose style is the drop shadow.
 *
 * @ghidraAddress USA: 0x8028cf00
 * @ghidraAddress JPN: 0x8028f0d0
 * @ghidraAddress EUR: 0x8028f660
 */
extern Sprite g_texPaintToolText;

/**
 * The picture of the rectangle tool, whose style is RECT_STYLE_.
 *
 * @ghidraAddress USA: 0x8028cf30
 * @ghidraAddress JPN: 0x8028f100
 * @ghidraAddress EUR: 0x8028f690
 */
extern Sprite g_texPaintToolRect;

/**
 * The picture of the oval tool, whose style is OVAL_STYLE_.
 *
 * @ghidraAddress USA: 0x8028cf60
 * @ghidraAddress JPN: 0x8028f130
 * @ghidraAddress EUR: 0x8028f6c0
 */
extern Sprite g_texPaintToolOval;

/**
 * The picture of the clear tool.
 *
 * @ghidraAddress USA: 0x8028cfc0
 * @ghidraAddress JPN: 0x8028f190
 * @ghidraAddress EUR: 0x8028f720
 */
extern Sprite g_texPaintToolClear;

/**
 * The cursor, whose origin is where it points.
 *
 * @ghidraAddress USA: 0x8028d110
 * @ghidraAddress JPN: 0x8028f2e0
 * @ghidraAddress EUR: 0x8028f870
 */
extern Sprite g_texPaintCursor;

/**
 * The highlight of a key of the keyboard, the size of a key.
 *
 * @ghidraAddress USA: 0x8028d290
 * @ghidraAddress JPN: 0x8028f460
 * @ghidraAddress EUR: 0x8028f9f0
 */
extern Sprite g_texKeyboardHighlight;

/**
 * The font of the decals, 16 texels to a character.
 *
 * @ghidraAddress USA: 0x8028d2c0
 * @ghidraAddress JPN: 0x8028f490
 * @ghidraAddress EUR: 0x8028fa20
 */
extern Sprite g_texDecalFont;

/**
 * The icon of the round brush.
 *
 * @ghidraAddress USA: 0x8028d410
 * @ghidraAddress JPN: 0x8028f5e0
 * @ghidraAddress EUR: 0x8028fb70
 */
extern Sprite g_texPaintShapeFilledCircle;

/**
 * The area of the screen the decal is centered in.
 *
 * @ghidraAddress USA: 0x8028d470
 * @ghidraAddress JPN: 0x8028f640
 * @ghidraAddress EUR: 0x8028fbd0
 */
extern ScreenRect g_rcDecalCanvasFrame;

/**
 * The black areas behind the swatches of the palette.
 *
 * @ghidraAddress USA: 0x8028d480
 * @ghidraAddress JPN: 0x8028f650
 * @ghidraAddress EUR: 0x8028fbe0
 */
extern ScreenRect g_aPaletteFrameRects[2];

/** The sizes of the brush, in texels, as rectangles. @ghidraAddress USA: 0x8028d4a0 */
extern ScreenRect g_aBrushSizeRects[BRUSH_SIZES];

/**
 * The buttons of the sizes of the brush.
 *
 * @ghidraAddress USA: 0x8028d4f0
 * @ghidraAddress JPN: 0x8028f6c0
 * @ghidraAddress EUR: 0x8028fc50
 */
extern ScreenRect g_aBrushSizeButtonRects[BRUSH_SIZES];

/**
 * The characters of the font of the decals.
 *
 * @ghidraAddress USA: 0x8028d540
 * @ghidraAddress JPN: 0x8028f710
 * @ghidraAddress EUR: 0x8028fca0
 */
extern DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];

/**
 * The colors of the marching dashes of a shape not yet drawn.
 *
 * @ghidraAddress USA: 0x8028dab8
 * @ghidraAddress JPN: 0x8028fc88
 * @ghidraAddress EUR: 0x80290218
 */
extern u8 g_bDashColorA;

/**
 * @ghidraAddress USA: 0x8028dabc
 * @ghidraAddress JPN: 0x8028fc8c
 * @ghidraAddress EUR: 0x8029021c
 */
extern u8 g_bDashColorB;

/**
 * The size of the brush, an index of g_aBrushSizeRects.
 *
 * @ghidraAddress USA: 0x8028dac0
 * @ghidraAddress JPN: 0x8028fc90
 * @ghidraAddress EUR: 0x80290220
 */
extern u8 g_bBrushSize;

/** The rectangle of the size of the brush. @ghidraAddress USA: 0x8028dac4 */
extern ScreenRect g_rcBrush;

/**
 * The icon of the decal being edited.
 *
 * @ghidraAddress USA: 0x8028db08
 * @ghidraAddress JPN: 0x8028fcd8
 * @ghidraAddress EUR: 0x80290268
 */
extern Sprite *g_pDecalSlotIcon;

/**
 * The icons of the decals.
 *
 * @ghidraAddress USA: 0x8028db0c
 * @ghidraAddress JPN: 0x8028fcdc
 * @ghidraAddress EUR: 0x8029026c
 */
extern Sprite *g_apDecalSlotIcons[DECAL_SLOTS];

/**
 * The icons of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8028db34
 * @ghidraAddress JPN: 0x8028fd04
 * @ghidraAddress EUR: 0x80290294
 */
extern Sprite *g_apRectStyleIcons[4];

/**
 * The icons of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x8028db44
 * @ghidraAddress JPN: 0x8028fd14
 * @ghidraAddress EUR: 0x802902a4
 */
extern Sprite *g_apOvalStyleIcons[4];

/**
 * The color picked before the last, or the shadow color the text menu restores.
 *
 * @ghidraAddress USA: 0x8028db54
 * @ghidraAddress JPN: 0x8028fd24
 * @ghidraAddress EUR: 0x802902b4
 */
extern u8 g_bSavedColor;

/**
 * The color painted with, an index of the palette.
 *
 * @ghidraAddress USA: 0x8028db58
 * @ghidraAddress JPN: 0x8028fd28
 * @ghidraAddress EUR: 0x802902b8
 */
extern u8 g_bDecalColor;

/**
 * The color of the shadow of text.
 *
 * @ghidraAddress USA: 0x8028db5c
 * @ghidraAddress JPN: 0x8028fd2c
 * @ghidraAddress EUR: 0x802902bc
 */
extern u8 g_bTextShadowColor;

/**
 * The tool in use, a PAINT_TOOL_ value.
 *
 * @ghidraAddress USA: 0x8028db60
 * @ghidraAddress JPN: 0x8028fd30
 * @ghidraAddress EUR: 0x802902c0
 */
extern u8 g_bPaintTool;

/**
 * The tool in use before the last was picked.
 *
 * @ghidraAddress USA: 0x8028db64
 * @ghidraAddress JPN: 0x8028fd34
 * @ghidraAddress EUR: 0x802902c4
 */
extern u8 g_bPreviousPaintTool;

/**
 * The decal being edited.
 *
 * @ghidraAddress USA: 0x8028db68
 * @ghidraAddress JPN: 0x8028fd38
 * @ghidraAddress EUR: 0x802902c8
 */
extern u8 g_bDecalSlot;

/**
 * The decal edited before the last was picked.
 *
 * @ghidraAddress USA: 0x8028db6c
 * @ghidraAddress JPN: 0x8028fd3c
 * @ghidraAddress EUR: 0x802902cc
 */
extern u8 g_bPreviousDecalSlot;

/**
 * @ghidraAddress USA: 0x8028db70
 * @ghidraAddress JPN: 0x8028fd40
 * @ghidraAddress EUR: 0x802902d0
 */
extern u8 g_bSavedDecalSlot;

/**
 * The decal the copy that undo restores was taken of.
 *
 * @ghidraAddress USA: 0x8028db74
 * @ghidraAddress JPN: 0x8028fd44
 * @ghidraAddress EUR: 0x802902d4
 */
extern u8 g_bUndoDecalSlot;

/**
 * The texels of the decal being edited, 4 bits each.
 *
 * @ghidraAddress USA: 0x8028db78
 * @ghidraAddress JPN: 0x8028fd48
 * @ghidraAddress EUR: 0x802902d8
 */
extern u8 *g_pDecalTexels;

/**
 * The mask of the decal being edited, or NULL.
 *
 * @ghidraAddress USA: 0x8028db7c
 * @ghidraAddress JPN: 0x8028fd4c
 * @ghidraAddress EUR: 0x802902dc
 */
extern u8 *g_pDecalMask;

/**
 * A copy of the texels the flood fill works in.
 *
 * @ghidraAddress USA: 0x8028db84
 * @ghidraAddress JPN: 0x8028fd54
 * @ghidraAddress EUR: 0x802902e4
 */
extern u8 *g_pDecalScratchTexels;

/**
 * The size of the decal being edited, in texels.
 *
 * @ghidraAddress USA: 0x8028db88
 * @ghidraAddress JPN: 0x8028fd58
 * @ghidraAddress EUR: 0x802902e8
 */
extern u8 g_bDecalWidth;

/**
 * @ghidraAddress USA: 0x8028db8c
 * @ghidraAddress JPN: 0x8028fd5c
 * @ghidraAddress EUR: 0x802902ec
 */
extern u8 g_bDecalHeight;

/**
 * The palette of the decal being edited, in RGBA 5551.
 *
 * @ghidraAddress USA: 0x8028db90
 * @ghidraAddress JPN: 0x8028fd60
 * @ghidraAddress EUR: 0x802902f0
 */
extern u16 *g_pDecalPalette;

/**
 * Where the decal is shown, 4 pixels to a texel.
 *
 * @ghidraAddress USA: 0x8028db94
 * @ghidraAddress JPN: 0x8028fd64
 * @ghidraAddress EUR: 0x802902f4
 */
extern ScreenRect g_rcDecalCanvas;

/**
 * The characters of the text to paint.
 *
 * @ghidraAddress USA: 0x8028dba8
 * @ghidraAddress JPN: 0x8028fd78
 * @ghidraAddress EUR: 0x80290308
 */
extern u8 g_bDecalTextLength;

/**
 * The width of the text to paint, in texels.
 *
 * @ghidraAddress USA: 0x8028dbac
 * @ghidraAddress JPN: 0x8028fd7c
 * @ghidraAddress EUR: 0x8029030c
 */
extern u8 g_bDecalTextWidth;

/**
 * The edits not saved, for the warning on exit.
 *
 * @ghidraAddress USA: 0x8028dbb4
 * @ghidraAddress JPN: 0x8028fd84
 * @ghidraAddress EUR: 0x80290314
 */
extern u8 g_bDecalEditCount;

/**
 * The style under the cursor of a style menu.
 *
 * @ghidraAddress USA: 0x8028dbb8
 * @ghidraAddress JPN: 0x8028fd88
 * @ghidraAddress EUR: 0x80290318
 */
extern u8 g_bStyleMenuCursor;

/**
 * The player who paints, an index of g_aPlayerInputState.
 *
 * @ghidraAddress USA: 0x8028dbbc
 * @ghidraAddress JPN: 0x8028fd8c
 * @ghidraAddress EUR: 0x8029031c
 */
extern u8 g_bPaintShopPlayer;

/**
 * Set while a tool waits for its second click. Opening the menu of the styles of a tool sets it
 * too, and the menu clears it.
 *
 * @ghidraAddress USA: 0x8028dbc0
 * @ghidraAddress JPN: 0x8028fd90
 * @ghidraAddress EUR: 0x80290320
 */
extern u8 g_bPaintToolStarted;

/**
 * Set while a press on the decal is being dealt with.
 *
 * @ghidraAddress USA: 0x8028dbc4
 * @ghidraAddress JPN: 0x8028fd94
 * @ghidraAddress EUR: 0x80290324
 */
extern u8 g_bDecalDragActive;

/**
 * Whether the palette can be clicked.
 *
 * @ghidraAddress USA: 0x8028dbc8
 * @ghidraAddress JPN: 0x8028fd98
 * @ghidraAddress EUR: 0x80290328
 */
extern u8 g_bPaletteEnabled;

/**
 * Whether the decal being edited has a mirror image.
 *
 * @ghidraAddress USA: 0x8028dbcc
 * @ghidraAddress JPN: 0x8028fd9c
 * @ghidraAddress EUR: 0x8029032c
 */
extern u8 g_bDecalSlotHasMirror;

/**
 * Set when another decal is picked.
 *
 * @ghidraAddress USA: 0x8028dbd0
 * @ghidraAddress JPN: 0x8028fda0
 * @ghidraAddress EUR: 0x80290330
 */
extern u8 g_bDecalSlotChanged;

/**
 * Whether the color editor is open.
 *
 * @ghidraAddress USA: 0x8028dbd4
 * @ghidraAddress JPN: 0x8028fda4
 * @ghidraAddress EUR: 0x80290334
 */
extern u8 g_bColorEditorOpen;

/**
 * Whether the menu of the styles of the tool is open.
 *
 * @ghidraAddress USA: 0x8028dbe0
 * @ghidraAddress JPN: 0x8028fdb0
 * @ghidraAddress EUR: 0x80290340
 */
extern u8 g_bStyleMenuOpen;

/**
 * Whether the text menu shows the colors of the shadow.
 *
 * @ghidraAddress USA: 0x8028dbe4
 * @ghidraAddress JPN: 0x8028fdb4
 * @ghidraAddress EUR: 0x80290344
 */
extern u8 g_bTextShadowPickerOpen;

/**
 * Set once the paint shop fades out.
 *
 * @ghidraAddress USA: 0x8028dbe8
 * @ghidraAddress JPN: 0x8028fdb8
 * @ghidraAddress EUR: 0x80290348
 */
extern u8 g_bPaintShopExiting;

/**
 * The colors of the bars of the color editor.
 *
 * @ghidraAddress USA: 0x8028dcc8
 * @ghidraAddress JPN: 0x8028fee8
 * @ghidraAddress EUR: 0x80290468
 */
extern u8 g_aabColorEditorBars[3][3];

/**
 * The names of the bars of the color editor.
 *
 * @ghidraAddress USA: 0x8028dcd4
 * @ghidraAddress JPN: 0x8028fef4
 * @ghidraAddress EUR: 0x80290474
 */
extern const char *g_apszColorChannelNames[3];

/**
 * The bar of the color editor under its cursor.
 *
 * @ghidraAddress USA: 0x8028dce0
 * @ghidraAddress JPN: 0x8028ff00
 * @ghidraAddress EUR: 0x80290480
 */
extern u8 g_bColorEditorChannel;

/**
 * The names of the styles of the brush tool.
 *
 * @ghidraAddress USA: 0x8028dce4
 * @ghidraAddress EUR: 0x80290484
 */
extern const char *g_apszBrushStyleNames[2];

/**
 * The names of the styles of the text tool.
 *
 * @ghidraAddress USA: 0x8028dcec
 * @ghidraAddress EUR: 0x8029048c
 */
extern const char *g_apszTextStyleNames[2];

/**
 * The names of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8028dcf4
 * @ghidraAddress EUR: 0x80290494
 */
extern const char *g_apszRectStyleNames[4];

/**
 * The names of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x8028dd04
 * @ghidraAddress EUR: 0x802904a4
 */
extern const char *g_apszOvalStyleNames[4];

/**
 * The first corner of a shape, or where text was clicked.
 *
 * @ghidraAddress USA: 0x80369b80
 * @ghidraAddress JPN: 0x8036b018
 * @ghidraAddress EUR: 0x8036c080
 */
extern s32 g_nPaintAnchorX;

/**
 * @ghidraAddress USA: 0x80369b84
 * @ghidraAddress JPN: 0x8036b01c
 * @ghidraAddress EUR: 0x8036c084
 */
extern s32 g_nPaintAnchorY;

/**
 * When the last click was, in milliseconds.
 *
 * @ghidraAddress USA: 0x80369b88
 * @ghidraAddress JPN: 0x8036b020
 * @ghidraAddress EUR: 0x8036c088
 */
extern u32 g_dwLastClickTime;

/**
 * Set once the first corner is first taken from the cursor.
 *
 * @ghidraAddress USA: 0x80369b8c
 * @ghidraAddress JPN: 0x8036b024
 * @ghidraAddress EUR: 0x8036c08c
 */
extern s32 g_dwAnchorXInitialized;

/**
 * @ghidraAddress USA: 0x80369b90
 * @ghidraAddress JPN: 0x8036acf0
 * @ghidraAddress EUR: 0x8036c090
 */
extern s32 g_dwAnchorYInitialized;

/**
 * The palette of the decal as it is shown.
 *
 * @ghidraAddress USA: 0x80369b98
 * @ghidraAddress JPN: 0x8036acf8
 * @ghidraAddress EUR: 0x8036c098
 */
extern PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];

/**
 * The buttons of the tools.
 *
 * @ghidraAddress USA: 0x80369cd8
 * @ghidraAddress JPN: 0x8036ae38
 * @ghidraAddress EUR: 0x8036c1d8
 */
extern ScreenRect g_aPaintToolButtonRects[PAINT_TOOLS];

/**
 * The color the color editor restores when it is cancelled.
 *
 * @ghidraAddress USA: 0x80369da8
 * @ghidraAddress JPN: 0x8036af08
 * @ghidraAddress EUR: 0x8036c2a8
 */
extern u8 g_abSavedSwatchColor[3];

/**
 * The buttons of the decals.
 *
 * @ghidraAddress USA: 0x80369db0
 * @ghidraAddress JPN: 0x8036af10
 * @ghidraAddress EUR: 0x8036c2b0
 */
extern ScreenRect g_aDecalSlotRects[DECAL_SLOTS];

/**
 * The glyphs of the text to paint.
 *
 * @ghidraAddress USA: 0x80369e68
 * @ghidraAddress JPN: 0x8036afc8
 * @ghidraAddress EUR: 0x8036c368
 */
extern DecalGlyph *g_apDecalTextGlyphs[DECAL_TEXT_LENGTH];

/**
 * Save a copy of the decal being edited for undo, and count an edit.
 *
 * @ghidraAddress USA: 0x80244ca8
 * @ghidraAddress JPN: 0x80245cc0
 * @ghidraAddress EUR: 0x802461cc
 */
void SaveDecalUndo(void);

/**
 * Restore the copy of the decal saved for undo.
 *
 * @ghidraAddress USA: 0x80244d04
 * @ghidraAddress JPN: 0x80245d1c
 * @ghidraAddress EUR: 0x80246228
 */
void UndoDecalEdit(void);

/**
 * Run one frame of the keyboard of the text tool.
 *
 * @ghidraAddress USA: 0x8024ac70
 * @ghidraAddress JPN: 0x8024bbd0 (stub)
 * @ghidraAddress EUR: 0x8024c1d0 (stub)
 */
void ProcessTextInputKeyboard(void);

/**
 * Run one frame of the color editor of the color in use.
 *
 * @ghidraAddress USA: 0x8024b144
 * @ghidraAddress JPN: 0x8024c0a8
 * @ghidraAddress EUR: 0x8024c6a8
 */
void ProcessColorEditorDialog(void);

/**
 * Move the cursor by the stick, the D-pad and the C buttons.
 *
 * @ghidraAddress USA: 0x8024be78
 * @ghidraAddress JPN: 0x8024cddc
 * @ghidraAddress EUR: 0x8024d3dc
 */
void UpdateCursorFromInput(void);

/**
 * Deal with a frame of the input of the decal editor: the decals, the palette, the tools and
 * painting.
 *
 * @ghidraAddress USA: 0x8024c184
 * @ghidraAddress JPN: 0x8024d0e8 (stub)
 * @ghidraAddress EUR: 0x8024d6e8
 */
void ProcessDecalEditorInput(void);

/**
 * Whether the cursor is on a sprite as it is drawn.
 *
 * @param sprite The sprite.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d374
 * @ghidraAddress JPN: 0x8024e2b8
 * @ghidraAddress EUR: 0x8024e8d8
 */
s32 IsCursorInButton(Sprite *sprite);

/**
 * Whether the cursor is in a rectangle.
 *
 * @param rect The rectangle.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d3f0
 * @ghidraAddress JPN: 0x8024e3a0
 * @ghidraAddress EUR: 0x8024e9c0
 */
s32 IsCursorInRect(ScreenRect *rect);

/**
 * Whether the cursor is on a swatch of the palette.
 *
 * @param swatch The swatch.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d45c
 * @ghidraAddress JPN: 0x8024e334
 * @ghidraAddress EUR: 0x8024e954
 */
s32 IsCursorInSwatch(PaletteSwatch *swatch);

/**
 * Whether the cursor is on a key of the keyboard.
 *
 * @param key The key.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d4c8
 * @ghidraAddress JPN: 0x8024e40c
 * @ghidraAddress EUR: 0x8024ea2c
 */
s32 IsCursorOnKeyboardKey(DecalGlyph *key);

/**
 * Set the colors of the swatches from the palette of the decal.
 *
 * @ghidraAddress USA: 0x8024d53c
 * @ghidraAddress JPN: 0x8024e480
 * @ghidraAddress EUR: 0x8024eaa0
 */
void ConvertPaletteToRgb(void);

/**
 * Draw the palette, with the color in use outlined.
 *
 * @ghidraAddress USA: 0x8024d6b8
 * @ghidraAddress JPN: 0x8024e5fc
 * @ghidraAddress EUR: 0x8024ec1c
 */
void DrawDecalColorPalette(void);

/**
 * Run one frame of the menu of the styles of the brush tool.
 *
 * @ghidraAddress USA: 0x8024d89c
 * @ghidraAddress JPN: 0x8024e7e0 (stub)
 * @ghidraAddress EUR: 0x8024ee00
 */
void ProcessBrushStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the text tool, and of its colors of the shadow.
 *
 * @ghidraAddress USA: 0x8024e128
 * @ghidraAddress JPN: 0x8024efec (stub)
 * @ghidraAddress EUR: 0x8024f68c
 */
void ProcessTextStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8024f39c
 * @ghidraAddress JPN: 0x802501c4 (stub)
 * @ghidraAddress EUR: 0x80250900
 */
void ProcessRectStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x80250698
 * @ghidraAddress JPN: 0x80251434 (stub)
 * @ghidraAddress EUR: 0x80251bfc (stub)
 */
void ProcessOvalStyleMenu(void);

/**
 * Run one frame of the dialog that warns of edits not saved on exit from the paint shop, or fade
 * out at once when there are none.
 *
 * @ghidraAddress USA: 0x802531c0
 * @ghidraAddress JPN: 0x80253ed4 (stub)
 * @ghidraAddress EUR: 0x80254728 (stub)
 */
void ProcessPaintShopExitConfirm(void);

/**
 * Set the brush to its size and every remembered decal to the decal being edited.
 *
 * @ghidraAddress USA: 0x802533f4
 */
void ResetBrushAndSlotHistory(void);

#endif

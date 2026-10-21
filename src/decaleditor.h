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
    PAINT_TOOL_BRUSH_SIZE,
    PAINT_TOOL_BRUSH,
    PAINT_TOOL_FILL,
    PAINT_TOOL_LINE,
    PAINT_TOOL_TEXT,
    PAINT_TOOL_RECTANGLE,
    PAINT_TOOL_OVAL,
    PAINT_TOOL_MIRROR,
    PAINT_TOOL_CLEAR,
    PAINT_TOOL_LOAD,
    PAINT_TOOL_SAVE,
    PAINT_TOOL_UNDO,
    PAINT_TOOL_EXIT,
    PAINT_TOOLS,
};

/** The styles of the rectangle tool. */
enum {
    RECT_STYLE_FILLED,
    RECT_STYLE_OUTLINED,
    RECT_STYLE_FILLED_ROUND,
    RECT_STYLE_OUTLINED_ROUND,
};

/** The styles of the oval tool. */
enum {
    OVAL_STYLE_FILLED_ELLIPSE,
    OVAL_STYLE_OUTLINED_ELLIPSE,
    OVAL_STYLE_FILLED_CIRCLE,
    OVAL_STYLE_OUTLINED_CIRCLE,
};

/** A rectangle of the screen of the paint shop, 640 by 480. */
typedef struct {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
} ScreenRect;

/** A color of the palette of the decal, and where it is shown to be picked. */
typedef struct {
    ScreenRect rect;
    /** The red, green and blue of the color, from 0 to 255. */
    u8 color[3];
} PaletteSwatch;

/** A character of the font of the decals, and its key on the keyboard of the text tool. */
typedef struct {
    /** Where the key is on the screen. */
    s32 x;
    s32 y;
    /** The ASCII of the character. */
    u8 character;
    /** Where the character starts in its cell of 16 texels of the font. */
    s32 cellX;
    s32 width;
    /** How far the character may be moved towards the one before and the one after it. */
    s32 kernLeft;
    s32 kernRight;
} DecalGlyph;

/** The picture of the brush tool, whose style is round or square. @ghidraAddress USA: 0x8028ce70 */
extern Sprite g_texPaintToolBrush;

/** The picture of the text tool, whose style is the drop shadow. @ghidraAddress USA: 0x8028cf00 */
extern Sprite g_texPaintToolText;

/** The picture of the rectangle tool, whose style is RECT_STYLE_. @ghidraAddress USA: 0x8028cf30 */
extern Sprite g_texPaintToolRect;

/** The picture of the oval tool, whose style is OVAL_STYLE_. @ghidraAddress USA: 0x8028cf60 */
extern Sprite g_texPaintToolOval;

/** The picture of the clear tool. @ghidraAddress USA: 0x8028cfc0 */
extern Sprite g_texPaintToolClear;

/** The cursor, whose origin is where it points. @ghidraAddress USA: 0x8028d110 */
extern Sprite g_texPaintCursor;

/** The highlight of a key of the keyboard, the size of a key. @ghidraAddress USA: 0x8028d290 */
extern Sprite g_texKeyboardHighlight;

/** The font of the decals, 16 texels to a character. @ghidraAddress USA: 0x8028d2c0 */
extern Sprite g_texDecalFont;

/** The icon of the round brush. @ghidraAddress USA: 0x8028d410 */
extern Sprite g_texPaintShapeFilledCircle;

/** The area of the screen the decal is centered in. @ghidraAddress USA: 0x8028d470 */
extern ScreenRect g_rcDecalCanvasFrame;

/** The black areas behind the swatches of the palette. @ghidraAddress USA: 0x8028d480 */
extern ScreenRect g_aPaletteFrameRects[2];

/** The sizes of the brush, in texels, as rectangles. @ghidraAddress USA: 0x8028d4a0 */
extern ScreenRect g_aBrushSizeRects[BRUSH_SIZES];

/** The buttons of the sizes of the brush. @ghidraAddress USA: 0x8028d4f0 */
extern ScreenRect g_aBrushSizeButtonRects[BRUSH_SIZES];

/** The characters of the font of the decals. @ghidraAddress USA: 0x8028d540 */
extern DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];

/** The colors of the marching dashes of a shape not yet drawn. @ghidraAddress USA: 0x8028dab8 */
extern u8 g_bDashColorA;

/** @ghidraAddress USA: 0x8028dabc */
extern u8 g_bDashColorB;

/** The size of the brush, an index of g_aBrushSizeRects. @ghidraAddress USA: 0x8028dac0 */
extern u8 g_bBrushSize;

/** The rectangle of the size of the brush. @ghidraAddress USA: 0x8028dac4 */
extern ScreenRect g_rcBrush;

/** The icon of the decal being edited. @ghidraAddress USA: 0x8028db08 */
extern Sprite *g_pDecalSlotIcon;

/** The icons of the decals. @ghidraAddress USA: 0x8028db0c */
extern Sprite *g_apDecalSlotIcons[DECAL_SLOTS];

/** The icons of the styles of the rectangle tool. @ghidraAddress USA: 0x8028db34 */
extern Sprite *g_apRectStyleIcons[4];

/** The icons of the styles of the oval tool. @ghidraAddress USA: 0x8028db44 */
extern Sprite *g_apOvalStyleIcons[4];

/**
 * The color picked before the last, or the shadow color the text menu restores.
 *
 * @ghidraAddress USA: 0x8028db54
 */
extern u8 g_bSavedColor;

/** The color painted with, an index of the palette. @ghidraAddress USA: 0x8028db58 */
extern u8 g_bDecalColor;

/** The color of the shadow of text. @ghidraAddress USA: 0x8028db5c */
extern u8 g_bTextShadowColor;

/** The tool in use, a PAINT_TOOL_ value. @ghidraAddress USA: 0x8028db60 */
extern u8 g_bPaintTool;

/** The tool in use before the last was picked. @ghidraAddress USA: 0x8028db64 */
extern u8 g_bPreviousPaintTool;

/** The decal being edited. @ghidraAddress USA: 0x8028db68 */
extern u8 g_bDecalSlot;

/** The decal edited before the last was picked. @ghidraAddress USA: 0x8028db6c */
extern u8 g_bPreviousDecalSlot;

/** @ghidraAddress USA: 0x8028db70 */
extern u8 g_bSavedDecalSlot;

/** The decal the copy that undo restores was taken of. @ghidraAddress USA: 0x8028db74 */
extern u8 g_bUndoDecalSlot;

/** The texels of the decal being edited, 4 bits each. @ghidraAddress USA: 0x8028db78 */
extern u8 *g_pDecalTexels;

/** The mask of the decal being edited, or NULL. @ghidraAddress USA: 0x8028db7c */
extern u8 *g_pDecalMask;

/** A copy of the texels the flood fill works in. @ghidraAddress USA: 0x8028db84 */
extern u8 *g_pDecalScratchTexels;

/** The size of the decal being edited, in texels. @ghidraAddress USA: 0x8028db88 */
extern u8 g_bDecalWidth;

/** @ghidraAddress USA: 0x8028db8c */
extern u8 g_bDecalHeight;

/** The palette of the decal being edited, in RGBA 5551. @ghidraAddress USA: 0x8028db90 */
extern u16 *g_pDecalPalette;

/** Where the decal is shown, 4 pixels to a texel. @ghidraAddress USA: 0x8028db94 */
extern ScreenRect g_rcDecalCanvas;

/** The characters of the text to paint. @ghidraAddress USA: 0x8028dba8 */
extern u8 g_bDecalTextLength;

/** The width of the text to paint, in texels. @ghidraAddress USA: 0x8028dbac */
extern u8 g_bDecalTextWidth;

/** The edits not saved, for the warning on exit. @ghidraAddress USA: 0x8028dbb4 */
extern u8 g_bDecalEditCount;

/** The style under the cursor of a style menu. @ghidraAddress USA: 0x8028dbb8 */
extern u8 g_bStyleMenuCursor;

/** The player who paints, an index of g_aPlayerInputState. @ghidraAddress USA: 0x8028dbbc */
extern u8 g_bPaintShopPlayer;

/**
 * Set while a tool waits for its second click. Opening the menu of the styles of a tool sets it
 * too, and the menu clears it.
 *
 * @ghidraAddress USA: 0x8028dbc0
 */
extern u8 g_bPaintToolStarted;

/** Set while a press on the decal is being dealt with. @ghidraAddress USA: 0x8028dbc4 */
extern u8 g_bDecalDragActive;

/** Whether the palette can be clicked. @ghidraAddress USA: 0x8028dbc8 */
extern u8 g_bPaletteEnabled;

/** Whether the decal being edited has a mirror image. @ghidraAddress USA: 0x8028dbcc */
extern u8 g_bDecalSlotHasMirror;

/** Set when another decal is picked. @ghidraAddress USA: 0x8028dbd0 */
extern u8 g_bDecalSlotChanged;

/** Whether the color editor is open. @ghidraAddress USA: 0x8028dbd4 */
extern u8 g_bColorEditorOpen;

/** Whether the menu of the styles of the tool is open. @ghidraAddress USA: 0x8028dbe0 */
extern u8 g_bStyleMenuOpen;

/** Whether the text menu shows the colors of the shadow. @ghidraAddress USA: 0x8028dbe4 */
extern u8 g_bTextShadowPickerOpen;

/** Set once the paint shop fades out. @ghidraAddress USA: 0x8028dbe8 */
extern u8 g_bPaintShopExiting;

/** The colors of the bars of the color editor. @ghidraAddress USA: 0x8028dcc8 */
extern u8 g_aabColorEditorBars[3][3];

/** The names of the bars of the color editor. @ghidraAddress USA: 0x8028dcd4 */
extern const char *g_apszColorChannelNames[3];

/** The bar of the color editor under its cursor. @ghidraAddress USA: 0x8028dce0 */
extern u8 g_bColorEditorChannel;

/** The names of the styles of the brush tool. @ghidraAddress USA: 0x8028dce4 */
extern const char *g_apszBrushStyleNames[2];

/** The names of the styles of the text tool. @ghidraAddress USA: 0x8028dcec */
extern const char *g_apszTextStyleNames[2];

/** The names of the styles of the rectangle tool. @ghidraAddress USA: 0x8028dcf4 */
extern const char *g_apszRectStyleNames[4];

/** The names of the styles of the oval tool. @ghidraAddress USA: 0x8028dd04 */
extern const char *g_apszOvalStyleNames[4];

/** The first corner of a shape, or where text was clicked. @ghidraAddress USA: 0x80369b80 */
extern s32 g_nPaintAnchorX;

/** @ghidraAddress USA: 0x80369b84 */
extern s32 g_nPaintAnchorY;

/** When the last click was, in milliseconds. @ghidraAddress USA: 0x80369b88 */
extern u32 g_dwLastClickTime;

/** Set once the first corner is first taken from the cursor. @ghidraAddress USA: 0x80369b8c */
extern s32 g_dwAnchorXInitialized;

/** @ghidraAddress USA: 0x80369b90 */
extern s32 g_dwAnchorYInitialized;

/** The palette of the decal as it is shown. @ghidraAddress USA: 0x80369b98 */
extern PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];

/** The buttons of the tools. @ghidraAddress USA: 0x80369cd8 */
extern ScreenRect g_aPaintToolButtonRects[PAINT_TOOLS];

/** The color the color editor restores when it is cancelled. @ghidraAddress USA: 0x80369da8 */
extern u8 g_abSavedSwatchColor[3];

/** The buttons of the decals. @ghidraAddress USA: 0x80369db0 */
extern ScreenRect g_aDecalSlotRects[DECAL_SLOTS];

/** The glyphs of the text to paint. @ghidraAddress USA: 0x80369e68 */
extern DecalGlyph *g_apDecalTextGlyphs[DECAL_TEXT_LENGTH];

/**
 * Save a copy of the decal being edited for undo, and count an edit.
 *
 * @ghidraAddress USA: 0x80244ca8
 */
void SaveDecalUndo(void);

/**
 * Restore the copy of the decal saved for undo.
 *
 * @ghidraAddress USA: 0x80244d04
 */
void UndoDecalEdit(void);

/**
 * Run one frame of the keyboard of the text tool.
 *
 * @ghidraAddress USA: 0x8024ac70
 */
void ProcessTextInputKeyboard(void);

/**
 * Run one frame of the color editor of the color in use.
 *
 * @ghidraAddress USA: 0x8024b144
 */
void ProcessColorEditorDialog(void);

/**
 * Move the cursor by the stick, the D-pad and the C buttons.
 *
 * @ghidraAddress USA: 0x8024be78
 */
void UpdateCursorFromInput(void);

/**
 * Deal with a frame of the input of the decal editor: the decals, the palette, the tools and
 * painting.
 *
 * @ghidraAddress USA: 0x8024c184
 */
void ProcessDecalEditorInput(void);

/**
 * Whether the cursor is on a sprite as it is drawn.
 *
 * @param sprite The sprite.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d374
 */
s32 IsCursorInButton(Sprite *sprite);

/**
 * Whether the cursor is in a rectangle.
 *
 * @param rect The rectangle.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d3f0
 */
s32 IsCursorInRect(ScreenRect *rect);

/**
 * Whether the cursor is on a swatch of the palette.
 *
 * @param swatch The swatch.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d45c
 */
s32 IsCursorInSwatch(PaletteSwatch *swatch);

/**
 * Whether the cursor is on a key of the keyboard.
 *
 * @param key The key.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d4c8
 */
s32 IsCursorOnKeyboardKey(DecalGlyph *key);

/**
 * Set the colors of the swatches from the palette of the decal.
 *
 * @ghidraAddress USA: 0x8024d53c
 */
void ConvertPaletteToRgb(void);

/**
 * Draw the palette, with the color in use outlined.
 *
 * @ghidraAddress USA: 0x8024d6b8
 */
void DrawDecalColorPalette(void);

/**
 * Run one frame of the menu of the styles of the brush tool.
 *
 * @ghidraAddress USA: 0x8024d89c
 */
void ProcessBrushStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the text tool, and of its colors of the shadow.
 *
 * @ghidraAddress USA: 0x8024e128
 */
void ProcessTextStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8024f39c
 */
void ProcessRectStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x80250698
 */
void ProcessOvalStyleMenu(void);

/**
 * Run one frame of the dialog that warns of edits not saved on exit from the paint shop, or fade
 * out at once when there are none.
 *
 * @ghidraAddress USA: 0x802531c0
 */
void ProcessPaintShopExitConfirm(void);

/**
 * Set the brush to its size and every remembered decal to the decal being edited.
 *
 * @ghidraAddress USA: 0x802533f4
 */
void ResetBrushAndSlotHistory(void);

#endif

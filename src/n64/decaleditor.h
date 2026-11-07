#ifndef COASTLINE_DECALEDITOR_H
#define COASTLINE_DECALEDITOR_H

/**
 * @file
 * The decal editor of the paint shop: the cursor, the palette and its color editor, the tools and
 * the menus of their styles, and the dialog shown on exit from the paint shop.
 */

#include <PR/ultratypes.h>

#include "sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @ghidraAddress Asia: 0x8028C9C0
 * @ghidraAddress EUR: 0x8028f5d0
 * @ghidraAddress JPN: 0x8028f040
 */
extern Sprite g_texPaintToolBrush;

/**
 * The picture of the text tool, whose style is the drop shadow.
 *
 * @ghidraAddress USA: 0x8028cf00
 * @ghidraAddress Asia: 0x8028CA50
 * @ghidraAddress EUR: 0x8028f660
 * @ghidraAddress JPN: 0x8028f0d0
 */
extern Sprite g_texPaintToolText;

/**
 * The picture of the rectangle tool, whose style is RECT_STYLE_.
 *
 * @ghidraAddress USA: 0x8028cf30
 * @ghidraAddress Asia: 0x8028CA80
 * @ghidraAddress EUR: 0x8028f690
 * @ghidraAddress JPN: 0x8028f100
 */
extern Sprite g_texPaintToolRect;

/**
 * The picture of the oval tool, whose style is OVAL_STYLE_.
 *
 * @ghidraAddress USA: 0x8028cf60
 * @ghidraAddress Asia: 0x8028CAB0
 * @ghidraAddress EUR: 0x8028f6c0
 * @ghidraAddress JPN: 0x8028f130
 */
extern Sprite g_texPaintToolOval;

/**
 * The picture of the clear tool.
 *
 * @ghidraAddress USA: 0x8028cfc0
 * @ghidraAddress Asia: 0x8028CB10
 * @ghidraAddress EUR: 0x8028f720
 * @ghidraAddress JPN: 0x8028f190
 */
extern Sprite g_texPaintToolClear;

/**
 * The cursor, whose origin is where it points.
 *
 * @ghidraAddress USA: 0x8028d110
 * @ghidraAddress Asia: 0x8028CC60
 * @ghidraAddress EUR: 0x8028f870
 * @ghidraAddress JPN: 0x8028f2e0
 */
extern Sprite g_texPaintCursor;

/**
 * The highlight of a key of the keyboard, the size of a key.
 *
 * @ghidraAddress USA: 0x8028d290
 * @ghidraAddress Asia: 0x8028CDE0
 * @ghidraAddress EUR: 0x8028f9f0
 * @ghidraAddress JPN: 0x8028f460
 */
extern Sprite g_texKeyboardHighlight;

/**
 * The font of the decals, 16 texels to a character.
 *
 * @ghidraAddress USA: 0x8028d2c0
 * @ghidraAddress Asia: 0x8028CE10
 * @ghidraAddress EUR: 0x8028fa20
 * @ghidraAddress JPN: 0x8028f490
 */
extern Sprite g_texDecalFont;

/**
 * The icon of the round brush.
 *
 * @ghidraAddress USA: 0x8028d410
 * @ghidraAddress Asia: 0x8028CF60
 * @ghidraAddress EUR: 0x8028fb70
 * @ghidraAddress JPN: 0x8028f5e0
 */
extern Sprite g_texPaintShapeFilledCircle;

/**
 * Three contiguous paint shop frames, starting with the area around the decal.
 *
 * @ghidraAddress USA: 0x8028d470
 * @ghidraAddress Asia: 0x8028CFC0
 * @ghidraAddress EUR: 0x8028fbd0
 * @ghidraAddress JPN: 0x8028f640
 */
extern ScreenRect g_rcDecalCanvasFrame[3];

/**
 * The black area behind the two wide swatches of the palette.
 *
 * @ghidraAddress USA: 0x8028d480
 * @ghidraAddress Asia: 0x8028CFD0
 * @ghidraAddress EUR: 0x8028fbe0
 * @ghidraAddress JPN: 0x8028f650
 */
extern ScreenRect g_rcPaletteFrameLeft;

/**
 * The black area behind the narrow swatches of the palette, right of the wide ones.
 *
 * @ghidraAddress USA: 0x8028d490
 * @ghidraAddress Asia: 0x8028CFE0
 * @ghidraAddress EUR: 0x8028fbf0
 * @ghidraAddress JPN: 0x8028f660
 */
extern ScreenRect g_rcPaletteFrameRight;

/**
 * The sizes of the brush, in texels, as rectangles.
 *
 * @ghidraAddress USA: 0x8028d4a0
 * @ghidraAddress Asia: 0x8028CFF0
 * @ghidraAddress EUR: 0x8028fc00
 * @ghidraAddress JPN: 0x8028f670
 */
extern ScreenRect g_aBrushSizeRects[BRUSH_SIZES];

/**
 * The buttons of the sizes of the brush.
 *
 * @ghidraAddress USA: 0x8028d4f0
 * @ghidraAddress Asia: 0x8028D040
 * @ghidraAddress EUR: 0x8028fc50
 * @ghidraAddress JPN: 0x8028f6c0
 */
extern ScreenRect g_aBrushSizeButtonRects[BRUSH_SIZES];

/**
 * The characters of the font of the decals.
 *
 * @ghidraAddress USA: 0x8028d540
 * @ghidraAddress Asia: 0x8028D090
 * @ghidraAddress EUR: 0x8028fca0
 * @ghidraAddress JPN: 0x8028f710
 */
extern DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];

/**
 * The colors of the marching dashes of a shape not yet drawn.
 *
 * @ghidraAddress USA: 0x8028dab8
 * @ghidraAddress Asia: 0x8028D608
 * @ghidraAddress EUR: 0x80290218
 * @ghidraAddress JPN: 0x8028fc88
 */
extern u8 g_bDashColorA;

/**
 * @ghidraAddress USA: 0x8028dabc
 * @ghidraAddress Asia: 0x8028D60C
 * @ghidraAddress EUR: 0x8029021c
 * @ghidraAddress JPN: 0x8028fc8c
 */
extern u8 g_bDashColorB;

/**
 * The size of the brush, an index of g_aBrushSizeRects.
 *
 * @ghidraAddress USA: 0x8028dac0
 * @ghidraAddress Asia: 0x8028D610
 * @ghidraAddress EUR: 0x80290220
 * @ghidraAddress JPN: 0x8028fc90
 */
extern u8 g_bBrushSize;

/**
 * The rectangle of the size of the brush.
 *
 * @ghidraAddress USA: 0x8028dac4
 * @ghidraAddress Asia: 0x8028D614
 * @ghidraAddress EUR: 0x80290224
 * @ghidraAddress JPN: 0x8028fc94
 */
extern ScreenRect g_rcBrush;

/**
 * The icon of the decal being edited.
 *
 * @ghidraAddress USA: 0x8028db08
 * @ghidraAddress Asia: 0x8028D658
 * @ghidraAddress EUR: 0x80290268
 * @ghidraAddress JPN: 0x8028fcd8
 */
extern Sprite *g_pDecalSlotIcon;

/**
 * The icons of the decals.
 *
 * @ghidraAddress USA: 0x8028db0c
 * @ghidraAddress Asia: 0x8028D65C
 * @ghidraAddress EUR: 0x8029026c
 * @ghidraAddress JPN: 0x8028fcdc
 */
extern Sprite *g_apDecalSlotIcons[DECAL_SLOTS];

/**
 * The icons of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8028db34
 * @ghidraAddress Asia: 0x8028D684
 * @ghidraAddress EUR: 0x80290294
 * @ghidraAddress JPN: 0x8028fd04
 */
extern Sprite *g_apRectStyleIcons[4];

/**
 * The icons of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x8028db44
 * @ghidraAddress Asia: 0x8028D694
 * @ghidraAddress EUR: 0x802902a4
 * @ghidraAddress JPN: 0x8028fd14
 */
extern Sprite *g_apOvalStyleIcons[4];

/**
 * The color picked before the last, or the shadow color the text menu restores.
 *
 * @ghidraAddress USA: 0x8028db54
 * @ghidraAddress Asia: 0x8028D6A4
 * @ghidraAddress EUR: 0x802902b4
 * @ghidraAddress JPN: 0x8028fd24
 */
extern u8 g_bSavedColor;

/**
 * The color painted with, an index of the palette.
 *
 * @ghidraAddress USA: 0x8028db58
 * @ghidraAddress Asia: 0x8028D6A8
 * @ghidraAddress EUR: 0x802902b8
 * @ghidraAddress JPN: 0x8028fd28
 */
extern u8 g_bDecalColor;

/**
 * The color of the shadow of text.
 *
 * @ghidraAddress USA: 0x8028db5c
 * @ghidraAddress Asia: 0x8028D6AC
 * @ghidraAddress EUR: 0x802902bc
 * @ghidraAddress JPN: 0x8028fd2c
 */
extern u8 g_bTextShadowColor;

/**
 * The tool in use, a PAINT_TOOL_ value.
 *
 * @ghidraAddress USA: 0x8028db60
 * @ghidraAddress Asia: 0x8028D6B0
 * @ghidraAddress EUR: 0x802902c0
 * @ghidraAddress JPN: 0x8028fd30
 */
extern u8 g_bPaintTool;

/**
 * The tool in use before the last was picked.
 *
 * @ghidraAddress USA: 0x8028db64
 * @ghidraAddress Asia: 0x8028D6B4
 * @ghidraAddress EUR: 0x802902c4
 * @ghidraAddress JPN: 0x8028fd34
 */
extern u8 g_bPreviousPaintTool;

/**
 * The decal being edited.
 *
 * @ghidraAddress USA: 0x8028db68
 * @ghidraAddress Asia: 0x8028D6B8
 * @ghidraAddress EUR: 0x802902c8
 * @ghidraAddress JPN: 0x8028fd38
 */
extern u8 g_bDecalSlot;

/**
 * The decal edited before the last was picked.
 *
 * @ghidraAddress USA: 0x8028db6c
 * @ghidraAddress Asia: 0x8028D6BC
 * @ghidraAddress EUR: 0x802902cc
 * @ghidraAddress JPN: 0x8028fd3c
 */
extern u8 g_bPreviousDecalSlot;

/**
 * @ghidraAddress USA: 0x8028db70
 * @ghidraAddress Asia: 0x8028D6C0
 * @ghidraAddress EUR: 0x802902d0
 * @ghidraAddress JPN: 0x8028fd40
 */
extern u8 g_bSavedDecalSlot;

/**
 * The decal the copy that undo restores was taken of.
 *
 * @ghidraAddress USA: 0x8028db74
 * @ghidraAddress Asia: 0x8028D6C4
 * @ghidraAddress EUR: 0x802902d4
 * @ghidraAddress JPN: 0x8028fd44
 */
extern u8 g_bUndoDecalSlot;

/**
 * The texels of the decal being edited, 4 bits each.
 *
 * @ghidraAddress USA: 0x8028db78
 * @ghidraAddress Asia: 0x8028D6C8
 * @ghidraAddress EUR: 0x802902d8
 * @ghidraAddress JPN: 0x8028fd48
 */
extern u8 *g_pDecalTexels;

/**
 * The mask of the decal being edited, or NULL.
 *
 * @ghidraAddress USA: 0x8028db7c
 * @ghidraAddress Asia: 0x8028D6CC
 * @ghidraAddress EUR: 0x802902dc
 * @ghidraAddress JPN: 0x8028fd4c
 */
extern u8 *g_pDecalMask;

/**
 * A copy of the texels the flood fill works in.
 *
 * @ghidraAddress USA: 0x8028db84
 * @ghidraAddress Asia: 0x8028D6D4
 * @ghidraAddress EUR: 0x802902e4
 * @ghidraAddress JPN: 0x8028fd54
 */
extern u8 *g_pDecalScratchTexels;

/**
 * The size of the decal being edited, in texels.
 *
 * @ghidraAddress USA: 0x8028db88
 * @ghidraAddress Asia: 0x8028D6D8
 * @ghidraAddress EUR: 0x802902e8
 * @ghidraAddress JPN: 0x8028fd58
 */
extern u8 g_bDecalWidth;

/**
 * @ghidraAddress USA: 0x8028db8c
 * @ghidraAddress Asia: 0x8028D6DC
 * @ghidraAddress EUR: 0x802902ec
 * @ghidraAddress JPN: 0x8028fd5c
 */
extern u8 g_bDecalHeight;

/**
 * The palette of the decal being edited, in RGBA 5551.
 *
 * @ghidraAddress USA: 0x8028db90
 * @ghidraAddress Asia: 0x8028D6E0
 * @ghidraAddress EUR: 0x802902f0
 * @ghidraAddress JPN: 0x8028fd60
 */
extern u16 *g_pDecalPalette;

/**
 * Where the decal is shown, 4 pixels to a texel.
 *
 * @ghidraAddress USA: 0x8028db94
 * @ghidraAddress Asia: 0x8028D6E4
 * @ghidraAddress EUR: 0x802902f4
 * @ghidraAddress JPN: 0x8028fd64
 */
extern ScreenRect g_rcDecalCanvas;

/**
 * The characters of the text to paint.
 *
 * @ghidraAddress USA: 0x8028dba8
 * @ghidraAddress Asia: 0x8028D6F8
 * @ghidraAddress EUR: 0x80290308
 * @ghidraAddress JPN: 0x8028fd78
 */
extern u8 g_bDecalTextLength;

/**
 * The width of the text to paint, in texels.
 *
 * @ghidraAddress USA: 0x8028dbac
 * @ghidraAddress Asia: 0x8028D6FC
 * @ghidraAddress EUR: 0x8029030c
 * @ghidraAddress JPN: 0x8028fd7c
 */
extern u8 g_bDecalTextWidth;

/**
 * The edits not saved, for the warning on exit.
 *
 * @ghidraAddress USA: 0x8028dbb4
 * @ghidraAddress Asia: 0x8028D704
 * @ghidraAddress EUR: 0x80290314
 * @ghidraAddress JPN: 0x8028fd84
 */
extern u8 g_bDecalEditCount;

/**
 * The style under the cursor of a style menu.
 *
 * @ghidraAddress USA: 0x8028dbb8
 * @ghidraAddress Asia: 0x8028D708
 * @ghidraAddress EUR: 0x80290318
 * @ghidraAddress JPN: 0x8028fd88
 */
extern u8 g_bStyleMenuCursor;

/**
 * The player who paints, an index of g_aPlayerInputState.
 *
 * @ghidraAddress USA: 0x8028dbbc
 * @ghidraAddress Asia: 0x8028D70C
 * @ghidraAddress EUR: 0x8029031c
 * @ghidraAddress JPN: 0x8028fd8c
 */
extern u8 g_bPaintShopPlayer;

/**
 * Set while a tool waits for its second click. Opening the menu of the styles of a tool sets it
 * too, and the menu clears it.
 *
 * @ghidraAddress USA: 0x8028dbc0
 * @ghidraAddress Asia: 0x8028D710
 * @ghidraAddress EUR: 0x80290320
 * @ghidraAddress JPN: 0x8028fd90
 */
extern u8 g_bPaintToolStarted;

/**
 * Set while a press on the decal is being dealt with.
 *
 * @ghidraAddress USA: 0x8028dbc4
 * @ghidraAddress Asia: 0x8028D714
 * @ghidraAddress EUR: 0x80290324
 * @ghidraAddress JPN: 0x8028fd94
 */
extern u8 g_bDecalDragActive;

/**
 * Whether the palette can be clicked.
 *
 * @ghidraAddress USA: 0x8028dbc8
 * @ghidraAddress Asia: 0x8028D718
 * @ghidraAddress EUR: 0x80290328
 * @ghidraAddress JPN: 0x8028fd98
 */
extern u8 g_bPaletteEnabled;

/**
 * Whether the decal being edited has a mirror image.
 *
 * @ghidraAddress USA: 0x8028dbcc
 * @ghidraAddress Asia: 0x8028D71C
 * @ghidraAddress EUR: 0x8029032c
 * @ghidraAddress JPN: 0x8028fd9c
 */
extern u8 g_bDecalSlotHasMirror;

/**
 * Set when another decal is picked.
 *
 * @ghidraAddress USA: 0x8028dbd0
 * @ghidraAddress Asia: 0x8028D720
 * @ghidraAddress EUR: 0x80290330
 * @ghidraAddress JPN: 0x8028fda0
 */
extern u8 g_bDecalSlotChanged;

/**
 * Whether the color editor is open.
 *
 * @ghidraAddress USA: 0x8028dbd4
 * @ghidraAddress Asia: 0x8028D724
 * @ghidraAddress EUR: 0x80290334
 * @ghidraAddress JPN: 0x8028fda4
 */
extern u8 g_bColorEditorOpen;

/**
 * Whether the menu of the styles of the tool is open.
 *
 * @ghidraAddress USA: 0x8028dbe0
 * @ghidraAddress Asia: 0x8028D730
 * @ghidraAddress EUR: 0x80290340
 * @ghidraAddress JPN: 0x8028fdb0
 */
extern u8 g_bStyleMenuOpen;

/**
 * Whether the text menu shows the colors of the shadow.
 *
 * @ghidraAddress USA: 0x8028dbe4
 * @ghidraAddress Asia: 0x8028D734
 * @ghidraAddress EUR: 0x80290344
 * @ghidraAddress JPN: 0x8028fdb4
 */
extern u8 g_bTextShadowPickerOpen;

/**
 * Set once the paint shop fades out.
 *
 * @ghidraAddress USA: 0x8028dbe8
 * @ghidraAddress Asia: 0x8028D738
 * @ghidraAddress EUR: 0x80290348
 * @ghidraAddress JPN: 0x8028fdb8
 */
extern u8 g_bPaintShopExiting;

/**
 * The colors of the bars of the color editor.
 *
 * @ghidraAddress USA: 0x8028dcc8
 * @ghidraAddress Asia: 0x8028D818
 * @ghidraAddress EUR: 0x80290468
 * @ghidraAddress JPN: 0x8028fee8
 */
extern u8 g_aabColorEditorBars[3][3];

/**
 * The names of the bars of the color editor.
 *
 * @ghidraAddress USA: 0x8028dcd4
 * @ghidraAddress Asia: 0x8028D824
 * @ghidraAddress EUR: 0x80290474
 * @ghidraAddress JPN: 0x8028fef4
 */
extern const char *g_apszColorChannelNames[3];

/**
 * The bar of the color editor under its cursor.
 *
 * @ghidraAddress USA: 0x8028dce0
 * @ghidraAddress Asia: 0x8028D830
 * @ghidraAddress EUR: 0x80290480
 * @ghidraAddress JPN: 0x8028ff00
 */
extern u8 g_bColorEditorChannel;

/**
 * The names of the styles of the brush tool.
 *
 * @ghidraAddress USA: 0x8028dce4
 * @ghidraAddress Asia: 0x8028D834
 * @ghidraAddress EUR: 0x80290484
 */
extern const char *g_apszBrushStyleNames[2];

/**
 * The names of the styles of the text tool.
 *
 * @ghidraAddress USA: 0x8028dcec
 * @ghidraAddress Asia: 0x8028D83C
 * @ghidraAddress EUR: 0x8029048c
 */
extern const char *g_apszTextStyleNames[2];

/**
 * The names of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8028dcf4
 * @ghidraAddress Asia: 0x8028D844
 * @ghidraAddress EUR: 0x80290494
 */
extern const char *g_apszRectStyleNames[4];

/**
 * The names of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x8028dd04
 * @ghidraAddress Asia: 0x8028D854
 * @ghidraAddress EUR: 0x802904a4
 */
extern const char *g_apszOvalStyleNames[4];

/**
 * The palette of the decal as it is shown.
 *
 * @ghidraAddress USA: 0x80369b98
 * @ghidraAddress Asia: 0x80369648
 * @ghidraAddress EUR: 0x8036c098
 * @ghidraAddress JPN: 0x8036acf8
 */
extern PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];

/**
 * The buttons of the tools.
 *
 * @ghidraAddress USA: 0x80369cd8
 * @ghidraAddress Asia: 0x80369788
 * @ghidraAddress EUR: 0x8036c1d8
 * @ghidraAddress JPN: 0x8036ae38
 */
extern ScreenRect g_aPaintToolButtonRects[PAINT_TOOLS];

/**
 * The color the color editor restores when it is cancelled.
 *
 * @ghidraAddress USA: 0x80369da8
 * @ghidraAddress Asia: 0x80369858
 * @ghidraAddress EUR: 0x8036c2a8
 * @ghidraAddress JPN: 0x8036af08
 */
extern u8 g_abSavedSwatchColor[3];

/**
 * The buttons of the decals.
 *
 * @ghidraAddress USA: 0x80369db0
 * @ghidraAddress Asia: 0x80369860
 * @ghidraAddress EUR: 0x8036c2b0
 * @ghidraAddress JPN: 0x8036af10
 */
extern ScreenRect g_aDecalSlotRects[DECAL_SLOTS];

/**
 * The glyphs of the text to paint.
 *
 * @ghidraAddress USA: 0x80369e68
 * @ghidraAddress Asia: 0x80369918
 * @ghidraAddress EUR: 0x8036c368
 * @ghidraAddress JPN: 0x8036afc8
 */
extern DecalGlyph *g_apDecalTextGlyphs[DECAL_TEXT_LENGTH];

/**
 * Save a copy of the decal being edited for undo, and count an edit.
 *
 * @ghidraAddress USA: 0x80244ca8
 * @ghidraAddress Asia: 0x802447F8
 * @ghidraAddress EUR: 0x802461cc
 * @ghidraAddress JPN: 0x80245cc0
 */
void SaveDecalUndo(void);

/**
 * Restore the copy of the decal saved for undo.
 *
 * @ghidraAddress USA: 0x80244d04
 * @ghidraAddress Asia: 0x80244854
 * @ghidraAddress EUR: 0x80246228
 * @ghidraAddress JPN: 0x80245d1c
 */
void UndoDecalEdit(void);

#ifdef ENABLE_PATCHES
/** Reapply the most recently undone paint edit. */
void RedoDecalEdit(void);

/** Record every decal before a whole-car edit. */
void SaveAllDecalUndo(void);

/** Record the paint palette before editing it. */
void SaveDecalPaletteUndo(void);

/** Discard the palette entry when the colour editor is cancelled. */
void CancelDecalPaletteUndo(void);
#endif

/**
 * Run one frame of the keyboard of the text tool.
 *
 * @ghidraAddress USA: 0x8024ac70
 * @ghidraAddress Asia: 0x8024A7C0
 * @ghidraAddress EUR: 0x8024c1d0
 * @ghidraAddress JPN: 0x8024bbd0
 */
void ProcessTextInputKeyboard(void);

/**
 * Run one frame of the color editor of the color in use.
 *
 * @ghidraAddress USA: 0x8024b144
 * @ghidraAddress Asia: 0x8024AC94
 * @ghidraAddress EUR: 0x8024c6a8
 * @ghidraAddress JPN: 0x8024c0a8
 */
void ProcessColorEditorDialog(void);

/**
 * Move the cursor by the stick, the D-pad and the C buttons.
 *
 * @ghidraAddress USA: 0x8024be78
 * @ghidraAddress Asia: 0x8024B9C8
 * @ghidraAddress EUR: 0x8024d3dc
 * @ghidraAddress JPN: 0x8024cddc
 */
void UpdateCursorFromInput(void);

/**
 * Deal with a frame of the input of the decal editor: the decals, the palette, the tools and
 * painting.
 *
 * @ghidraAddress USA: 0x8024c184
 * @ghidraAddress Asia: 0x8024BCD4
 * @ghidraAddress EUR: 0x8024d6e8
 * @ghidraAddress JPN: 0x8024d0e8
 */
void ProcessDecalEditorInput(void);

/**
 * Whether the cursor is on a sprite as it is drawn.
 *
 * @param sprite The sprite.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d374
 * @ghidraAddress Asia: 0x8024CEC4
 * @ghidraAddress EUR: 0x8024e8d8
 * @ghidraAddress JPN: 0x8024e2b8
 */
s32 IsCursorInButton(Sprite *sprite);

/**
 * Whether the cursor is in a rectangle.
 *
 * @param rect The rectangle.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d3f0
 * @ghidraAddress Asia: 0x8024CF40
 * @ghidraAddress EUR: 0x8024e954
 * @ghidraAddress JPN: 0x8024e334
 */
s32 IsCursorInRect(ScreenRect *rect);

/**
 * Whether the cursor is on a swatch of the palette.
 *
 * @param swatch The swatch.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d45c
 * @ghidraAddress Asia: 0x8024CFAC
 * @ghidraAddress EUR: 0x8024e9c0
 * @ghidraAddress JPN: 0x8024e3a0
 */
s32 IsCursorInSwatch(PaletteSwatch *swatch);

/**
 * Whether the cursor is on a key of the keyboard.
 *
 * @param key The key.
 * @return Whether it is, the edges included.
 * @ghidraAddress USA: 0x8024d4c8
 * @ghidraAddress Asia: 0x8024D018
 * @ghidraAddress EUR: 0x8024ea2c
 * @ghidraAddress JPN: 0x8024e40c
 */
s32 IsCursorOnKeyboardKey(DecalGlyph *key);

/**
 * Set the colors of the swatches from the palette of the decal.
 *
 * @ghidraAddress USA: 0x8024d53c
 * @ghidraAddress Asia: 0x8024D08C
 * @ghidraAddress EUR: 0x8024eaa0
 * @ghidraAddress JPN: 0x8024e480
 */
void ConvertPaletteToRgb(void);

/**
 * Draw the palette, with the color in use outlined.
 *
 * @ghidraAddress USA: 0x8024d6b8
 * @ghidraAddress Asia: 0x8024D208
 * @ghidraAddress EUR: 0x8024ec1c
 * @ghidraAddress JPN: 0x8024e5fc
 */
void DrawDecalColorPalette(void);

/**
 * Run one frame of the menu of the styles of the brush tool.
 *
 * @ghidraAddress USA: 0x8024d89c
 * @ghidraAddress Asia: 0x8024D3EC
 * @ghidraAddress EUR: 0x8024ee00
 * @ghidraAddress JPN: 0x8024e7e0
 */
void ProcessBrushStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the text tool, and of its colors of the shadow.
 *
 * @ghidraAddress USA: 0x8024e128
 * @ghidraAddress Asia: 0x8024DC78
 * @ghidraAddress EUR: 0x8024f68c
 * @ghidraAddress JPN: 0x8024efec
 */
void ProcessTextStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the rectangle tool.
 *
 * @ghidraAddress USA: 0x8024f39c
 * @ghidraAddress Asia: 0x8024EEEC
 * @ghidraAddress EUR: 0x80250900
 * @ghidraAddress JPN: 0x802501c4
 */
void ProcessRectStyleMenu(void);

/**
 * Run one frame of the menu of the styles of the oval tool.
 *
 * @ghidraAddress USA: 0x80250698
 * @ghidraAddress Asia: 0x802501E8
 * @ghidraAddress EUR: 0x80251bfc
 * @ghidraAddress JPN: 0x80251434
 */
void ProcessOvalStyleMenu(void);

/**
 * Run one frame of the dialog that warns of edits not saved on exit from the paint shop, or fade
 * out at once when there are none.
 *
 * @ghidraAddress USA: 0x802531c0
 * @ghidraAddress Asia: 0x80252D10
 * @ghidraAddress EUR: 0x80254728
 * @ghidraAddress JPN: 0x80253ed4
 */
void ProcessPaintShopExitConfirm(void);

/**
 * Set the brush to its size and every remembered decal to the decal being edited.
 *
 * @ghidraAddress USA: 0x802533f4
 * @ghidraAddress Asia: 0x80252F44
 * @ghidraAddress EUR: 0x8025495c
 * @ghidraAddress JPN: 0x80254140
 */
void ResetBrushAndSlotHistory(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_DECALEDITOR_H */

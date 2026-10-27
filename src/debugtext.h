#ifndef COASTLINE_DEBUGTEXT_H
#define COASTLINE_DEBUGTEXT_H

/**
 * @file
 * The bitmap text the debug tools draw straight into the frame buffer, and the other debug aids
 * beside it.
 */

#include <PR/ultratypes.h>

/** The characters of the bitmap fonts, from space to the last of `{|}~`. */
#define BITMAP_FONT_GLYPHS 69

/** The rows of a glyph. */
#define BITMAP_FONT_ROWS 7

/* The columns of a glyph of each font. */
#define BITMAP_FONT_WIDE_COLUMNS 5   /*!< The columns of a glyph of the wide font. */
#define BITMAP_FONT_NARROW_COLUMNS 3 /*!< The columns of a glyph of the narrow font. */

/**
 * The wide font. A pixel is drawn in the text colour where its entry is not zero.
 *
 * @ghidraAddress USA: 0x8028dfc0
 * @ghidraAddress EUR: 0x80290780
 * @ghidraAddress JPN: 0x80290220
 */
extern s16 g_aBitmapFontWide[BITMAP_FONT_GLYPHS][BITMAP_FONT_ROWS][BITMAP_FONT_WIDE_COLUMNS];

/**
 * The narrow font. Each entry not zero is the colour of its pixel.
 *
 * @ghidraAddress USA: 0x8028f2a0
 * @ghidraAddress EUR: 0x80291a60
 * @ghidraAddress JPN: 0x80291500
 */
extern s16 g_aBitmapFontNarrow[BITMAP_FONT_GLYPHS][BITMAP_FONT_ROWS][BITMAP_FONT_NARROW_COLUMNS];

/**
 * The column where the bitmap text starts its line.
 *
 * @ghidraAddress USA: 0x8028ddd0
 * @ghidraAddress EUR: 0x80290570
 * @ghidraAddress JPN: 0x80290010
 */
extern s32 g_dwBitmapTextX;

/**
 * The row where the bitmap text starts.
 *
 * @ghidraAddress USA: 0x8028ddd4
 * @ghidraAddress EUR: 0x80290574
 * @ghidraAddress JPN: 0x80290014
 */
extern s32 g_dwBitmapTextY;

/**
 * Whether the bitmap text uses the narrow font.
 *
 * @ghidraAddress USA: 0x8028ddd8
 * @ghidraAddress EUR: 0x80290578
 * @ghidraAddress JPN: 0x80290018
 */
extern s32 g_bBitmapTextNarrow;

/**
 * The colour of the wide font.
 *
 * @ghidraAddress USA: 0x8028dddc
 * @ghidraAddress EUR: 0x8029057c
 * @ghidraAddress JPN: 0x8029001c
 */
extern u16 g_wBitmapTextColor;

/**
 * The step StepDebugHiddenInstance() takes through the placed models, or 0 to stay.
 *
 * @ghidraAddress USA: 0x8028dde0
 * @ghidraAddress EUR: 0x80290580
 * @ghidraAddress JPN: 0x80290020
 * @ghidraAddress BRD3D: 0x10277b20
 * @ghidraAddress BRGlide: 0x10273308
 */
extern s32 g_nDebugInstanceCycleStep;

/**
 * A per-frame debug hook with an empty body.
 *
 * @ghidraAddress USA: 0x80254870
 * @ghidraAddress EUR: 0x80255e10
 * @ghidraAddress JPN: 0x802557a0
 */
void DebugFrameStubA(void);

/**
 * Draw text into the frame buffer on display, outlined in black. Lower case draws as upper case,
 * a newline moves down a line, and another control character draws as a space.
 *
 * @param text The text, terminated by a null character.
 * @ghidraAddress USA: 0x80254878
 * @ghidraAddress EUR: 0x80255e18
 * @ghidraAddress JPN: 0x802557a8
 */
void DrawBitmapTextToFramebuffer(const char *text);

/**
 * Start the bitmap text at the top left in white, in the wide font.
 *
 * @ghidraAddress USA: 0x80254d4c
 * @ghidraAddress EUR: 0x802562ec
 * @ghidraAddress JPN: 0x80255c7c
 */
void InitBitmapText(void);

/**
 * Step the hidden placed model by the cycle step until it reaches one drawn this frame, or the
 * first, then stop stepping.
 *
 * @ghidraAddress USA: 0x80254d90
 * @ghidraAddress EUR: 0x80256330
 * @ghidraAddress JPN: 0x80255cc0
 * @ghidraAddress BRD3D: 0x1000ba70
 * @ghidraAddress BRGlide: 0x10008f90
 */
void StepDebugHiddenInstance(void);

/**
 * Start the debug statistics of a frame.
 *
 * @ghidraAddress USA: 0x80254e3c
 * @ghidraAddress EUR: 0x802563dc
 * @ghidraAddress JPN: 0x80255d6c
 */
void BeginDebugStats(void);

/**
 * A per-frame debug hook with an empty body.
 *
 * @ghidraAddress USA: 0x80254f2c
 * @ghidraAddress EUR: 0x802564cc
 * @ghidraAddress JPN: 0x80255e5c
 */
void DebugFrameStubB(void);

#endif

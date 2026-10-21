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

/** The columns of a glyph of each font. */
#define BITMAP_FONT_WIDE_COLUMNS 5
#define BITMAP_FONT_NARROW_COLUMNS 3

/**
 * The wide font. A pixel is drawn in the text colour where its entry is not zero.
 *
 * @ghidraAddress USA: 0x8028dfc0
 */
extern s16 g_aBitmapFontWide[BITMAP_FONT_GLYPHS][BITMAP_FONT_ROWS][BITMAP_FONT_WIDE_COLUMNS];

/**
 * The narrow font. Each entry not zero is the colour of its pixel.
 *
 * @ghidraAddress USA: 0x8028f2a0
 */
extern s16 g_aBitmapFontNarrow[BITMAP_FONT_GLYPHS][BITMAP_FONT_ROWS][BITMAP_FONT_NARROW_COLUMNS];

/**
 * The column where the bitmap text starts its line.
 *
 * @ghidraAddress USA: 0x8028ddd0
 */
extern s32 g_dwBitmapTextX;

/**
 * The row where the bitmap text starts.
 *
 * @ghidraAddress USA: 0x8028ddd4
 */
extern s32 g_dwBitmapTextY;

/**
 * Whether the bitmap text uses the narrow font.
 *
 * @ghidraAddress USA: 0x8028ddd8
 */
extern s32 g_bBitmapTextNarrow;

/**
 * The colour of the wide font.
 *
 * @ghidraAddress USA: 0x8028dddc
 */
extern u16 g_wBitmapTextColor;

/**
 * The step StepDebugHiddenInstance() takes through the placed models, or 0 to stay.
 *
 * @ghidraAddress USA: 0x8028dde0
 */
extern s32 g_nDebugInstanceCycleStep;

/**
 * A per-frame debug hook with an empty body.
 *
 * @ghidraAddress USA: 0x80254870
 */
void DebugFrameStubA(void);

/**
 * Draw text into the frame buffer on display, outlined in black. Lower case draws as upper case,
 * a newline moves down a line, and another control character draws as a space.
 *
 * @param text The text, terminated by a null character.
 * @ghidraAddress USA: 0x80254878
 */
void DrawBitmapTextToFramebuffer(const char *text);

/**
 * Start the bitmap text at the top left in white, in the wide font.
 *
 * @ghidraAddress USA: 0x80254d4c
 */
void InitBitmapText(void);

/**
 * Step the hidden placed model by the cycle step until it reaches one drawn this frame, or the
 * first, then stop stepping.
 *
 * @ghidraAddress USA: 0x80254d90
 */
void StepDebugHiddenInstance(void);

/**
 * Start the debug statistics of a frame.
 *
 * @ghidraAddress USA: 0x80254e3c
 */
void BeginDebugStats(void);

/**
 * A per-frame debug hook with an empty body.
 *
 * @ghidraAddress USA: 0x80254f2c
 */
void DebugFrameStubB(void);

#endif

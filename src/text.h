#ifndef COASTLINE_TEXT_H
#define COASTLINE_TEXT_H

/**
 * @file
 * The drawing of text with the game's two bitmap fonts.
 *
 * A glyph is an intensity and alpha cutout of a font texture. The second texture of the combiner
 * is a vertical gradient that blends the environment color at the top of a glyph into the
 * primitive color at its bottom. A string may change both colors with a `%` code of two letters.
 */

#include <PR/ultratypes.h>

/** Where DrawText() places text against its position. */
enum TextAlignment {
    TEXT_ALIGN_LEFT = 0,   /*!< The text starts at its position. */
    TEXT_ALIGN_RIGHT = 1,  /*!< The text ends at its position. */
    TEXT_ALIGN_CENTER = 2, /*!< The text is centred on its position. */
};

/**
 * Whether text draws in the bold colors.
 *
 * @ghidraAddress USA: 0x8028bdc0
 */
extern u8 g_bTextStyleBold;

/**
 * The alignment of DrawText(), one of the TextAlignment values.
 *
 * @ghidraAddress USA: 0x8028bdc4
 */
extern u8 g_nTextAlignment;

/**
 * Whether glyphs draw with the shadow gradient.
 *
 * @ghidraAddress USA: 0x8028bdc8
 */
extern u8 g_bTextShadowEnabled;

/**
 * Whether text draws in the colors of SetTextCustomColors() rather than the style.
 *
 * @ghidraAddress USA: 0x8028bdcc
 */
extern s32 g_bTextCustomColorsEnabled;

/**
 * The red of the top of custom text.
 *
 * @ghidraAddress USA: 0x8028bdd0
 */
extern s32 g_dwTextCustomColorR;

/**
 * The green of the top of custom text.
 *
 * @ghidraAddress USA: 0x8028bdd4
 */
extern s32 g_dwTextCustomColorG;

/**
 * The blue of the top of custom text.
 *
 * @ghidraAddress USA: 0x8028bdd8
 */
extern s32 g_dwTextCustomColorB;

/**
 * The red of the bottom of custom text.
 *
 * @ghidraAddress USA: 0x8028bddc
 */
extern s32 g_dwTextCustomColor2R;

/**
 * The green of the bottom of custom text.
 *
 * @ghidraAddress USA: 0x8028bde0
 */
extern s32 g_dwTextCustomColor2G;

/**
 * The blue of the bottom of custom text.
 *
 * @ghidraAddress USA: 0x8028bde4
 */
extern s32 g_dwTextCustomColor2B;

/**
 * The gradient of large glyphs.
 *
 * @ghidraAddress USA: 0x8028bdf0
 */
extern u8 g_abLargeFontGradient[];

/**
 * The gradient of large glyphs with a shadow.
 *
 * @ghidraAddress USA: 0x8028bf30
 */
extern u8 g_abLargeFontShadowGradient[];

/**
 * The gradient of small glyphs.
 *
 * @ghidraAddress USA: 0x8028c070
 */
extern u8 g_abSmallFontGradient[];

/**
 * The gradient of small glyphs with a shadow.
 *
 * @ghidraAddress USA: 0x8028c1b0
 */
extern u8 g_abSmallFontShadowGradient[];

/**
 * The large font, two rows of glyphs in 8-bit intensity and alpha.
 *
 * @ghidraAddress USA: 0x8028fe00
 */
extern u8 g_aLargeFontTexture[];

/**
 * The small font, two rows of glyphs in 8-bit intensity and alpha.
 *
 * @ghidraAddress USA: 0x8029da00
 */
extern u8 g_aSmallFontTexture[];

/**
 * The glyph of each character from `!` on.
 *
 * @ghidraAddress USA: 0x802a1740
 */
extern u8 g_abHudFontCharToGlyph[];

/**
 * The first column of each glyph of the large font, and one past the last glyph.
 *
 * @ghidraAddress USA: 0x802a17a0
 */
extern s32 g_adwLargeFontGlyphOffsets[];

/**
 * The first column of each glyph of the small font, and one past the last glyph.
 *
 * @ghidraAddress USA: 0x802a187c
 */
extern s32 g_adwSmallFontGlyphOffsets[];

/**
 * The left of the next text RenderTextToDisplayList() draws.
 *
 * @ghidraAddress USA: 0x803519d0
 */
extern s32 g_nTextDrawX;

/**
 * The baseline of the next text RenderTextToDisplayList() draws.
 *
 * @ghidraAddress USA: 0x803519d4
 */
extern s32 g_nTextDrawY;

/**
 * The height of text in pixels of the low-resolution screen.
 *
 * @ghidraAddress USA: 0x803519d8
 */
extern s32 g_nFontSize;

/**
 * Draw text at g_nTextDrawX and g_nTextDrawY.
 *
 * @param text The text to draw.
 * @ghidraAddress USA: 0x8022e4e0
 */
void RenderTextToDisplayList(const char *text);

/**
 * Draw text in the bold colors.
 *
 * @ghidraAddress USA: 0x8022f4cc
 */
void SetTextStyleBold(void);

/**
 * Draw glyphs with the shadow gradient.
 *
 * @ghidraAddress USA: 0x8022f4dc
 */
void EnableTextShadow(void);

/**
 * Draw glyphs with the plain gradient.
 *
 * @ghidraAddress USA: 0x8022f4ec
 */
void DisableTextShadow(void);

/**
 * Draw text in the normal style.
 *
 * @ghidraAddress USA: 0x8022f4f8
 */
void SetTextStyleNormal(void);

/**
 * Center text on its position.
 *
 * @ghidraAddress USA: 0x8022f504
 */
void SetTextAlignCenter(void);

/**
 * Start text at its position.
 *
 * @ghidraAddress USA: 0x8022f514
 */
void SetTextAlignLeft(void);

/**
 * End text at its position.
 *
 * @ghidraAddress USA: 0x8022f520
 */
void SetTextAlignRight(void);

/**
 * Set the colors of the top and the bottom of text.
 *
 * @param red The red component of the top, from 0 to 255.
 * @param green The green component of the top, from 0 to 255.
 * @param blue The blue component of the top, from 0 to 255.
 * @param red2 The red component of the bottom, from 0 to 255.
 * @param green2 The green component of the bottom, from 0 to 255.
 * @param blue2 The blue component of the bottom, from 0 to 255.
 * @ghidraAddress USA: 0x8022f530
 */
void SetTextCustomColors(s32 red, s32 green, s32 blue, s32 red2, s32 green2, s32 blue2);

/**
 * Set the colors of the top and the bottom of text from two red, green and blue triplets.
 *
 * @param color The red, green, and blue of the top.
 * @param color2 The red, green, and blue of the bottom.
 * @ghidraAddress USA: 0x8022f578
 */
void SetTextGradientColorsFromBytes(const u8 *color, const u8 *color2);

/**
 * Set the size of the font.
 *
 * @param size The height of text in pixels of the low-resolution screen.
 * @ghidraAddress USA: 0x8022f5d0
 */
void SetTextSize(s32 size);

/**
 * Draw text at a position, aligned as the last alignment set.
 *
 * @param text The text to draw.
 * @param x The horizontal position the alignment applies to, in pixels.
 * @param y The baseline, in pixels.
 * @ghidraAddress USA: 0x8022f5dc
 */
void DrawText(const char *text, s32 x, s32 y);

/**
 * Draw text at a position in fractions of the screen, measured from the bottom left. A unit is
 * a little under a thousandth of the screen.
 *
 * @param text The text to draw.
 * @param x The distance from the left edge, in units of the width.
 * @param y The distance from the bottom edge, in units of the height.
 * @param unused An argument that is not read.
 * @ghidraAddress USA: 0x8022f694
 */
void DrawHudLabelAtFraction(const char *text, f32 x, f32 y, s32 unused);

/**
 * The width in pixels of text drawn at a size.
 *
 * @param text The text to measure.
 * @param size The height of text in pixels of the low-resolution screen.
 * @return The width of the text, in pixels.
 * @ghidraAddress USA: 0x8022f720
 */
s32 MeasureTextWidth(const char *text, s32 size);

#endif

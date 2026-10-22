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
 * @ghidraAddress JPN: 0x8028df70
 * @ghidraAddress EUR: 0x8028e520
 */
extern u8 g_bTextStyleBold;

/**
 * The alignment of DrawText(), one of the TextAlignment values.
 *
 * @ghidraAddress USA: 0x8028bdc4
 * @ghidraAddress JPN: 0x8028df74
 * @ghidraAddress EUR: 0x8028e524
 */
extern u8 g_nTextAlignment;

/**
 * Whether glyphs draw with the shadow gradient.
 *
 * @ghidraAddress USA: 0x8028bdc8
 * @ghidraAddress JPN: 0x8028df78
 * @ghidraAddress EUR: 0x8028e528
 */
extern u8 g_bTextShadowEnabled;

/**
 * Whether text draws in the colors of SetTextCustomColors() rather than the style.
 *
 * @ghidraAddress USA: 0x8028bdcc
 * @ghidraAddress JPN: 0x8028df7c
 * @ghidraAddress EUR: 0x8028e52c
 */
extern s32 g_bTextCustomColorsEnabled;

/**
 * The red of the top of custom text.
 *
 * @ghidraAddress USA: 0x8028bdd0
 * @ghidraAddress JPN: 0x8028df80
 * @ghidraAddress EUR: 0x8028e530
 */
extern s32 g_dwTextCustomColorR;

/**
 * The green of the top of custom text.
 *
 * @ghidraAddress USA: 0x8028bdd4
 * @ghidraAddress JPN: 0x8028df84
 * @ghidraAddress EUR: 0x8028e534
 */
extern s32 g_dwTextCustomColorG;

/**
 * The blue of the top of custom text.
 *
 * @ghidraAddress USA: 0x8028bdd8
 * @ghidraAddress JPN: 0x8028df88
 * @ghidraAddress EUR: 0x8028e538
 */
extern s32 g_dwTextCustomColorB;

/**
 * The red of the bottom of custom text.
 *
 * @ghidraAddress USA: 0x8028bddc
 * @ghidraAddress JPN: 0x8028df8c
 * @ghidraAddress EUR: 0x8028e53c
 */
extern s32 g_dwTextCustomColor2R;

/**
 * The green of the bottom of custom text.
 *
 * @ghidraAddress USA: 0x8028bde0
 * @ghidraAddress JPN: 0x8028df90
 * @ghidraAddress EUR: 0x8028e540
 */
extern s32 g_dwTextCustomColor2G;

/**
 * The blue of the bottom of custom text.
 *
 * @ghidraAddress USA: 0x8028bde4
 * @ghidraAddress JPN: 0x8028df94
 * @ghidraAddress EUR: 0x8028e544
 */
extern s32 g_dwTextCustomColor2B;

/**
 * The gradient of large glyphs.
 *
 * @ghidraAddress USA: 0x8028bdf0
 * @ghidraAddress EUR: 0x8028e550
 */
extern u8 g_abLargeFontGradient[];

/**
 * The gradient of large glyphs with a shadow.
 *
 * @ghidraAddress USA: 0x8028bf30
 * @ghidraAddress EUR: 0x8028e690
 */
extern u8 g_abLargeFontShadowGradient[];

/**
 * The gradient of small glyphs.
 *
 * @ghidraAddress USA: 0x8028c070
 * @ghidraAddress JPN: 0x8028e220
 * @ghidraAddress EUR: 0x8028e7d0
 */
extern u8 g_abSmallFontGradient[];

/**
 * The gradient of small glyphs with a shadow.
 *
 * @ghidraAddress USA: 0x8028c1b0
 * @ghidraAddress JPN: 0x8028e360
 * @ghidraAddress EUR: 0x8028e910
 */
extern u8 g_abSmallFontShadowGradient[];

/**
 * The large font, two rows of glyphs in 8-bit intensity and alpha.
 *
 * @ghidraAddress USA: 0x8028fe00
 * @ghidraAddress EUR: 0x802925c0
 */
extern u8 g_aLargeFontTexture[];

/**
 * The small font, two rows of glyphs in 8-bit intensity and alpha.
 *
 * @ghidraAddress USA: 0x8029da00
 * @ghidraAddress JPN: 0x80292060
 * @ghidraAddress EUR: 0x802a01c0
 */
extern u8 g_aSmallFontTexture[];

/**
 * The glyph of each character from `!` on.
 *
 * @ghidraAddress USA: 0x802a1740
 * @ghidraAddress JPN: 0x80295da0
 * @ghidraAddress EUR: 0x802a3f00
 */
extern u8 g_abHudFontCharToGlyph[];

/**
 * The first column of each glyph of the large font, and one past the last glyph.
 *
 * @ghidraAddress USA: 0x802a17a0
 * @ghidraAddress EUR: 0x802a3f60
 */
extern s32 g_adwLargeFontGlyphOffsets[];

/**
 * The first column of each glyph of the small font, and one past the last glyph.
 *
 * @ghidraAddress USA: 0x802a187c
 * @ghidraAddress JPN: 0x8029fc60
 * @ghidraAddress EUR: 0x802a403c
 */
extern s32 g_adwSmallFontGlyphOffsets[];

/**
 * The left of the next text RenderTextToDisplayList() draws.
 *
 * @ghidraAddress USA: 0x803519d0
 * @ghidraAddress JPN: 0x80352b40
 * @ghidraAddress EUR: 0x80353ed0
 */
extern s32 g_nTextDrawX;

/**
 * The baseline of the next text RenderTextToDisplayList() draws.
 *
 * @ghidraAddress USA: 0x803519d4
 * @ghidraAddress JPN: 0x80352b44
 * @ghidraAddress EUR: 0x80353ed4
 */
extern s32 g_nTextDrawY;

/**
 * The height of text in pixels of the low-resolution screen.
 *
 * @ghidraAddress USA: 0x803519d8
 * @ghidraAddress JPN: 0x80352b48
 * @ghidraAddress EUR: 0x80353ed8
 */
extern s32 g_nFontSize;

#if TGR_REGION == TGR_REGION_JP
/** The characters the glyph cache maps, one for each byte value. */
#define GLYPH_CACHE_CHARACTERS 256

/** The bytes of the texture of a cached glyph. */
#define GLYPH_CACHE_SLOT_SIZE 0x5A0

/** The entries of the table of glyph offsets, one more than the glyphs it covers. */
#define GLYPH_ROM_OFFSETS 209

/**
 * The offset of each glyph from the start of the glyphs on the cartridge, from the character
 * 0x20, then one past the last. A negative offset marks a glyph stored without compression, and
 * the size of a glyph is the distance to the absolute offset of the next.
 *
 * @ghidraAddress JPN: 0x80352258
 */
extern s32 g_aGlyphRomOffsets[GLYPH_ROM_OFFSETS];

/**
 * Whether g_aGlyphRomOffsets has been read from the cartridge.
 *
 * @ghidraAddress JPN: 0x8028df60
 */
extern s32 g_bGlyphOffsetTableLoaded;

/**
 * The slot of the glyph cache of each character used since ResetGlyphCache(), or 0 for none.
 *
 * @ghidraAddress JPN: 0x80352058
 */
extern u8 g_abGlyphCharToSlot[GLYPH_CACHE_CHARACTERS];

/**
 * The character whose glyph is in each slot of the glyph cache, or 0 for none.
 *
 * @ghidraAddress JPN: 0x80351f50
 */
extern u8 g_abGlyphSlotChar[];

/**
 * The compressed glyph LoadCachedGlyphTextures() reads before it expands the glyph into its slot.
 *
 * @ghidraAddress JPN: 0x803525a0
 */
extern u8 g_abGlyphRleBuffer[GLYPH_CACHE_SLOT_SIZE];

/**
 * The next free slot of the glyph cache, from 1.
 *
 * @ghidraAddress JPN: 0x8028df50
 */
extern s32 g_nGlyphCacheNextSlot;

/**
 * The texture of slot 0 of the glyph cache, a slot before its memory.
 *
 * @ghidraAddress JPN: 0x8028df54
 */
extern u8 *g_pGlyphCacheBuffer;

/**
 * The end of the slots of the glyph cache.
 *
 * @ghidraAddress JPN: 0x8028df58
 */
extern u8 *g_pGlyphCacheBufferEnd;

/**
 * The number of slots that fit in the memory of the glyph cache.
 *
 * @ghidraAddress JPN: 0x8028df5c
 */
extern s32 g_nGlyphCacheSlotLimit;

/**
 * The memory after the BSS where the glyph cache starts, once aligned to 8 bytes. The link script
 * places it.
 *
 * @ghidraAddress JPN: 0x803837a0
 */
extern u8 g_abGlyphCacheMemory[];

/** The table of glyph offsets and the glyphs on the cartridge. The link script places them. */
extern u8 _cachedGlyphsSegmentRomStart[];

/**
 * Forget the glyph of every character and size the glyph cache to the memory after the BSS. The
 * start of each display list calls it.
 *
 * @ghidraAddress JPN: 0x8022f1a0
 * @stub{JPN}
 */
void ResetGlyphCache(void);

/**
 * Give a character a slot of the glyph cache if it has none.
 *
 * @param character The character.
 * @return The texture of the slot of the character, filled by the next LoadCachedGlyphTextures().
 * @ghidraAddress JPN: 0x8022f258
 */
u8 *GetCachedGlyphTexture(u32 character);

/**
 * Read the glyph of each character given a slot into the slot, unless the glyph is already there,
 * and forget the characters of the slots not given out. The table of offsets is read the first
 * time.
 *
 * @ghidraAddress JPN: 0x8022f2b0
 * @stub{JPN}
 */
void LoadCachedGlyphTextures(void);
#endif

/**
 * Draw text at g_nTextDrawX and g_nTextDrawY.
 *
 * @param text The text to draw.
 * @ghidraAddress USA: 0x8022e4e0
 * @ghidraAddress JPN: 0x8022f460
 * @ghidraAddress EUR: 0x8022f9f0
 * @stub{JPN}
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
 * @ghidraAddress JPN: 0x8023070c
 * @ghidraAddress EUR: 0x802309ec
 */
void EnableTextShadow(void);

/**
 * Draw glyphs with the plain gradient.
 *
 * @ghidraAddress USA: 0x8022f4ec
 * @ghidraAddress JPN: 0x8023071c
 * @ghidraAddress EUR: 0x802309fc
 */
void DisableTextShadow(void);

/**
 * Draw text in the normal style.
 *
 * @ghidraAddress USA: 0x8022f4f8
 * @ghidraAddress JPN: 0x80230728
 * @ghidraAddress EUR: 0x80230a08
 */
void SetTextStyleNormal(void);

/**
 * Center text on its position.
 *
 * @ghidraAddress USA: 0x8022f504
 * @ghidraAddress JPN: 0x80230734
 * @ghidraAddress EUR: 0x80230a14
 */
void SetTextAlignCenter(void);

/**
 * Start text at its position.
 *
 * @ghidraAddress USA: 0x8022f514
 * @ghidraAddress JPN: 0x80230744
 * @ghidraAddress EUR: 0x80230a24
 */
void SetTextAlignLeft(void);

/**
 * End text at its position.
 *
 * @ghidraAddress USA: 0x8022f520
 * @ghidraAddress JPN: 0x80230750
 * @ghidraAddress EUR: 0x80230a30
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
 * @ghidraAddress JPN: 0x80230760
 * @ghidraAddress EUR: 0x80230a40
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
 * @ghidraAddress JPN: 0x80230800
 * @ghidraAddress EUR: 0x80230ae0
 */
void SetTextSize(s32 size);

/**
 * Draw text at a position, aligned as the last alignment set.
 *
 * @param text The text to draw.
 * @param x The horizontal position the alignment applies to, in pixels.
 * @param y The baseline, in pixels.
 * @ghidraAddress USA: 0x8022f5dc
 * @ghidraAddress JPN: 0x8023080c
 * @ghidraAddress EUR: 0x80230aec
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
 * @ghidraAddress JPN: 0x80230950
 * @ghidraAddress EUR: 0x80230c30
 * @stub{JPN}
 */
s32 MeasureTextWidth(const char *text, s32 size);

#endif

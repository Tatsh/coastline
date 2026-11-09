#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** A menu-font glyph's dimensions and image index. */
typedef struct MenuGlyph {
    short nWidth;       /*!< The glyph width. */
    short nHeight;      /*!< The glyph height. */
    short nImage;       /*!< The glyph image index. */
    char abUnknown6[6]; /*!< The remaining glyph-record bytes at offset 0x06. */
} MenuGlyph;

/** The printable ASCII glyph records of the menu font. @ghidraAddress BRD3D: 0x100ac6e4 */
extern MenuGlyph g_MenuFontGlyphTable[95];
/** The printable ASCII glyph records of the alternate font. @ghidraAddress BRD3D: 0x100acb5c */
extern MenuGlyph g_AltFontGlyphTable[95];

/**
 * Byte offsets of the large-font glyphs in the first pixel buffer.
 *
 * @ghidraAddress BRD3D: 0x100a6070
 * @ghidraAddress BRGlide: 0x100a5978
 */
extern int g_adwHudFont0GlyphOffsets[28];

/**
 * Byte offsets of the large-font glyphs in the second pixel buffer.
 *
 * @ghidraAddress BRD3D: 0x100a60e0
 * @ghidraAddress BRGlide: 0x100a59e8
 */
extern int g_adwHudFont1GlyphOffsets[27];

/**
 * Byte offsets of the small-font glyphs in the first pixel buffer.
 *
 * @ghidraAddress BRD3D: 0x100a6150
 * @ghidraAddress BRGlide: 0x100a5a58
 */
extern int g_adwHudFont2GlyphOffsets[28];

/**
 * Byte offsets of the small-font glyphs in the second pixel buffer.
 *
 * @ghidraAddress BRD3D: 0x100a61c0
 * @ghidraAddress BRGlide: 0x100a5ac8
 */
extern int g_adwHudFont3GlyphOffsets[27];

/**
 * Texture handles of the first large-font glyph sheet.
 *
 * @ghidraAddress BRD3D: 0x11829238
 */
extern unsigned g_adwLargeFontGlyphTexturesA[27];

/**
 * Texture handles of the second large-font glyph sheet.
 *
 * @ghidraAddress BRD3D: 0x118292a8
 */
extern unsigned g_adwLargeFontGlyphTexturesB[26];

/**
 * Texture handles of the first small-font glyph sheet.
 *
 * @ghidraAddress BRD3D: 0x11829158
 */
extern unsigned g_adwSmallFontGlyphTexturesA[27];

/**
 * Texture handles of the second small-font glyph sheet.
 *
 * @ghidraAddress BRD3D: 0x118291c8
 */
extern unsigned g_adwSmallFontGlyphTexturesB[26];

#ifdef __cplusplus
}
#endif

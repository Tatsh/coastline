#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MenuGlyph {
    short nWidth;
    short nHeight;
    short nImage;
    char abUnknown6[6]; /* +0x06 */
} MenuGlyph;

/** @ghidraAddress BRD3D: 0x100ac6e4 */
extern MenuGlyph g_MenuFontGlyphTable[95];
/** @ghidraAddress BRD3D: 0x100acb5c */
extern MenuGlyph g_AltFontGlyphTable[95];

/**
 * @ghidraAddress BRD3D: 0x100a6070
 */
extern int g_adwHudFont0GlyphOffsets[28];

/**
 * @ghidraAddress BRD3D: 0x100a60e0
 */
extern int g_adwHudFont1GlyphOffsets[27];

/**
 * @ghidraAddress BRD3D: 0x100a6150
 */
extern int g_adwHudFont2GlyphOffsets[28];

/**
 * @ghidraAddress BRD3D: 0x100a61c0
 */
extern int g_adwHudFont3GlyphOffsets[27];

/**
 * @ghidraAddress BRD3D: 0x11829238
 */
extern unsigned g_adwLargeFontGlyphTexturesA[27];

/**
 * @ghidraAddress BRD3D: 0x118292a8
 */
extern unsigned g_adwLargeFontGlyphTexturesB[26];

/**
 * @ghidraAddress BRD3D: 0x11829158
 */
extern unsigned g_adwSmallFontGlyphTexturesA[27];

/**
 * @ghidraAddress BRD3D: 0x118291c8
 */
extern unsigned g_adwSmallFontGlyphTexturesB[26];

#ifdef __cplusplus
}
#endif

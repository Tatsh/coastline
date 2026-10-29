/**
 * @file
 * The atlases of the HUD fonts of the Glide renderer and their textures.
 */
#ifndef COASTLINE_FONTATLAS_H
#define COASTLINE_FONTATLAS_H

#ifdef __cplusplus
extern "C" {
#endif

/** The bytes of a glyph of g_abLargeFontAtlas, 64 texels across and 40 down. */
#define LARGE_FONT_ATLAS_GLYPH_BYTES 0xa00

/** The bytes of a glyph of g_abSmallFontAtlas, 32 texels across and 20 down. */
#define SMALL_FONT_ATLAS_GLYPH_BYTES 0x280

/** The number of glyphs of each atlas. */
#define FONT_ATLAS_GLYPHS 54

/**
 * The glyphs of the large HUD font, in the alpha-intensity 4:4 format of Glide, one after another.
 *
 * @ghidraAddress BRGlide: 0x1007b618
 */
extern unsigned char g_abLargeFontAtlas[LARGE_FONT_ATLAS_GLYPH_BYTES * FONT_ATLAS_GLYPHS];

/**
 * The glyphs of the small HUD font, in the alpha-intensity 4:4 format of Glide, one after another.
 *
 * @ghidraAddress BRGlide: 0x1009d218
 */
extern unsigned char g_abSmallFontAtlas[SMALL_FONT_ATLAS_GLYPH_BYTES * FONT_ATLAS_GLYPHS];

/**
 * The entry of g_pTextureEntries that draws a glyph of g_abLargeFontAtlas.
 *
 * @ghidraAddress BRGlide: 0x1184c47c
 */
extern int g_dwLargeFontAtlasTextureId;

/**
 * The entry of g_pTextureEntries that draws a glyph of g_abSmallFontAtlas.
 *
 * @ghidraAddress BRGlide: 0x1184c46c
 */
extern int g_dwSmallFontAtlasTextureId;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_FONTATLAS_H */

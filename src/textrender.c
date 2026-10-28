#include <PR/gbi.h>

#include "displaylistptr.h"
#include "render.h"
#include "screen.h"
#include "text.h"
#if TGR_PC
#include <stdio.h>

#include "win32/fontdata.h"
#include "win32/gbicommand.h"
#include "win32/settings.h"
#if TGR_RENDERER == TGR_RENDERER_GLIDE
#include "win32/glide/fontatlas.h"
#endif

#undef gDisplayListWords
#define gDisplayListWords(pkt, c0, c1)                                                             \
    {                                                                                              \
        GbiCommand *command = (pkt);                                                               \
        command->w0 = (c0);                                                                        \
        command->w1 = (c1);                                                                        \
    }
#define SPACE_ADVANCE 14

u8 g_bTextShadowEnabled;
s32 g_bTextCustomColorsEnabled;
s32 g_dwTextCustomColor2R;
s32 g_dwTextCustomColor2G;
s32 g_dwTextCustomColor2B;
#else
#define SPACE_ADVANCE 12
#endif /* TGR_PC */

#define SMALL_FONT_MAX_SIZE 25
#define SMALL_FONT_CELL 20
#define SMALL_FONT_WIDTH 392
#define SMALL_FONT_PADDING 4
#define LARGE_FONT_CELL 40
#define LARGE_FONT_WIDTH 704
#define LARGE_FONT_PADDING 7
#define FIRST_ROW_GLYPH 27
#define SIZE_UNITS 40
#define ASCENT 30
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define GRADIENT_TMEM 0x1B0
#if TGR_PLATFORM == TGR_PLATFORM_SDL
/* The 320 bytes of a gradient, which the console loads twice over into the next gradient. */
#define GRADIENT_LOAD_TEXELS 160
#else
#define GRADIENT_LOAD_TEXELS 320
#endif
#define TEXEL_STEP 0x400
#define HALF_TEXEL 16

#if TGR_PC && TGR_RENDERER == TGR_RENDERER_GLIDE
/* The glyphs of an atlas before this one have their offsets in the first table. */
#define FIRST_TABLE_GLYPHS 28

/*
 * The Glide renderer reads the offsets of all 54 glyphs of an atlas through the first table. The
 * second table follows the first in the original's data.
 */
static inline s32 GetAtlasGlyphOffset(const s32 *first, const s32 *second, s32 glyph) {
    return glyph < FIRST_TABLE_GLYPHS ? first[glyph] : second[glyph - FIRST_TABLE_GLYPHS];
}
#endif /* TGR_PC && TGR_RENDERER == TGR_RENDERER_GLIDE */

#if TGR_REGION != TGR_REGION_JP || TGR_PC
void RenderTextToDisplayList(const char *text) {
    const u8 *s = (const u8 *)text;
    s32 size = g_nFontSize;
    s32 y = g_nTextDrawY - (size * ASCENT) / SIZE_UNITS;
    s32 x = g_nTextDrawX;
    s32 cell, padding, glyph, columns, width;
#if TGR_PC && TGR_RENDERER == TGR_RENDERER_GLIDE
    const s32 *nextOffsets;
    u8 *atlas;
    s32 atlasTexture, glyphBytes;
#elif TGR_PC
    unsigned *glyphTextures;
#else
    s32 textureWidth, row, left, right, dsdx, dtdy, offset;
    s32 sStart;
    s16 left4, top4, right4, bottom4;
#endif
    s32 *offsets;
    u8 *gradient, *shadowGradient;
#if !TGR_PC
    u8 *texture;
#endif
    u8 c;

    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        size *= 2;
    }
    if (size < SMALL_FONT_MAX_SIZE
#if TGR_BRD3D
        || g_dwTextureQualityLevel > 1
#endif
    ) {
#if TGR_PC && TGR_RENDERER == TGR_RENDERER_GLIDE
        padding = 1;
        offsets = g_adwHudFont2GlyphOffsets;
        nextOffsets = g_adwHudFont3GlyphOffsets;
        atlas = g_abSmallFontAtlas;
        atlasTexture = g_dwSmallFontAtlasTextureId;
        glyphBytes = SMALL_FONT_ATLAS_GLYPH_BYTES;
#elif TGR_PC
        padding = 1;
        offsets = g_adwHudFont2GlyphOffsets;
        glyphTextures = g_adwSmallFontGlyphTexturesA;
#else
        padding = SMALL_FONT_PADDING;
        offsets = g_adwSmallFontGlyphOffsets;
        textureWidth = SMALL_FONT_WIDTH;
        texture = g_aSmallFontTexture;
#endif /* TGR_PC */
        gradient = g_abSmallFontGradient;
        shadowGradient = g_abSmallFontShadowGradient;
        cell = SMALL_FONT_CELL;
    } else {
#if TGR_PC && TGR_RENDERER == TGR_RENDERER_GLIDE
        padding = 1;
        offsets = g_adwHudFont0GlyphOffsets;
        nextOffsets = g_adwHudFont1GlyphOffsets;
        atlas = g_abLargeFontAtlas;
        atlasTexture = g_dwLargeFontAtlasTextureId;
        glyphBytes = LARGE_FONT_ATLAS_GLYPH_BYTES;
#elif TGR_PC
        padding = 1;
        offsets = g_adwHudFont0GlyphOffsets;
        glyphTextures = g_adwLargeFontGlyphTexturesA;
#else
        padding = LARGE_FONT_PADDING;
        offsets = g_adwLargeFontGlyphOffsets;
        textureWidth = LARGE_FONT_WIDTH;
        texture = g_aLargeFontTexture;
#endif /* TGR_PC */
        gradient = g_abLargeFontGradient;
        shadowGradient = g_abLargeFontShadowGradient;
        cell = LARGE_FONT_CELL;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    if (g_bTextShadowEnabled) {
        gDPSetCombineLERP(gDisplayListPtr++,
                          PRIMITIVE,
                          ENVIRONMENT,
                          TEXEL1,
                          ENVIRONMENT,
                          0,
                          0,
                          0,
                          TEXEL0,
                          COMBINED,
                          TEXEL0,
                          TEXEL1,
                          0,
                          0,
                          0,
                          0,
                          TEXEL1);
    } else {
        gDPSetCombineLERP(gDisplayListPtr++,
                          PRIMITIVE,
                          ENVIRONMENT,
                          TEXEL1,
                          ENVIRONMENT,
                          0,
                          0,
                          0,
                          TEXEL0,
                          COMBINED,
                          0,
                          TEXEL1,
                          0,
                          0,
                          0,
                          0,
                          TEXEL1);
    }
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_FULL | IM_RD |
                         GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPTileSync(gDisplayListPtr++);
    gDPLoadSync(gDisplayListPtr++);
    gDPPipeSync(gDisplayListPtr++);

    /* The gradient is the second texture of the combiner, 8 texels wide and 40 tall. */
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_RGBA,
               G_IM_SIZ_16b,
               0,
               GRADIENT_TMEM,
               G_TX_LOADTILE,
               0,
               G_TX_WRAP,
               0,
               G_TX_NOLOD,
               G_TX_WRAP,
               0,
               G_TX_NOLOD);
    gDPSetTextureImage(gDisplayListPtr++,
                       G_IM_FMT_RGBA,
                       G_IM_SIZ_16b,
                       1,
                       g_bTextShadowEnabled ? shadowGradient : gradient);
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, GRADIENT_LOAD_TEXELS - 1, 0);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_IA,
               G_IM_SIZ_8b,
               1,
               GRADIENT_TMEM,
               1,
               0,
               G_TX_CLAMP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               3,
               G_TX_NOLOD);
    gDPSetTileSize(gDisplayListPtr++, 1, 2, 2, 7 * 4 + 2, 39 * 4 + 2);

    if (g_bTextCustomColorsEnabled) {
        gDPSetEnvColor(gDisplayListPtr++,
                       g_dwTextCustomColorR,
                       g_dwTextCustomColorG,
                       g_dwTextCustomColorB,
                       0xFF);
        gDPSetPrimColor(gDisplayListPtr++,
                        0xFF,
                        0xFF,
                        g_dwTextCustomColor2R,
                        g_dwTextCustomColor2G,
                        g_dwTextCustomColor2B,
                        0xFF);
    } else if (g_bTextStyleBold) {
        gDPSetEnvColor(gDisplayListPtr++, 0xFF, 0x7F, 0x00, 0xFF);
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF);
    } else {
        gDPSetEnvColor(gDisplayListPtr++, 0xC8, 0x00, 0x00, 0xFF);
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xE6, 0xE6, 0x00, 0xFF);
    }

    for (; *s != '\0'; s++) {
        c = *s;
        if (c == ' ') {
            x += (size * SPACE_ADVANCE) / SIZE_UNITS + 1;
            continue;
        }
        /* %% draws a percent sign, %i and %n draw nothing, and two letters set the colors. */
        if (c == '%' && s[1] != '\0') {
            if (s[1] == '%') {
                s++;
                c = *s;
            } else if (s[1] == 'i' || s[1] == 'n') {
                s++;
                continue;
#if TGR_PC
            } else if (s[1] == 'x') {
                unsigned red, green, blue;
                sscanf((const char *)s + 2, "%02x%02x%02x", &red, &green, &blue);
                gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, red, green, blue, 0xFF);
                red += 128;
                green += 128;
                blue += 128;
                if (red > 255) {
                    red = 255;
                }
                if (green > 255) {
                    green = 255;
                }
                if (blue > 255) {
                    blue = 255;
                }
                gDPSetEnvColor(gDisplayListPtr++, red, green, blue, 0xFF);
                s += 7;
                continue;
#endif /* TGR_PC */
            } else if (s[2] != '\0') {
                gDPPipeSync(gDisplayListPtr++);
                switch (s[1]) {
                case 'r':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xBE, 0x00, 0x00, 0xFF);
                    break;
                case 'o':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xCD, 0x5F, 0x00, 0xFF);
                    break;
                case 'O':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xFF, 0x78, 0x00, 0xFF);
                    break;
                case 'y':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xFF, 0xF5, 0x00, 0xFF);
                    break;
                case 'Y':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xFF, 0xFA, 0x80, 0xFF);
                    break;
                case 'g':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x00, 0x96, 0x00, 0xFF);
                    break;
                case 'b':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x00, 0x00, 0xC8, 0xFF);
                    break;
                case 'p':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xC8, 0x00, 0xC8, 0xFF);
                    break;
                case 'w':
                case '1':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
                    break;
                case '5':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0xFF);
                    break;
                case '0':
                    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF);
                    break;
                }
                switch (s[2]) {
                case 'r':
                    gDPSetEnvColor(gDisplayListPtr++, 0xC8, 0x00, 0x00, 0xFF);
                    break;
                case 'o':
                    gDPSetEnvColor(gDisplayListPtr++, 0xCD, 0x5F, 0x00, 0xFF);
                    break;
                case 'O':
                    gDPSetEnvColor(gDisplayListPtr++, 0xFF, 0x78, 0x00, 0xFF);
                    break;
                case 'y':
                    gDPSetEnvColor(gDisplayListPtr++, 0xD2, 0xBE, 0x00, 0xFF);
                    break;
                case 'Y':
                    gDPSetEnvColor(gDisplayListPtr++, 0xD2, 0xC8, 0x69, 0xFF);
                    break;
                case 'g':
                    gDPSetEnvColor(gDisplayListPtr++, 0x00, 0x96, 0x00, 0xFF);
                    break;
                case 'b':
                    gDPSetEnvColor(gDisplayListPtr++, 0x00, 0x00, 0xC8, 0xFF);
                    break;
                case 'p':
                    gDPSetEnvColor(gDisplayListPtr++, 0xC8, 0x00, 0xC8, 0xFF);
                    break;
                case 'w':
                case '1':
                    gDPSetEnvColor(gDisplayListPtr++, 0xFF, 0xFF, 0xFF, 0xFF);
                    break;
                case '5':
                    gDPSetEnvColor(gDisplayListPtr++, 0x80, 0x80, 0x80, 0xFF);
                    break;
                case '0':
                    gDPSetEnvColor(gDisplayListPtr++, 0x00, 0x00, 0x00, 0xFF);
                    break;
                }
                s += 2;
                continue;
            }
        }
        if (c < '!' || c >= 0x80) {
            continue;
        }

#if TGR_PC
        glyph = (s8)g_abHudFontCharToGlyph[c - '!'];
#if TGR_RENDERER == TGR_RENDERER_GLIDE
        columns = GetAtlasGlyphOffset(offsets, nextOffsets, glyph + 1) -
                  GetAtlasGlyphOffset(offsets, nextOffsets, glyph) + 1;
        /* The texture of the atlas samples the glyph from its bytes in the atlas. */
        gDisplayListWords(gDisplayListPtr++,
                          0xdd000000 | ((u32)atlasTexture & 0xffffff),
                          (u32)(atlas + glyphBytes * glyph));
        gDisplayListWords(gDisplayListPtr++, 0xdc000000 | ((u32)atlasTexture & 0xffffff), 1);
#else
        if (glyph >= 28) {
            offsets =
                cell == SMALL_FONT_CELL ? g_adwHudFont3GlyphOffsets : g_adwHudFont1GlyphOffsets;
            glyphTextures = cell == SMALL_FONT_CELL ? g_adwSmallFontGlyphTexturesB :
                                                      g_adwLargeFontGlyphTexturesB;
            glyph -= 28;
        } else {
            offsets =
                cell == SMALL_FONT_CELL ? g_adwHudFont2GlyphOffsets : g_adwHudFont0GlyphOffsets;
            glyphTextures = cell == SMALL_FONT_CELL ? g_adwSmallFontGlyphTexturesA :
                                                      g_adwLargeFontGlyphTexturesA;
        }
        columns = offsets[glyph + 1] - offsets[glyph] + 1;
        gDisplayListWords(gDisplayListPtr++, 0xdc000000 | (glyphTextures[glyph] & 0xffffff), 1);
#endif /* TGR_RENDERER == TGR_RENDERER_GLIDE */
        gDisplayListWords(gDisplayListPtr++, 0xde000000, 0x3f800000);
        gDisplayListWords(gDisplayListPtr++, 0xdf000000, 0xbf800000);
        gDPSetTileSize(
            gDisplayListPtr++, G_TX_RENDERTILE, 2, 2, (columns - 1) * 4 + 2, (cell - 1) * 4 + 2);
        width = (columns * size) / cell;
        if (x >= 0 && x + width <= SCREEN_WIDTH && y >= 0 && y + size <= SCREEN_HEIGHT) {
            gDisplayListWords(gDisplayListPtr++,
                              0xe3000000 | (((x + width) & 0xfff) << 12) | ((y + size) & 0xfff),
                              ((x & 0xfff) << 12) | (y & 0xfff));
        } else {
            s32 left = (s16)x;
            s32 top = (s16)y;
            s32 right = (s16)(x + width);
            s32 bottom = (s16)(y + size);
            gDisplayListWords(gDisplayListPtr++,
                              0xe3000000 | (((right > 0 ? right : 0) & 0xfff) << 12) |
                                  ((bottom > 0 ? bottom : 0) & 0xfff),
                              (((left > 0 ? left : 0) & 0xfff) << 12) |
                                  ((top > 0 ? top : 0) & 0xfff));
        }
#else
        /* Load the columns of the glyph, then sample them with the render tile. */
        glyph = g_abHudFontCharToGlyph[c - '!'];
        row = glyph < FIRST_ROW_GLYPH ? cell : 0;
        left = offsets[glyph];
        columns = offsets[glyph + 1] - left + 1;
        right = left + columns;
        dtdy = -(((cell << 10) - TEXEL_STEP) / size);
        gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_IA, G_IM_SIZ_8b, textureWidth, texture);
        /* The original takes the width of the load back out of its right edge. */
        gDPSetTile(gDisplayListPtr++,
                   G_IM_FMT_IA,
                   G_IM_SIZ_8b,
                   (right - left + 8) >> 3,
                   0,
                   G_TX_LOADTILE,
                   0,
                   G_TX_CLAMP,
                   6,
                   G_TX_NOLOD,
                   G_TX_CLAMP,
                   6,
                   G_TX_NOLOD);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadTile(
            gDisplayListPtr++, G_TX_LOADTILE, left * 4, row * 4, right * 4, (row + cell) * 4);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++,
                   G_IM_FMT_IA,
                   G_IM_SIZ_8b,
                   (columns + 8) >> 3,
                   0,
                   G_TX_RENDERTILE,
                   0,
                   G_TX_CLAMP,
                   6,
                   G_TX_NOLOD,
                   G_TX_CLAMP,
                   6,
                   G_TX_NOLOD);
        gDPSetTileSize(
            gDisplayListPtr++, G_TX_RENDERTILE, 2, 2, (columns - 1) * 4 + 2, (cell - 1) * 4 + 2);

        /* The glyph is stored upside down, so t starts at its last row and steps back. */
        if (x >= 0 && x + (columns * size) / cell <= SCREEN_WIDTH && y >= 0 &&
            y + size <= SCREEN_HEIGHT) {
            width = (columns * size) / cell;
            gSPTextureRectangle(gDisplayListPtr++,
                                x * 4,
                                y * 4,
                                (x + width) * 4,
                                (y + size) * 4,
                                G_TX_RENDERTILE,
                                HALF_TEXEL,
                                ((cell - 1) << 5) + HALF_TEXEL,
                                (columns << 10) / width,
                                dtdy);
        } else {
            /* A corner off the screen moves to its edge, and the texture start moves with it. */
            width = (columns * size) / cell;
            left4 = x * 4;
            top4 = y * 4;
            right4 = (x + width) * 4;
            bottom4 = (y + size) * 4;
            if (left4 < 0) {
                dsdx = (columns << 10) / width;
                if ((s16)dsdx < 0) {
                    offset = (left4 * (s16)dsdx) >> 7;
                    offset = offset > 0 ? offset : 0;
                } else {
                    offset = (left4 * (s16)dsdx) >> 7;
                    offset = offset < 0 ? offset : 0;
                }
            } else {
                dsdx = (columns << 10) / width;
                offset = 0;
            }
            sStart = HALF_TEXEL - offset;
            offset = 0;
            /* The original tests the top edge before it is narrowed to 16 bits. */
            if (y * 4 < 0) {
                offset = (top4 * (s16)dtdy) >> 7;
                if ((s16)dtdy < 0) {
                    offset = offset > 0 ? offset : 0;
                } else {
                    offset = offset < 0 ? offset : 0;
                }
            }
            gSPTextureRectangle(gDisplayListPtr++,
                                left4 > 0 ? left4 : 0,
                                top4 > 0 ? top4 : 0,
                                right4 > 0 ? right4 : 0,
                                bottom4 > 0 ? bottom4 : 0,
                                G_TX_RENDERTILE,
                                sStart,
                                ((cell - 1) << 5) - offset + HALF_TEXEL,
                                dsdx,
                                dtdy);
        }
#endif /* TGR_PC */
        x += ((columns - padding) * size) / cell;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}
#endif /* TGR_REGION != TGR_REGION_JP || TGR_PC */

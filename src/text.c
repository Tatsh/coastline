#include "text.h"

#include <PR/gbi.h>

#include "render.h"
#include "system.h"

/* Text smaller than this draws with the small font. */
#define SMALL_FONT_MAX_SIZE 25

/* The height of a glyph cell, the width of a texture, and the columns of spacing each glyph holds.
 */
#define SMALL_FONT_CELL 20
#define SMALL_FONT_WIDTH 392
#define SMALL_FONT_PADDING 4
#define LARGE_FONT_CELL 40
#define LARGE_FONT_WIDTH 704
#define LARGE_FONT_PADDING 7

/* Glyphs before this one sit in the second row of a font texture. */
#define FIRST_ROW_GLYPH 27

/* The size the advance of a space and the height above the baseline are given in. */
#define SIZE_UNITS 40
#define SPACE_ADVANCE 12
#define ASCENT 30

/* The low-resolution screen a glyph is drawn whole within. */
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

/* The gradient goes after the glyph in texture memory. The load reads twice the gradient. */
#define GRADIENT_TMEM 0x1B0
#define GRADIENT_LOAD_TEXELS 320

/* One texel in the 5.10 fixed point of a texture coordinate step. */
#define TEXEL_STEP 0x400

/* Half a texel in the 10.5 fixed point of a texture coordinate. */
#define HALF_TEXEL 16

void RenderTextToDisplayList(const char *text) {
    const u8 *s = (const u8 *)text;
    s32 size = g_nFontSize;
    s32 y = g_nTextDrawY - (size * ASCENT) / SIZE_UNITS;
    s32 x = g_nTextDrawX;
    s32 cell, padding, textureWidth, glyph, row, left, right, columns, width, dsdx, dtdy, offset;
    s32 sStart;
    s16 left4, top4, right4, bottom4;
    s32 *offsets;
    u8 *texture, *gradient, *shadowGradient;
    u8 c;

    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        size *= 2;
    }
    if (size < SMALL_FONT_MAX_SIZE) {
        padding = SMALL_FONT_PADDING;
        offsets = g_adwSmallFontGlyphOffsets;
        textureWidth = SMALL_FONT_WIDTH;
        texture = g_aSmallFontTexture;
        gradient = g_abSmallFontGradient;
        shadowGradient = g_abSmallFontShadowGradient;
        cell = SMALL_FONT_CELL;
    } else {
        padding = LARGE_FONT_PADDING;
        offsets = g_adwLargeFontGlyphOffsets;
        textureWidth = LARGE_FONT_WIDTH;
        texture = g_aLargeFontTexture;
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
        x += ((columns - padding) * size) / cell;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

void SetTextStyleBold(void) {
    g_bTextStyleBold = TRUE;
}

void EnableTextShadow(void) {
    g_bTextShadowEnabled = TRUE;
}

void DisableTextShadow(void) {
    g_bTextShadowEnabled = FALSE;
}

void SetTextStyleNormal(void) {
    g_bTextStyleBold = FALSE;
}

void SetTextAlignCenter(void) {
    g_nTextAlignment = TEXT_ALIGN_CENTER;
}

void SetTextAlignLeft(void) {
    g_nTextAlignment = TEXT_ALIGN_LEFT;
}

void SetTextAlignRight(void) {
    g_nTextAlignment = TEXT_ALIGN_RIGHT;
}

void SetTextCustomColors(s32 red, s32 green, s32 blue, s32 red2, s32 green2, s32 blue2) {
    g_bTextCustomColorsEnabled = TRUE;
    g_dwTextCustomColorR = red;
    g_dwTextCustomColorG = green;
    g_dwTextCustomColorB = blue;
    g_dwTextCustomColor2R = red2;
    g_dwTextCustomColor2G = green2;
    g_dwTextCustomColor2B = blue2;
}

void SetTextGradientColorsFromBytes(const u8 *color, const u8 *color2) {
    g_bTextCustomColorsEnabled = TRUE;
    g_dwTextCustomColorR = color[0];
    g_dwTextCustomColorG = color[1];
    g_dwTextCustomColorB = color[2];
    g_dwTextCustomColor2R = color2[0];
    g_dwTextCustomColor2G = color2[1];
    g_dwTextCustomColor2B = color2[2];
}

void SetTextSize(s32 size) {
    g_nFontSize = size;
}

void DrawText(const char *text, s32 x, s32 y) {
    switch (g_nTextAlignment) {
    case TEXT_ALIGN_LEFT:
        g_nTextDrawX = x;
        break;
    case TEXT_ALIGN_RIGHT:
        g_nTextDrawX = x - MeasureTextWidth(text, g_nFontSize);
        break;
    case TEXT_ALIGN_CENTER:
        g_nTextDrawX = x - (MeasureTextWidth(text, g_nFontSize) >> 1);
        break;
    }
    g_nTextDrawY = y;
    RenderTextToDisplayList(text);
}

void DrawHudLabelAtFraction(const char *text, f32 x, f32 y, s32 unused) {
    DrawText(text,
             (s32)((f32)g_dwScreenWidth * x * 0.0009267578134f) + 8,
             g_dwScreenHeight - (s32)(y * (f32)g_dwScreenHeight * 0.001214843825f) - 9);
}

s32 MeasureTextWidth(const char *text, s32 size) {
    const u8 *s = (const u8 *)text;
    s32 width = 0;
    s32 cell, padding, glyph;
    s32 *offsets;
    u8 c;

    if (g_dwScreenScaleShift) {
        size *= 2;
    }
    if (size < SMALL_FONT_MAX_SIZE) {
        cell = SMALL_FONT_CELL;
        padding = SMALL_FONT_PADDING;
        offsets = g_adwSmallFontGlyphOffsets;
    } else {
        cell = LARGE_FONT_CELL;
        padding = LARGE_FONT_PADDING;
        offsets = g_adwLargeFontGlyphOffsets;
    }
    for (; *s != '\0'; s++) {
        c = *s;
        if (c < '!' || c >= 0x80) {
            width += (size * SPACE_ADVANCE) / SIZE_UNITS;
            continue;
        }
        if (c == '%' && s[1] != '\0') {
            if (s[1] == '%') {
                s++;
            } else if (s[1] == 'i' || s[1] == 'n') {
                s++;
                continue;
            } else if (s[2] != '\0') {
                s += 2;
                continue;
            }
        }
        glyph = g_abHudFontCharToGlyph[c - '!'];
        width += ((offsets[glyph + 1] - offsets[glyph] - padding + 1) * size) / cell;
    }
    if (g_dwScreenScaleShift) {
        width >>= 1;
    }
    return width;
}

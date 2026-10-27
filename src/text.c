#include "text.h"

#include <PR/gbi.h>

#include "render.h"
#include "replay.h"
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

/* The advance of a Japanese glyph, in the same units. */
#define JAPANESE_ADVANCE 36

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

#if TGR_REGION == TGR_REGION_JP
/* The memory of the glyph cache ends where the frame buffers start. */
#define GLYPH_CACHE_END ((u8 *)0x803B1000)

/* The first character of the table of glyph offsets. */
#define FIRST_CACHED_GLYPH 0x20

/* The offset table and the glyphs on the cartridge. */
#define ROM_GLYPHS ((u32)_cachedGlyphsSegmentRomStart)

void ResetGlyphCache(void) {
    s32 i;
    u8 *start;
    u32 slotSize;

    for (i = 0; i < GLYPH_CACHE_CHARACTERS; i++) {
        g_abGlyphCharToSlot[i] = 0;
    }
    start = (u8 *)(((u32)g_abGlyphCacheMemory + 7) & ~7);
    slotSize = GLYPH_CACHE_SLOT_SIZE;
    g_pGlyphCacheBuffer = start;
    g_pGlyphCacheBufferEnd = GLYPH_CACHE_END;
    g_nGlyphCacheSlotLimit = (u32)(g_pGlyphCacheBufferEnd - g_pGlyphCacheBuffer) / slotSize;
    g_pGlyphCacheBufferEnd = g_pGlyphCacheBuffer + g_nGlyphCacheSlotLimit * slotSize;
    g_nGlyphCacheNextSlot = 1;
    /* Slot 0 marks a character without a slot. The buffer starts a slot early. */
    g_pGlyphCacheBuffer -= slotSize;
}

u8 *GetCachedGlyphTexture(u32 character) {
    s32 slot = g_abGlyphCharToSlot[character];

    if (slot == 0) {
        slot = g_nGlyphCacheNextSlot++;
        g_abGlyphCharToSlot[character] = slot;
    }
    return g_pGlyphCacheBuffer + slot * GLYPH_CACHE_SLOT_SIZE;
}

void LoadCachedGlyphTextures(void) {
    s32 character;
    s32 slot;
    s32 start, end;
    u8 *texture;
    s32 i;

    if (!g_bGlyphOffsetTableLoaded) {
        g_bGlyphOffsetTableLoaded = TRUE;
        DmaRead(g_aGlyphRomOffsets, ROM_GLYPHS, sizeof(g_aGlyphRomOffsets));
    }
    for (character = 0; character < GLYPH_CACHE_CHARACTERS; character++) {
        /* The original reads the slot through the counter of the last loop. */
        i = g_abGlyphCharToSlot[character];
        slot = i;
        if (slot == 0 || g_abGlyphSlotChar[slot] == character) {
            continue;
        }
        g_abGlyphSlotChar[slot] = character;
        texture = g_pGlyphCacheBuffer + slot * GLYPH_CACHE_SLOT_SIZE;
        /* The offset table starts at the first cached glyph. */
        character -= FIRST_CACHED_GLYPH;
        end = g_aGlyphRomOffsets[character + 1];
        if (end < 0) {
            end = -end;
        }
        start = g_aGlyphRomOffsets[character];
        /* A negative offset marks a glyph stored without compression. */
        if (start < 0) {
            DmaRead(texture, (u32)(_cachedGlyphsSegmentRomStart - start), start + end);
        } else {
            DmaRead(
                g_abGlyphRleBuffer, (u32)((u32)start + _cachedGlyphsSegmentRomStart), end - start);
            DecompressInterleavedRle(texture, GLYPH_CACHE_SLOT_SIZE, g_abGlyphRleBuffer, 1);
        }
        character += FIRST_CACHED_GLYPH;
    }
    for (i = g_nGlyphCacheNextSlot; i < g_nGlyphCacheSlotLimit; i++) {
        g_abGlyphSlotChar[i] = 0;
    }
}
#endif

#if TGR_REGION == TGR_REGION_JP
void RenderTextToDisplayList(const char *text) {
    const u8 *s = (const u8 *)text;
    s32 size = g_nFontSize;
    s32 y = g_nTextDrawY - (size * ASCENT) / SIZE_UNITS;
    s32 x = g_nTextDrawX;
    s32 cell, padding, textureWidth, glyph, row, left, right, columns, width, dsdx, dtdy, offset;
    s32 sStart, rowStep, line;
    s32 lastLine = -1;
    s32 lastColumns = -1;
    s32 lastCell = -1;
    s32 japanese = FALSE;
    s32 smallFont;
    s16 left4, top4, right4, bottom4;
    s32 *offsets;
    u8 *texture, *gradient, *shadowGradient;
    u8 c;

    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        size *= 2;
    }
    smallFont = size < SMALL_FONT_MAX_SIZE;
    if (smallFont) {
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
    rowStep = ((cell << 10) - TEXEL_STEP) / size;

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
        /*
         * %j starts Japanese glyphs and %e ends them. Japanese glyphs draw at the large cell, so
         * small text takes the large font while they last.
         */
        if (c == '%' && s[1] != '\0') {
            if (s[1] == '%') {
                s++;
                c = *s;
            } else if (s[1] == 'j') {
                japanese = TRUE;
                if (smallFont) {
                    textureWidth = LARGE_FONT_WIDTH;
                    texture = g_aLargeFontTexture;
                    cell = LARGE_FONT_CELL;
                    padding = LARGE_FONT_PADDING;
                    offsets = g_adwLargeFontGlyphOffsets;
                    rowStep = ((LARGE_FONT_CELL << 10) - TEXEL_STEP) / size;
                }
                s++;
                continue;
            } else if (s[1] == 'e') {
                japanese = FALSE;
                if (smallFont) {
                    textureWidth = SMALL_FONT_WIDTH;
                    texture = g_aSmallFontTexture;
                    cell = SMALL_FONT_CELL;
                    padding = SMALL_FONT_PADDING;
                    offsets = g_adwSmallFontGlyphOffsets;
                    rowStep = ((SMALL_FONT_CELL << 10) - TEXEL_STEP) / size;
                }
                s++;
                continue;
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
        if ((c < '!' || c >= 0x80) && !japanese) {
            continue;
        }

        /*
         * Load the columns of the glyph, then sample them with the render tile. The load tile and
         * the size of the render tile are set only when they change.
         */
        dtdy = -rowStep;
        if (japanese) {
            columns = cell * JAPANESE_ADVANCE / SIZE_UNITS;
            line = (columns + 8) >> 3;
            gDPSetTextureImage(gDisplayListPtr++,
                               G_IM_FMT_IA,
                               G_IM_SIZ_8b,
                               JAPANESE_ADVANCE,
                               GetCachedGlyphTexture(c));
            if (line != lastLine) {
                gDPSetTile(gDisplayListPtr++,
                           G_IM_FMT_IA,
                           G_IM_SIZ_8b,
                           line,
                           0,
                           G_TX_LOADTILE,
                           0,
                           G_TX_CLAMP,
                           6,
                           G_TX_NOLOD,
                           G_TX_CLAMP,
                           6,
                           G_TX_NOLOD);
                lastLine = line;
            }
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadTile(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, columns * 4, cell * 4);
            gDPSetTile(gDisplayListPtr++,
                       G_IM_FMT_IA,
                       G_IM_SIZ_8b,
                       line,
                       0,
                       G_TX_RENDERTILE,
                       0,
                       G_TX_CLAMP,
                       6,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       6,
                       G_TX_NOLOD);
        } else {
            glyph = g_abHudFontCharToGlyph[c - '!'];
            row = glyph < FIRST_ROW_GLYPH ? cell : 0;
            left = offsets[glyph];
            columns = offsets[glyph + 1] - left + 1;
            right = left + columns;
            gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_IA, G_IM_SIZ_8b, textureWidth, texture);
            /* The original takes the width of the load back out of its right edge. */
            line = (right - left + 8) >> 3;
            if (line != lastLine) {
                gDPSetTile(gDisplayListPtr++,
                           G_IM_FMT_IA,
                           G_IM_SIZ_8b,
                           line,
                           0,
                           G_TX_LOADTILE,
                           0,
                           G_TX_CLAMP,
                           6,
                           G_TX_NOLOD,
                           G_TX_CLAMP,
                           6,
                           G_TX_NOLOD);
                lastLine = line;
            }
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadTile(
                gDisplayListPtr++, G_TX_LOADTILE, left * 4, row * 4, right * 4, (row + cell) * 4);
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
        }
        if (columns != lastColumns || cell != lastCell) {
            gDPSetTileSize(gDisplayListPtr++,
                           G_TX_RENDERTILE,
                           2,
                           2,
                           (columns - 1) * 4 + 2,
                           (cell - 1) * 4 + 2);
            lastColumns = columns;
            lastCell = cell;
        }

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
#endif

void SetTextStyleBold(void) {
    g_bTextStyleBold = TRUE;
}

void EnableTextShadow(void) {
    g_bTextShadowEnabled = TRUE;
}

void DisableTextShadow(void) {
    g_bTextShadowEnabled = FALSE;
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

void DrawHudLabelAtFraction(const char *text, f32 x, f32 y, s32 unused) {
    DrawText(text,
             (s32)((f32)g_dwScreenWidth * x * 0.0009267578134f) + 8,
             g_dwScreenHeight - (s32)((f32)g_dwScreenHeight * y * 0.001214843825f) - 9);
}

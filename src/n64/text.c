#include "common/text.h"

#include <PR/gbi.h>

#include "common/render.h"
#include "common/replay.h"
#include "common/system.h"

/* The original text commands use different word expressions from the SDK macros. */
#define TEXT_SHIFTL(v, s, siz) ((u32)(((u32)(v) & ((0x01 << (siz)) - 1)) << (s)))
#define TEXT_DP_NO_PARAM(pkt, cmd)                                                                 \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = TEXT_SHIFTL(cmd, 24, 8);                                                    \
        _g->words.w1 = 0;                                                                          \
    }
#define TEXT_SP_SET_OTHER_MODE(pkt, cmd, sft, len, data)                                           \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 =                                                                             \
            (TEXT_SHIFTL(cmd, 24, 8) | TEXT_SHIFTL(sft, 8, 8) | TEXT_SHIFTL(len, 0, 8));           \
        _g->words.w1 = (unsigned int)(data);                                                       \
    }
#define TEXT_DP_SET_COMBINE(pkt, muxs0, muxs1)                                                     \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = TEXT_SHIFTL(0xFC, 24, 8) | TEXT_SHIFTL(muxs0, 0, 24);                       \
        _g->words.w1 = (unsigned int)(muxs1);                                                      \
    }
#define TEXT_DP_SET_COLOR(pkt, c, d)                                                               \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = TEXT_SHIFTL(c, 24, 8);                                                      \
        _g->words.w1 = (unsigned int)(d);                                                          \
    }
#define TEXT_DP_SET_PRIM_COLOR(pkt, m, l, r, g, b, a)                                              \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = (TEXT_SHIFTL(0xFA, 24, 8) | TEXT_SHIFTL(m, 8, 8) | TEXT_SHIFTL(l, 0, 8));   \
        _g->words.w1 = (TEXT_SHIFTL(r, 24, 8) | TEXT_SHIFTL(g, 16, 8) | TEXT_SHIFTL(b, 8, 8) |     \
                        TEXT_SHIFTL(a, 0, 8));                                                     \
    }
#define TEXT_DP_PIPE_SYNC(pkt) TEXT_DP_NO_PARAM(pkt, 0xE7)
#define TEXT_DP_SET_TEXTURE_IMAGE(pkt, f, s, w, i)                                                 \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = TEXT_SHIFTL(0xFD, 24, 8) | TEXT_SHIFTL(f, 21, 3) | TEXT_SHIFTL(s, 19, 2) |  \
                       TEXT_SHIFTL((w) - 1, 0, 12);                                                \
        _g->words.w1 = (unsigned int)(i);                                                          \
    }
#define TEXT_DP_LOAD_SYNC(pkt) TEXT_DP_NO_PARAM(pkt, 0xE6)
#define TEXT_DP_TILE_SYNC(pkt) TEXT_DP_NO_PARAM(pkt, 0xE8)
#define TEXT_SP_TEXTURE(pkt, s, t, level, tile, on)                                                \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = (TEXT_SHIFTL(0xBB, 24, 8) | TEXT_SHIFTL((level), 11, 3) |                   \
                        TEXT_SHIFTL((tile), 8, 3) | TEXT_SHIFTL((on), 0, 8));                      \
        _g->words.w1 = (TEXT_SHIFTL((s), 16, 16) | TEXT_SHIFTL((t), 0, 16));                       \
    }
#define TEXT_DP_SET_CYCLE_TYPE(pkt, type) TEXT_SP_SET_OTHER_MODE(pkt, 0xBA, 20, 2, type)
#define TEXT_DP_SET_RENDER_MODE(pkt, c0, c1) TEXT_SP_SET_OTHER_MODE(pkt, 0xB9, 3, 29, (c0) | (c1))
#define TEXT_DP_SET_TEXTURE_FILTER(pkt, type) TEXT_SP_SET_OTHER_MODE(pkt, 0xBA, 12, 2, type)
#define TEXT_DP_SET_TEXTURE_LUT(pkt, type) TEXT_SP_SET_OTHER_MODE(pkt, 0xBA, 14, 2, type)
#define TEXT_DP_SET_ENV_COLOR(pkt, r, g, b, a)                                                     \
    TEXT_DP_SET_COLOR(pkt,                                                                         \
                      0xFB,                                                                        \
                      (TEXT_SHIFTL(r, 24, 8) | TEXT_SHIFTL(g, 16, 8) | TEXT_SHIFTL(b, 8, 8) |      \
                       TEXT_SHIFTL(a, 0, 8)))
#define TEXT_DP_SET_TEXTURE_PERSP(pkt, type) TEXT_SP_SET_OTHER_MODE(pkt, 0xBA, 19, 1, type)
#define TEXT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define TEXT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TEXT_IMMEDIATE(pkt, c, p0)                                                                 \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = TEXT_SHIFTL(c, 24, 8);                                                      \
        _g->words.w1 = (u32)(p0);                                                                  \
    }

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

u8 *GetCachedGlyphTexture(s32 character) {
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
#endif /* TGR_REGION == TGR_REGION_JP */

typedef struct {
    s32 x;
    s32 y;
} TextDrawPosition;
#define g_textDrawPosition (*(TextDrawPosition *)&g_nTextDrawX)
#ifdef ENABLE_PATCHES
#define TEXT_DRAW_X g_nTextDrawX
#define TEXT_DRAW_Y g_nTextDrawY
#else
#define TEXT_DRAW_X g_textDrawPosition.x
#define TEXT_DRAW_Y g_textDrawPosition.y
#endif

#if TGR_REGION == TGR_REGION_JP
void RenderTextToDisplayList(const char *s) {
    int w;
    int adv;
    int uls;
    int lrs;
    int row;
    int y;
    int cell;
    unsigned char *rampA;
    unsigned char *rampB;
    int dtdy;
    int line;
    int lastW;
    int lastCell;
    int lastLine;
    int jp;
    int c;
    unsigned char n;
    unsigned char g;
    int pad;
    int textureWidth;
    unsigned char *tex;
    int *tbl;
    int small;
    int x;
    const unsigned char *p;
    int size;
    size = g_nFontSize;
    x = TEXT_DRAW_X;
    y = TEXT_DRAW_Y;
    lastW = -1;
    lastCell = -1;
    lastLine = -1;
    jp = 0;
    y -= (size * 30) / 40;
    if (g_dwScreenScaleShift != 0) {
        x <<= 1;
        y <<= 1;
        size <<= 1;
    }
    small = size < 25;
    if (small) {
        cell = 20;
        pad = 4;
        tbl = g_adwSmallFontGlyphOffsets;
        textureWidth = 392;
        tex = g_aSmallFontTexture;
        rampA = g_abSmallFontGradient;
        rampB = g_abSmallFontShadowGradient;
    } else {
        cell = 40;
        pad = 7;
        tbl = g_adwLargeFontGlyphOffsets;
        textureWidth = 704;
        tex = g_aLargeFontTexture;
        rampA = g_abLargeFontGradient;
        rampB = g_abLargeFontShadowGradient;
    }
    dtdy = ((cell << 10) - (1 << 10)) / size;
    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
    TEXT_DP_SET_CYCLE_TYPE(gDisplayListPtr++, 1 << 20);
    if (g_bTextShadowEnabled) {
        TEXT_DP_SET_COMBINE(gDisplayListPtr++, 0x317E02, 0x51FEF3FA);
    } else {
        TEXT_DP_SET_COMBINE(gDisplayListPtr++, 0x317E02, 0x5FFEF3FA);
    }
    TEXT_DP_SET_RENDER_MODE(
        gDisplayListPtr++,
        0x0c080000,
        (((0x0040 | 0x0200) | 0x4000) | 0x0000) |
            ((((((u32)0) << 28) | (((u32)0) << 24)) | (((u32)1) << 20)) | (((u32)0) << 16)));
    TEXT_DP_SET_TEXTURE_FILTER(gDisplayListPtr++, g_dwTextureFilterMode);
    TEXT_DP_SET_TEXTURE_LUT(gDisplayListPtr++, 0);
    TEXT_DP_SET_TEXTURE_PERSP(gDisplayListPtr++, 0 << 19);
    /* clang-format off */
    {
      Gfx *_g = (Gfx *)(gDisplayListPtr++); _g->words.w0 =
          (((unsigned int)(((unsigned int)( 0xba) & ((0x01 << ( 8)) - 1)) << ( 24))) |
           ((unsigned int)(((unsigned int)( 16) & ((0x01 << ( 8)) - 1)) << ( 8))) |
           ((unsigned int)(((unsigned int)( 1) & ((0x01 << ( 8)) - 1)) << ( 0))) );
      _g->words.w1 = (unsigned int)( 0);
    }
    /* clang-format on */
    TEXT_SP_TEXTURE(gDisplayListPtr++, 0xffff, 0xffff, 0, 0, 1);
    TEXT_DP_TILE_SYNC(gDisplayListPtr++);
    TEXT_DP_LOAD_SYNC(gDisplayListPtr++);
    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 =
            (((TEXT_SHIFTL(0xF5, 24, 8) | TEXT_SHIFTL(0, 21, 3)) | TEXT_SHIFTL(2, 19, 2)) |
             TEXT_SHIFTL(0, 9, 9)) |
            TEXT_SHIFTL(0x1B0, 0, 9);
        _g->words.w1 =
            ((((((TEXT_SHIFTL(7, 24, 3) | TEXT_SHIFTL(0, 20, 4)) | TEXT_SHIFTL(0, 18, 2)) |
                TEXT_SHIFTL(0, 14, 4)) |
               TEXT_SHIFTL(0, 10, 4)) |
              TEXT_SHIFTL(0, 8, 2)) |
             TEXT_SHIFTL(0, 4, 4)) |
            TEXT_SHIFTL(0, 0, 4);
    }
    TEXT_DP_SET_TEXTURE_IMAGE(
        gDisplayListPtr++, 0, 2, 1, (g_bTextShadowEnabled) ? (rampB) : (rampA));
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = (((unsigned int)((((unsigned int)0xf3) & ((0x01 << 8) - 1)) << 24)) |
                        ((unsigned int)((((unsigned int)0) & ((0x01 << 12) - 1)) << 12))) |
                       ((unsigned int)((((unsigned int)0) & ((0x01 << 12) - 1)) << 0));
        _g->words.w1 =
            (((unsigned int)((((unsigned int)7) & ((0x01 << 3) - 1)) << 24)) |
             ((unsigned int)((((unsigned int)((319 < 2047) ? (319) : (2047))) & ((0x01 << 12) - 1))
                             << 12))) |
            ((unsigned int)((((unsigned int)0) & ((0x01 << 12) - 1)) << 0));
    }
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = (((((unsigned int)((((unsigned int)0xf5) & ((0x01 << 8) - 1)) << 24)) |
                          ((unsigned int)((((unsigned int)3) & ((0x01 << 3) - 1)) << 21))) |
                         ((unsigned int)((((unsigned int)1) & ((0x01 << 2) - 1)) << 19))) |
                        ((unsigned int)((((unsigned int)1) & ((0x01 << 9) - 1)) << 9))) |
                       ((unsigned int)((((unsigned int)0x1B0) & ((0x01 << 9) - 1)) << 0));
        _g->words.w1 = ((((((((unsigned int)((((unsigned int)1) & ((0x01 << 3) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 20))) |
                            ((unsigned int)((((unsigned int)2) & ((0x01 << 2) - 1)) << 18))) |
                           ((unsigned int)((((unsigned int)6) & ((0x01 << 4) - 1)) << 14))) |
                          ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 10))) |
                         ((unsigned int)((((unsigned int)0) & ((0x01 << 2) - 1)) << 8))) |
                        ((unsigned int)((((unsigned int)3) & ((0x01 << 4) - 1)) << 4))) |
                       ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 0));
    }
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = (((unsigned int)((((unsigned int)0xf2) & ((0x01 << 8) - 1)) << 24)) |
                        ((unsigned int)((((unsigned int)2) & ((0x01 << 12) - 1)) << 12))) |
                       ((unsigned int)((((unsigned int)2) & ((0x01 << 12) - 1)) << 0));
        _g->words.w1 = (((unsigned int)((((unsigned int)1) & ((0x01 << 3) - 1)) << 24)) |
                        ((unsigned int)((((unsigned int)0x1E) & ((0x01 << 12) - 1)) << 12))) |
                       ((unsigned int)((((unsigned int)0x9E) & ((0x01 << 12) - 1)) << 0));
    }
    if (g_bTextCustomColorsEnabled != 0) {
        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++,
                              g_dwTextCustomColorR,
                              g_dwTextCustomColorG,
                              g_dwTextCustomColorB,
                              0xff);
        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++,
                               0xff,
                               0xff,
                               g_dwTextCustomColor2R,
                               g_dwTextCustomColor2G,
                               g_dwTextCustomColor2B,
                               0xff);
    } else if (g_bTextStyleBold) {
        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xff, 0x7f, 0, 0xff);
        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff);
    } else {
        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xc8, 0, 0, 0xff);
        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xe6, 0xe6, 0, 0xff);
    }
    p = (const unsigned char *)s;
    while ((*p) != 0) {
        c = *p;
        if (c != ' ') {
            if ((c == '%') && (n = p[1])) {
                if (n == '%') {
                    if (n) {
                        ;
                    }
                    c = *(++p);
                } else if (n == 'j') {
                    jp = 1;
                    if (small) {
                        cell = 40;
                        pad = 7;
                        tbl = g_adwLargeFontGlyphOffsets;
                        textureWidth = 704;
                        tex = g_aLargeFontTexture;
                        dtdy = ((cell << 10) - (1 << 10)) / size;
                    }
                    p++;
                    goto next;
                } else if (n == 'e') {
                    jp = 0;
                    if (small) {
                        cell = 20;
                        pad = 4;
                        tbl = g_adwSmallFontGlyphOffsets;
                        textureWidth = 392;
                        tex = g_aSmallFontTexture;
                        dtdy = ((cell << 10) - (1 << 10)) / size;
                    }
                    p++;
                    goto next;
                } else if (n == 'i') {
                    p++;
                    goto next;
                } else if (n == 'n') {
                    p++;
                    goto next;
                } else if (p[2] != 0) {
                    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
                    switch (p[1]) {
                    case 'r':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xbe, 0, 0, 0xff);
                        break;

                    case 'o':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xcd, 0x5f, 0, 0xff);
                        break;

                    case 'O':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0x78, 0, 0xff);
                        break;

                    case 'y':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0xf5, 0, 0xff);
                        break;

                    case 'Y':
                        TEXT_DP_SET_PRIM_COLOR(
                            gDisplayListPtr++, 0xff, 0xff, 0xff, 0xfa, 0x80, 0xff);
                        break;

                    case 'g':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0, 0x96, 0, 0xff);
                        break;

                    case 'b':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0, 0, 0xc8, 0xff);
                        break;

                    case 'p':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xc8, 0, 0xc8, 0xff);
                        break;

                    case '1':

                    case 'w':
                        TEXT_DP_SET_PRIM_COLOR(
                            gDisplayListPtr++, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
                        break;

                    case '5':
                        TEXT_DP_SET_PRIM_COLOR(
                            gDisplayListPtr++, 0xff, 0xff, 0x80, 0x80, 0x80, 0xff);
                        break;

                    case '0':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0, 0, 0, 0xff);
                        break;
                    }

                    switch (p[2]) {
                    case 'r':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xc8, 0, 0, 0xff);
                        break;

                    case 'o':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xcd, 0x5f, 0, 0xff);
                        break;

                    case 'O':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xff, 0x78, 0, 0xff);
                        break;

                    case 'y':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xd2, 0xbe, 0, 0xff);
                        break;

                    case 'Y':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xd2, 0xc8, 0x69, 0xff);
                        break;

                    case 'g':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0, 0x96, 0, 0xff);
                        break;

                    case 'b':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0, 0, 0xc8, 0xff);
                        break;

                    case 'p':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xc8, 0, 0xc8, 0xff);
                        break;

                    case '1':

                    case 'w':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0xff);
                        break;

                    case '5':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0x80, 0x80, 0x80, 0xff);
                        break;

                    case '0':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0, 0, 0, 0xff);
                        break;
                    }

                    p += 2;
                    goto next;
                }
            }
            if (((c >= 0x21) && (c < 0x80)) || jp) {
                if (jp) {
                    g = (int)(*p);
                    w = (cell * 36) / 40;
                    adv = w - pad;
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 = ((TEXT_SHIFTL(0xFD, 24, 8) | TEXT_SHIFTL(3, 21, 3)) |
                                        TEXT_SHIFTL(1, 19, 2)) |
                                       TEXT_SHIFTL(36 - 1, 0, 12);
                        _g->words.w1 = (unsigned int)GetCachedGlyphTexture(g);
                    }
                    line = ((w + 1) + 7) >> 3;
                    if (lastLine != line) {
                        {
                            Gfx *_g = (Gfx *)(gDisplayListPtr++);
                            _g->words.w0 = (((TEXT_SHIFTL(0xF5, 24, 8) | TEXT_SHIFTL(3, 21, 3)) |
                                             TEXT_SHIFTL(1, 19, 2)) |
                                            TEXT_SHIFTL(line, 9, 9)) |
                                           TEXT_SHIFTL(0, 0, 9);
                            _g->words.w1 = ((((((TEXT_SHIFTL(7, 24, 3) | TEXT_SHIFTL(0, 20, 4)) |
                                                TEXT_SHIFTL(2, 18, 2)) |
                                               TEXT_SHIFTL(6, 14, 4)) |
                                              TEXT_SHIFTL(0, 10, 4)) |
                                             TEXT_SHIFTL(2, 8, 2)) |
                                            TEXT_SHIFTL(6, 4, 4)) |
                                           TEXT_SHIFTL(0, 0, 4);
                        }
                        lastLine = line;
                    }
                    TEXT_DP_LOAD_SYNC(gDisplayListPtr++);
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (((unsigned int)((((unsigned int)0xf4) & ((0x01 << 8) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)0) & ((0x01 << 12) - 1)) << 12))) |
                            ((unsigned int)((((unsigned int)0) & ((0x01 << 12) - 1)) << 0));
                        _g->words.w1 =
                            (((unsigned int)((((unsigned int)7) & ((0x01 << 3) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)(w << 2)) & ((0x01 << 12) - 1))
                                             << 12))) |
                            ((unsigned int)((((unsigned int)(cell << 2)) & ((0x01 << 12) - 1))
                                            << 0));
                    }
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (((((unsigned int)((((unsigned int)0xf5) & ((0x01 << 8) - 1)) << 24)) |
                               ((unsigned int)((((unsigned int)3) & ((0x01 << 3) - 1)) << 21))) |
                              ((unsigned int)((((unsigned int)1) & ((0x01 << 2) - 1)) << 19))) |
                             ((unsigned int)((((unsigned int)(((w + 1) + 7) >> 3)) &
                                              ((0x01 << 9) - 1))
                                             << 9))) |
                            ((unsigned int)((((unsigned int)0) & ((0x01 << 9) - 1)) << 0));
                        _g->words.w1 =
                            ((((((((unsigned int)((((unsigned int)0) & ((0x01 << 3) - 1)) << 24)) |
                                  ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 20))) |
                                 ((unsigned int)((((unsigned int)2) & ((0x01 << 2) - 1)) << 18))) |
                                ((unsigned int)((((unsigned int)6) & ((0x01 << 4) - 1)) << 14))) |
                               ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 10))) |
                              ((unsigned int)((((unsigned int)2) & ((0x01 << 2) - 1)) << 8))) |
                             ((unsigned int)((((unsigned int)6) & ((0x01 << 4) - 1)) << 4))) |
                            ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 0));
                    }
                } else {
                    g = g_abHudFontCharToGlyph[c - 0x21];
                    row = (g < 27) ? (cell) : (0);
                    uls = tbl[g];
                    w = (tbl[g + 1] - uls) + 1;
                    lrs = uls + w;
                    adv = w - pad;
                    TEXT_DP_SET_TEXTURE_IMAGE(gDisplayListPtr++, 3, 1, textureWidth, tex);
                    line = (((lrs - uls) + 1) + 7) >> 3;
                    if (lastLine != line) {
                        {
                            Gfx *_g = (Gfx *)(gDisplayListPtr++);
                            _g->words.w0 = (((TEXT_SHIFTL(0xF5, 24, 8) | TEXT_SHIFTL(3, 21, 3)) |
                                             TEXT_SHIFTL(1, 19, 2)) |
                                            TEXT_SHIFTL(line, 9, 9)) |
                                           TEXT_SHIFTL(0, 0, 9);
                            _g->words.w1 = ((((((TEXT_SHIFTL(7, 24, 3) | TEXT_SHIFTL(0, 20, 4)) |
                                                TEXT_SHIFTL(2, 18, 2)) |
                                               TEXT_SHIFTL(6, 14, 4)) |
                                              TEXT_SHIFTL(0, 10, 4)) |
                                             TEXT_SHIFTL(2, 8, 2)) |
                                            TEXT_SHIFTL(6, 4, 4)) |
                                           TEXT_SHIFTL(0, 0, 4);
                        }
                        lastLine = line;
                    }
                    TEXT_DP_LOAD_SYNC(gDisplayListPtr++);
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (((unsigned int)((((unsigned int)0xf4) & ((0x01 << 8) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)(uls << 2)) & ((0x01 << 12) - 1))
                                             << 12))) |
                            ((unsigned int)((((unsigned int)(row << 2)) & ((0x01 << 12) - 1))
                                            << 0));
                        _g->words.w1 =
                            (((unsigned int)((((unsigned int)7) & ((0x01 << 3) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)(lrs << 2)) & ((0x01 << 12) - 1))
                                             << 12))) |
                            ((unsigned int)((((unsigned int)((row + cell) << 2)) &
                                             ((0x01 << 12) - 1))
                                            << 0));
                    }
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (((((unsigned int)((((unsigned int)0xf5) & ((0x01 << 8) - 1)) << 24)) |
                               ((unsigned int)((((unsigned int)3) & ((0x01 << 3) - 1)) << 21))) |
                              ((unsigned int)((((unsigned int)1) & ((0x01 << 2) - 1)) << 19))) |
                             ((unsigned int)((((unsigned int)(((w + 1) + 7) >> 3)) &
                                              ((0x01 << 9) - 1))
                                             << 9))) |
                            ((unsigned int)((((unsigned int)0) & ((0x01 << 9) - 1)) << 0));
                        _g->words.w1 =
                            ((((((((unsigned int)((((unsigned int)0) & ((0x01 << 3) - 1)) << 24)) |
                                  ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 20))) |
                                 ((unsigned int)((((unsigned int)2) & ((0x01 << 2) - 1)) << 18))) |
                                ((unsigned int)((((unsigned int)6) & ((0x01 << 4) - 1)) << 14))) |
                               ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 10))) |
                              ((unsigned int)((((unsigned int)2) & ((0x01 << 2) - 1)) << 8))) |
                             ((unsigned int)((((unsigned int)6) & ((0x01 << 4) - 1)) << 4))) |
                            ((unsigned int)((((unsigned int)0) & ((0x01 << 4) - 1)) << 0));
                    }
                }
                if ((w != lastW) || (cell != lastCell)) {
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (((unsigned int)((((unsigned int)0xf2) & ((0x01 << 8) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)2) & ((0x01 << 12) - 1)) << 12))) |
                            ((unsigned int)((((unsigned int)2) & ((0x01 << 12) - 1)) << 0));
                        _g->words.w1 =
                            (((unsigned int)((((unsigned int)0) & ((0x01 << 3) - 1)) << 24)) |
                             ((unsigned int)((((unsigned int)(((w - 1) << 2) + 2)) &
                                              ((0x01 << 12) - 1))
                                             << 12))) |
                            ((unsigned int)((((unsigned int)(((cell - 1) << 2) + 2)) &
                                             ((0x01 << 12) - 1))
                                            << 0));
                    }
                    lastW = w;
                    lastCell = cell;
                }
                lrs = (w * size) / cell;
                if ((((x < 0) || ((x + lrs) > 320)) || (y < 0)) || ((y + size) > 240)) {
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (TEXT_SHIFTL(0xE4, 24, 8) |
                             TEXT_SHIFTL((((s16)((x + lrs) << 2)) > 0) ? ((s16)((x + lrs) << 2)) :
                                                                         (0),
                                         12,
                                         12)) |
                            TEXT_SHIFTL((((s16)((y + size) << 2)) > 0) ? ((s16)((y + size) << 2)) :
                                                                         (0),
                                        0,
                                        12);
                        _g->words.w1 =
                            (TEXT_SHIFTL(0, 24, 3) |
                             TEXT_SHIFTL((((s16)(x << 2)) > 0) ? ((s16)(x << 2)) : (0), 12, 12)) |
                            TEXT_SHIFTL((((s16)(y << 2)) > 0) ? ((s16)(y << 2)) : (0), 0, 12);
                        TEXT_IMMEDIATE(
                            gDisplayListPtr++,
                            0xB4,
                            (TEXT_SHIFTL(
                                 ((16) -
                                  (((s16)(x << 2) < 0) ?
                                       (((s16)((w << 10) / (lrs)) < 0) ?
                                            (TEXT_MAX(
                                                (((s16)(x << 2) * (s16)((w << 10) / (lrs))) >> 7),
                                                0)) :
                                            (TEXT_MIN(
                                                (((s16)(x << 2) * (s16)((w << 10) / (lrs))) >> 7),
                                                0))) :
                                       0)),
                                 16,
                                 16) |
                             TEXT_SHIFTL(
                                 ((((cell - 1) << 5) + 16) -
                                  (((y << 2) < 0) ?
                                       (((s16)(-dtdy) < 0) ?
                                            (TEXT_MAX((((s16)(y << 2) * (s16)(-dtdy)) >> 7), 0)) :
                                            (TEXT_MIN((((s16)(y << 2) * (s16)(-dtdy)) >> 7), 0))) :
                                       0)),
                                 0,
                                 16)));
                        TEXT_IMMEDIATE(gDisplayListPtr++,
                                       0xB3,
                                       (TEXT_SHIFTL(((w << 10) / (lrs)), 16, 16) |
                                        TEXT_SHIFTL((-dtdy), 0, 16)));
                    }
                } else {
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (TEXT_SHIFTL(0xE4, 24, 8) | TEXT_SHIFTL((x + lrs) << 2, 12, 12)) |
                            TEXT_SHIFTL((y + size) << 2, 0, 12);
                        _g->words.w1 = (TEXT_SHIFTL(0, 24, 3) | TEXT_SHIFTL(x << 2, 12, 12)) |
                                       TEXT_SHIFTL(y << 2, 0, 12);
                        {
                            Gfx *_g = (Gfx *)(gDisplayListPtr++);
                            _g->words.w0 = TEXT_SHIFTL(0xB4, 24, 8);
                            _g->words.w1 =
                                (unsigned int)(TEXT_SHIFTL(16, 16, 16) |
                                               TEXT_SHIFTL(((cell - 1) << 5) + 16, 0, 16));
                        };
                        {
                            Gfx *_g = (Gfx *)(gDisplayListPtr++);
                            _g->words.w0 = TEXT_SHIFTL(0xB3, 24, 8);
                            _g->words.w1 = (unsigned int)(TEXT_SHIFTL((w << 10) / lrs, 16, 16) |
                                                          TEXT_SHIFTL(-dtdy, 0, 16));
                        };
                    }
                }
                x += (adv * size) / cell;
            }
        } else {
            x += ((size * 12) / 40) + 1;
        }
    next:
        p++;
    }

    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
    TEXT_DP_SET_TEXTURE_PERSP(gDisplayListPtr++, 1 << 19);
    TEXT_DP_SET_CYCLE_TYPE(gDisplayListPtr++, 0 << 20);
}
#endif /* TGR_REGION == TGR_REGION_JP */

#if TGR_REGION != TGR_REGION_JP
void RenderTextToDisplayList(const char *s) {
    int size;
    int x;
    int y;
    int cell;
    unsigned char *rampA;
    unsigned char *rampB;
    const unsigned char *p;
    int c;
    int n;
    /* The unused word preserves IDO's original stack slot order. */
    int frameWord TGR_MATCHING_UNUSED;
    int u1;
    int pad;
    int *tbl;
    int textureWidth;
    unsigned char *tex;
    unsigned char g;
    int w;
    int uls;
    int lrs;
    int row;

    size = g_nFontSize;
    x = TEXT_DRAW_X;
    y = TEXT_DRAW_Y;
    y -= size * 30 / 40;
    if (g_dwScreenScaleShift != 0) {
        x <<= 1;
        y <<= 1;
        size <<= 1;
    }
    if (size < 25) {
        cell = 20;
        pad = 4;
        tbl = g_adwSmallFontGlyphOffsets;
        textureWidth = 392;
        tex = g_aSmallFontTexture;
        rampA = g_abSmallFontGradient;
        rampB = g_abSmallFontShadowGradient;
    } else {
        cell = 40;
        pad = 7;
        tbl = g_adwLargeFontGlyphOffsets;
        textureWidth = 704;
        tex = g_aLargeFontTexture;
        rampA = g_abLargeFontGradient;
        rampB = g_abLargeFontShadowGradient;
    }
    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
    TEXT_DP_SET_CYCLE_TYPE(gDisplayListPtr++, G_CYC_2CYCLE);
    if (g_bTextShadowEnabled) {
        TEXT_DP_SET_COMBINE(gDisplayListPtr++, 0x317E02, 0x51FEF3FA);
    } else {
        TEXT_DP_SET_COMBINE(gDisplayListPtr++, 0x317E02, 0x5FFEF3FA);
    }
    TEXT_DP_SET_RENDER_MODE(
        gDisplayListPtr++, 0x0c080000, ((0x0040 | 0x0200 | 0x4000) | (1 << 20)));
    TEXT_DP_SET_TEXTURE_FILTER(gDisplayListPtr++, g_dwTextureFilterMode);
    TEXT_DP_SET_TEXTURE_LUT(gDisplayListPtr++, 0);
    TEXT_DP_SET_TEXTURE_PERSP(gDisplayListPtr++, G_TP_NONE);
    /* clang-format off */
    {
      Gfx *_g = (Gfx *)(gDisplayListPtr++); _g->words.w0 =
          (((unsigned int)(((unsigned int)( 0xba) & ((0x01 << ( 8)) - 1)) << ( 24))) |
           ((unsigned int)(((unsigned int)( 16) & ((0x01 << ( 8)) - 1)) << ( 8))) |
           ((unsigned int)(((unsigned int)( 1) & ((0x01 << ( 8)) - 1)) << ( 0))) );
      _g->words.w1 = (unsigned int)( 0);
    }
    /* clang-format on */
    TEXT_SP_TEXTURE(gDisplayListPtr++, 0xffff, 0xffff, 0, 0, 1);
    TEXT_DP_TILE_SYNC(gDisplayListPtr++);
    TEXT_DP_LOAD_SYNC(gDisplayListPtr++);
    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0x1B0, 7, 0, 0, 0, 0, 0, 0, 0);
    TEXT_DP_SET_TEXTURE_IMAGE(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, g_bTextShadowEnabled ? rampB : rampA);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = (((unsigned int)(((unsigned int)(0xf3) & ((0x01 << (8)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (12)) - 1)) << (12))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (12)) - 1)) << (0))));
        _g->words.w1 = (((unsigned int)(((unsigned int)(7) & ((0x01 << (3)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)((((319) < (2047) ? (319) : (2047)))) &
                                         ((0x01 << (12)) - 1))
                                        << (12))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (12)) - 1)) << (0))));
    }
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = ((unsigned int)(((unsigned int)(0xf5) & ((0x01 << (8)) - 1)) << (24))) |
                       ((unsigned int)(((unsigned int)(3) & ((0x01 << (3)) - 1)) << (21))) |
                       ((unsigned int)(((unsigned int)(1) & ((0x01 << (2)) - 1)) << (19))) |
                       ((unsigned int)(((unsigned int)(1) & ((0x01 << (9)) - 1)) << (9))) |
                       ((unsigned int)(((unsigned int)(0x1B0) & ((0x01 << (9)) - 1)) << (0)));
        _g->words.w1 = ((unsigned int)(((unsigned int)(1) & ((0x01 << (3)) - 1)) << (24))) |
                       ((unsigned int)(((unsigned int)(0) & ((0x01 << (4)) - 1)) << (20))) |
                       ((unsigned int)(((unsigned int)(2) & ((0x01 << (2)) - 1)) << (18))) |
                       ((unsigned int)(((unsigned int)(6) & ((0x01 << (4)) - 1)) << (14))) |
                       ((unsigned int)(((unsigned int)(0) & ((0x01 << (4)) - 1)) << (10))) |
                       ((unsigned int)(((unsigned int)(0) & ((0x01 << (2)) - 1)) << (8))) |
                       ((unsigned int)(((unsigned int)(3) & ((0x01 << (4)) - 1)) << (4))) |
                       ((unsigned int)(((unsigned int)(0) & ((0x01 << (4)) - 1)) << (0)));
    }
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = ((unsigned int)(((unsigned int)(0xf2) & ((0x01 << (8)) - 1)) << (24))) |
                       ((unsigned int)(((unsigned int)(2) & ((0x01 << (12)) - 1)) << (12))) |
                       ((unsigned int)(((unsigned int)(2) & ((0x01 << (12)) - 1)) << (0)));
        _g->words.w1 = ((unsigned int)(((unsigned int)(1) & ((0x01 << (3)) - 1)) << (24))) |
                       ((unsigned int)(((unsigned int)(0x1E) & ((0x01 << (12)) - 1)) << (12))) |
                       ((unsigned int)(((unsigned int)(0x9E) & ((0x01 << (12)) - 1)) << (0)));
    }
    if (g_bTextCustomColorsEnabled != 0) {
        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++,
                              g_dwTextCustomColorR,
                              g_dwTextCustomColorG,
                              g_dwTextCustomColorB,
                              0xff);
        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++,
                               0xff,
                               0xff,
                               g_dwTextCustomColor2R,
                               g_dwTextCustomColor2G,
                               g_dwTextCustomColor2B,
                               0xff);
    } else if (g_bTextStyleBold) {
        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xff, 0x7f, 0, 0xff);
        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff);
    } else {
        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xc8, 0, 0, 0xff);
        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xe6, 0xe6, 0, 0xff);
    }
    p = (const unsigned char *)s;
    while (*p != 0) {
        c = *p;
        if (c != ' ') {
            if (c == '%' && p[1] != 0) {
                n = p[1];
                if (n == '%') {
                    p++;
                    c = *p;
                } else if (n == 'i') {
                    p++;
                    goto next;
                } else if (n == 'n') {
                    p++;
                    goto next;
                } else if (p[2] != 0) {
                    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
                    switch (p[1]) {
                    case 'r':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xbe, 0, 0, 0xff);
                        break;
                    case 'o':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xcd, 0x5f, 0, 0xff);
                        break;
                    case 'O':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0x78, 0, 0xff);
                        break;
                    case 'y':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0xf5, 0, 0xff);
                        break;
                    case 'Y':
                        TEXT_DP_SET_PRIM_COLOR(
                            gDisplayListPtr++, 0xff, 0xff, 0xff, 0xfa, 0x80, 0xff);
                        break;
                    case 'g':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0, 0x96, 0, 0xff);
                        break;
                    case 'b':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0, 0, 0xc8, 0xff);
                        break;
                    case 'p':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xc8, 0, 0xc8, 0xff);
                        break;
                    case '1':
                    case 'w':
                        TEXT_DP_SET_PRIM_COLOR(
                            gDisplayListPtr++, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
                        break;
                    case '5':
                        TEXT_DP_SET_PRIM_COLOR(
                            gDisplayListPtr++, 0xff, 0xff, 0x80, 0x80, 0x80, 0xff);
                        break;
                    case '0':
                        TEXT_DP_SET_PRIM_COLOR(gDisplayListPtr++, 0xff, 0xff, 0, 0, 0, 0xff);
                        break;
                    }
                    switch (p[2]) {
                    case 'r':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xc8, 0, 0, 0xff);
                        break;
                    case 'o':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xcd, 0x5f, 0, 0xff);
                        break;
                    case 'O':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xff, 0x78, 0, 0xff);
                        break;
                    case 'y':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xd2, 0xbe, 0, 0xff);
                        break;
                    case 'Y':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xd2, 0xc8, 0x69, 0xff);
                        break;
                    case 'g':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0, 0x96, 0, 0xff);
                        break;
                    case 'b':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0, 0, 0xc8, 0xff);
                        break;
                    case 'p':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xc8, 0, 0xc8, 0xff);
                        break;
                    case '1':
                    case 'w':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0xff, 0xff, 0xff, 0xff);
                        break;
                    case '5':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0x80, 0x80, 0x80, 0xff);
                        break;
                    case '0':
                        TEXT_DP_SET_ENV_COLOR(gDisplayListPtr++, 0, 0, 0, 0xff);
                        break;
                    }
                    p += 2;
                    goto next;
                }
            }
            if (c >= 0x21 && c < 0x80) {
                g = g_abHudFontCharToGlyph[c - 0x21];
                row = g < 27 ? cell : 0;
                uls = tbl[g];
                w = tbl[g + 1] - uls + 1;
                lrs = uls + w;
                n = w * size / cell;
                TEXT_DP_SET_TEXTURE_IMAGE(
                    gDisplayListPtr++, G_IM_FMT_IA, G_IM_SIZ_8b, textureWidth, tex);
                {
                    Gfx *_g = (Gfx *)(gDisplayListPtr++);

                    _g->words.w0 = TEXT_SHIFTL(G_SETTILE, 24, 8) | TEXT_SHIFTL(G_IM_FMT_IA, 21, 3) |
                                   TEXT_SHIFTL(G_IM_SIZ_8b, 19, 2) |
                                   TEXT_SHIFTL(((lrs - uls + 1) + 7) >> 3, 9, 9) |
                                   TEXT_SHIFTL(0, 0, 9);
                    _g->words.w1 = TEXT_SHIFTL(7, 24, 3) | TEXT_SHIFTL(0, 20, 4) |
                                   TEXT_SHIFTL(2, 18, 2) | TEXT_SHIFTL(6, 14, 4) |
                                   TEXT_SHIFTL(0, 10, 4) | TEXT_SHIFTL(2, 8, 2) |
                                   TEXT_SHIFTL(6, 4, 4) | TEXT_SHIFTL(0, 0, 4);
                }
                TEXT_DP_LOAD_SYNC(gDisplayListPtr++);
                {
                    Gfx *_g = (Gfx *)(gDisplayListPtr++);
                    _g->words.w0 =
                        ((unsigned int)(((unsigned int)(0xf4) & ((0x01 << (8)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(uls << 2) & ((0x01 << (12)) - 1))
                                        << (12))) |
                        ((unsigned int)(((unsigned int)(row << 2) & ((0x01 << (12)) - 1)) << (0)));
                    _g->words.w1 =
                        ((unsigned int)(((unsigned int)(7) & ((0x01 << (3)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(lrs << 2) & ((0x01 << (12)) - 1))
                                        << (12))) |
                        ((unsigned int)(((unsigned int)((row + cell) << 2) & ((0x01 << (12)) - 1))
                                        << (0)));
                }
                TEXT_DP_TILE_SYNC(gDisplayListPtr++);
                {
                    Gfx *_g = (Gfx *)(gDisplayListPtr++);

                    _g->words.w0 =
                        ((unsigned int)(((unsigned int)(0xf5) & ((0x01 << (8)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(3) & ((0x01 << (3)) - 1)) << (21))) |
                        ((unsigned int)(((unsigned int)(1) & ((0x01 << (2)) - 1)) << (19))) |
                        ((unsigned int)(((unsigned int)(((w + 1) + 7) >> 3) & ((0x01 << (9)) - 1))
                                        << (9))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (9)) - 1)) << (0)));
                    _g->words.w1 =
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (3)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (4)) - 1)) << (20))) |
                        ((unsigned int)(((unsigned int)(2) & ((0x01 << (2)) - 1)) << (18))) |
                        ((unsigned int)(((unsigned int)(6) & ((0x01 << (4)) - 1)) << (14))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (4)) - 1)) << (10))) |
                        ((unsigned int)(((unsigned int)(2) & ((0x01 << (2)) - 1)) << (8))) |
                        ((unsigned int)(((unsigned int)(6) & ((0x01 << (4)) - 1)) << (4))) |
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (4)) - 1)) << (0)));
                }
                {
                    Gfx *_g = (Gfx *)(gDisplayListPtr++);
                    _g->words.w0 =
                        ((unsigned int)(((unsigned int)(0xf2) & ((0x01 << (8)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(2) & ((0x01 << (12)) - 1)) << (12))) |
                        ((unsigned int)(((unsigned int)(2) & ((0x01 << (12)) - 1)) << (0)));
                    _g->words.w1 =
                        ((unsigned int)(((unsigned int)(0) & ((0x01 << (3)) - 1)) << (24))) |
                        ((unsigned int)(((unsigned int)(((w - 1) << 2) + 2) & ((0x01 << (12)) - 1))
                                        << (12))) |
                        ((unsigned int)(((unsigned int)(((cell - 1) << 2) + 2) &
                                         ((0x01 << (12)) - 1))
                                        << (0)));
                }
                if (x < 0 || x + n > 320 || y < 0 || y + size > 240) {
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 = (TEXT_SHIFTL(G_TEXRECT, 24, 8) |
                                        TEXT_SHIFTL(TEXT_MAX((s16)((x + n) << 2), 0), 12, 12) |
                                        TEXT_SHIFTL(TEXT_MAX((s16)((y + size) << 2), 0), 0, 12));
                        _g->words.w1 = (TEXT_SHIFTL((0), 24, 3) |
                                        TEXT_SHIFTL(TEXT_MAX((s16)(x << 2), 0), 12, 12) |
                                        TEXT_SHIFTL(TEXT_MAX((s16)(y << 2), 0), 0, 12));
                        u1 = (cell - 1) << 5;
                        TEXT_IMMEDIATE(
                            gDisplayListPtr++,
                            G_RDPHALF_1,
                            (TEXT_SHIFTL(
                                 ((16) -
                                  (((s16)(x << 2) < 0) ?
                                       (((s16)((w << 10) / (n)) < 0) ?
                                            (TEXT_MAX(
                                                (((s16)(x << 2) * (s16)((w << 10) / (n))) >> 7),
                                                0)) :
                                            (TEXT_MIN(
                                                (((s16)(x << 2) * (s16)((w << 10) / (n))) >> 7),
                                                0))) :
                                       0)),
                                 16,
                                 16) |
                             TEXT_SHIFTL(
                                 ((u1 + 16) -
                                  (((y << 2) < 0) ?
                                       (((s16)(-(((cell << 10) - (1 << 10)) / size)) < 0) ?
                                            (TEXT_MAX(
                                                (((s16)(y << 2) *
                                                  (s16)(-(((cell << 10) - (1 << 10)) / size))) >>
                                                 7),
                                                0)) :
                                            (TEXT_MIN(
                                                (((s16)(y << 2) *
                                                  (s16)(-(((cell << 10) - (1 << 10)) / size))) >>
                                                 7),
                                                0))) :
                                       0)),
                                 0,
                                 16)));
                        TEXT_IMMEDIATE(
                            gDisplayListPtr++,
                            G_RDPHALF_2,
                            (TEXT_SHIFTL(((w << 10) / (n)), 16, 16) |
                             TEXT_SHIFTL((-(((cell << 10) - (1 << 10)) / size)), 0, 16)));
                    }
                } else {
                    {
                        Gfx *_g = (Gfx *)(gDisplayListPtr++);
                        _g->words.w0 =
                            (TEXT_SHIFTL(G_TEXRECT, 24, 8) | TEXT_SHIFTL((x + n) << 2, 12, 12) |
                             TEXT_SHIFTL((y + size) << 2, 0, 12));
                        _g->words.w1 = (TEXT_SHIFTL(0, 24, 3) | TEXT_SHIFTL(x << 2, 12, 12) |
                                        TEXT_SHIFTL(y << 2, 0, 12));
                        TEXT_IMMEDIATE(
                            gDisplayListPtr++,
                            G_RDPHALF_1,
                            (TEXT_SHIFTL(16, 16, 16) | TEXT_SHIFTL(((cell - 1) << 5) + 16, 0, 16)));
                        TEXT_IMMEDIATE(gDisplayListPtr++,
                                       G_RDPHALF_2,
                                       (TEXT_SHIFTL((w << 10) / (n), 16, 16) |
                                        TEXT_SHIFTL(-(((cell << 10) - (1 << 10)) / size), 0, 16)));
                    }
                }
                x += (w - pad) * size / cell;
            }
        } else {
            x += size * 12 / 40 + 1;
        }
    next:
        p++;
    }
    TEXT_DP_PIPE_SYNC(gDisplayListPtr++);
    TEXT_DP_SET_TEXTURE_PERSP(gDisplayListPtr++, G_TP_PERSP);
    TEXT_DP_SET_CYCLE_TYPE(gDisplayListPtr++, G_CYC_1CYCLE);
}
#endif /* TGR_REGION != TGR_REGION_JP */

void SetTextStyleBold(void) {
    g_bTextStyleBold = TRUE;
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
    s32 height = g_dwScreenHeight;
    f32 floatHeight = height;

    DrawText(text,
             (s32)((f32)g_dwScreenWidth * x * 0.0009267578134f) + 8,
             height - (s32)(y * floatHeight * 0.001214843825f) - 9);
}

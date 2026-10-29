#include "common/displaylistptr.h"
#include "common/screen.h"
#include "common/text.h"
#if TGR_BOSS_RALLY
#include "common/settings.h"
#include "win32/fontdata.h"
#include "win32/gbicommand.h"

u8 g_bTextStyleBold;
u8 g_nTextAlignment;
s32 g_nTextDrawX;
s32 g_nTextDrawY;
s32 g_nFontSize;
#endif /* TGR_BOSS_RALLY */

#define SMALL_FONT_MAX_SIZE 25
#define SMALL_FONT_CELL 20
#define SMALL_FONT_PADDING 4
#define LARGE_FONT_CELL 40
#define LARGE_FONT_PADDING 7
#define SIZE_UNITS 40
#if TGR_BOSS_RALLY
#define SPACE_ADVANCE 14
#else
#define SPACE_ADVANCE 12
#endif
#define JAPANESE_ADVANCE 36

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

void SetTextStyleNormal(void) {
    g_bTextStyleBold = FALSE;
}

void SetTextAlignCenter(void) {
    g_nTextAlignment = TEXT_ALIGN_CENTER;
}

void SetTextSize(s32 size) {
    g_nFontSize = size;
}

void DrawText(const char *text, s32 x, s32 y) {
#if TGR_BOSS_RALLY
    gDisplayListPtr->w0 = 0xb6000000;
    gDisplayListPtr->w1 = 1;
    gDisplayListPtr++;
#endif
    switch (g_nTextAlignment) {
    case TEXT_ALIGN_LEFT:
        g_nTextDrawX = x;
        break;
    case TEXT_ALIGN_CENTER:
        g_nTextDrawX = x - (MeasureTextWidth(text, g_nFontSize) >> 1);
        break;
    case TEXT_ALIGN_RIGHT:
        g_nTextDrawX = x - MeasureTextWidth(text, g_nFontSize);
        break;
    }
    g_nTextDrawY = y;
    RenderTextToDisplayList(text);
}

s32 MeasureTextWidth(const char *text, s32 size) {
    s32 width = 0;
    s32 cell, padding;
    s32 *offsets;
    s32 c, next;
    s32 glyph;
#if TGR_REGION == TGR_REGION_JP && !TGR_BOSS_RALLY
    s32 japanese = FALSE;
#endif

    if (g_dwScreenScaleShift) {
        size *= 2;
    }
    if (size < SMALL_FONT_MAX_SIZE
#if TGR_BRD3D
        || g_dwTextureQualityLevel > 1
#endif
    ) {
        cell = SMALL_FONT_CELL;
#if TGR_BOSS_RALLY
        padding = 1;
#else
        padding = SMALL_FONT_PADDING;
#endif
#if TGR_BOSS_RALLY
        offsets = g_adwHudFont2GlyphOffsets;
#else
        offsets = g_adwSmallFontGlyphOffsets;
#endif
    } else {
        cell = LARGE_FONT_CELL;
#if TGR_BOSS_RALLY
        padding = 1;
#else
        padding = LARGE_FONT_PADDING;
#endif
#if TGR_BOSS_RALLY
        offsets = g_adwHudFont0GlyphOffsets;
#else
        offsets = g_adwLargeFontGlyphOffsets;
#endif
    }
    for (; *text != '\0'; text++) {
        c = (u8)*text;
#if TGR_REGION == TGR_REGION_JP && !TGR_BOSS_RALLY
        if ((c < '!' || c >= 0x80) && !japanese) {
#else
        if (c < '!' || c >= 0x80) {
#endif
            width += (size * SPACE_ADVANCE) / SIZE_UNITS;
            continue;
        }
        if (c == '%' && text[1] != '\0') {
            next = text[1];
            if (next == '%') {
                text++;
#if TGR_REGION == TGR_REGION_JP && !TGR_BOSS_RALLY
            } else if (next == 'j') {
                /* %j starts Japanese text and %e ends it. */
                text++;
                japanese = TRUE;
                continue;
            } else if (next == 'e') {
                text++;
                japanese = FALSE;
                continue;
#endif /* TGR_REGION == TGR_REGION_JP && !TGR_BOSS_RALLY */
            } else if (next == 'i' || next == 'n') {
                text++;
                continue;
            } else if (text[2] != '\0') {
                text += 2;
                continue;
            }
        }
#if TGR_REGION == TGR_REGION_JP && !TGR_BOSS_RALLY
        /* Every Japanese glyph has the same advance. */
        if (japanese) {
            width += ((cell * JAPANESE_ADVANCE / SIZE_UNITS - padding) * size) / cell;
            continue;
        }
#endif
#if TGR_BOSS_RALLY
        glyph = (s8)g_abHudFontCharToGlyph[c - '!'];
        if (glyph >= 28) {
            s32 *secondRow =
                cell == SMALL_FONT_CELL ? g_adwHudFont3GlyphOffsets : g_adwHudFont1GlyphOffsets;
            glyph -= 28;
            width += ((secondRow[glyph + 1] - secondRow[glyph]) * size) / cell;
            continue;
        }
#else
        glyph = g_abHudFontCharToGlyph[c - '!'];
#endif /* TGR_BOSS_RALLY */
        width += ((offsets[glyph + 1] - offsets[glyph] - padding + 1) * size) / cell;
    }
    if (g_dwScreenScaleShift) {
        width >>= 1;
    }
    return width;
}

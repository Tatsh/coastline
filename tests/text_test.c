/* Host check of src/text.c against the command words of the disassembly. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "render.h"
#include "system.h"
#include "text.h"

Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode = 0x2000;
s32 g_dwScreenScaleShift, g_dwScreenWidth = 320, g_dwScreenHeight = 240;
u8 g_bTextStyleBold, g_nTextAlignment, g_bTextShadowEnabled;
s32 g_bTextCustomColorsEnabled, g_dwTextCustomColorR, g_dwTextCustomColorG, g_dwTextCustomColorB;
s32 g_dwTextCustomColor2R, g_dwTextCustomColor2G, g_dwTextCustomColor2B;
u8 g_abLargeFontGradient[8], g_abLargeFontShadowGradient[8], g_abSmallFontGradient[8],
    g_abSmallFontShadowGradient[8], g_aLargeFontTexture[8], g_aSmallFontTexture[8];
u8 g_abHudFontCharToGlyph[96];
s32 g_adwLargeFontGlyphOffsets[64], g_adwSmallFontGlyphOffsets[64];
s32 g_nTextDrawX, g_nTextDrawY, g_nFontSize;

#define P(x) ((u32)(uintptr_t)(x))

static Gfx list[128];

static void check(const u32 (*expected)[2], u32 n) {
    u32 i;

    assert((u32)(gDisplayListPtr - list) == n);
    for (i = 0; i < n; i++) {
        if (list[i].words.w0 != expected[i][0] || list[i].words.w1 != expected[i][1]) {
            printf("%u: %08X %08X, expected %08X %08X\n",
                   i,
                   list[i].words.w0,
                   list[i].words.w1,
                   expected[i][0],
                   expected[i][1]);
            assert(0);
        }
    }
}

int main(void) {
    const u32 normal[][2] = {
        {0xE7000000, 0},
        {0xBA001402, 0x00100000},
        {0xFC317E02, 0x5FFEF3FA},
        {0xB900031D, 0x0C184240},
        {0xBA000C02, 0x2000},
        {0xBA000E02, 0},
        {0xBA001301, 0},
        {0xBA001001, 0},
        {0xBB000001, 0xFFFFFFFF},
        {0xE8000000, 0},
        {0xE6000000, 0},
        {0xE7000000, 0},
        {0xF51001B0, 0x07000000},
        {0xFD100000, P(g_abSmallFontGradient)},
        {0xF3000000, 0x0713F000},
        {0xF56803B0, 0x01098030},
        {0xF2002002, 0x0101E09E},
        {0xFB000000, 0xC80000FF},
        {0xFA00FFFF, 0xE6E600FF},
        /* A */
        {0xFD680187, P(g_aSmallFontTexture)},
        {0xF5680400, 0x07098260},
        {0xE6000000, 0},
        {0xF4190000, 0x071B8050},
        {0xE8000000, 0},
        {0xF5680400, 0x00098260},
        {0xF2002002, 0x0002604E},
        {0xE40DC19C, 0x000C8174},
        {0xB4000000, 0x00100270},
        {0xB3000000, 0x0800F867},
        /* %rb */
        {0xE7000000, 0},
        {0xFA00FFFF, 0xBE0000FF},
        {0xFB000000, 0x0000C8FF},
        /* B */
        {0xFD680187, P(g_aSmallFontTexture)},
        {0xF5680400, 0x07098260},
        {0xE6000000, 0},
        {0xF40C8050, 0x070E80A0},
        {0xE8000000, 0},
        {0xF5680400, 0x00098260},
        {0xF2002002, 0x0001E04E},
        {0xE40E419C, 0x000D4174},
        {0xB4000000, 0x00100270},
        {0xB3000000, 0x0800F867},
        {0xE7000000, 0},
        {0xBA001301, 0x00080000},
        {0xBA001402, 0},
    };
    const u32 clipped[][2] = {
        {0xE400C020, 0},
        {0xB4000000, 0x009001F7},
        {0xB3000000, 0x0800F867},
    };
    f32 k1 = 0.0009267578134f, k2 = 0.001214843825f;
    u32 bits;

    g_abHudFontCharToGlyph['A' - '!'] = 30;
    g_abHudFontCharToGlyph['B' - '!'] = 5;
    g_abHudFontCharToGlyph['%' - '!'] = 2;
    g_adwSmallFontGlyphOffsets[30] = 100;
    g_adwSmallFontGlyphOffsets[31] = 109;
    g_adwSmallFontGlyphOffsets[5] = 50;
    g_adwSmallFontGlyphOffsets[6] = 57;
    g_adwSmallFontGlyphOffsets[2] = 20;
    g_adwSmallFontGlyphOffsets[3] = 31;

    /* A glyph inside the screen, a color code, and a glyph from the other row. */
    SetTextSize(10);
    SetTextAlignLeft();
    gDisplayListPtr = list;
    DrawText("A%rbB", 50, 100);
    check(normal, sizeof(normal) / sizeof(normal[0]));

    /* A glyph over the top left corner. */
    gDisplayListPtr = list;
    DrawText("A", -2, 5);
    memmove(list, list + 26, 3 * sizeof(Gfx));
    gDisplayListPtr = list + 3;
    check(clipped, 3);

    /* The shadow and bold variants. */
    EnableTextShadow();
    SetTextStyleBold();
    gDisplayListPtr = list;
    DrawText("", 0, 0);
    assert(list[2].words.w1 == 0x51FEF3FA && list[13].words.w1 == P(g_abSmallFontShadowGradient));
    assert(list[17].words.w1 == 0xFF7F00FF && list[18].words.w1 == 0xFFFF7FFF);
    DisableTextShadow();
    SetTextStyleNormal();
    assert(!g_bTextShadowEnabled && !g_bTextStyleBold);

    /* Custom colors. */
    SetTextGradientColorsFromBytes((const u8 *)"\x01\x02\x03", (const u8 *)"\x04\x05\x06");
    gDisplayListPtr = list;
    DrawText("", 0, 0);
    assert(list[17].words.w1 == 0x010203FF && list[18].words.w1 == 0x040506FF);
    g_bTextCustomColorsEnabled = 0;

    /* Measuring, at low and high resolution, and the alignments. */
    assert(MeasureTextWidth("A B%rb%%%iA", 10) == 15);
    g_dwScreenScaleShift = 1;
    assert(MeasureTextWidth("A B%rb%%%iA", 10) == 15);
    g_dwScreenScaleShift = 0;
    gDisplayListPtr = list;
    SetTextAlignCenter();
    DrawText("A B%rb%%%iA", 100, 50);
    assert(g_nTextDrawX == 93 && g_nTextDrawY == 50);
    gDisplayListPtr = list;
    SetTextAlignRight();
    DrawText("A B%rb%%%iA", 100, 50);
    assert(g_nTextDrawX == 85);

    /* The two scales of DrawHudLabelAtFraction(). */
    memcpy(&bits, &k1, 4);
    assert(bits == 0x3A72F1AA);
    memcpy(&bits, &k2, 4);
    assert(bits == 0x3A9F3B65);
    SetTextAlignLeft();
    gDisplayListPtr = list;
    DrawHudLabelAtFraction("", 500.0f, 100.0f, 0);
    assert(g_nTextDrawX == (s32)(320.0f * 500.0f * k1) + 8);
    assert(g_nTextDrawY == 240 - (s32)(100.0f * 240.0f * k2) - 9);
    puts("text ok");
    return 0;
}

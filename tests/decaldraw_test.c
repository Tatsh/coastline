/* Host check of the texels src/n64/decaldraw.c paints and the commands and pixels it draws. */
#include <assert.h>
#include <string.h>

#include "common/render.h"
#include "common/system.h"
#include "decaldraw.h"
#include "decaleditor.h"

/* A decal of 16 by 16 texels shown at (100, 100), 4 pixels to a texel. */
#define SIZE 16
#define LEFT 100
#define TOP 100

ScreenRect g_rcDecalCanvas = {LEFT, TOP, SIZE * 4, SIZE * 4};
u8 g_bDecalWidth = SIZE, g_bDecalHeight = SIZE;
u8 *g_pDecalTexels, *g_pDecalMask, *g_pDecalScratchTexels;
u8 g_bDecalColor, g_bTextShadowColor, g_bBrushSize, g_bDecalTextLength, g_bDecalTextWidth;
u8 g_bDashColorA = 0x00, g_bDashColorB = 0xFF;
u16 g_wScreenFrameCount;
ScreenRect g_aBrushSizeRects[BRUSH_SIZES] = {
    {0, 0, 1, 1}, {0, 0, 2, 2}, {0, 0, 4, 4}, {0, 0, 6, 6}, {0, 0, 8, 8}};
DecalGlyph g_aDecalGlyphs[DECAL_GLYPHS];
DecalGlyph *g_apDecalTextGlyphs[DECAL_TEXT_LENGTH];
PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];
Sprite g_texDecalFont, g_texPaintToolText;
Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode = 0x2000;
s32 g_dwScreenScaleShift = 1;

static u8 texels[0x800], scratch[0x800], mask[0x40], font[2][16];
static s32 pixelCount, pixelX[64], pixelY[64], pixelShade[64], rectCount, rectX[64], rectShade[64];
static s32 glyphCalls, glyphX[4], glyphY[4], glyphRed[4];

void DrawPixel(s32 x, s32 y, u8 shade) {
    pixelX[pixelCount] = x;
    pixelY[pixelCount] = y;
    pixelShade[pixelCount++] = shade;
}

void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
    rectX[rectCount] = x;
    rectShade[rectCount++] = red;
}

void DrawScaledTexturedRect(Sprite *sprite,
                            s32 srcX,
                            s32 srcY,
                            s32 srcWidth,
                            s32 srcHeight,
                            s32 x,
                            s32 y,
                            s32 width,
                            s32 height,
                            u8 red,
                            u8 green,
                            u8 blue) {
    assert(sprite == &g_texDecalFont && srcWidth == 16 && width == 64);
    glyphX[glyphCalls] = x;
    glyphY[glyphCalls] = y;
    glyphRed[glyphCalls++] = srcX + red;
}

/* The screen point inside a texel. */
static s32 ScreenX(s32 x) {
    return LEFT + x * 4 + 1;
}

static s32 ScreenY(s32 y) {
    return TOP + SIZE * 4 - y * 4 - 2;
}

static void Clear(void) {
    memset(texels, 0, sizeof(texels));
    g_pDecalTexels = texels;
    g_pDecalMask = NULL;
}

static s32 Count(u8 color) {
    s32 x, y, n = 0;

    for (y = 0; y < SIZE; y++) {
        for (x = 0; x < SIZE; x++) {
            n += GetPixel4Bit(x, y) == color;
        }
    }
    return n;
}

static void TestTexels(void) {
    Clear();
    SetPixel4Bit(0, 0, 0xA);
    SetPixel4Bit(1, 0, 0x5);
    assert(texels[0] == 0xA5);
    /* An odd row has its words swapped: texel 0 of row 1 is in byte 4 of the row. */
    SetPixel4Bit(0, 1, 0xC);
    assert(texels[SIZE / 2 + 4] == 0xC0 && GetPixel4Bit(0, 1) == 0xC);
    SetPixel4Bit(9, 1, 0x3);
    assert(texels[SIZE / 2] == 0x03 && ReadPixel4Bit(texels, 9, 1) == 0x3);
    assert(GetPixel4Bit(1, 0) == 5 && ReadPixel4Bit(texels, 0, 0) == 0xA);
    /* Outside the decal nothing is written. */
    SetPixel4Bit(-1, 0, 1);
    SetPixel4Bit(SIZE, 0, 1);
    SetPixel4Bit(0, SIZE, 1);
    assert(Count(1) == 0);
    /* The low nibble takes the color unmasked. */
    SetPixel4Bit(1, 0, 0x1F);
    assert(texels[0] == 0xBF);
    /* A set bit of the mask protects its texel, the first texel in the highest bit. */
    Clear();
    memset(mask, 0, sizeof(mask));
    mask[0 + 2 * 2] = 1 << 4;
    g_pDecalMask = mask;
    SetPixel4Bit(3, 2, 7);
    SetPixel4Bit(4, 2, 7);
    assert(GetPixel4Bit(3, 2) == 0 && GetPixel4Bit(4, 2) == 7);
}

static void TestRectangles(void) {
    s32 x;

    Clear();
    g_bDecalColor = 2;
    /* The far column and row are excluded, and the corners may come in any order. */
    FillRect4Bit(ScreenX(5), ScreenY(6), ScreenX(2), ScreenY(3));
    assert(Count(2) == 9 && GetPixel4Bit(2, 3) == 2 && GetPixel4Bit(4, 5) == 2);
    assert(GetPixel4Bit(5, 5) == 0 && GetPixel4Bit(4, 6) == 0);

    Clear();
    g_bBrushSize = 0;
    StrokeDecalRectangle(ScreenX(2), ScreenY(2), ScreenX(6), ScreenY(5));
    assert(Count(2) == 14 && GetPixel4Bit(6, 5) == 2 && GetPixel4Bit(6, 2) == 2);
    assert(GetPixel4Bit(4, 3) == 0);

    /* The brush of 4 paints bands 4 wide centered on the edges. */
    Clear();
    g_bBrushSize = 2;
    StrokeDecalRectangle(ScreenX(4), ScreenY(4), ScreenX(10), ScreenY(10));
    assert(GetPixel4Bit(2, 2) == 2 && GetPixel4Bit(5, 7) == 2 && GetPixel4Bit(6, 7) == 0);
    assert(GetPixel4Bit(11, 11) == 2 && GetPixel4Bit(12, 12) == 0);

    Clear();
    FillDecalRoundRect(ScreenX(2), ScreenY(2), ScreenX(14), ScreenY(14));
    /* Filled, with the corners rounded by a radius of 3. */
    assert(GetPixel4Bit(8, 8) == 2 && GetPixel4Bit(2, 8) == 2 && GetPixel4Bit(8, 2) == 2);
    assert(GetPixel4Bit(2, 2) == 0 && GetPixel4Bit(13, 13) == 0 && GetPixel4Bit(14, 8) == 0);
    assert(GetPixel4Bit(4, 2) == 2 && GetPixel4Bit(3, 2) == 0 && GetPixel4Bit(3, 3) == 2);
    for (x = 0; x < SIZE; x++) {
        assert(GetPixel4Bit(x, 8) == (x >= 2 && x < 14 ? 2 : 0));
    }

    Clear();
    g_bBrushSize = 0;
    DrawDecalRoundedRectOutline(ScreenX(2), ScreenY(2), ScreenX(14), ScreenY(14));
    assert(GetPixel4Bit(2, 8) == 2 && GetPixel4Bit(14, 8) == 2 && GetPixel4Bit(8, 2) == 2);
    assert(GetPixel4Bit(8, 8) == 0 && GetPixel4Bit(2, 2) == 0 && GetPixel4Bit(3, 8) == 0);
}

static void TestLines(void) {
    s32 x;

    Clear();
    g_bDecalColor = 4;
    g_bBrushSize = 0;
    DrawLine4Bit(ScreenX(1), ScreenY(1), ScreenX(6), ScreenY(1));
    for (x = 0; x < SIZE; x++) {
        assert(GetPixel4Bit(x, 1) == (x >= 1 && x <= 6 ? 4 : 0));
    }
    assert(Count(4) == 6);

    /* A shallow line steps once it has gone half its length. */
    Clear();
    DrawLine4Bit(ScreenX(0), ScreenY(0), ScreenX(4), ScreenY(2));
    assert(Count(4) == 5 && GetPixel4Bit(0, 0) == 4 && GetPixel4Bit(4, 2) == 4);

    /* An exact diagonal drawn downwards draws nothing; upwards it draws. */
    Clear();
    DrawLine4Bit(ScreenX(2), ScreenY(6), ScreenX(6), ScreenY(2));
    assert(Count(4) == 0);
    DrawLine4Bit(ScreenX(2), ScreenY(2), ScreenX(6), ScreenY(6));
    assert(Count(4) == 5 && GetPixel4Bit(4, 4) == 4);
}

static void TestCircles(void) {
    s32 x, y;

    Clear();
    g_bDecalColor = 3;
    FillCircle4Bit(8, 8, 2, FALSE);
    assert(Count(3) == 21);
    for (y = 6; y <= 10; y++) {
        for (x = 5; x <= 11; x++) {
            s32 row = y - 8 < 0 ? 8 - y : y - 8;
            s32 half = row == 2 ? 1 : 2;

            assert(GetPixel4Bit(x, y) == (x >= 8 - half && x <= 8 + half ? 3 : 0));
        }
    }
    /* From the screen, the same circle. */
    Clear();
    FillCircle4Bit(ScreenX(8), ScreenY(8), 2, TRUE);
    assert(Count(3) == 21 && GetPixel4Bit(6, 8) == 3);

    /* The outline of radius 3 with the thinnest brush. */
    Clear();
    g_bBrushSize = 0;
    DrawDecalCircleOutline(ScreenX(8), ScreenY(8), 3);
    assert(GetPixel4Bit(11, 8) == 3 && GetPixel4Bit(5, 8) == 3 && GetPixel4Bit(8, 11) == 3);
    assert(GetPixel4Bit(8, 5) == 3 && GetPixel4Bit(8, 8) == 0 && GetPixel4Bit(12, 8) == 0);

    Clear();
    DrawDecalFilledEllipse(ScreenX(2), ScreenY(5), ScreenX(14), ScreenY(11));
    assert(GetPixel4Bit(8, 8) == 3 && GetPixel4Bit(3, 8) == 3 && GetPixel4Bit(8, 6) == 3);
    assert(GetPixel4Bit(1, 8) == 0 && GetPixel4Bit(8, 12) == 0 && GetPixel4Bit(2, 5) == 0);
    for (y = 0; y < SIZE; y++) {
        for (x = 0; x < SIZE; x++) {
            /* The rows are symmetric about the center row. The spans exclude their right end,
             * and the columns are symmetric about 7.5. */
            assert(GetPixel4Bit(x, y) == GetPixel4Bit(15 - x, y));
            if (y >= 1) {
                assert(GetPixel4Bit(x, y) == GetPixel4Bit(x, 16 - y));
            }
        }
    }

    Clear();
    DrawDecalEllipseOutline(ScreenX(2), ScreenY(5), ScreenX(14), ScreenY(11));
    assert(GetPixel4Bit(8, 8) == 0 && GetPixel4Bit(8, 11) == 3 && GetPixel4Bit(8, 5) == 3);
    assert(GetPixel4Bit(2, 8) == 3 && GetPixel4Bit(14, 8) == 3);
}

static void TestFloodFill(void) {
    s32 x, y;

    /* A closed square of color 1, filled inside with 3, one texel protected by the mask. */
    Clear();
    g_bDecalColor = 1;
    g_bBrushSize = 0;
    StrokeDecalRectangle(ScreenX(2), ScreenY(2), ScreenX(8), ScreenY(8));
    memset(mask, 0, sizeof(mask));
    mask[0 + 4 * 2] = 1 << 3;
    g_pDecalMask = mask;
    g_pDecalScratchTexels = scratch;
    g_bDecalColor = 3;
    FloodFill4Bit(ScreenX(5), ScreenY(5));
    assert(g_pDecalTexels == texels && g_pDecalMask == mask);
    for (y = 0; y < SIZE; y++) {
        for (x = 0; x < SIZE; x++) {
            u8 want = 0;

            if (x >= 2 && x <= 8 && y >= 2 && y <= 8) {
                want = x == 2 || x == 8 || y == 2 || y == 8 ? 1 : 3;
            }
            if (x == 4 && y == 4) {
                want = 0;
            }
            assert(GetPixel4Bit(x, y) == want);
        }
    }
    /* Filling with the color already there does nothing. */
    FloodFill4Bit(ScreenX(5), ScreenY(5));
    assert(Count(3) == 24);
    /* Outside, the fill covers every texel up to the square. */
    g_pDecalMask = NULL;
    g_bDecalColor = 5;
    FloodFill4Bit(ScreenX(12), ScreenY(12));
    assert(Count(5) == SIZE * SIZE - 49 && Count(1) == 24);
}

static void TestText(void) {
    Clear();
    /* A font of two cells; the ink of the second is in its column 1 of row 0. */
    memset(font, 0, sizeof(font));
    font[0][8] = 0x0F;
    g_texDecalFont.pixels = &font[0][0];
    g_texDecalFont.width = 32;
    g_texDecalFont.height = 2;
    g_aDecalGlyphs[1].character = 'B';
    g_aDecalGlyphs[1].cellX = 1;
    g_aDecalGlyphs[1].width = 1;
    g_aDecalGlyphs[1].kernLeft = 0;
    g_aDecalGlyphs[1].kernRight = 1;
    g_apDecalTextGlyphs[0] = &g_aDecalGlyphs[1];
    g_apDecalTextGlyphs[1] = &g_aDecalGlyphs[1];
    g_bDecalTextLength = 2;
    g_bDecalTextWidth = 4;
    g_bDecalColor = 6;
    g_bTextShadowColor = 9;
    g_texPaintToolText.style = 0;
    /* The text starts a texel right of the point less half its width, and advances by its width,
     * 2, less the smaller kerning. */
    StampTextOnDecal(ScreenX(5), ScreenY(7));
    assert(Count(6) == 2 && GetPixel4Bit(4, 7) == 6 && GetPixel4Bit(7, 7) == 6);
    assert(Count(9) == 0);
    /* The shadow is a texel right and down. */
    Clear();
    g_texPaintToolText.style = 1;
    StampTextOnDecal(ScreenX(5), ScreenY(7));
    assert(Count(6) == 2 && GetPixel4Bit(5, 6) == 9 && GetPixel4Bit(8, 6) == 9);

    g_aDecalSwatches[6].color[0] = 0x60;
    g_aDecalSwatches[9].color[0] = 0x90;
    g_texPaintToolText.style = 0;
    glyphCalls = 0;
    DrawDecalTextPreview(LEFT + 20, TOP + 40);
    assert(glyphCalls == 2 && glyphX[0] == LEFT + 20 - 8 - 4 && glyphY[0] == TOP + 40 - 8);
    assert(glyphRed[0] == 16 + 0x60 && glyphX[1] == glyphX[0] + 12);
    g_texPaintToolText.style = 1;
    glyphCalls = 0;
    DrawDecalTextPreview(LEFT + 20, TOP + 40);
    assert(glyphCalls == 4 && glyphX[0] == LEFT + 20 - 12 + 4 && glyphRed[0] == 16 + 0x90);
    /* Off the decal nothing is drawn. */
    glyphCalls = 0;
    DrawDecalTextPreview(LEFT - 100, TOP + 40);
    assert(glyphCalls == 0);
}

static void TestDashes(void) {
    u8 a = 1, b = 2;

    SwapBytes(&a, &b);
    assert(a == 2 && b == 1);

    /* The odd pixels of the line are drawn, 4 steps in each color. */
    pixelCount = 0;
    g_wScreenFrameCount = 0;
    DrawDashedLine(20, 5, 10, 5);
    assert(g_wScreenFrameCount == 1 && pixelCount == 5);
    assert(pixelX[0] == 11 && pixelX[4] == 19 && pixelY[2] == 5);
    assert(pixelShade[0] == 0x00 && pixelShade[1] == 0xFF && pixelShade[2] == 0xFF);
    assert(pixelShade[3] == 0x00 && pixelShade[4] == 0x00);

    /* Every 8 frames the colors swap. */
    g_wScreenFrameCount = 7;
    pixelCount = 0;
    DrawDashedLine(10, 5, 12, 5);
    assert(g_bDashColorA == 0xFF && g_bDashColorB == 0x00 && pixelShade[0] == 0xFF);
    SwapBytes(&g_bDashColorA, &g_bDashColorB);

    rectCount = 0;
    g_wScreenFrameCount = 0;
    DrawDashedRectangle(30, 30, 10, 10);
    /* Four edges of four dashes, alternating, and a short piece at each lower corner. */
    assert(rectCount == 18 && rectX[0] == 10 && rectShade[0] == 0x00 && rectShade[1] == 0xFF);
    assert(rectX[8] == 30 && rectShade[8] == rectShade[7]);

    rectCount = 0;
    pixelCount = 0;
    DrawDashedRoundedRect(10, 10, 50, 30);
    /* Radius 5: three dashes down each side and eight along the top and the bottom. */
    assert(rectCount == 22 && pixelCount > 0);

    pixelCount = 0;
    DrawDashedEllipse(10, 10, 50, 30);
    assert(pixelCount > 0);

    /* Only the points over the decal are drawn. */
    pixelCount = 0;
    DrawDashedCircle(LEFT, TOP + 32, 10);
    {
        s32 i;

        assert(pixelCount > 0);
        for (i = 0; i < pixelCount; i++) {
            assert(pixelX[i] >= LEFT && pixelY[i] >= TOP && (pixelX[i] & 1 || pixelY[i] & 1));
        }
    }
}

static void TestTextureRect(void) {
    static const u32 expected[][2] = {
        {0xE7000000, 0},
        {0xBA001402, 0},
        {0xBA000C02, 0x2000},
        {0xFCFFFFFF, 0xFFFCF87C},
        {0xB900031D, 0x0F0A4000},
        {0xBA001301, 0},
        {0xE43901C8, 0x001900C8},
        {0xB4000000, 0x00000200},
        {0xB3000000, 0x0100FF00},
        {0xE7000000, 0},
        {0xBA001301, 0x00080000},
    };
    static Gfx list[16];
    u32 i;

    gDisplayListPtr = list;
    EmitScaledTextureRect(32, 16, 100, 50, 128, 64);
    assert(gDisplayListPtr - list == 11);
    for (i = 0; i < 11; i++) {
        assert(list[i].words.w0 == expected[i][0] && list[i].words.w1 == expected[i][1]);
    }
    /* At the low resolution everything is halved. */
    g_dwScreenScaleShift = 0;
    gDisplayListPtr = list;
    EmitScaledTextureRect(32, 16, 100, 50, 128, 64);
    assert(list[6].words.w0 == 0xE41C80E4 && list[8].words.w1 == 0x0200FE00);
    g_dwScreenScaleShift = 1;
}

int main(void) {
    TestTexels();
    TestRectangles();
    TestLines();
    TestCircles();
    TestFloodFill();
    TestText();
    TestDashes();
    TestTextureRect();
    return 0;
}

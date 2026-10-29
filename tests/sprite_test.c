/* Host check of src/n64/sprite.c against the command words of the disassembly. */
#include <stdio.h>

#include "common/render.h"
#include "common/system.h"
#include "pakmenu.h"
#include "sprite.h"

Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode = 0x2000;
s32 g_dwScreenScaleShift = 1;
Sprite g_texBackground, g_texLogo;

/* A scale of 0x100 a texel, and the bits of the next power of two. */
void GetTextureScaleAndMask(u32 size, s32 *scale, s32 *maskBits) {
    s32 bits = 0;

    while ((1 << bits) < size) {
        bits++;
    }
    *scale = size * 0x100;
    *maskBits = bits;
}

/* The words of one strip of a 4-bit sprite 32 by 16 in strips of 8 rows, drawn at (10, 20) at
 * 64 by 32, from the commands the disassembly writes. */
static const u32 strip4[][2] = {
    {0xE7000000, 0},          {0xBA001402, 0},          {0xBB000001, 0x20000800},
    {0xBA000C02, 0x2000},     {0xFCFFFFFF, 0xFFFDF2F9}, {0xB900031D, 0x00504240},
    {0xFD900000, 0x1080},     {0xF5900000, 0x0700C050}, {0xE6000000, 0},
    {0xF3000000, 0x0703F400}, {0xE7000000, 0},          {0xF5800400, 0x0000C050},
    {0xF2000000, 0x0007C01C}, {0xBA000E02, 0},          {0xBA001301, 0},
    {0xFA00FFFF, 0x000000FF}, {0xFB000000, 0x000000FF}, {0xE4128090, 0x00028050},
    {0xB4000000, 0x000000E0}, {0xB3000000, 0x01F0FE40}, {0xE7000000, 0},
    {0xBA001301, 0x00080000},
};

/* A rectangle of a 4-bit sprite 32 wide, from (2, 3) at 8 by 4, drawn at (100, 50) at 16 by 8. */
static const u32 scaled[][2] = {
    {0xE7000000, 0},          {0xBA001402, 0},          {0xBA001001, 0},
    {0xBA000C02, 0x2000},     {0xFCFFFFFF, 0xFFFDF2F9}, {0xB900031D, 0x00504240},
    {0xFD88000F, 0x2000},     {0xF5880200, 0x07080200}, {0xE6000000, 0},
    {0xF400400C, 0x0701401C}, {0xE7000000, 0},          {0xF5800200, 0x00080200},
    {0xF200800C, 0x0002801C}, {0xBA000E02, 0},          {0xBA001301, 0},
    {0xFA00FFFF, 0xFFCA00FF}, {0xE41D00E8, 0x001900C8}, {0xB4000000, 0x002000C0},
    {0xB3000000, 0x01C0FE80}, {0xE7000000, 0},          {0xBA001301, 0x00080000},
};

/* A pixel at (10, 20) of the high-resolution screen in white, on the low-resolution screen. */
static const u32 pixel[][2] = {
    {0xE7000000, 0},
    {0xBA001402, 0x00300000},
    {0xB900031D, 0x0F0A4000},
    {0xF7000000, 0xFFFFFFFF},
    {0xF601802C, 0x00014028},
    {0xE7000000, 0},
    {0xBA001402, 0},
};

static int compare(Gfx *list, const u32 (*expected)[2], u32 n) {
    u32 i;

    for (i = 0; i < n; i++) {
        if (list[i].words.w0 != expected[i][0] || list[i].words.w1 != expected[i][1]) {
            printf("%u: %08X %08X, not %08X %08X\n",
                   i,
                   list[i].words.w0,
                   list[i].words.w1,
                   expected[i][0],
                   expected[i][1]);
            return 1;
        }
    }
    return 0;
}

static void SetSprite(Sprite *s, u8 depth, u32 width, u32 height, u32 stripHeight) {
    s->pixels = (u8 *)0x1000;
    s->bitsPerTexel = depth;
    s->width = width;
    s->height = height;
    s->stripHeight = stripHeight;
    s->originX = 10;
    s->originY = 20;
    s->drawWidth = 64;
    s->drawHeight = 32;
    s->style = 0;
}

int main(void) {
    static Gfx list[128];
    Sprite sprite;
    u32 n = sizeof(strip4) / sizeof(strip4[0]);

    /* Two strips, the last in memory first, the second 16 rows lower. */
    SetSprite(&sprite, 4, 32, 16, 8);
    gDisplayListPtr = list;
    DrawSpriteAtOrigin(&sprite);
    if ((u32)(gDisplayListPtr - list) != 2 * n || compare(list, strip4, n)) {
        return 1;
    }
    if (list[n + 6].words.w1 != 0x1000 || list[n + 17].words.w0 != 0xE41280D0 ||
        list[n + 17].words.w1 != 0x00028090) {
        printf("second strip %08X %08X\n", list[n + 6].words.w1, list[n + 17].words.w1);
        return 1;
    }

    /* The same position given rather than taken from the origin writes the same words. */
    sprite.originX = 0;
    sprite.originY = 0;
    gDisplayListPtr = list;
    DrawTexturedSprite(&sprite, 10, 20);
    if (compare(list, strip4, n)) {
        return 1;
    }

    /* A tint goes to the primitive colour; a negative width mirrors from the last column. */
    gDisplayListPtr = list;
    DrawSpriteRegion(&sprite, 10, 20, -64, 32, 0x20, 0xC8, 0xFF);
    if (list[15].words.w1 != 0x20C8FFFF || list[18].words.w1 != 0x03E000E0 ||
        list[19].words.w1 != 0xFE10FE40) {
        printf("mirror %08X %08X %08X\n", list[15].words.w1, list[18].words.w1, list[19].words.w1);
        return 1;
    }

    /* The background takes the opaque shaded combiner. */
    SetSprite(&g_texBackground, 4, 32, 16, 8);
    gDisplayListPtr = list;
    DrawSpriteTinted(&g_texBackground, 1, 2, 3);
    if (list[4].words.w0 != 0xFC309861 || list[4].words.w1 != 0x5532FF7F ||
        list[5].words.w1 != 0x0F0A4000 || list[15].words.w1 != 0x010203FF) {
        printf("background %08X %08X\n", list[4].words.w0, list[5].words.w1);
        return 1;
    }

    /* An 8-bit tinted sprite, 16 by 8 in one strip. */
    SetSprite(&sprite, 8, 16, 8, 8);
    sprite.style = SPRITE_STYLE_TINTED;
    gDisplayListPtr = list;
    DrawSpriteAtOrigin(&sprite);
    if (list[4].words.w0 != 0xFC11FE23 || list[4].words.w1 != 0xFFFFF3F9 ||
        list[6].words.w0 != 0xFD700000 || list[7].words.w0 != 0xF5700000 ||
        list[7].words.w1 != 0x0700C040 || list[9].words.w1 != 0x0703F400 ||
        list[11].words.w0 != 0xF5680400 || list[12].words.w1 != 0x0003C01C) {
        printf("8-bit %08X %08X %08X\n", list[4].words.w0, list[9].words.w1, list[11].words.w0);
        return 1;
    }

    /* A 16-bit sprite is colour. The logo is intensity and alpha. */
    SetSprite(&sprite, 16, 16, 8, 8);
    gDisplayListPtr = list;
    DrawSpriteAtOrigin(&sprite);
    if (list[4].words.w0 != 0xFCFFFFFF || list[4].words.w1 != 0xFFFCF279 ||
        list[6].words.w0 != 0xFD100000 || list[9].words.w1 != 0x0707F200 ||
        list[11].words.w0 != 0xF5100800) {
        printf("16-bit %08X %08X\n", list[6].words.w0, list[9].words.w1);
        return 1;
    }
    SetSprite(&g_texLogo, 16, 16, 8, 8);
    gDisplayListPtr = list;
    DrawSpriteAtOrigin(&g_texLogo);
    if (list[6].words.w0 != 0xFD700000 || list[11].words.w0 != 0xF5700800) {
        return 1;
    }

    /* Another depth loads nothing and still draws. */
    SetSprite(&sprite, 32, 16, 8, 8);
    gDisplayListPtr = list;
    DrawSpriteAtOrigin(&sprite);
    if (gDisplayListPtr - list != 13 || list[4].words.w0 != 0xBA000E02) {
        return 1;
    }

    SetSprite(&sprite, 4, 32, 16, 8);
    sprite.pixels = (u8 *)0x2000;
    gDisplayListPtr = list;
    DrawScaledTexturedRect(&sprite, 2, 3, 8, 4, 100, 50, 16, 8, 0xFF, 0xCA, 0);
    n = sizeof(scaled) / sizeof(scaled[0]);
    if ((u32)(gDisplayListPtr - list) != n || compare(list, scaled, n)) {
        return 1;
    }

    g_dwScreenScaleShift = 0;
    gDisplayListPtr = list;
    DrawPixel(10, 20, 0xFF);
    n = sizeof(pixel) / sizeof(pixel[0]);
    if ((u32)(gDisplayListPtr - list) != n || compare(list, pixel, n)) {
        return 1;
    }

    /* The outline halves to a thickness of 0, raised to 1. */
    gDisplayListPtr = list;
    DrawRectOutline(20, 40, 60, 80, 1, 0, 0xFF, 0xFF);
    if (gDisplayListPtr - list != 10 || list[3].words.w1 != 0x07FF07FF ||
        list[4].words.w0 != 0xF60A0054 || list[4].words.w1 != 0x00028050 ||
        list[5].words.w0 != 0xF60A00F0 || list[5].words.w1 != 0x000280EC ||
        list[6].words.w0 != 0xF602C0F0 || list[6].words.w1 != 0x00028050 ||
        list[7].words.w0 != 0xF60A00F0 || list[7].words.w1 != 0x0009C050) {
        printf("outline %08X %08X %08X\n", list[4].words.w0, list[5].words.w0, list[7].words.w1);
        return 1;
    }

    /* A rectangle of 1 by 1 halves to 0 by 0, raised to 1 by 1. */
    gDisplayListPtr = list;
    DrawFilledRectHiRes(20, 40, 1, 1, 0x80, 0x80, 0x80);
    if (gDisplayListPtr - list != 7 || list[3].words.w1 != 0x84218421 ||
        list[4].words.w0 != 0xF602C054 || list[4].words.w1 != 0x00028050) {
        printf("filled %08X %08X\n", list[4].words.w0, list[4].words.w1);
        return 1;
    }
    return 0;
}

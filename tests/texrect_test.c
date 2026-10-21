/* Host check of src/texrect.c against the command words of the disassembly. */
#include <stdio.h>

#include "render.h"
#include "system.h"
#include "texrect.h"

Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode = 0x2000;
s32 g_dwScreenScaleShift;

void DmaRead(void *dst, u32 rom, u32 size) {
}
u32 DmaReadAssetSize(u32 rom) {
    return 0;
}
void DecompressFromRom(void *dst, u32 rom, DecompressJob *job) {
}

void GetTextureScaleAndMask(s32 size, s32 *scale, s32 *maskBits) {
    s32 bits = 0;

    while ((1 << bits) < size) {
        bits++;
    }
    *scale = 0xFFFF;
    *maskBits = bits;
}

static const u32 expected[][2] = {
    {0xE7000000, 0},          {0xBA001402, 0},          {0xBB000001, 0xFFFFFFFF},
    {0xBA000C02, 0x2000},     {0xFC309861, 0x5532FF7F}, {0xB900031D, 0x0F0A4000},
    {0xFD900000, 0x1000},     {0xF5900000, 0x07010050}, {0xE6000000, 0},
    {0xF3000000, 0x0707F400}, {0xE7000000, 0},          {0xF5800400, 0x00010050},
    {0xF2000000, 0x0007C03C}, {0xBA000E02, 0},          {0xBA001301, 0},
    {0xFA00FFFF, 0xFF5858FF}, {0xFB000000, 0x000000FF}, {0xE41280D0, 0x00028050},
    {0xB4000000, 0x000001E0}, {0xB3000000, 0x01F0FE20}, {0xE7000000, 0},
    {0xBA001301, 0x00080000},
};

static int compare(Gfx *list, u32 n, u32 skip) {
    u32 i, j = 0;

    for (i = 0; i < n; i++, j++) {
        if (i == skip) {
            j++;
        }
        if (list[i].words.w0 != expected[j][0] || list[i].words.w1 != expected[j][1]) {
            printf("%u: %08X %08X\n", i, list[i].words.w0, list[i].words.w1);
            return 1;
        }
    }
    return 0;
}

int main(void) {
    static Gfx list[64];
    u32 n = sizeof(expected) / sizeof(expected[0]);

    gDisplayListPtr = list;
    DrawRedIntensityRect((u8 *)0x1000, 32, 16, 10, 20, 64, 32);
    if ((u32)(gDisplayListPtr - list) != n || compare(list, n, n)) {
        return 1;
    }

    gDisplayListPtr = list;
    DrawColoredTexturedRect((u8 *)0x1000, 32, 16, 10, 20, -64, 32, 0xFF, 0x58, 0x58);
    if ((u32)(gDisplayListPtr - list) != n - 1) {
        return 1;
    }
    /* The coloured rectangle has its own combiner and blender and no environment colour. */
    if (list[4].words.w0 != 0xFCFFFFFF || list[4].words.w1 != 0xFFFDF2F9 ||
        list[5].words.w1 != 0x00504240 || list[16].words.w0 != 0xE41280D0 ||
        list[17].words.w1 != 0x03E001E0 || list[18].words.w1 != 0xFE10FE20) {
        printf("%08X %08X %08X %08X\n",
               list[4].words.w1,
               list[5].words.w1,
               list[17].words.w1,
               list[18].words.w1);
        return 1;
    }
    puts("texrect ok");
    return 0;
}

/* Host check of DrawTexturedRect() in src/n64/texrect.c against words worked out from the
 * disassembly.
 */
#include <stdio.h>

#include "common/render.h"
#include "common/system.h"
#include "common/texrect.h"

Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode = 0x2000;
s32 g_dwScreenScaleShift;

void GetTextureScaleAndMask(u32 size, s32 *scale, s32 *maskBits) {
    s32 bits = 0;

    while ((1 << bits) < size) {
        bits++;
    }
    *scale = 0xFFFF;
    *maskBits = bits;
}
u32 DmaReadAssetSize(u32 romOffset) {
    return 0;
}
void DmaRead(void *dst, u32 romOffset, s32 size) {
}
u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
    return 0;
}

static const u32 expected[][2] = {
    {0xE7000000, 0},          {0xBA001402, 0},          {0xBB000001, 0xFFFFFFFF},
    {0xBA000C02, 0x2000},     {0xFCFFFFFF, 0xFFFDF2F9}, {0xFA00FFFF, 0x11223344},
    {0xB900031D, 0x00504240}, {0xBA000E02, 0},          {0xBA001301, 0},
    {0xFD100000, 0x1000},     {0xF5100000, 0x07090250}, {0xE6000000, 0},
    {0xF3000000, 0x0721F100}, {0xE7000000, 0},          {0xF5101000, 0x00090250},
    {0xF2000000, 0x0007C040}, {0xE41280D0, 0x00028050}, {0xB4000000, 0x000001E0},
    {0xB3000000, 0x0200FE00}, {0xE7000000, 0},          {0xBA001301, 0x00080000},
};

int main(void) {
    static Gfx list[64];
    Texture texture = {0, 0, (u8 *)0x1000, 32, 16, 0, 2};
    u32 i, n = sizeof(expected) / sizeof(expected[0]);

    gDisplayListPtr = list;
    DrawTexturedRect(&texture, 10, 20, -64, 32, 0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0, 1);
    if ((u32)(gDisplayListPtr - list) != n) {
        printf("count %d\n", (int)(gDisplayListPtr - list));
        return 1;
    }
    for (i = 0; i < n; i++) {
        if (list[i].words.w0 != expected[i][0] || list[i].words.w1 != expected[i][1]) {
            printf("%u: %08X %08X\n", i, list[i].words.w0, list[i].words.w1);
            return 1;
        }
    }
    puts("drawtexturedrect ok");
    return 0;
}

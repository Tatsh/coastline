#include "common/hudquad.h"

#include "common/displaylistops.h"

void EmitIndexedQuadDrawCommand(s32 textureEntry, s32 x, s32 y, s32 width, s32 height) {
    gDisplayListWords(gDisplayListPtr++, 0xdc000000u | ((u32)textureEntry & 0xffffffu), 1);
    gDisplayListWords(gDisplayListPtr++,
                      0xf2002002u,
                      ((((u32)width * 4 - 2) & 0xfffu) << 12) | (((u32)height * 4 - 2) & 0xfffu));
    gDisplayListWords(gDisplayListPtr++,
                      0xe3000000u | ((((u32)x + (u32)width) & 0xfffu) << 12) |
                          (((u32)y + (u32)height) & 0xfffu),
                      (((u32)x & 0xfffu) << 12) | ((u32)y & 0xfffu));
}

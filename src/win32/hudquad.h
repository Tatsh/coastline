#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Select an indexed texture and emit its tile bounds and screen rectangle.
 * @param textureEntry The texture entry encoded in the command's low 24 bits.
 * @param x The left screen coordinate.
 * @param y The top screen coordinate.
 * @param width The rectangle width.
 * @param height The rectangle height.
 * @ghidraAddress BRD3D: 0x10016a60
 * @ghidraAddress BRGlide: 0x10013fd0
 */
void EmitIndexedQuadDrawCommand(s32 textureEntry, s32 x, s32 y, s32 width, s32 height);
#ifdef __cplusplus
}
#endif

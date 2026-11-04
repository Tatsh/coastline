#ifndef COASTLINE_COORDINATES_H
#define COASTLINE_COORDINATES_H

#include <PR/gbi.h>

#include "common/types.h"

#if TGR_BOSS_RALLY
#define COORD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define COORD_SUB(a, b) ((s32)((u32)(a) - (u32)(b)))
#define COORD_DOUBLE(value) ((s32)((u32)(value) * 2u))
#define COORD_SHIFT(value, shift) ((s32)((u32)(value) << ((u32)(shift) & 31u)))
#define FILL_COLOR(red, green, blue)                                                               \
    ((u32)(u16)GPACK_RGBA5551((u32)(red), (u32)(green), (u32)(blue), 1) * 0x10001u)
#else
#define COORD_ADD(a, b) ((a) + (b))
#define COORD_SUB(a, b) ((a) - (b))
#define COORD_DOUBLE(value) ((value) * 2)
#define COORD_SHIFT(value, shift) ((value) << (shift))
/* A fill of the frame buffer with a colour of 8-bit components, as two pixels. */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u16)GPACK_RGBA5551(red, green, blue, 1) | (u32)(u16)GPACK_RGBA5551(red, green, blue, 1) << 16)
#endif

#endif /* COASTLINE_COORDINATES_H */

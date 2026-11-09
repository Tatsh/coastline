#ifndef COASTLINE_COORDINATES_H
#define COASTLINE_COORDINATES_H

#include <PR/gbi.h>

#include "common/types.h"

#if TGR_BOSS_RALLY
/** Add two coordinate components with 32-bit wraparound. */
#define COORD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
/** Subtract two coordinate components with 32-bit wraparound. */
#define COORD_SUB(a, b) ((s32)((u32)(a) - (u32)(b)))
/** Double a coordinate component with 32-bit wraparound. */
#define COORD_DOUBLE(value) ((s32)((u32)(value) * 2u))
/** Shift a coordinate component left with a masked shift count. */
#define COORD_SHIFT(value, shift) ((s32)((u32)(value) << ((u32)(shift) & 31u)))
/** Pack 8-bit RGB components into two identical RGBA5551 pixels. */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u32)(u16)GPACK_RGBA5551((u32)(red), (u32)(green), (u32)(blue), 1) * 0x10001u)
#else
/** Add two coordinate components. */
#define COORD_ADD(a, b) ((a) + (b))
/** Subtract one coordinate component from another. */
#define COORD_SUB(a, b) ((a) - (b))
/** Double a coordinate component. */
#define COORD_DOUBLE(value) ((value) * 2)
/** Shift a coordinate component left by the specified bit count. */
#define COORD_SHIFT(value, shift) ((value) << (shift))
/** Pack 8-bit RGB components into two identical RGBA5551 pixels. */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u16)GPACK_RGBA5551(red, green, blue, 1) | (u32)(u16)GPACK_RGBA5551(red, green, blue, 1) << 16)
#endif

#endif /* COASTLINE_COORDINATES_H */

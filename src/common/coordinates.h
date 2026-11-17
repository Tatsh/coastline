#ifndef COASTLINE_COORDINATES_H
#define COASTLINE_COORDINATES_H

/**
 * @file
 * Arithmetic on screen coordinate components and the packing of fill colours. Boss Rally builds
 * wrap at 32 bits and mask shift counts; Nintendo 64 builds use plain C arithmetic.
 */

#include <PR/gbi.h>

#include "common/types.h"

#if TGR_BOSS_RALLY
/**
 * Add two coordinate components with 32-bit wraparound.
 *
 * @param a The first component.
 * @param b The second component.
 * @return The wrapped sum, as an `s32`.
 */
#define COORD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
/**
 * Subtract two coordinate components with 32-bit wraparound.
 *
 * @param a The component to subtract from.
 * @param b The component to subtract.
 * @return The wrapped difference, as an `s32`.
 */
#define COORD_SUB(a, b) ((s32)((u32)(a) - (u32)(b)))
/**
 * Double a coordinate component with 32-bit wraparound.
 *
 * @param value The component.
 * @return The wrapped double, as an `s32`.
 */
#define COORD_DOUBLE(value) ((s32)((u32)(value) * 2u))
/**
 * Shift a coordinate component left with a masked shift count.
 *
 * @param value The component.
 * @param shift The bit count, of which only the low 5 bits are used.
 * @return The shifted component, as an `s32`.
 */
#define COORD_SHIFT(value, shift) ((s32)((u32)(value) << ((u32)(shift) & 31u)))
/**
 * Pack 8-bit RGB components into two identical RGBA5551 pixels.
 *
 * @param red The red component, from 0 to 255.
 * @param green The green component, from 0 to 255.
 * @param blue The blue component, from 0 to 255.
 * @return The opaque pixel in both halves of a 32-bit word.
 */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u32)(u16)GPACK_RGBA5551((u32)(red), (u32)(green), (u32)(blue), 1) * 0x10001u)
#else
/**
 * Add two coordinate components.
 *
 * @param a The first component.
 * @param b The second component.
 * @return The sum.
 */
#define COORD_ADD(a, b) ((a) + (b))
/**
 * Subtract one coordinate component from another.
 *
 * @param a The component to subtract from.
 * @param b The component to subtract.
 * @return The difference.
 */
#define COORD_SUB(a, b) ((a) - (b))
/**
 * Double a coordinate component.
 *
 * @param value The component.
 * @return Twice the component.
 */
#define COORD_DOUBLE(value) ((value) * 2)
/**
 * Shift a coordinate component left by the specified bit count.
 *
 * @param value The component.
 * @param shift The bit count.
 * @return The shifted component.
 */
#define COORD_SHIFT(value, shift) ((value) << (shift))
/**
 * Pack 8-bit RGB components into two identical RGBA5551 pixels.
 *
 * @param red The red component, from 0 to 255.
 * @param green The green component, from 0 to 255.
 * @param blue The blue component, from 0 to 255.
 * @return The opaque pixel in both halves of a 32-bit word.
 */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u16)GPACK_RGBA5551(red, green, blue, 1) | (u32)(u16)GPACK_RGBA5551(red, green, blue, 1) << 16)
#endif

#endif /* COASTLINE_COORDINATES_H */

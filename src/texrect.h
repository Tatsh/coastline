#ifndef COASTLINE_TEXRECT_H
#define COASTLINE_TEXRECT_H

/**
 * @file
 * The drawing of 4-bit intensity textures as rectangles of the screen.
 */

#include <PR/ultratypes.h>

/**
 * Get the texture coordinate scale of a texture side, and the bits of the mask that wraps it. A
 * side of more than 1024 texels is reported on the debug display and leaves the mask unset.
 *
 * @ghidraAddress USA: 0x80217614
 */
void GetTextureScaleAndMask(s32 size, s32 *scale, s32 *maskBits);

/**
 * Draw a texture stretched over a rectangle and upside down, shaded from black to red by its
 * intensity. Nothing calls it.
 *
 * @ghidraAddress USA: 0x80239220
 */
void DrawRedIntensityRect(
    u8 *texture, s32 width, s32 height, s32 x, s32 y, s32 drawWidth, s32 drawHeight);

/**
 * Draw a colour through a texture stretched over a rectangle and upside down, with the intensity
 * of the texture as its opacity. A negative drawn width also mirrors it. Nothing calls it.
 *
 * @ghidraAddress USA: 0x80239750
 */
void DrawColoredTexturedRect(u8 *texture,
                             s32 width,
                             s32 height,
                             s32 x,
                             s32 y,
                             s32 drawWidth,
                             s32 drawHeight,
                             s32 red,
                             s32 green,
                             s32 blue);

#endif

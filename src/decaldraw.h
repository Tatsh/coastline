#ifndef COASTLINE_DECALDRAW_H
#define COASTLINE_DECALDRAW_H

/**
 * @file
 * The drawing of the paint shop: the texels of the decal being edited and the shapes painted into
 * them, the dashed outlines of a shape not yet painted, and the text and the decal on the screen.
 *
 * The decal is 4 bits a texel, the even texel of a pair in the high nibble, with the words of every
 * odd row swapped as the RDP loads them. Its rows go upwards from the bottom of g_rcDecalCanvas,
 * where each texel is 4 pixels square. A routine that takes screen coordinates converts them.
 */

#include <PR/ultratypes.h>

/**
 * Read a texel of the decal being edited.
 *
 * @param x The column of the texel.
 * @param y The row of the texel.
 * @return The texel.
 * @ghidraAddress USA: 0x8024d7d8
 */
u8 GetPixel4Bit(s32 x, s32 y);

/**
 * Read a texel of a decal the size of the one being edited.
 *
 * @param texels The texels of the decal.
 * @param x The column of the texel.
 * @param y The row of the texel.
 * @return The texel.
 * @ghidraAddress USA: 0x8024d844
 */
u8 ReadPixel4Bit(u8 *texels, s32 x, s32 y);

/**
 * Fill the area of the color under a point with the color painted with. The mask protects the
 * texels only as the result is copied back.
 *
 * @param x The X coordinate of the point on the screen.
 * @param y The Y coordinate of the point on the screen.
 * @ghidraAddress USA: 0x8024dca0
 */
void FloodFill4Bit(s32 x, s32 y);

/**
 * Paint the text into the decal, centered on a point, with its shadow when the text tool has it.
 *
 * @param x The X coordinate of the point on the screen.
 * @param y The Y coordinate of the point on the screen.
 * @ghidraAddress USA: 0x8024ec30
 */
void StampTextOnDecal(s32 x, s32 y);

/**
 * Paint a line with the brush.
 *
 * @param x0 The X coordinate of the start on the screen.
 * @param y0 The Y coordinate of the start on the screen.
 * @param x1 The X coordinate of the end on the screen.
 * @param y1 The Y coordinate of the end on the screen.
 * @ghidraAddress USA: 0x8024f000
 */
void DrawLine4Bit(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Write a texel of the decal being edited, unless it is outside the decal or the mask protects it.
 *
 * @param x The column of the texel.
 * @param y The row of the texel.
 * @param color The color, an index of the palette.
 * @ghidraAddress USA: 0x8024f25c
 */
void SetPixel4Bit(s32 x, s32 y, u8 color);

/**
 * Fill a rectangle, excluding its far column and row.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x8024f7d4
 */
void FillRect4Bit(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Paint the outline of a rectangle with the brush.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x8024f8cc
 */
void StrokeDecalRectangle(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Fill a rectangle with rounded corners.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x8024fbc8
 */
void FillDecalRoundRect(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Paint the outline of a rectangle with rounded corners with the brush.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x8024feb8
 */
void DrawDecalRoundedRectOutline(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Fill the ellipse in a rectangle.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x80250b58
 */
void DrawDecalFilledEllipse(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Paint the outline of the ellipse in a rectangle with the brush.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x80250fcc
 */
void DrawDecalEllipseOutline(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Fill a circle.
 *
 * @param x The X coordinate of the center.
 * @param y The Y coordinate of the center.
 * @param radius The radius, in texels.
 * @param fromScreen Whether the center is on the screen rather than in texels.
 * @ghidraAddress USA: 0x8025159c
 */
void FillCircle4Bit(s32 x, s32 y, s32 radius, u8 fromScreen);

/**
 * Paint the outline of a circle with the brush.
 *
 * @param x The X coordinate of the center on the screen.
 * @param y The Y coordinate of the center on the screen.
 * @param radius The radius, in texels.
 * @ghidraAddress USA: 0x802517b4
 */
void DrawDecalCircleOutline(s32 x, s32 y, s32 radius);

/**
 * Draw the dashed line from the first point of a line to the cursor.
 *
 * @param x0 The X coordinate of the start on the screen.
 * @param y0 The Y coordinate of the start on the screen.
 * @param x1 The X coordinate of the end on the screen.
 * @param y1 The Y coordinate of the end on the screen.
 * @ghidraAddress USA: 0x80251a54
 */
void DrawDashedLine(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Draw the dashed outline of a rectangle.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x80251cd4
 */
void DrawDashedRectangle(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Draw the dashed outline of a rectangle with rounded corners.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x80251f68
 */
void DrawDashedRoundedRect(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Draw the dashed outline of the ellipse in a rectangle.
 *
 * @param x0 The X coordinate of a corner on the screen.
 * @param y0 The Y coordinate of a corner on the screen.
 * @param x1 The X coordinate of the opposite corner on the screen.
 * @param y1 The Y coordinate of the opposite corner on the screen.
 * @ghidraAddress USA: 0x802523cc
 */
void DrawDashedEllipse(s32 x0, s32 y0, s32 x1, s32 y1);

/**
 * Draw the dashed outline of a circle, the part over the decal only.
 *
 * @param x The X coordinate of the center on the screen.
 * @param y The Y coordinate of the center on the screen.
 * @param radius The radius, in pixels of the screen.
 * @ghidraAddress USA: 0x802528f8
 */
void DrawDashedCircle(s32 x, s32 y, s32 radius);

/**
 * Draw the text not yet painted on the screen, centered on a point, with its shadow when the text
 * tool has it.
 *
 * @param x The X coordinate of the point on the screen.
 * @param y The Y coordinate of the point on the screen.
 * @ghidraAddress USA: 0x80252c9c
 */
void DrawDecalTextPreview(s32 x, s32 y);

/**
 * Swap two bytes.
 *
 * @param a The first byte.
 * @param b The second byte.
 * @ghidraAddress USA: 0x80252f50
 */
void SwapBytes(u8 *a, u8 *b);

/**
 * Draw the texture already loaded over a rectangle of the screen, upside down: the decal being
 * edited.
 *
 * @param textureWidth The width of the texture.
 * @param textureHeight The height of the texture.
 * @param x The left edge of the rectangle on the 640 by 480 screen.
 * @param y The top edge of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @ghidraAddress USA: 0x80252f64
 */
void EmitScaledTextureRect(
    s32 textureWidth, s32 textureHeight, s32 x, s32 y, s32 width, s32 height);

#endif

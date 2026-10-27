#ifndef COASTLINE_SCREENDRAW_H
#define COASTLINE_SCREENDRAW_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add a fill of the whole frame with a colour to the display list.
 *
 * @param red The red component, from 0 to 255.
 * @param green The green component, from 0 to 255.
 * @param blue The blue component, from 0 to 255.
 * @ghidraAddress USA: 0x80217fb8
 * @ghidraAddress EUR: 0x80218fd8
 * @ghidraAddress JPN: 0x80218928
 * @ghidraAddress BRD3D: 0x100314e8
 */
void ClearScreen(s32 red, s32 green, s32 blue);

/**
 * Set the view to a rectangle of the screen, and the scissor to it as well if asked.
 *
 * @param x The left edge, in pixels.
 * @param y The top edge, in pixels.
 * @param width The width, in pixels.
 * @param height The height, in pixels.
 * @param setScissor Non-zero to set the scissor to the rectangle as well.
 * @ghidraAddress USA: 0x80219bf0
 * @ghidraAddress EUR: 0x8021ac10
 * @ghidraAddress JPN: 0x8021a568
 * @ghidraAddress BRD3D: 0x10032a42
 */
void SetViewport(s32 x, s32 y, s32 width, s32 height, s32 setScissor);

/**
 * Set the scissor to a rectangle, and the viewport to the whole frame buffer (320 by 240 on N64).
 *
 * @param x The left edge of the scissor.
 * @param y The upper edge of the scissor.
 * @param width The width of the scissor, of which only the size counts.
 * @param height The height of the scissor.
 * @param setScissor Whether the scissor is set.
 * @ghidraAddress USA: 0x80219df0
 * @ghidraAddress EUR: 0x8021ae10
 * @ghidraAddress JPN: 0x8021a768
 * @ghidraAddress BRD3D: 0x10032c38
 */
void SetupViewportAndScissor(s32 x, s32 y, s32 width, s32 height, s32 setScissor);

/**
 * Load the viewport of the view being drawn again.
 *
 * @ghidraAddress USA: 0x80219f04
 * @ghidraAddress EUR: 0x8021af24
 * @ghidraAddress JPN: 0x8021a87c
 * @ghidraAddress BRD3D: 0x10032df2
 */
void RestoreCurrentViewport(void);

/**
 * Limit drawing to a rectangle, clipped to the frame.
 *
 * @param x The left edge, in pixels.
 * @param y The top edge, in pixels.
 * @param width The width, in pixels.
 * @param height The height, in pixels.
 * @ghidraAddress USA: 0x80219a78
 * @ghidraAddress EUR: 0x8021aa98
 * @ghidraAddress JPN: 0x8021a3f0
 * @ghidraAddress BRD3D: 0x1003289f
 */
void SetScissorRect(s32 x, s32 y, s32 width, s32 height);

/**
 * The shift from the size of the frame to the size of the frame buffer, 1 in high resolution.
 *
 * @ghidraAddress USA: 0x8028a850
 * @ghidraAddress EUR: 0x8028ca50
 * @ghidraAddress JPN: 0x8028c520
 * @ghidraAddress BRD3D: 0x106c65e4
 */
extern s32 g_dwScreenScaleShift;

/**
 * Fill a rectangle of the screen with a colour.
 *
 * @param x The left edge, in the pixels of the low resolution, which high resolution doubles.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height.
 * @param red The red of the colour, from 0 to 255, of which the upper 5 bits count.
 * @param green The green of the colour.
 * @param blue The blue of the colour.
 * @ghidraAddress USA: 0x80218104
 * @ghidraAddress EUR: 0x80219124
 * @ghidraAddress JPN: 0x80218a74
 * @ghidraAddress BRD3D: 0x10031688
 */
void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue);

/**
 * Outline a rectangle of the screen with lines a pixel wide on N64. Windows retains an empty hook.
 *
 * @param x The left edge.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height.
 * @ghidraAddress USA: 0x80219f6c
 * @ghidraAddress EUR: 0x8021af8c
 * @ghidraAddress JPN: 0x8021a8e4
 * @ghidraAddress BRD3D: 0x10032e58
 */
void DrawRdpOutlineRect(s32 x, s32 y, s32 width, s32 height);

/**
 * Frame a rectangle of the screen, such as the rear-view mirror, with a dark border and light
 * rings.
 *
 * @param x The left edge.
 * @param y The upper edge.
 * @param width The width.
 * @param height The height.
 * @ghidraAddress USA: 0x8021a0f8
 * @ghidraAddress EUR: 0x8021b118
 * @ghidraAddress JPN: 0x8021aa70
 * @ghidraAddress BRD3D: 0x10032e5d
 */
void DrawRdpBorderRect(s32 x, s32 y, s32 width, s32 height);

#ifdef __cplusplus
}
#endif

#endif

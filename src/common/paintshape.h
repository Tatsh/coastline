#ifndef COASTLINE_PAINTSHAPE_H
#define COASTLINE_PAINTSHAPE_H

/**
 * @file
 * The constraint of the paint shop on the endpoint of a line or a box drawn from an anchor.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Constrain an endpoint to the anchor's axis or to an equal-sided box.
 *
 * @param anchorX The X coordinate of the anchor, in canvas pixels.
 * @param anchorY The Y coordinate of the anchor, in canvas pixels.
 * @param x The X coordinate of the endpoint, replaced with the constrained coordinate.
 * @param y The Y coordinate of the endpoint, replaced with the constrained coordinate.
 * @param square Non-zero to move the endpoint to the corner of a square of the shorter side on the
 * diagonal from the anchor. Zero to move the endpoint onto the horizontal or the vertical line
 * through the anchor, whichever is nearer.
 */
void ConstrainPaintEndpoint(s32 anchorX, s32 anchorY, s32 *x, s32 *y, s32 square);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PAINTSHAPE_H */

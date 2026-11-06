#ifndef COASTLINE_PAINTSHAPE_H
#define COASTLINE_PAINTSHAPE_H

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Constrain an endpoint to the anchor's axis or to an equal-sided box. */
void ConstrainPaintEndpoint(s32 anchorX, s32 anchorY, s32 *x, s32 *y, s32 square);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PAINTSHAPE_H */

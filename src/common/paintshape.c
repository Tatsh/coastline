#include "common/paintshape.h"

void ConstrainPaintEndpoint(s32 anchorX, s32 anchorY, s32 *x, s32 *y, s32 square) {
    s32 dx = *x - anchorX;
    s32 dy = *y - anchorY;
    s32 width = dx < 0 ? -dx : dx;
    s32 height = dy < 0 ? -dy : dy;
    s32 side;

    if (!square) {
        if (width >= height) {
            *y = anchorY;
        } else {
            *x = anchorX;
        }
        return;
    }
    side = width < height ? width : height;
    *x = anchorX + (dx < 0 ? -side : side);
    *y = anchorY + (dy < 0 ? -side : side);
}

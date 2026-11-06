#include <math.h>

#include <PR/gu.h>

#include "common/vecmath.h"

static inline u32 TruncateHiliteCoordinate(long double value) {
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    return (u32)(s64)value;
}

void guLookAtHiliteF(float mf[4][4],
                     LookAt *l,
                     Hilite *h,
                     float xEye,
                     float yEye,
                     float zEye,
                     float xAt,
                     float yAt,
                     float zAt,
                     float xUp,
                     float yUp,
                     float zUp,
                     float xl1,
                     float yl1,
                     float zl1,
                     float xl2,
                     float yl2,
                     float zl2,
                     int twidth,
                     int theight) {
    Vec3d lights[2] = {{xl1, yl1, zl1}, {xl2, yl2, zl2}};
    s32 width = (s32)((u32)twidth * 4);
    s32 height = (s32)((u32)theight * 4);
    int i;

    guLookAtReflectF(mf, l, xEye, yEye, zEye, xAt, yAt, zAt, xUp, yUp, zUp);
    for (i = 0; i < 2; ++i) {
        Vec3d *light = &lights[i];
        long double right, look, angle;
        double up;

        Vec3dNormalize(light);
        up = ((long double)mf[2][1] * light->z + (long double)mf[0][1] * light->x) +
             (long double)mf[1][1] * light->y;
        right = ((long double)mf[2][0] * light->z + (long double)mf[1][0] * light->y) +
                (long double)mf[0][0] * light->x;
        look = ((long double)mf[1][2] * light->y + (long double)mf[2][2] * light->z) +
               (long double)mf[0][2] * light->x;
        angle = atan2l(right, look);
        if (i == 0) {
            h->h.x1 = (s32)(256u - TruncateHiliteCoordinate(angle * -81.48734781601175));
            h->h.y1 = (s32)(256u - TruncateHiliteCoordinate(asinl(up) * -81.48734781601175));
        } else {
            h->h.x2 =
                (s32)((u32)width - TruncateHiliteCoordinate((angle * width) * -0.3183099524062959));
            h->h.y2 = (s32)((u32)height -
                            TruncateHiliteCoordinate((asinl(up) * height) * -0.3183099524062959));
        }
    }
}

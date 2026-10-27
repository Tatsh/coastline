#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>

#include <PR/gu.h>

#include "vecmath.h"

static u32 TruncateHiliteCoordinate(long double value) {
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    return (u32)(s64)value;
}
#else
#include "guint.h"
#endif

/* Below this length the halfway vector is too short to give a direction. */
#define THRESH2 0.1

/* Place the highlight of a light at the halfway vector between it and the view on the texture. */
#define HILITE(xl, yl, zl, hx, hy)                                                                 \
    len = 1.0 / sqrtf(xl * xl + yl * yl + zl * zl);                                                \
    xl *= len;                                                                                     \
    yl *= len;                                                                                     \
    zl *= len;                                                                                     \
    xHilite = xl + xLook;                                                                          \
    yHilite = yl + yLook;                                                                          \
    zHilite = zl + zLook;                                                                          \
    len = sqrtf(xHilite * xHilite + yHilite * yHilite + zHilite * zHilite);                        \
    if (len > THRESH2) {                                                                           \
        len = 1.0 / len;                                                                           \
        xHilite *= len;                                                                            \
        yHilite *= len;                                                                            \
        zHilite *= len;                                                                            \
        h->h.hx =                                                                                  \
            twidth * 4 + (xHilite * xRight + yHilite * yRight + zHilite * zRight) * twidth * 2;    \
        h->h.hy = theight * 4 + (xHilite * xUp + yHilite * yUp + zHilite * zUp) * theight * 2;     \
    } else {                                                                                       \
        h->h.hx = twidth * 2;                                                                      \
        h->h.hy = theight * 2;                                                                     \
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#else
    float len, xLook, yLook, zLook, xRight, yRight, zRight, xHilite, yHilite, zHilite;

    guMtxIdentF(mf);

    xLook = xAt - xEye;
    yLook = yAt - yEye;
    zLook = zAt - zEye;
    /* The reciprocals are taken in double precision. */
    len = -1.0 / sqrtf(xLook * xLook + yLook * yLook + zLook * zLook);
    xLook *= len;
    yLook *= len;
    zLook *= len;

    xRight = yUp * zLook - zUp * yLook;
    yRight = zUp * xLook - xUp * zLook;
    zRight = xUp * yLook - yUp * xLook;
    len = 1.0 / sqrtf(xRight * xRight + yRight * yRight + zRight * zRight);
    xRight *= len;
    yRight *= len;
    zRight *= len;

    xUp = yLook * zRight - zLook * yRight;
    yUp = zLook * xRight - xLook * zRight;
    zUp = xLook * yRight - yLook * xRight;
    len = 1.0 / sqrtf(xUp * xUp + yUp * yUp + zUp * zUp);
    xUp *= len;
    yUp *= len;
    zUp *= len;

    HILITE(xl1, yl1, zl1, x1, y1)
    HILITE(xl2, yl2, zl2, x2, y2)

    /* The right and up vectors are the directions of the reflection map. */
    l->l[0].l.dir[0] = FTOFRAC8(xRight);
    l->l[0].l.dir[1] = FTOFRAC8(yRight);
    l->l[0].l.dir[2] = FTOFRAC8(zRight);
    l->l[1].l.dir[0] = FTOFRAC8(xUp);
    l->l[1].l.dir[1] = FTOFRAC8(yUp);
    l->l[1].l.dir[2] = FTOFRAC8(zUp);

    l->l[0].l.col[0] = 0x00;
    l->l[0].l.col[1] = 0x00;
    l->l[0].l.col[2] = 0x00;
    l->l[0].l.pad1 = 0x00;
    l->l[0].l.colc[0] = 0x00;
    l->l[0].l.colc[1] = 0x00;
    l->l[0].l.colc[2] = 0x00;
    l->l[0].l.pad2 = 0x00;
    l->l[1].l.col[0] = 0x00;
    l->l[1].l.col[1] = 0x80;
    l->l[1].l.col[2] = 0x00;
    l->l[1].l.pad1 = 0x00;
    l->l[1].l.colc[0] = 0x00;
    l->l[1].l.colc[1] = 0x80;
    l->l[1].l.colc[2] = 0x00;
    l->l[1].l.pad2 = 0x00;

    mf[0][0] = xRight;
    mf[1][0] = yRight;
    mf[2][0] = zRight;
    mf[3][0] = -(xEye * xRight + yEye * yRight + zEye * zRight);

    mf[0][1] = xUp;
    mf[1][1] = yUp;
    mf[2][1] = zUp;
    mf[3][1] = -(xEye * xUp + yEye * yUp + zEye * zUp);

    mf[0][2] = xLook;
    mf[1][2] = yLook;
    mf[2][2] = zLook;
    mf[3][2] = -(xEye * xLook + yEye * yLook + zEye * zLook);

    mf[0][3] = 0.0f;
    mf[1][3] = 0.0f;
    mf[2][3] = 0.0f;
    mf[3][3] = 1.0f;
#endif
}

#if TGR_PLATFORM != TGR_PLATFORM_WIN
void guLookAtHilite(Mtx *m,
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
    float mf[4][4];

    guLookAtHiliteF(mf,
                    l,
                    h,
                    xEye,
                    yEye,
                    zEye,
                    xAt,
                    yAt,
                    zAt,
                    xUp,
                    yUp,
                    zUp,
                    xl1,
                    yl1,
                    zl1,
                    xl2,
                    yl2,
                    zl2,
                    twidth,
                    theight);
    guMtxF2L(mf, m);
}
#endif

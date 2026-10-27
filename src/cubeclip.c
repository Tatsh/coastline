#include "physics.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define CUBE_VALUE(a) ((long double)(a))
#define CUBE_LESS(a, b) (!((a) >= (b)))
#else
#define CUBE_VALUE(a) (a)
#define CUBE_LESS(a, b) ((a) < (b))
#endif

/* The faces of the unit cube a point is outside, two bits an axis. */
static inline s32 FaceOutcode(Vec3f *v) {
    s32 code = 0;

    if ((f64)v->x > 0.5) {
        code = 0x1;
    } else if (CUBE_LESS(v->x, -0.5)) {
        code = 0x2;
    }
    if ((f64)v->y > 0.5) {
        code |= 0x4;
    } else if (CUBE_LESS(v->y, -0.5)) {
        code |= 0x8;
    }
    if ((f64)v->z > 0.5) {
        code |= 0x10;
    } else if (CUBE_LESS(v->z, -0.5)) {
        code |= 0x20;
    }
    return code;
}

/* The edges of the unit cube a point is outside, by the planes through them. */
static inline s32 EdgeOutcode(Vec3f *v, s32 mask) {
    s32 code = 0;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double sum;
#else
    f32 sum;
#endif

    if (mask & 0x3) {
        sum = CUBE_VALUE(v->y) + v->x;
        if ((mask & 0x1) && sum > 1.0) {
            code = 0x1;
        } else if ((mask & 0x2) && CUBE_LESS(sum, -1.0)) {
            code = 0x2;
        }
    }
    if (mask & 0xC) {
        sum = CUBE_VALUE(v->x) - v->y;
        if ((mask & 0x4) && sum > 1.0) {
            code |= 0x4;
        } else if ((mask & 0x8) && CUBE_LESS(sum, -1.0)) {
            code |= 0x8;
        }
    }
    if (mask & 0x30) {
        sum = CUBE_VALUE(v->z) + v->x;
        if ((mask & 0x10) && sum > 1.0) {
            code |= 0x10;
        } else if ((mask & 0x20) && CUBE_LESS(sum, -1.0)) {
            code |= 0x20;
        }
    }
    if (mask & 0xC0) {
        sum = CUBE_VALUE(v->x) - v->z;
        if ((mask & 0x40) && sum > 1.0) {
            code |= 0x40;
        } else if ((mask & 0x80) && CUBE_LESS(sum, -1.0)) {
            code |= 0x80;
        }
    }
    if (mask & 0x300) {
        sum = CUBE_VALUE(v->z) + v->y;
        if ((mask & 0x100) && sum > 1.0) {
            code |= 0x100;
        } else if ((mask & 0x200) && CUBE_LESS(sum, -1.0)) {
            code |= 0x200;
        }
    }
    if (mask & 0xC00) {
        sum = CUBE_VALUE(v->y) - v->z;
        if ((mask & 0x400) && sum > 1.0) {
            code |= 0x400;
        } else if ((mask & 0x800) && CUBE_LESS(sum, -1.0)) {
            code |= 0x800;
        }
    }
    return code;
}

/* The corners of the unit cube a point is outside, by the planes through them. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
s32 ComputeAabbCornerPlaneClipCode(Vec3f *v, s32 mask) {
#else
static inline s32 ComputeAabbCornerPlaneClipCode(Vec3f *v, s32 mask) {
#endif
    s32 code = 0;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double sum;
#else
    f32 sum;
#endif

    if (mask & 0x3) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        sum = ((long double)v->y + v->z) + v->x;
#else
        sum = v->z + (v->x + v->y);
#endif
        if ((mask & 0x1) && sum > 1.5) {
            code = 0x1;
        } else if ((mask & 0x2) && CUBE_LESS(sum, -1.5)) {
            code = 0x2;
        }
    }
    if (mask & 0xC) {
        sum = (CUBE_VALUE(v->x) + v->y) - v->z;
        if ((mask & 0x4) && sum > 1.5) {
            code |= 0x4;
        } else if ((mask & 0x8) && CUBE_LESS(sum, -1.5)) {
            code |= 0x8;
        }
    }
    if (mask & 0x30) {
        sum = v->z + (CUBE_VALUE(v->x) - v->y);
        if ((mask & 0x10) && sum > 1.5) {
            code |= 0x10;
        } else if ((mask & 0x20) && CUBE_LESS(sum, -1.5)) {
            code |= 0x20;
        }
    }
    if (mask & 0xC0) {
        sum = (CUBE_VALUE(v->x) - v->y) - v->z;
        if ((mask & 0x40) && sum > 1.5) {
            code |= 0x40;
        } else if ((mask & 0x80) && CUBE_LESS(sum, -1.5)) {
            code |= 0x80;
        }
    }
    return code;
}

s32 ClassifyTriangleInUnitCube(Vec3f triangle[3]) {
    s32 shared = -1;
    s32 code, i;

    /* A corner inside, or every corner beyond the same face. */
    for (i = 0; i < 3; i++) {
        code = FaceOutcode(&triangle[i]);
        if (code == 0) {
            return 1;
        }
        shared &= code;
    }
    if (shared) {
        return 0;
    }
    /* Every corner beyond the same edge, then the same corner, of the cube. */
    code = -1;
    for (i = 0; i < 3; i++) {
        code = EdgeOutcode(&triangle[i], code);
        if (code == 0) {
            break;
        }
    }
    if (code) {
        return 0;
    }
    code = -1;
    for (i = 0; i < 3; i++) {
        code = ComputeAabbCornerPlaneClipCode(&triangle[i], code);
        if (code == 0) {
            break;
        }
    }
    if (code) {
        return 0;
    }
    return -1;
}

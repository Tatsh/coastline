#include "physics.h"

#define COMPONENT(v, axis) ((&(v)->x)[axis])

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define CUBE_VALUE(a) ((long double)(a))
#define CUBE_LESS(a, b) (!((a) >= (b)))
#else
#define CUBE_VALUE(a) (a)
#define CUBE_LESS(a, b) ((a) < (b))
#endif

s32 CountTriangleWinding(Vec3f triangle[3], Vec3f *normal, Vec3f *point) {
    f32 n[3];
    f32 p0, p1, a0, a1, b0, b1;
    s32 i, axis, i0, i1, edge, next, cross0, cross1;
    s32 winding = 0;

    for (i = 0; i < 3; i++) {
        n[i] = (COMPONENT(normal, i) < 0.0f ? -COMPONENT(normal, i) : COMPONENT(normal, i));
    }
    if (n[2] < n[0]) {
        axis = n[1] < n[0] ? 0 : 1;
    } else {
        axis = (n[2] < n[1] ? 0 : 1) + 1;
    }
    if (CUBE_LESS(COMPONENT(normal, axis), 0.0f)) {
        i0 = (axis + 2) % 3;
        i1 = (axis + 1) % 3;
    } else {
        i0 = (axis + 1) % 3;
        i1 = (axis + 2) % 3;
    }
    p0 = COMPONENT(point, i0);
    for (edge = 0; edge < 3; edge++) {
        next = (edge + 1) % 3;
        a0 = COMPONENT(&triangle[edge], i0);
        b0 = COMPONENT(&triangle[next], i0);
        cross0 = CUBE_LESS(p0, b0) - CUBE_LESS(p0, a0);
        if (cross0) {
            b1 = COMPONENT(&triangle[next], i1);
            p1 = COMPONENT(point, i1);
            a1 = COMPONENT(&triangle[edge], i1);
            cross1 = CUBE_LESS(p1, b1) - CUBE_LESS(p1, a1);
            if (cross1) {
                if ((f32)cross0 * (CUBE_VALUE(p0) - a0) * (CUBE_VALUE(b1) - a1) <=
                    (CUBE_VALUE(b0) - a0) * ((f32)cross0 * (CUBE_VALUE(p1) - a1))) {
                    winding += cross0;
                }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            } else if (!(a1 > p1)) {
#else
            } else if (a1 <= p1) {
#endif
                winding += cross0;
            }
        }
    }
    return winding;
}

s32 DoesSegmentCrossUnitCube(Vec3f *start, Vec3f *end) {
    s32 i, j, k;
    s32 sign[3];
    f32 d[3];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double across, reach;
    f32 storedAcross;
#else
    f32 across, reach;
#endif

    d[0] = end->x - start->x;
    d[1] = end->y - start->y;
    d[2] = end->z - start->z;
    for (i = 0; i < 3; i++) {
        sign[i] = CUBE_LESS(d[i], 0.0f) ? -1 : 1;
    }
    /* Both ends beyond a face. */
    for (i = 0; i < 3; i++) {
        if ((f64)(COMPONENT(start, i) * (f32)sign[i]) > 0.5) {
            return 0;
        }
        if (CUBE_LESS(COMPONENT(end, i) * (f32)sign[i], -0.5)) {
            return 0;
        }
    }
    /* The line passes beside the cube, seen along an axis. */
    for (i = 0; i < 3; i++) {
        j = (i + 1) % 3;
        k = (i + 2) % 3;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        across = CUBE_VALUE(d[k]) * COMPONENT(start, j) - CUBE_VALUE(d[j]) * COMPONENT(start, k);
        reach = (CUBE_VALUE(d[k]) * sign[j] + CUBE_VALUE(d[j]) * sign[k]) * 0.5f;
        /* Only one factor of the squared cross term is rounded to float. */
        storedAcross = across;
        if (CUBE_LESS(reach * reach, across * storedAcross)) {
#else
        across = d[k] * COMPONENT(start, j) - d[j] * COMPONENT(start, k);
        reach = (f32)((f64)(d[k] * (f32)sign[j] + d[j] * (f32)sign[k]) * 0.5);
        if (reach * reach < across * across) {
#endif
            return 0;
        }
    }
    return 1;
}

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

#include "vecmath.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "system.h"
#else
#include <math.h>
#include <string.h>
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x10a9bfd0 */
u32 g_dwRandomSeed;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/* NormalizeLightMatrix() scales a matrix to this range, less a margin, for the lighting. */
#define LIGHT_MATRIX_RANGE 1.9997559f
#define LIGHT_MATRIX_MARGIN 0.375f

/* Scale the four elements of a row of a matrix. */
#define SCALE_ROW(row, scale)                                                                      \
    {                                                                                              \
        (row)[0] *= scale;                                                                         \
        (row)[1] *= scale;                                                                         \
        (row)[2] *= scale;                                                                         \
        (row)[3] *= scale;                                                                         \
    }

void NormalizeLightMatrix(f32 m[4][4]) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    f32 *element;
    f32 max, min;

    max = 0.0f;
    min = 0.0f;
    for (element = &m[0][0]; element < &m[3][0]; element++) {
        if (*element < 0.0f) {
            if (*element < min) {
                min = *element;
            }
        } else if (max < *element) {
            max = *element;
        }
    }
    min = -min;
    if (max < min) {
        max = min;
    }
    /* The largest magnitude becomes the scale. */
    max = LIGHT_MATRIX_RANGE / (max + LIGHT_MATRIX_MARGIN);
    SCALE_ROW(m[0], max);
    SCALE_ROW(m[1], max);
    SCALE_ROW(m[2], max);
    SCALE_ROW(m[3], max);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
}

f32 GetMaxAbsValue12(f32 m[4][4]) {
    f32 *element;
    f32 max, min;

    max = 0.0f;
    min = 0.0f;
    for (element = &m[0][0]; element < &m[3][0]; element++) {
        if (*element < 0.0f) {
            if (*element < min) {
                min = *element;
            }
        } else if (max < *element) {
            max = *element;
        }
    }
    min = -min;
    if (max < min) {
        return min;
    }
    return max;
}

void FinishFrameMatrix(f32 m[4][4]) {
    m[2][3] = 0.0f;
    m[1][3] = 0.0f;
    m[0][3] = 0.0f;
    m[3][3] = 1.0f;
}

/* The terms of the binomial series of Powf(). */
#define POWF_TERMS 30

/* The relative size of a determinant below which a matrix counts as singular. */
#define MTX_INVERT_EPSILON 1.0e-15f

/* The angles of the arctangent. */
#define PI_F 3.14159265f
#define HALF_PI_F 1.57079633f
#define QUARTER_PI_F 0.78539816f
#define EIGHTH_PI_F 0.39269908f

/* The steps and the tolerance of the search of Atan2f(). */
#define ATAN2_STEPS 16
#define ATAN2_TOLERANCE 0.005f

/* The multiplier and the mask of the random numbers. */
#define RNG_MULTIPLIER 16807
#define RNG_MASK 0x07FFFFFF

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define VEC3F_INTERMEDIATE(value) ((long double)(value))
#else
#define VEC3F_INTERMEDIATE(value) (value)
#endif

/* The routines under TGR_PLATFORM_N64 alone are absent from the PC libraries. */
#if TGR_PLATFORM == TGR_PLATFORM_N64
f32 Powf(f32 base, f32 exponent) {
    f32 result = 1.0f;
    f32 n, term, sum;

    while (exponent >= 1.0f) {
        exponent -= 1.0f;
        result *= base;
    }
    if (exponent == 0.0f) {
        return result;
    }

    /* The rest of the power, as the binomial series of base to the remaining exponent, with base
     * reduced to the x of (1 + x). */
    base -= 1.0f;
    term = exponent * base;
    sum = 1.0f + term;
    for (n = 1.0f; n < POWF_TERMS;) {
        term *= (exponent - n) * base;
        n += 1.0f;
        term /= n;
        sum += term;
    }
    return result * sum;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void Vec3fCross(Vec3f *out, Vec3f *a, Vec3f *b) {
    f32 ax = a->x, ay = a->y, az = a->z;
    f32 bx = b->x, by = b->y, bz = b->z;

    out->z = VEC3F_INTERMEDIATE(ax) * by - VEC3F_INTERMEDIATE(bx) * ay;
    out->y = VEC3F_INTERMEDIATE(az) * bx - VEC3F_INTERMEDIATE(bz) * ax;
    out->x = VEC3F_INTERMEDIATE(ay) * bz - VEC3F_INTERMEDIATE(by) * az;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec3fDot(const Vec3f *a, const Vec3f *b) {
    return (long double)a->x * b->x + ((long double)a->y * b->y + (long double)a->z * b->z);
}
#else
f32 Vec3fDot(const Vec3f *a, const Vec3f *b) {
    return b->z * a->z + (a->x * b->x + a->y * b->y);
}
#endif

void Vec3fNegate(Vec3f *out, Vec3f *v) {
    out->x = -v->x;
    out->y = -v->y;
    out->z = -v->z;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void NegateVectorInPlace(Vec3f *v) {
    v->x = -v->x;
    v->y = -v->y;
    v->z = -v->z;
}

void Vec2fMul(Vec2f *out, Vec2f *a, Vec2f *b) {
    out->x = b->x * a->x;
    out->y = b->y * a->y;
}

void Vec2fScale(Vec2f *out, Vec2f *v, f32 scale) {
    out->x = v->x * scale;
    out->y = v->y * scale;
}

void Vec3fComponentProduct(Vec3f *out, Vec3f *a, Vec3f *b) {
    out->x = b->x * a->x;
    out->y = b->y * a->y;
    out->z = b->z * a->z;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void Vec3fScale(Vec3f *out, Vec3f *v, f32 scale) {
    out->x = v->x * scale;
    out->y = v->y * scale;
    out->z = v->z * scale;
}

void Vec3fScaleInPlace(Vec3f *v, f32 scale) {
    v->x *= scale;
    v->y *= scale;
    v->z *= scale;
}

void Vec3fScaleByInverse(Vec3f *out, Vec3f *v, f32 divisor) {
    divisor = 1.0f / divisor;
    out->x = v->x * divisor;
    out->y = v->y * divisor;
    out->z = v->z * divisor;
}

void Vec3fDivideInPlace(Vec3f *v, f32 divisor) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double inverse = 1.0L / divisor;
    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
#else
    divisor = 1.0f / divisor;
    v->x *= divisor;
    v->y *= divisor;
    v->z *= divisor;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

void Vec3fCopy(Vec3f *out, Vec3f *v) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u32 bits;

    /* Each component is copied before the next is read, including overlapping vectors. */
    memcpy(&bits, &v->x, sizeof(bits));
    memcpy(&out->x, &bits, sizeof(bits));
    memcpy(&bits, &v->y, sizeof(bits));
    memcpy(&out->y, &bits, sizeof(bits));
    memcpy(&bits, &v->z, sizeof(bits));
    memcpy(&out->z, &bits, sizeof(bits));
#else
    out->x = v->x;
    out->y = v->y;
    out->z = v->z;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

void Vec3fDirection(Vec3f *out, Vec3f *from, Vec3f *to) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highDx = (long double)to->x - from->x;
    long double highDy = (long double)to->y - from->y;
    f32 dx = highDx;
    f32 dy = highDy;
    f32 dz = (long double)to->z - from->z;
    f32 square = highDx * dx + highDy * dy + (long double)dz * dz;
    long double length = sqrtl(square);

    if (length != 0.0L && !isnan(length)) {
#else
    f32 dx = to->x - from->x;
    f32 dy = to->y - from->y;
    f32 dz = to->z - from->z;
    f32 length = sqrtf(dx * dx + dy * dy + dz * dz);

    if (length != 0.0f) {
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        length = 1.0f / length;
        out->x = dx * length;
        out->y = dy * length;
        out->z = dz * length;
    } else {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 1.0f;
    }
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void Vec3fNormalizeSafe(Vec3f *out, Vec3f *v) {
    f32 length = sqrtf(v->z * v->z + (v->x * v->x + v->y * v->y));

    if (length != 0.0f) {
        length = 1.0f / length;
        out->x = v->x * length;
        out->y = v->y * length;
        out->z = v->z * length;
    } else {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 1.0f;
    }
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void Vec3fNormalize(Vec3f *v) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    f32 square = (long double)v->x * v->x + ((long double)v->y * v->y + (long double)v->z * v->z);
    long double length = sqrtl(square);

    if (length != 0.0L && !isnan(length)) {
        length = 1.0L / length;
        v->x *= length;
        v->y *= length;
        v->z *= length;
    } else {
        v->x = 0.0f;
        v->y = 0.0f;
        v->z = 1.0f;
    }
#else
    f32 length = sqrtf(v->z * v->z + (v->x * v->x + v->y * v->y));

    if (length != 0.0f) {
        length = 1.0f / length;
        v->x *= length;
        v->y *= length;
        v->z *= length;
    } else {
        v->x = 0.0f;
        v->y = 0.0f;
        v->z = 1.0f;
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

void Vec3fSubtract(Vec3f *out, const Vec3f *a, const Vec3f *b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

void Vec3fSubtractInPlace(Vec3f *v, Vec3f *b) {
    v->x -= b->x;
    v->y -= b->y;
    v->z -= b->z;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void Vec2fAdd(Vec2f *out, Vec2f *a, Vec2f *b) {
    out->x = b->x + a->x;
    out->y = b->y + a->y;
}
#endif

void Vec3fAdd(Vec3f *out, Vec3f *a, Vec3f *b) {
    out->x = b->x + a->x;
    out->y = b->y + a->y;
    out->z = b->z + a->z;
}

void Vec3fAddInPlace(Vec3f *v, Vec3f *b) {
    v->x += b->x;
    v->y += b->y;
    v->z += b->z;
}

void Vec3fLerp(Vec3f *out, Vec3f *a, Vec3f *b, f32 t) {
    out->x = b->x + (VEC3F_INTERMEDIATE(a->x) - b->x) * t;
    out->y = b->y + (VEC3F_INTERMEDIATE(a->y) - b->y) * t;
    out->z = b->z + (VEC3F_INTERMEDIATE(a->z) - b->z) * t;
}

void Vec3fMulAdd(Vec3f *out, Vec3f *a, Vec3f *b, f32 scale) {
    out->x = VEC3F_INTERMEDIATE(b->x) * scale + a->x;
    out->y = VEC3F_INTERMEDIATE(b->y) * scale + a->y;
    out->z = VEC3F_INTERMEDIATE(b->z) * scale + a->z;
}

void Vec3fMulAddInPlace(Vec3f *v, Vec3f *b, f32 scale) {
    v->x += VEC3F_INTERMEDIATE(b->x) * scale;
    v->y += VEC3F_INTERMEDIATE(b->y) * scale;
    v->z += VEC3F_INTERMEDIATE(b->z) * scale;
}

void Vec3fMidpoint(Vec3f *out, Vec3f *a, Vec3f *b) {
    out->x = (VEC3F_INTERMEDIATE(b->x) + a->x) * 0.5f;
    out->y = (VEC3F_INTERMEDIATE(b->y) + a->y) * 0.5f;
    out->z = (VEC3F_INTERMEDIATE(b->z) + a->z) * 0.5f;
}

void Vec3fClear(Vec3f *v) {
    v->z = 0.0f;
    v->y = 0.0f;
    v->x = 0.0f;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec2fDistance(Vec2f *a, Vec2f *b) {
    long double dx = (long double)a->x - b->x;
    long double dy = (long double)a->y - b->y;
    f32 roundedDx = dx;
    f32 square = dx * roundedDx + dy * dy;

    return sqrtl(square);
}
#else
f32 Vec2fDistance(Vec2f *a, Vec2f *b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;

    return sqrtf(dx * dx + dy * dy);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec3fDistance(Vec3f *a, Vec3f *b) {
    long double dx = (long double)a->x - b->x;
    long double dy = (long double)a->y - b->y;
    long double dz = (long double)a->z - b->z;
    f32 roundedDx = dx;
    f32 roundedDy = dy;
    f32 square = dx * roundedDx + dy * roundedDy + dz * dz;

    return sqrtl(square);
}
#else
f32 Vec3fDistance(Vec3f *a, Vec3f *b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;
    f32 dz = a->z - b->z;

    return sqrtf(dx * dx + dy * dy + dz * dz);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec3fDistanceSquared(Vec3f *a, Vec3f *b) {
    long double dx = (long double)a->x - b->x;
    long double dy = (long double)a->y - b->y;
    long double dz = (long double)a->z - b->z;
    f32 roundedDx = dx;
    f32 roundedDy = dy;

    return dx * roundedDx + dy * roundedDy + dz * dz;
}
#else
f32 Vec3fDistanceSquared(Vec3f *a, Vec3f *b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;
    f32 dz = a->z - b->z;

    return dx * dx + dy * dy + dz * dz;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec3fLength(Vec3f *v) {
    f32 squared =
        (f32)((long double)v->x * v->x + ((long double)v->y * v->y + (long double)v->z * v->z));

    return sqrtl(squared);
}
#else
f32 Vec3fLength(Vec3f *v) {
    return sqrtf(v->z * v->z + (v->x * v->x + v->y * v->y));
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec2fLength(Vec2f *v) {
    f32 squared = (long double)v->y * v->y + (long double)v->x * v->x;
    return sqrtl(squared);
}
#else
f32 Vec2fLength(Vec2f *v) {
    return sqrtf(v->y * v->y + v->x * v->x);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_N64
void Vec3fRotateAroundAxis(Vec3f *v, f32 angle, Vec3f *axis) {
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    f32 t = 1.0f - c;
    f32 vx = v->x, vy = v->y, vz = v->z;
    f32 x = axis->x, y = axis->y, z = axis->z;
    f32 tx = t * x;
    f32 txy = tx * y;
    f32 sz = s * z;
    f32 txz = tx * z;
    f32 sy = s * y;
    f32 ty = t * y;
    f32 tyz = ty * z;
    f32 sx = s * x;

    /* The rotation matrix about the axis by the formula of Rodrigues. */
    v->x = (tx * x + c) * vx + (txy + sz) * vy + (txz - sy) * vz;
    v->y = (txy - sz) * vx + (t * y * y + c) * vy + (tyz + sx) * vz;
    v->z = (sy + txz) * vx + (tyz - sx) * vy + (t * z * z + c) * vz;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void Mtx4x4ProjectPoint(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
#if TGR_PLATFORM == TGR_PLATFORM_WIN && TGR_RENDERER == TGR_RENDERER_GLIDE
    /* BRGlide keeps the reciprocal of w and every sum on the x87 until the stores. */
    long double inverseW =
        1.0L / ((((long double)y * m[1][3] + (long double)x * m[0][3]) + (long double)z * m[2][3]) +
                m[3][3]);

    out->x = ((((long double)y * m[1][0] + (long double)x * m[0][0]) + (long double)z * m[2][0]) +
              m[3][0]) *
             inverseW;
    out->y = ((((long double)y * m[1][1] + (long double)x * m[0][1]) + (long double)z * m[2][1]) +
              m[3][1]) *
             inverseW;
    out->z = ((((long double)y * m[1][2] + (long double)x * m[0][2]) + (long double)z * m[2][2]) +
              m[3][2]) *
             inverseW;
#else
    f32 inverseW = 1.0f / (x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3]);

    out->x = (x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0]) * inverseW;
    out->y = (x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1]) * inverseW;
    out->z = (x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2]) * inverseW;
#endif
}

void Vec3fTransformByMatrix(f32 out[4], const Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    s32 i;

    for (i = 0; i < 4; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (i == 0) {
            out[i] = ((long double)z * m[2][i] + (long double)x * m[0][i]) +
                     (long double)y * m[1][i] + m[3][i];
        } else {
            out[i] = ((long double)z * m[2][i] + (long double)y * m[1][i]) +
                     (long double)x * m[0][i] + m[3][i];
        }
#else
        out[i] = x * m[0][i] + y * m[1][i] + z * m[2][i] + m[3][i];
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void Vec3sTransformToVec4f(f32 out[4], s16 in[3], f32 m[4][4]) {
    f32 x = in[0], y = in[1], z = in[2];
    s32 i;

    for (i = 0; i < 4; i++) {
        out[i] = x * m[0][i] + y * m[1][i] + z * m[2][i] + m[3][i];
    }
}

void ProjectVec3fToScreen(Vec2f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 inverseW = 1.0f / (x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3]);

    out->x = (x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0]) * inverseW;
    out->y = (x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1]) * inverseW;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void Mtx4x4TransformNoTranslation(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
#if TGR_PLATFORM == TGR_PLATFORM_WIN && TGR_RENDERER == TGR_RENDERER_GLIDE
    /* BRGlide adds the products in another order than BRD3D. */
    long double inverseW = 1.0L / (((long double)y * m[1][3] + (long double)x * m[0][3]) +
                                   (long double)z * m[2][3] + m[3][3]);

    out->x = (((long double)y * m[1][0] + (long double)z * m[2][0]) + (long double)x * m[0][0]) *
             inverseW;
    out->y = (((long double)y * m[1][1] + (long double)x * m[0][1]) + (long double)z * m[2][1]) *
             inverseW;
    out->z = (((long double)y * m[1][2] + (long double)x * m[0][2]) + (long double)z * m[2][2]) *
             inverseW;
#elif TGR_PLATFORM == TGR_PLATFORM_WIN
    long double inverseW = 1.0L / (((long double)y * m[1][3] + (long double)z * m[2][3]) +
                                   (long double)x * m[0][3] + m[3][3]);

    out->x = (((long double)x * m[0][0] + (long double)z * m[2][0]) + (long double)y * m[1][0]) *
             inverseW;
    out->y = (((long double)y * m[1][1] + (long double)z * m[2][1]) + (long double)x * m[0][1]) *
             inverseW;
    out->z = (((long double)y * m[1][2] + (long double)z * m[2][2]) + (long double)x * m[0][2]) *
             inverseW;
#else
    f32 inverseW = 1.0f / (x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3]);

    out->x = (x * m[0][0] + y * m[1][0] + z * m[2][0]) * inverseW;
    out->y = (x * m[0][1] + y * m[1][1] + z * m[2][1]) * inverseW;
    out->z = (x * m[0][2] + y * m[1][2] + z * m[2][2]) * inverseW;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

void Mtx3x3TransformVec(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;

    out->x = x * m[0][0] + y * m[1][0] + z * m[2][0];
    out->y = x * m[0][1] + y * m[1][1] + z * m[2][1];
    out->z = x * m[0][2] + y * m[1][2] + z * m[2][2];
}

void Mtx4x4Multiply(f32 out[4][4], f32 a[4][4], f32 b[4][4]) {
    f32 product[4][4];
    s32 row, column, i;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double total;
#endif

    for (row = 0; row < 4; row++) {
        for (column = 0; column < 4; column++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            total = 0.0L;
            for (i = 0; i < 4; i++) {
                total += (long double)a[row][i] * b[i][column];
            }
            product[row][column] = total;
#else
            product[row][column] = 0.0f;
            for (i = 0; i < 4; i++) {
                product[row][column] += a[row][i] * b[i][column];
            }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        }
    }
    for (row = 0; row < 4; row++) {
        for (column = 0; column < 4; column++) {
            out[row][column] = product[row][column];
        }
    }
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
/* Swap an entry above the diagonal with its mirror below it. */
#define SWAP_ENTRY(row, column)                                                                    \
    {                                                                                              \
        temp = in[row][column];                                                                    \
        out[row][column] = in[column][row];                                                        \
        out[column][row] = temp;                                                                   \
    }

void Mtx4fTranspose(f32 out[4][4], f32 in[4][4]) {
    f32 temp;

    SWAP_ENTRY(0, 1);
    SWAP_ENTRY(0, 2);
    SWAP_ENTRY(0, 3);
    SWAP_ENTRY(1, 2);
    SWAP_ENTRY(1, 3);
    SWAP_ENTRY(2, 3);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void CopyMatrix4x4(f32 in[4][4], f32 out[4][4]) {
    u32 word;
    s32 i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            memcpy(&word, &in[i][j], sizeof(word));
            memcpy(&out[i][j], &word, sizeof(word));
        }
    }
}
#else
void CopyMatrix4x4(f32 out[4][4], f32 in[4][4]) {
    memcpy(out, in, sizeof(f32[4][4]));
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

s32 MtxInvert4x4(f32 out[4][4], f32 in[4][4]) {
    f32 negative = 0.0f, positive = 0.0f;
    f32 term, determinant, precision;
    /* The original reserves stack for a word of locals it does not use. */
    f32 unused;

    (void)unused;

    /* The determinant of the 3 by 3 part, summed by sign to measure its precision. */
    term = in[0][0] * in[1][1] * in[2][2];
    if (term >= 0.0f) {
        positive += term;
    } else {
        negative += term;
    }
    term = in[0][1] * in[1][2] * in[2][0];
    if (term >= 0.0f) {
        positive += term;
    } else {
        negative += term;
    }
    term = in[0][2] * in[1][0] * in[2][1];
    if (term >= 0.0f) {
        positive += term;
    } else {
        negative += term;
    }
    term = -in[0][2] * in[1][1] * in[2][0];
    if (term >= 0.0f) {
        positive += term;
    } else {
        negative += term;
    }
    term = -in[0][1] * in[1][0] * in[2][2];
    if (term >= 0.0f) {
        positive += term;
    } else {
        negative += term;
    }
    term = -in[0][0] * in[1][2] * in[2][1];
    if (term >= 0.0f) {
        positive += term;
    } else {
        negative += term;
    }
    determinant = positive + negative;
    precision = determinant / (positive - negative);
    if (precision < 0.0f) {
        precision = -precision;
    }
    if (determinant == 0.0f || precision < MTX_INVERT_EPSILON) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
        SetDebugMessage("Matrix4Inverse: singular matrix");
        return 0;
#else
        for (s32 row = 0; row < 4; row++) {
            for (s32 column = 0; column < 4; column++) {
                out[row][column] = row == column ? 1.0f : 0.0f;
            }
        }
        return 1;
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
    }

    /* The determinant becomes its reciprocal. */
    determinant = 1.0f / determinant;
    out[0][0] = (in[1][1] * in[2][2] - in[1][2] * in[2][1]) * determinant;
    out[1][0] = -(in[1][0] * in[2][2] - in[1][2] * in[2][0]) * determinant;
    out[2][0] = (in[1][0] * in[2][1] - in[1][1] * in[2][0]) * determinant;
    out[0][1] = -(in[0][1] * in[2][2] - in[0][2] * in[2][1]) * determinant;
    out[1][1] = (in[0][0] * in[2][2] - in[0][2] * in[2][0]) * determinant;
    out[2][1] = -(in[0][0] * in[2][1] - in[0][1] * in[2][0]) * determinant;
    out[0][2] = (in[0][1] * in[1][2] - in[0][2] * in[1][1]) * determinant;
    out[1][2] = -(in[0][0] * in[1][2] - in[0][2] * in[1][0]) * determinant;
    out[2][2] = (in[0][0] * in[1][1] - in[0][1] * in[1][0]) * determinant;

    /* The translation is the old one carried back through the inverted rotation. */
    out[3][0] = -(in[3][0] * out[0][0] + in[3][1] * out[1][0] + in[3][2] * out[2][0]);
    out[3][1] = -(in[3][0] * out[0][1] + in[3][1] * out[1][1] + in[3][2] * out[2][1]);
    out[3][2] = -(in[3][0] * out[0][2] + in[3][1] * out[1][2] + in[3][2] * out[2][2]);
    out[0][3] = 0.0f;
    out[1][3] = 0.0f;
    out[2][3] = 0.0f;
    out[3][3] = 1.0f;
    return 1;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
static inline long double AtanSine(f32 angle) {
    long double result;
    __asm__("fsin" : "=t"(result) : "0"((long double)angle));
    return result;
}

#define ATAN_LESS(a, b) (!((a) >= (b)))
#define ATAN_ZERO(value) (!((value) < 0.0L) && !((value) > 0.0L))
long double Atan2f(f32 x, f32 y) {
    f32 base = 0.0f, angle, step, swap, squared;
    long double s, length;
#else
#define ATAN_LESS(a, b) ((a) < (b))
#define ATAN_ZERO(value) ((value) == 0.0f)
f32 Atan2f(f32 x, f32 y) {
    f32 base = 0.0f, angle, step, s, swap, length;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    s32 direct = TRUE;
    s32 i;

    /* Fold the point into the first eighth of the circle, keeping the angle folded away. */
    if (ATAN_LESS(y, 0.0f)) {
        x = -x;
        y = -y;
        base += PI_F;
    }
    if (ATAN_LESS(x, 0.0f)) {
        swap = x;
        x = y;
        y = -swap;
        base += HALF_PI_F;
    }
    if (ATAN_LESS(x, y)) {
        swap = x;
        x = y;
        y = swap;
        direct = FALSE;
        base += QUARTER_PI_F;
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    squared = (long double)y * y + (long double)x * x;
    __asm__("fsqrt" : "=t"(length) : "0"((long double)squared));
#else
    length = sqrtf(y * y + x * x);
#endif
    if (ATAN_ZERO(length)) {
        return 0.0f;
    }
    y /= length;

    /* Search for the angle whose sine is y, halving the step each time. */
    angle = EIGHTH_PI_F;
    step = angle * 0.5f;
    for (i = 0; i < ATAN2_STEPS;) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        s = AtanSine(angle);
#else
        s = sinf(angle);
#endif
        i++;
        if (ATAN_LESS(s, y)) {
            if (ATAN_LESS(y - s, ATAN2_TOLERANCE)) {
                break;
            }
            angle += step;
        } else if (y < s) {
            if (ATAN_LESS(s - y, ATAN2_TOLERANCE)) {
                break;
            }
            angle -= step;
        } else {
            break;
        }
        step *= 0.5f;
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (direct) {
        return (long double)base + angle;
    }
    return (long double)base - ((long double)angle - QUARTER_PI_F);
#else
    if (direct) {
        base += angle;
    } else {
        base += QUARTER_PI_F - angle;
    }
    return base;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

#undef ATAN_LESS
#undef ATAN_ZERO

s32 PointInTriangle3D(Vec3f *point, Vec3f *v0, Vec3f *v1, Vec3f *v2, Vec3f *normal) {
    Vec3f toV0, toV1, edge01, edge12, edge20, cross;

    Vec3fSubtract(&edge01, v1, v0);
    Vec3fSubtract(&toV1, point, v1);
    Vec3fCross(&cross, &edge01, &toV1);
    if (Vec3fDot(&cross, normal) < 0.0f) {
        return FALSE;
    }
    Vec3fSubtract(&edge12, v2, v1);
    Vec3fCross(&cross, &edge12, &toV1);
    if (Vec3fDot(&cross, normal) < 0.0f) {
        return FALSE;
    }
    Vec3fSubtract(&edge20, v0, v2);
    Vec3fSubtract(&toV0, point, v0);
    Vec3fCross(&cross, &edge20, &toV0);
    if (Vec3fDot(&cross, normal) < 0.0f) {
        return FALSE;
    }
    return TRUE;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
s32 IsPointInsideTriangle(
    Vec3f *point, Vec3f *v0, Vec3f *v1, Vec3f *v2, Vec3f *normal, f32 tolerance) {
    Vec3f toV0, toV1, edge01, edge12, edge20, cross;

    Vec3fSubtract(&edge01, v1, v0);
    Vec3fSubtract(&toV1, point, v1);
    Vec3fCross(&cross, &edge01, &toV1);
    if (Vec3fDot(&cross, normal) < tolerance) {
        return FALSE;
    }
    Vec3fSubtract(&edge12, v2, v1);
    Vec3fCross(&cross, &edge12, &toV1);
    if (Vec3fDot(&cross, normal) < tolerance) {
        return FALSE;
    }
    Vec3fSubtract(&edge20, v0, v2);
    Vec3fSubtract(&toV0, point, v0);
    Vec3fCross(&cross, &edge20, &toV0);
    if (Vec3fDot(&cross, normal) < tolerance) {
        return FALSE;
    }
    return TRUE;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define INTERSECTION_REAL(value) ((long double)(value))
#define INTERSECTION_LESS(a, b) (!((a) >= (b)))
#define INTERSECTION_NONZERO(value) ((value) < 0.0L || (value) > 0.0L)
#else
#define INTERSECTION_REAL(value) (value)
#define INTERSECTION_LESS(a, b) ((a) < (b))
#define INTERSECTION_NONZERO(value) ((value) != 0.0f)
#endif

s32 TestLineSegmentIntersection(Vec2f *p1, Vec2f *p2, Vec2f *q1, Vec2f *q2) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double side1, side3;
    f32 side2, side4;
#else
    f32 side1, side2, side3, side4;
#endif

    /* The boxes of the two segments must overlap on each axis. */
    if (INTERSECTION_LESS((p2->x < p1->x ? p1->x : p2->x),
                          (INTERSECTION_LESS(q1->x, q2->x) ? q1->x : q2->x))) {
        return 0;
    }
    if (INTERSECTION_LESS((q2->x < q1->x ? q1->x : q2->x),
                          (INTERSECTION_LESS(p1->x, p2->x) ? p1->x : p2->x))) {
        return 0;
    }
    if (INTERSECTION_LESS((p2->y < p1->y ? p1->y : p2->y),
                          (INTERSECTION_LESS(q1->y, q2->y) ? q1->y : q2->y))) {
        return 0;
    }
    if (INTERSECTION_LESS((q2->y < q1->y ? q1->y : q2->y),
                          (INTERSECTION_LESS(p1->y, p2->y) ? p1->y : p2->y))) {
        return 0;
    }

    /* Each segment must have the ends of the other on both sides of its line. */
    side1 = (INTERSECTION_REAL(q1->x) - p1->x) * (f32)(INTERSECTION_REAL(p2->y) - p1->y) -
            (INTERSECTION_REAL(p2->x) - p1->x) * (INTERSECTION_REAL(q1->y) - p1->y);
    side2 = (INTERSECTION_REAL(q2->x) - p1->x) * (f32)(INTERSECTION_REAL(p2->y) - p1->y) -
            (INTERSECTION_REAL(p2->x) - p1->x) * (INTERSECTION_REAL(q2->y) - p1->y);
    if (INTERSECTION_NONZERO(side1) && INTERSECTION_NONZERO(side2) &&
        INTERSECTION_REAL((f32)side1) * side2 > 0.0f) {
        return 0;
    }
    side3 = (INTERSECTION_REAL(p1->x) - q1->x) * (f32)(INTERSECTION_REAL(q2->y) - q1->y) -
            (INTERSECTION_REAL(q2->x) - q1->x) * (INTERSECTION_REAL(p1->y) - q1->y);
    side4 = (INTERSECTION_REAL(p2->x) - q1->x) * (f32)(INTERSECTION_REAL(q2->y) - q1->y) -
            (INTERSECTION_REAL(q2->x) - q1->x) * (INTERSECTION_REAL(p2->y) - q1->y);
    if (INTERSECTION_NONZERO(side3) && INTERSECTION_NONZERO(side4) &&
        INTERSECTION_REAL((f32)side3) * side4 > 0.0f) {
        return 0;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!INTERSECTION_NONZERO((long double)(f32)side1 - side2)) {
#else
    if (side1 == side2) {
#endif
        return 2;
    }
    return 1;
}

#undef INTERSECTION_REAL
#undef INTERSECTION_LESS
#undef INTERSECTION_NONZERO

s32 SegmentCrossesLine2D(const Vec3f *a, const Vec3f *b, const Vec3f *p, const Vec3f *q) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    f32 dx = (long double)b->x - a->x;
    f32 dy = (long double)b->y - a->y;
    long double side1 = ((long double)p->x - a->x) * dy - ((long double)p->y - a->y) * dx;
    long double side2 = ((long double)q->x - a->x) * dy - ((long double)q->y - a->y) * dx;
    long double difference;

    if ((side1 < 0.0L || side1 > 0.0L) && (side2 < 0.0L || side2 > 0.0L) && side1 * side2 > 0.0L) {
        return 0;
    }
    difference = side1 - side2;
    return !(difference < 0.0L) && !(difference > 0.0L) ? 2 : 1;
#else
    f32 side1 = (p->x - a->x) * (b->y - a->y) - (b->x - a->x) * (p->y - a->y);
    f32 side2 = (q->x - a->x) * (b->y - a->y) - (b->x - a->x) * (q->y - a->y);

    if (side1 != 0.0f && side2 != 0.0f && side1 * side2 > 0.0f) {
        return 0;
    }
    if (side1 == side2) {
        return 2;
    }
    return 1;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
}

void SetRngSeed(u32 seed) {
    g_dwRandomSeed = seed;
}

u32 AdvanceRng(void) {
    g_dwRandomSeed = (g_dwRandomSeed * RNG_MULTIPLIER) & RNG_MASK;
    return g_dwRandomSeed;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec3dLengthSquared(Vec3d *v) {
    long double x = v->x;
    long double y = v->y;
    long double z = v->z;

    return x * x + (y * y + z * z);
}

long double Vec3dLength(Vec3d *v) {
    return sqrtl(Vec3dLengthSquared(v));
}

Vec3d *Vec3dNormalize(Vec3d *v) {
    long double length = Vec3dLength(v);

    if (length != 0.0L && !isnan(length)) {
        v->x /= length;
        v->y /= length;
        v->z /= length;
    }
    return v;
}

long double Vec3dDot(Vec3d *a, Vec3d *b) {
    return (long double)a->x * b->x + ((long double)a->y * b->y + (long double)a->z * b->z);
}

void Vec3dCross(Vec3d *a, Vec3d *b, Vec3d *out) {
    out->x = (long double)a->y * b->z - (long double)a->z * b->y;
    out->y = (long double)a->z * b->x - (long double)a->x * b->z;
    out->z = (long double)a->x * b->y - (long double)a->y * b->x;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

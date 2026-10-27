#include "vecmath.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "system.h"
#else
#include <math.h>
#include <string.h>
#endif

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
#endif

void Vec3fCross(Vec3f *out, Vec3f *a, Vec3f *b) {
    f32 ax = a->x, ay = a->y, az = a->z;
    f32 bx = b->x, by = b->y, bz = b->z;

    out->z = ax * by - bx * ay;
    out->y = az * bx - bz * ax;
    out->x = ay * bz - by * az;
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
long double Vec3fDot(Vec3f *a, Vec3f *b) {
    return (long double)a->x * b->x + ((long double)a->y * b->y + (long double)a->z * b->z);
}
#else
f32 Vec3fDot(Vec3f *a, Vec3f *b) {
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
#endif

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
    divisor = 1.0f / divisor;
    v->x *= divisor;
    v->y *= divisor;
    v->z *= divisor;
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void Vec3fCopy(Vec3f *out, Vec3f *v) {
    out->x = v->x;
    out->y = v->y;
    out->z = v->z;
}
#endif

void Vec3fDirection(Vec3f *out, Vec3f *from, Vec3f *to) {
    f32 dx = to->x - from->x;
    f32 dy = to->y - from->y;
    f32 dz = to->z - from->z;
    f32 length = sqrtf(dx * dx + dy * dy + dz * dz);

    if (length != 0.0f) {
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
#endif

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
#endif
}

void Vec3fSubtract(Vec3f *out, Vec3f *a, Vec3f *b) {
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
    out->x = b->x + (a->x - b->x) * t;
    out->y = b->y + (a->y - b->y) * t;
    out->z = b->z + (a->z - b->z) * t;
}

void Vec3fMulAdd(Vec3f *out, Vec3f *a, Vec3f *b, f32 scale) {
    out->x = b->x * scale + a->x;
    out->y = b->y * scale + a->y;
    out->z = b->z * scale + a->z;
}

void Vec3fMulAddInPlace(Vec3f *v, Vec3f *b, f32 scale) {
    v->x += b->x * scale;
    v->y += b->y * scale;
    v->z += b->z * scale;
}

void Vec3fMidpoint(Vec3f *out, Vec3f *a, Vec3f *b) {
    out->x = (b->x + a->x) * 0.5f;
    out->y = (b->y + a->y) * 0.5f;
    out->z = (b->z + a->z) * 0.5f;
}

void Vec3fClear(Vec3f *v) {
    v->z = 0.0f;
    v->y = 0.0f;
    v->x = 0.0f;
}

f32 Vec2fDistance(Vec2f *a, Vec2f *b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;

    return sqrtf(dx * dx + dy * dy);
}

f32 Vec3fDistance(Vec3f *a, Vec3f *b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;
    f32 dz = a->z - b->z;

    return sqrtf(dx * dx + dy * dy + dz * dz);
}

f32 Vec3fDistanceSquared(Vec3f *a, Vec3f *b) {
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;
    f32 dz = a->z - b->z;

    return dx * dx + dy * dy + dz * dz;
}

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
#endif

f32 Vec2fLength(Vec2f *v) {
    return sqrtf(v->y * v->y + v->x * v->x);
}

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
#endif

void Mtx4x4ProjectPoint(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 inverseW = 1.0f / (x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3]);

    out->x = (x * m[0][0] + y * m[1][0] + z * m[2][0] + m[3][0]) * inverseW;
    out->y = (x * m[0][1] + y * m[1][1] + z * m[2][1] + m[3][1]) * inverseW;
    out->z = (x * m[0][2] + y * m[1][2] + z * m[2][2] + m[3][2]) * inverseW;
}

void Vec3fTransformByMatrix(f32 out[4], Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    s32 i;

    for (i = 0; i < 4; i++) {
        out[i] = x * m[0][i] + y * m[1][i] + z * m[2][i] + m[3][i];
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
#endif

void Mtx4x4TransformNoTranslation(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 inverseW = 1.0f / (x * m[0][3] + y * m[1][3] + z * m[2][3] + m[3][3]);

    out->x = (x * m[0][0] + y * m[1][0] + z * m[2][0]) * inverseW;
    out->y = (x * m[0][1] + y * m[1][1] + z * m[2][1]) * inverseW;
    out->z = (x * m[0][2] + y * m[1][2] + z * m[2][2]) * inverseW;
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

    for (row = 0; row < 4; row++) {
        for (column = 0; column < 4; column++) {
            product[row][column] = 0.0f;
            for (i = 0; i < 4; i++) {
                product[row][column] += a[row][i] * b[i][column];
            }
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
#endif

void CopyMatrix4x4(f32 out[4][4], f32 in[4][4]) {
    memcpy(out, in, sizeof(f32[4][4]));
}

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
#endif
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

f32 Atan2f(f32 x, f32 y) {
    f32 base = 0.0f, angle, step, s, swap, length;
    s32 direct = TRUE;
    s32 i;

    /* Fold the point into the first eighth of the circle, keeping the angle folded away. */
    if (y < 0.0f) {
        x = -x;
        y = -y;
        base += PI_F;
    }
    if (x < 0.0f) {
        swap = x;
        x = y;
        y = -swap;
        base += HALF_PI_F;
    }
    if (x < y) {
        swap = x;
        x = y;
        y = swap;
        direct = FALSE;
        base += QUARTER_PI_F;
    }

    length = sqrtf(y * y + x * x);
    if (length == 0.0f) {
        return 0.0f;
    }
    y /= length;

    /* Search for the angle whose sine is y, halving the step each time. */
    angle = EIGHTH_PI_F;
    step = angle * 0.5f;
    for (i = 0; i < ATAN2_STEPS;) {
        s = sinf(angle);
        i++;
        if (s < y) {
            if (y - s < ATAN2_TOLERANCE) {
                break;
            }
            angle += step;
        } else if (y < s) {
            if (s - y < ATAN2_TOLERANCE) {
                break;
            }
            angle -= step;
        } else {
            break;
        }
        step *= 0.5f;
    }

    if (direct) {
        base += angle;
    } else {
        base += QUARTER_PI_F - angle;
    }
    return base;
}

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
#endif

s32 TestLineSegmentIntersection(Vec2f *p1, Vec2f *p2, Vec2f *q1, Vec2f *q2) {
    f32 side1, side2, side3, side4;

    /* The boxes of the two segments must overlap on each axis. */
    if ((p2->x < p1->x ? p1->x : p2->x) < (q1->x < q2->x ? q1->x : q2->x)) {
        return 0;
    }
    if ((q2->x < q1->x ? q1->x : q2->x) < (p1->x < p2->x ? p1->x : p2->x)) {
        return 0;
    }
    if ((p2->y < p1->y ? p1->y : p2->y) < (q1->y < q2->y ? q1->y : q2->y)) {
        return 0;
    }
    if ((q2->y < q1->y ? q1->y : q2->y) < (p1->y < p2->y ? p1->y : p2->y)) {
        return 0;
    }

    /* Each segment must have the ends of the other on both sides of its line. */
    side1 = (q1->x - p1->x) * (p2->y - p1->y) - (p2->x - p1->x) * (q1->y - p1->y);
    side2 = (q2->x - p1->x) * (p2->y - p1->y) - (p2->x - p1->x) * (q2->y - p1->y);
    if (side1 != 0.0f && side2 != 0.0f && side1 * side2 > 0.0f) {
        return 0;
    }
    side3 = (p1->x - q1->x) * (q2->y - q1->y) - (q2->x - q1->x) * (p1->y - q1->y);
    side4 = (p2->x - q1->x) * (q2->y - q1->y) - (q2->x - q1->x) * (p2->y - q1->y);
    if (side3 != 0.0f && side4 != 0.0f && side3 * side4 > 0.0f) {
        return 0;
    }
    if (side1 == side2) {
        return 2;
    }
    return 1;
}

s32 SegmentCrossesLine2D(Vec3f *a, Vec3f *b, Vec3f *p, Vec3f *q) {
    f32 side1 = (p->x - a->x) * (b->y - a->y) - (b->x - a->x) * (p->y - a->y);
    f32 side2 = (q->x - a->x) * (b->y - a->y) - (b->x - a->x) * (q->y - a->y);

    if (side1 != 0.0f && side2 != 0.0f && side1 * side2 > 0.0f) {
        return 0;
    }
    if (side1 == side2) {
        return 2;
    }
    return 1;
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
#endif

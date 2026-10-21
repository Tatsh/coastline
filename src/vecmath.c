#include "vecmath.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "system.h"

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

f32 Powf(f32 base, f32 exponent) {
    f32 result = 1.0f;
    f32 x, n, term, sum;

    while (exponent >= 1.0f) {
        exponent -= 1.0f;
        result *= base;
    }
    if (exponent == 0.0f) {
        return result;
    }

    /* The rest of the power, as the binomial series of (1 + x) to the remaining exponent. */
    x = base - 1.0f;
    n = 1.0f;
    term = exponent * x;
    sum = 1.0f + term;
    do {
        f32 factor = (exponent - n) * x;

        n += 1.0f;
        term = term * factor / n;
        sum += term;
    } while (n < POWF_TERMS);
    return result * sum;
}

void Vec3fCross(Vec3f *out, Vec3f *a, Vec3f *b) {
    f32 ax = a->x, ay = a->y, az = a->z;
    f32 bx = b->x, by = b->y, bz = b->z;

    out->z = ax * by - bx * ay;
    out->y = az * bx - bz * ax;
    out->x = ay * bz - by * az;
}

f32 Vec3fDot(Vec3f *a, Vec3f *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void Vec3fNegate(Vec3f *out, Vec3f *v) {
    out->x = -v->x;
    out->y = -v->y;
    out->z = -v->z;
}

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
    f32 inverse = 1.0f / divisor;

    out->x = v->x * inverse;
    out->y = v->y * inverse;
    out->z = v->z * inverse;
}

void Vec3fDivideInPlace(Vec3f *v, f32 divisor) {
    f32 inverse = 1.0f / divisor;

    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
}

void Vec3fCopy(Vec3f *out, Vec3f *v) {
    out->x = v->x;
    out->y = v->y;
    out->z = v->z;
}

void Vec3fDirection(Vec3f *out, Vec3f *from, Vec3f *to) {
    f32 dx = to->x - from->x;
    f32 dy = to->y - from->y;
    f32 dz = to->z - from->z;
    f32 length = sqrtf(dx * dx + dy * dy + dz * dz);
    f32 inverse;

    if (length == 0.0f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 1.0f;
        return;
    }
    inverse = 1.0f / length;
    out->x = dx * inverse;
    out->y = dy * inverse;
    out->z = dz * inverse;
}

void Vec3fNormalizeSafe(Vec3f *out, Vec3f *v) {
    f32 length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    f32 inverse;

    if (length == 0.0f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 1.0f;
        return;
    }
    inverse = 1.0f / length;
    out->x = v->x * inverse;
    out->y = v->y * inverse;
    out->z = v->z * inverse;
}

void Vec3fNormalize(Vec3f *v) {
    f32 length = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    f32 inverse;

    if (length == 0.0f) {
        v->x = 0.0f;
        v->y = 0.0f;
        v->z = 1.0f;
        return;
    }
    inverse = 1.0f / length;
    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
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

void Vec2fAdd(Vec2f *out, Vec2f *a, Vec2f *b) {
    out->x = b->x + a->x;
    out->y = b->y + a->y;
}

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

f32 Vec3fLength(Vec3f *v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

f32 Vec2fLength(Vec2f *v) {
    return sqrtf(v->y * v->y + v->x * v->x);
}

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
    v->y = (txy - sz) * vx + (ty * y + c) * vy + (tyz + sx) * vz;
    v->z = (sy + txz) * vx + (tyz - sx) * vy + (t * z * z + c) * vz;
}

void Mtx4x4ProjectPoint(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 inverseW = 1.0f / (m[3][3] + (x * m[0][3] + y * m[1][3] + z * m[2][3]));

    out->x = (m[3][0] + (x * m[0][0] + y * m[1][0] + z * m[2][0])) * inverseW;
    out->y = (m[3][1] + (x * m[0][1] + y * m[1][1] + z * m[2][1])) * inverseW;
    out->z = (m[3][2] + (x * m[0][2] + y * m[1][2] + z * m[2][2])) * inverseW;
}

void Vec3fTransformByMatrix(f32 out[4], Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    s32 i;

    for (i = 0; i < 4; i++) {
        out[i] = m[3][i] + (x * m[0][i] + y * m[1][i] + z * m[2][i]);
    }
}

void Vec3sTransformToVec4f(f32 out[4], s16 in[3], f32 m[4][4]) {
    f32 x = in[0], y = in[1], z = in[2];
    s32 i;

    for (i = 0; i < 4; i++) {
        out[i] = m[3][i] + (x * m[0][i] + y * m[1][i] + z * m[2][i]);
    }
}

void ProjectVec3fToScreen(Vec2f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 inverseW = 1.0f / (m[3][3] + (x * m[0][3] + y * m[1][3] + z * m[2][3]));

    out->x = (m[3][0] + (x * m[0][0] + y * m[1][0] + z * m[2][0])) * inverseW;
    out->y = (m[3][1] + (x * m[0][1] + y * m[1][1] + z * m[2][1])) * inverseW;
}

void Mtx4x4TransformNoTranslation(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 inverseW = 1.0f / (m[3][3] + (x * m[0][3] + y * m[1][3] + z * m[2][3]));

    out->x = (m[2][0] * z + (x * m[0][0] + y * m[1][0])) * inverseW;
    out->y = (m[2][1] * z + (x * m[0][1] + y * m[1][1])) * inverseW;
    out->z = (m[2][2] * z + (x * m[0][2] + y * m[1][2])) * inverseW;
}

void Mtx3x3TransformVec(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;

    out->x = m[2][0] * z + (x * m[0][0] + y * m[1][0]);
    out->y = m[2][1] * z + (x * m[0][1] + y * m[1][1]);
    out->z = m[2][2] * z + (x * m[0][2] + y * m[1][2]);
}

void Mtx4x4Multiply(f32 out[4][4], f32 a[4][4], f32 b[4][4]) {
    f32 product[4][4];
    s32 row, column;

    for (row = 0; row < 4; row++) {
        for (column = 0; column < 4; column++) {
            product[row][column] = 0.0f;
            product[row][column] += a[row][0] * b[0][column];
            product[row][column] += a[row][1] * b[1][column];
            product[row][column] += a[row][2] * b[2][column];
            product[row][column] += a[row][3] * b[3][column];
        }
    }
    for (row = 0; row < 4; row++) {
        for (column = 0; column < 4; column++) {
            out[row][column] = product[row][column];
        }
    }
}

void Mtx4fTranspose(f32 out[4][4], f32 in[4][4]) {
    s32 row, column;
    f32 upper, lower;

    for (row = 0; row < 4; row++) {
        for (column = row + 1; column < 4; column++) {
            upper = in[row][column];
            lower = in[column][row];
            out[column][row] = upper;
            out[row][column] = lower;
        }
    }
}

void CopyMatrix4x4(f32 out[4][4], f32 in[4][4]) {
    memcpy(out, in, sizeof(f32[4][4]));
}

s32 MtxInvert4x4(f32 out[4][4], f32 in[4][4]) {
    f32 m00 = in[0][0], m11 = in[1][1], m22 = in[2][2];
    f32 m01 = in[0][1], m12 = in[1][2], m20 = in[2][0];
    f32 m02 = in[0][2], m10 = in[1][0], m21 = in[2][1];
    f32 terms[6];
    f32 positive = 0.0f, negative = 0.0f;
    f32 determinant, precision, inverse;
    s32 i;

    /* The determinant of the 3 by 3 part, summed by sign to measure its precision. */
    terms[0] = m22 * (m00 * m11);
    terms[1] = m20 * (m01 * m12);
    terms[2] = m21 * (m02 * m10);
    terms[3] = m20 * (-m02 * m11);
    terms[4] = m22 * (-m01 * m10);
    terms[5] = m21 * (-m00 * m12);
    for (i = 0; i < 6; i++) {
        if (terms[i] >= 0.0f) {
            positive += terms[i];
        } else {
            negative += terms[i];
        }
    }
    determinant = positive + negative;
    precision = determinant / (positive - negative);
    if (precision < 0.0f) {
        precision = -precision;
    }
    if (determinant == 0.0f || precision < MTX_INVERT_EPSILON) {
        SetDebugMessage("Matrix4Inverse: singular matrix");
        return 0;
    }

    inverse = 1.0f / determinant;
    out[0][0] = (m11 * m22 - m21 * m12) * inverse;
    out[1][0] = -(in[1][0] * in[2][2] - in[2][0] * in[1][2]) * inverse;
    out[2][0] = (in[1][0] * in[2][1] - in[2][0] * in[1][1]) * inverse;
    out[0][1] = -(in[0][1] * in[2][2] - in[2][1] * in[0][2]) * inverse;
    out[1][1] = (in[0][0] * in[2][2] - in[2][0] * in[0][2]) * inverse;
    out[2][1] = -(in[0][0] * in[2][1] - in[2][0] * in[0][1]) * inverse;
    out[0][2] = (in[0][1] * in[1][2] - in[1][1] * in[0][2]) * inverse;
    out[1][2] = -(in[0][0] * in[1][2] - in[1][0] * in[0][2]) * inverse;
    out[2][2] = (in[0][0] * in[1][1] - in[1][0] * in[0][1]) * inverse;

    /* The translation is the old one carried back through the inverted rotation. */
    out[3][0] = -(out[2][0] * in[3][2] + (in[3][0] * out[0][0] + in[3][1] * out[1][0]));
    out[3][1] = -(out[2][1] * in[3][2] + (in[3][0] * out[0][1] + in[3][1] * out[1][1]));
    out[0][3] = 0.0f;
    out[1][3] = 0.0f;
    out[2][3] = 0.0f;
    out[3][3] = 1.0f;
    out[3][2] = -(out[2][2] * in[3][2] + (in[3][0] * out[0][2] + in[3][1] * out[1][2]));
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
    step = EIGHTH_PI_F * 0.5f;
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
        return base + angle;
    }
    return base + (QUARTER_PI_F - angle);
}

s32 PointInTriangle3D(Vec3f *point, Vec3f *v0, Vec3f *v1, Vec3f *v2, Vec3f *normal) {
    Vec3f edge, toPoint, cross;

    Vec3fSubtract(&edge, v1, v0);
    Vec3fSubtract(&toPoint, point, v1);
    Vec3fCross(&cross, &edge, &toPoint);
    if (Vec3fDot(&cross, normal) < 0.0f) {
        return FALSE;
    }
    Vec3fSubtract(&edge, v2, v1);
    Vec3fCross(&cross, &edge, &toPoint);
    if (Vec3fDot(&cross, normal) < 0.0f) {
        return FALSE;
    }
    Vec3fSubtract(&edge, v0, v2);
    Vec3fSubtract(&toPoint, point, v0);
    Vec3fCross(&cross, &edge, &toPoint);
    if (Vec3fDot(&cross, normal) < 0.0f) {
        return FALSE;
    }
    return TRUE;
}

s32 IsPointInsideTriangle(
    Vec3f *point, Vec3f *v0, Vec3f *v1, Vec3f *v2, Vec3f *normal, f32 tolerance) {
    Vec3f edge, toPoint, cross;

    Vec3fSubtract(&edge, v1, v0);
    Vec3fSubtract(&toPoint, point, v1);
    Vec3fCross(&cross, &edge, &toPoint);
    if (Vec3fDot(&cross, normal) < tolerance) {
        return FALSE;
    }
    Vec3fSubtract(&edge, v2, v1);
    Vec3fCross(&cross, &edge, &toPoint);
    if (Vec3fDot(&cross, normal) < tolerance) {
        return FALSE;
    }
    Vec3fSubtract(&edge, v0, v2);
    Vec3fSubtract(&toPoint, point, v0);
    Vec3fCross(&cross, &edge, &toPoint);
    if (Vec3fDot(&cross, normal) < tolerance) {
        return FALSE;
    }
    return TRUE;
}

/* The sides of two points relative to the line through a and b, as in SegmentCrossesLine2D(). */
static s32 CompareSides(f32 side1, f32 side2) {
    if (side1 != 0.0f && side2 != 0.0f && side1 * side2 > 0.0f) {
        return 0;
    }
    return side1 == side2 ? 2 : 1;
}

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
    return side1 == side2 ? 2 : 1;
}

s32 SegmentCrossesLine2D(Vec2f *a, Vec2f *b, Vec2f *p, Vec2f *q) {
    f32 dx = b->x - a->x;
    f32 dy = b->y - a->y;

    return CompareSides((p->x - a->x) * dy - dx * (p->y - a->y),
                        (q->x - a->x) * dy - dx * (q->y - a->y));
}

void SetRngSeed(u32 seed) {
    g_dwRandomSeed = seed;
}

u32 AdvanceRng(void) {
    g_dwRandomSeed = (g_dwRandomSeed * RNG_MULTIPLIER) & RNG_MASK;
    return g_dwRandomSeed;
}

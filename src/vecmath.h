#ifndef COASTLINE_VECMATH_H
#define COASTLINE_VECMATH_H

/**
 * @file
 * The vector and matrix arithmetic of the game, its arctangent and power, the triangle and segment
 * tests, and its random numbers.
 */

#include <PR/ultratypes.h>

/** A point or direction in space. */
typedef struct {
    f32 x; /*!< The component along the x axis. */
    f32 y; /*!< The component along the y axis. */
    f32 z; /*!< The component along the z axis. */
} Vec3f;

/** A point or direction in a plane. */
typedef struct {
    f32 x; /*!< The component along the x axis. */
    f32 y; /*!< The component along the y axis. */
} Vec2f;

/**
 * @ghidraAddress USA: 0x8028b790
 * @ghidraAddress JPN: 0x8028d470
 * @ghidraAddress EUR: 0x8028da40
 */
extern u32 g_dwRandomSeed;

/**
 * Raise a number to a power: by multiplication for the whole part of the exponent and by the
 * binomial series for the rest.
 *
 * @param base The number raised.
 * @param exponent The power, at least 0.
 * @return The base raised to the exponent.
 * @ghidraAddress USA: 0x802242e0
 * @ghidraAddress JPN: 0x80224e10
 * @ghidraAddress EUR: 0x80225620
 */
f32 Powf(f32 base, f32 exponent);

/**
 * Take the cross product of two vectors.
 *
 * @param out The vector that receives the cross product of a and b.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x8022439c
 * @ghidraAddress JPN: 0x80224ecc
 * @ghidraAddress EUR: 0x802256dc
 */
void Vec3fCross(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Take the dot product of two vectors.
 *
 * @param a The first vector.
 * @param b The second vector.
 * @return The dot product of the vectors.
 * @ghidraAddress USA: 0x80224404
 * @ghidraAddress JPN: 0x80224f34
 * @ghidraAddress EUR: 0x80225744
 */
f32 Vec3fDot(Vec3f *a, Vec3f *b);

/**
 * Negate a vector.
 *
 * @param out The vector that receives -v.
 * @param v The vector negated.
 * @ghidraAddress USA: 0x80224434
 * @ghidraAddress JPN: 0x80224f64
 * @ghidraAddress EUR: 0x80225774
 */
void Vec3fNegate(Vec3f *out, Vec3f *v);

/**
 * Negate a vector in place.
 *
 * @param v The vector negated.
 * @ghidraAddress USA: 0x8022445c
 */
void NegateVectorInPlace(Vec3f *v);

/**
 * Multiply two vectors component by component.
 *
 * @param out The vector that receives the product.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x80224484
 */
void Vec2fMul(Vec2f *out, Vec2f *a, Vec2f *b);

/**
 * Multiply a vector by a number.
 *
 * @param out The vector that receives v * scale.
 * @param v The vector scaled.
 * @param scale The factor.
 * @ghidraAddress USA: 0x802244a8
 */
void Vec2fScale(Vec2f *out, Vec2f *v, f32 scale);

/**
 * Multiply two vectors component by component.
 *
 * @param out The vector that receives the product.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x802244c8
 */
void Vec3fComponentProduct(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Multiply a vector by a number.
 *
 * @param out The vector that receives v * scale.
 * @param v The vector scaled.
 * @param scale The factor.
 * @ghidraAddress USA: 0x802244fc
 * @ghidraAddress JPN: 0x8022502c
 * @ghidraAddress EUR: 0x8022583c
 */
void Vec3fScale(Vec3f *out, Vec3f *v, f32 scale);

/**
 * Multiply a vector by a number in place.
 *
 * @param v The vector scaled.
 * @param scale The factor.
 * @ghidraAddress USA: 0x80224528
 * @ghidraAddress JPN: 0x80225058
 * @ghidraAddress EUR: 0x80225868
 */
void Vec3fScaleInPlace(Vec3f *v, f32 scale);

/**
 * Multiply a vector by the reciprocal of a number.
 *
 * @param out The vector that receives v / divisor.
 * @param v The vector divided.
 * @param divisor The number the vector is divided by.
 * @ghidraAddress USA: 0x8022455c
 * @ghidraAddress JPN: 0x8022508c
 * @ghidraAddress EUR: 0x8022589c
 */
void Vec3fScaleByInverse(Vec3f *out, Vec3f *v, f32 divisor);

/**
 * Divide a vector by a number in place.
 *
 * @param v The vector divided.
 * @param divisor The number the vector is divided by.
 * @ghidraAddress USA: 0x80224594
 * @ghidraAddress JPN: 0x802250c4
 * @ghidraAddress EUR: 0x802258d4
 */
void Vec3fDivideInPlace(Vec3f *v, f32 divisor);

/**
 * Copy a vector.
 *
 * @param out The vector that receives the copy.
 * @param v The vector copied.
 * @ghidraAddress USA: 0x802245d4
 */
void Vec3fCopy(Vec3f *out, Vec3f *v);

/**
 * The unit vector from one point towards another, or (0, 0, 1) when they coincide.
 *
 * @param out The vector that receives the direction.
 * @param from The point the direction starts at.
 * @param to The point the direction runs towards.
 * @ghidraAddress USA: 0x802245f0
 * @ghidraAddress JPN: 0x80225120
 * @ghidraAddress EUR: 0x80225930
 */
void Vec3fDirection(Vec3f *out, Vec3f *from, Vec3f *to);

/**
 * The unit vector along a vector, or (0, 0, 1) for the zero vector.
 *
 * @param out The vector that receives the unit vector.
 * @param v The vector normalised.
 * @ghidraAddress USA: 0x802246bc
 * @ghidraAddress JPN: 0x802251ec
 * @ghidraAddress EUR: 0x802259fc
 */
void Vec3fNormalizeSafe(Vec3f *out, Vec3f *v);

/**
 * Make a vector a unit vector, or (0, 0, 1) if it is the zero vector.
 *
 * @param v The vector normalised in place.
 * @ghidraAddress USA: 0x80224760
 * @ghidraAddress JPN: 0x80225290
 * @ghidraAddress EUR: 0x80225aa0
 */
void Vec3fNormalize(Vec3f *v);

/**
 * Subtract one vector from another.
 *
 * @param out The vector that receives a - b.
 * @param a The vector subtracted from.
 * @param b The vector subtracted.
 * @ghidraAddress USA: 0x80224808
 * @ghidraAddress JPN: 0x80225338
 * @ghidraAddress EUR: 0x80225b48
 */
void Vec3fSubtract(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Subtract one vector from another in place.
 *
 * @param v The vector subtracted from, which receives v - b.
 * @param b The vector subtracted.
 * @ghidraAddress USA: 0x8022483c
 * @ghidraAddress JPN: 0x8022536c
 * @ghidraAddress EUR: 0x80225b7c
 */
void Vec3fSubtractInPlace(Vec3f *v, Vec3f *b);

/**
 * Add two vectors.
 *
 * @param out The vector that receives a + b.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x80224870
 */
void Vec2fAdd(Vec2f *out, Vec2f *a, Vec2f *b);

/**
 * Add two vectors.
 *
 * @param out The vector that receives a + b.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x80224894
 * @ghidraAddress JPN: 0x802253c4
 * @ghidraAddress EUR: 0x80225bd4
 */
void Vec3fAdd(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Add one vector to another in place.
 *
 * @param v The vector added to, which receives v + b.
 * @param b The vector added.
 * @ghidraAddress USA: 0x802248c8
 * @ghidraAddress JPN: 0x802253f8
 * @ghidraAddress EUR: 0x80225c08
 */
void Vec3fAddInPlace(Vec3f *v, Vec3f *b);

/**
 * The point a share of the way from b to a: b + (a - b) * t.
 *
 * @param out The vector that receives the point.
 * @param a The point reached when t is 1.
 * @param b The point reached when t is 0.
 * @param t The share of the way from b to a.
 * @ghidraAddress USA: 0x802248fc
 * @ghidraAddress JPN: 0x8022542c
 * @ghidraAddress EUR: 0x80225c3c
 */
void Vec3fLerp(Vec3f *out, Vec3f *a, Vec3f *b, f32 t);

/**
 * Add a scaled vector to another.
 *
 * @param out The vector that receives a + b * scale.
 * @param a The vector added to.
 * @param b The vector scaled and added.
 * @param scale The factor for b.
 * @ghidraAddress USA: 0x8022494c
 * @ghidraAddress JPN: 0x8022547c
 * @ghidraAddress EUR: 0x80225c8c
 */
void Vec3fMulAdd(Vec3f *out, Vec3f *a, Vec3f *b, f32 scale);

/**
 * Add a scaled vector to another in place.
 *
 * @param v The vector added to, which receives v + b * scale.
 * @param b The vector scaled and added.
 * @param scale The factor for b.
 * @ghidraAddress USA: 0x80224990
 * @ghidraAddress JPN: 0x802254c0
 * @ghidraAddress EUR: 0x80225cd0
 */
void Vec3fMulAddInPlace(Vec3f *v, Vec3f *b, f32 scale);

/**
 * The point halfway between two points.
 *
 * @param out The vector that receives the midpoint.
 * @param a The first point.
 * @param b The second point.
 * @ghidraAddress USA: 0x802249d4
 * @ghidraAddress JPN: 0x80225504
 * @ghidraAddress EUR: 0x80225d14
 */
void Vec3fMidpoint(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Set a vector to zero.
 *
 * @param v The vector cleared.
 * @ghidraAddress USA: 0x80224a1c
 * @ghidraAddress JPN: 0x8022554c
 * @ghidraAddress EUR: 0x80225d5c
 */
void Vec3fClear(Vec3f *v);

/**
 * The distance between two points of the plane.
 *
 * @param a The first point.
 * @param b The second point.
 * @return The distance between the points.
 * @ghidraAddress USA: 0x80224a34
 * @ghidraAddress JPN: 0x80225564
 * @ghidraAddress EUR: 0x80225d74
 */
f32 Vec2fDistance(Vec2f *a, Vec2f *b);

/**
 * The distance between two points.
 *
 * @param a The first point.
 * @param b The second point.
 * @return The distance between the points.
 * @ghidraAddress USA: 0x80224a78
 * @ghidraAddress JPN: 0x802255a8
 * @ghidraAddress EUR: 0x80225db8
 */
f32 Vec3fDistance(Vec3f *a, Vec3f *b);

/**
 * The square of the distance between two points.
 *
 * @param a The first point.
 * @param b The second point.
 * @return The squared distance between the points.
 * @ghidraAddress USA: 0x80224acc
 * @ghidraAddress JPN: 0x802255fc
 * @ghidraAddress EUR: 0x80225e0c
 */
f32 Vec3fDistanceSquared(Vec3f *a, Vec3f *b);

/**
 * The length of a vector.
 *
 * @param v The vector measured.
 * @return The length of the vector.
 * @ghidraAddress USA: 0x80224b08
 * @ghidraAddress JPN: 0x80225638
 * @ghidraAddress EUR: 0x80225e48
 */
f32 Vec3fLength(Vec3f *v);

/**
 * The length of a vector of the plane.
 *
 * @param v The vector measured.
 * @return The length of the vector.
 * @ghidraAddress USA: 0x80224b48
 * @ghidraAddress JPN: 0x80225678
 * @ghidraAddress EUR: 0x80225e88
 */
f32 Vec2fLength(Vec2f *v);

/**
 * Rotate a vector by an angle about a unit axis, clockwise as seen from the tip of the axis.
 *
 * @param v The vector rotated in place.
 * @param angle The angle of the rotation, in radians.
 * @param axis The unit axis of the rotation.
 * @ghidraAddress USA: 0x80224b7c
 * @ghidraAddress JPN: 0x802256ac
 * @ghidraAddress EUR: 0x80225ebc
 */
void Vec3fRotateAroundAxis(Vec3f *v, f32 angle, Vec3f *axis);

/**
 * Transform a point by a matrix and divide by w.
 *
 * @param out The vector that receives the transformed point.
 * @param in The point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224d00
 * @ghidraAddress JPN: 0x80225830
 * @ghidraAddress EUR: 0x80226040
 */
void Mtx4x4ProjectPoint(Vec3f *out, Vec3f *in, f32 m[4][4]);

/**
 * Transform a point by a matrix into four components.
 *
 * @param out The x, y, z, and w of the transformed point.
 * @param in The point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224ddc
 * @ghidraAddress JPN: 0x8022590c
 * @ghidraAddress EUR: 0x8022611c
 */
void Vec3fTransformByMatrix(f32 out[4], Vec3f *in, f32 m[4][4]);

/**
 * Transform a point of short integers by a matrix into four components.
 *
 * @param out The x, y, z, and w of the transformed point.
 * @param in The x, y, and z of the point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224ea8
 */
void Vec3sTransformToVec4f(f32 out[4], s16 in[3], f32 m[4][4]);

/**
 * Transform a point by a matrix, divide by w, and keep x and y.
 *
 * @param out The vector that receives the x and y of the transformed point.
 * @param in The point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224f90
 */
void ProjectVec3fToScreen(Vec2f *out, Vec3f *in, f32 m[4][4]);

/**
 * Transform a direction by a matrix without its translation, and divide by w.
 *
 * @param out The vector that receives the transformed direction.
 * @param in The direction transformed.
 * @param m The matrix, applied to a row vector.
 * @ghidraAddress USA: 0x80225038
 * @ghidraAddress JPN: 0x80225b68
 * @ghidraAddress EUR: 0x80226378
 */
void Mtx4x4TransformNoTranslation(Vec3f *out, Vec3f *in, f32 m[4][4]);

/**
 * Transform a direction by the 3 by 3 part of a matrix.
 *
 * @param out The vector that receives the transformed direction.
 * @param in The direction transformed.
 * @param m The matrix, applied to a row vector.
 * @ghidraAddress USA: 0x802250fc
 * @ghidraAddress JPN: 0x80225c2c
 * @ghidraAddress EUR: 0x8022643c
 */
void Mtx3x3TransformVec(Vec3f *out, Vec3f *in, f32 m[4][4]);

/**
 * Multiply two matrices. The result may replace either of them.
 *
 * @param out The matrix that receives a * b.
 * @param a The matrix on the left.
 * @param b The matrix on the right.
 * @ghidraAddress USA: 0x80225180
 * @ghidraAddress JPN: 0x80225cb0
 * @ghidraAddress EUR: 0x802264c0
 */
void Mtx4x4Multiply(f32 out[4][4], f32 a[4][4], f32 b[4][4]);

/**
 * Transpose a matrix but its diagonal, which it leaves as it is in the result.
 *
 * @param out The matrix that receives the transpose.
 * @param in The matrix transposed.
 * @ghidraAddress USA: 0x802252ec
 */
void Mtx4fTranspose(f32 out[4][4], f32 in[4][4]);

/**
 * Copy a matrix.
 *
 * @param out The matrix that receives the copy.
 * @param in The matrix copied.
 * @ghidraAddress USA: 0x80225350
 * @ghidraAddress JPN: 0x80225e80
 * @ghidraAddress EUR: 0x80226690
 */
void CopyMatrix4x4(f32 out[4][4], f32 in[4][4]);

/**
 * Invert an affine matrix. A singular matrix is reported on the debug display and left alone.
 *
 * @param out The matrix that receives the inverse.
 * @param in The matrix inverted.
 * @return 1 once inverted, 0 for a singular matrix.
 * @ghidraAddress USA: 0x80225370
 * @ghidraAddress JPN: 0x80225ea0
 * @ghidraAddress EUR: 0x802266b0
 */
s32 MtxInvert4x4(f32 out[4][4], f32 in[4][4]);

/**
 * Set the last column of a matrix to that of an affine transform: 0, 0, 0 and 1.
 *
 * @param m The matrix completed in place.
 * @ghidraAddress USA: 0x8021eb30
 * @ghidraAddress JPN: 0x8021f620
 * @ghidraAddress EUR: 0x8021fe30
 */
void FinishFrameMatrix(f32 m[4][4]);

/**
 * The angle of a point counterclockwise from the x axis, from 0 to two pi, found by a search of
 * the sine. Unlike atan2() of the C library, it takes x first.
 *
 * @param x The x of the point.
 * @param y The y of the point.
 * @return The angle in radians, or 0 for the origin.
 * @ghidraAddress USA: 0x8022576c
 * @ghidraAddress JPN: 0x8022629c
 * @ghidraAddress EUR: 0x80226aac
 */
f32 Atan2f(f32 x, f32 y);

/**
 * Whether a point lies within a triangle, on the side of each edge the normal gives.
 *
 * @param point The point tested.
 * @param v0 The first corner of the triangle.
 * @param v1 The second corner of the triangle.
 * @param v2 The third corner of the triangle.
 * @param normal The normal of the triangle.
 * @return TRUE if the point lies within the triangle, FALSE otherwise.
 * @ghidraAddress USA: 0x8022591c
 * @ghidraAddress JPN: 0x8022644c
 * @ghidraAddress EUR: 0x80226c5c
 */
s32 PointInTriangle3D(Vec3f *point, Vec3f *v0, Vec3f *v1, Vec3f *v2, Vec3f *normal);

/**
 * PointInTriangle3D() with a tolerance for each edge.
 *
 * @param point The point tested.
 * @param v0 The first corner of the triangle.
 * @param v1 The second corner of the triangle.
 * @param v2 The third corner of the triangle.
 * @param normal The normal of the triangle.
 * @param tolerance The least value of the test against each edge for the point to pass it.
 * @return TRUE if the point passes the test against every edge, FALSE otherwise.
 * @ghidraAddress USA: 0x80225a40
 * @ghidraAddress JPN: 0x80226570
 * @ghidraAddress EUR: 0x80226d80
 */
s32 IsPointInsideTriangle(
    Vec3f *point, Vec3f *v0, Vec3f *v1, Vec3f *v2, Vec3f *normal, f32 tolerance);

/**
 * Whether two segments of the plane meet.
 *
 * @param p1 The first end of the first segment.
 * @param p2 The second end of the first segment.
 * @param q1 The first end of the second segment.
 * @param q2 The second end of the second segment.
 * @return 0 if they do not, 2 if they lie along one line, and 1 if they cross.
 * @ghidraAddress USA: 0x80225b64
 * @ghidraAddress JPN: 0x80226694
 * @ghidraAddress EUR: 0x80226ea4
 */
s32 TestLineSegmentIntersection(Vec2f *p1, Vec2f *p2, Vec2f *q1, Vec2f *q2);

/**
 * Whether two points lie across the line through two others, seen from above: only x and y are
 * read.
 *
 * @param a The first point of the line.
 * @param b The second point of the line.
 * @param p The first point tested.
 * @param q The second point tested.
 * @return 0 if they lie strictly on one side, 2 if both lie on the line, and 1 otherwise.
 * @ghidraAddress USA: 0x80225e1c
 * @ghidraAddress JPN: 0x8022694c
 * @ghidraAddress EUR: 0x8022715c
 */
s32 SegmentCrossesLine2D(Vec3f *a, Vec3f *b, Vec3f *p, Vec3f *q);

/**
 * Set the seed of the random numbers.
 *
 * @param seed The new seed.
 * @ghidraAddress USA: 0x80225ed8
 * @ghidraAddress JPN: 0x80226a08
 * @ghidraAddress EUR: 0x80227218
 */
void SetRngSeed(u32 seed);

/**
 * Advance the random numbers by the multiplier 16807 and keep 27 bits.
 *
 * @return The new seed, from 0 to 0x7ffffff.
 * @ghidraAddress USA: 0x80225ee4
 * @ghidraAddress JPN: 0x80226a14
 * @ghidraAddress EUR: 0x80227224
 */
u32 AdvanceRng(void);

#endif

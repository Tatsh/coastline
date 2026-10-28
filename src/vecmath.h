#ifndef COASTLINE_VECMATH_H
#define COASTLINE_VECMATH_H

/**
 * @file
 * The vector and matrix arithmetic of the game, its arctangent and power, the triangle and segment
 * tests, and its random numbers.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#if TGR_PC
/** A point or direction in space, in double precision. */
typedef struct {
    f64 x; /*!< The component along the x axis. */
    f64 y; /*!< The component along the y axis. */
    f64 z; /*!< The component along the z axis. */
} Vec3d;
#endif

/**
 * @ghidraAddress USA: 0x8028b790
 * @ghidraAddress EUR: 0x8028da40
 * @ghidraAddress JPN: 0x8028d470
 * @ghidraAddress BRD3D: 0x10a9bfd0
 * @ghidraAddress BRGlide: 0x10ac3060
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
 * @ghidraAddress EUR: 0x80225620
 * @ghidraAddress JPN: 0x80224e10
 */
f32 Powf(f32 base, f32 exponent);

/**
 * Take the cross product of two vectors.
 *
 * @param out The vector that receives the cross product of a and b.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x8022439c
 * @ghidraAddress EUR: 0x802256dc
 * @ghidraAddress JPN: 0x80224ecc
 * @ghidraAddress BRD3D: 0x1003ac30
 * @ghidraAddress BRGlide: 0x100342b0
 */
void Vec3fCross(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Take the dot product of two vectors.
 *
 * @param a The first vector.
 * @param b The second vector.
 * @return The dot product of the vectors.
 * @ghidraAddress USA: 0x80224404
 * @ghidraAddress EUR: 0x80225744
 * @ghidraAddress JPN: 0x80224f34
 * @ghidraAddress BRD3D: 0x1003ac90
 * @ghidraAddress BRGlide: 0x10034310
 */
#if TGR_PC
long double Vec3fDot(const Vec3f *a, const Vec3f *b);
#else
f32 Vec3fDot(const Vec3f *a, const Vec3f *b);
#endif

/**
 * Negate a vector.
 *
 * @param out The vector that receives -v.
 * @param v The vector negated.
 * @ghidraAddress USA: 0x80224434
 * @ghidraAddress EUR: 0x80225774
 * @ghidraAddress JPN: 0x80224f64
 * @ghidraAddress BRD3D: 0x1003acc0
 * @ghidraAddress BRGlide: 0x10034340
 */
void Vec3fNegate(Vec3f *out, Vec3f *v);

/**
 * Negate a vector in place.
 *
 * @param v The vector negated.
 * @ghidraAddress USA: 0x8022445c
 * @ghidraAddress EUR: 0x8022579c
 * @ghidraAddress JPN: 0x80224f8c
 */
void NegateVectorInPlace(Vec3f *v);

/**
 * Multiply two vectors component by component.
 *
 * @param out The vector that receives the product.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x80224484
 * @ghidraAddress EUR: 0x802257c4
 * @ghidraAddress JPN: 0x80224fb4
 */
void Vec2fMul(Vec2f *out, Vec2f *a, Vec2f *b);

/**
 * Multiply a vector by a number.
 *
 * @param out The vector that receives v * scale.
 * @param v The vector scaled.
 * @param scale The factor.
 * @ghidraAddress USA: 0x802244a8
 * @ghidraAddress EUR: 0x802257e8
 * @ghidraAddress JPN: 0x80224fd8
 */
void Vec2fScale(Vec2f *out, Vec2f *v, f32 scale);

/**
 * Multiply two vectors component by component.
 *
 * @param out The vector that receives the product.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x802244c8
 * @ghidraAddress EUR: 0x80225808
 * @ghidraAddress JPN: 0x80224ff8
 */
void Vec3fComponentProduct(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Multiply a vector by a number.
 *
 * @param out The vector that receives v * scale.
 * @param v The vector scaled.
 * @param scale The factor.
 * @ghidraAddress USA: 0x802244fc
 * @ghidraAddress EUR: 0x8022583c
 * @ghidraAddress JPN: 0x8022502c
 * @ghidraAddress BRD3D: 0x1003ace0
 * @ghidraAddress BRGlide: 0x10034360
 */
void Vec3fScale(Vec3f *out, Vec3f *v, f32 scale);

/**
 * Multiply a vector by a number in place.
 *
 * @param v The vector scaled.
 * @param scale The factor.
 * @ghidraAddress USA: 0x80224528
 * @ghidraAddress EUR: 0x80225868
 * @ghidraAddress JPN: 0x80225058
 * @ghidraAddress BRD3D: 0x1003ad10
 * @ghidraAddress BRGlide: 0x10034390
 */
void Vec3fScaleInPlace(Vec3f *v, f32 scale);

/**
 * Multiply a vector by the reciprocal of a number.
 *
 * @param out The vector that receives v / divisor.
 * @param v The vector divided.
 * @param divisor The number the vector is divided by.
 * @ghidraAddress USA: 0x8022455c
 * @ghidraAddress EUR: 0x8022589c
 * @ghidraAddress JPN: 0x8022508c
 * @ghidraAddress BRD3D: 0x1003ad40
 * @ghidraAddress BRGlide: 0x100343c0
 */
void Vec3fScaleByInverse(Vec3f *out, Vec3f *v, f32 divisor);

/**
 * Divide a vector by a number in place.
 *
 * @param v The vector divided.
 * @param divisor The number the vector is divided by.
 * @ghidraAddress USA: 0x80224594
 * @ghidraAddress EUR: 0x802258d4
 * @ghidraAddress JPN: 0x802250c4
 * @ghidraAddress BRD3D: 0x1003ad70
 * @ghidraAddress BRGlide: 0x100343f0
 */
void Vec3fDivideInPlace(Vec3f *v, f32 divisor);

/**
 * Copy a vector.
 *
 * @param out The vector that receives the copy.
 * @param v The vector copied.
 * @ghidraAddress USA: 0x802245d4
 * @ghidraAddress EUR: 0x80225914
 * @ghidraAddress JPN: 0x80225104
 * @ghidraAddress BRD3D: 0x10035c70
 * @ghidraAddress BRGlide: 0x1002f310
 */
void Vec3fCopy(Vec3f *out, Vec3f *v);

/**
 * The unit vector from one point towards another, or (0, 0, 1) when they coincide.
 *
 * @param out The vector that receives the direction.
 * @param from The point the direction starts at.
 * @param to The point the direction runs towards.
 * @ghidraAddress USA: 0x802245f0
 * @ghidraAddress EUR: 0x80225930
 * @ghidraAddress JPN: 0x80225120
 * @ghidraAddress BRD3D: 0x1003ada0
 * @ghidraAddress BRGlide: 0x10034420
 */
void Vec3fDirection(Vec3f *out, Vec3f *from, Vec3f *to);

/**
 * The unit vector along a vector, or (0, 0, 1) for the zero vector.
 *
 * @param out The vector that receives the unit vector.
 * @param v The vector normalised.
 * @ghidraAddress USA: 0x802246bc
 * @ghidraAddress EUR: 0x802259fc
 * @ghidraAddress JPN: 0x802251ec
 */
void Vec3fNormalizeSafe(Vec3f *out, Vec3f *v);

/**
 * Make a vector a unit vector, or (0, 0, 1) if it is the zero vector. Windows also uses that
 * direction when the length is unordered.
 *
 * @param v The vector normalised in place.
 * @ghidraAddress USA: 0x80224760
 * @ghidraAddress EUR: 0x80225aa0
 * @ghidraAddress JPN: 0x80225290
 * @ghidraAddress BRD3D: 0x1003ae50
 * @ghidraAddress BRGlide: 0x100344d0
 */
void Vec3fNormalize(Vec3f *v);

/**
 * Subtract one vector from another.
 *
 * @param out The vector that receives a - b.
 * @param a The vector subtracted from.
 * @param b The vector subtracted.
 * @ghidraAddress USA: 0x80224808
 * @ghidraAddress EUR: 0x80225b48
 * @ghidraAddress JPN: 0x80225338
 * @ghidraAddress BRD3D: 0x1003aee0
 * @ghidraAddress BRGlide: 0x10034560
 */
void Vec3fSubtract(Vec3f *out, const Vec3f *a, const Vec3f *b);

/**
 * Subtract one vector from another in place.
 *
 * @param v The vector subtracted from, which receives v - b.
 * @param b The vector subtracted.
 * @ghidraAddress USA: 0x8022483c
 * @ghidraAddress EUR: 0x80225b7c
 * @ghidraAddress JPN: 0x8022536c
 * @ghidraAddress BRD3D: 0x1003af10
 * @ghidraAddress BRGlide: 0x10034590
 */
void Vec3fSubtractInPlace(Vec3f *v, Vec3f *b);

/**
 * Add two vectors.
 *
 * @param out The vector that receives a + b.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x80224870
 * @ghidraAddress EUR: 0x80225bb0
 * @ghidraAddress JPN: 0x802253a0
 */
void Vec2fAdd(Vec2f *out, Vec2f *a, Vec2f *b);

/**
 * Add two vectors.
 *
 * @param out The vector that receives a + b.
 * @param a The first vector.
 * @param b The second vector.
 * @ghidraAddress USA: 0x80224894
 * @ghidraAddress EUR: 0x80225bd4
 * @ghidraAddress JPN: 0x802253c4
 * @ghidraAddress BRD3D: 0x1003af40
 * @ghidraAddress BRGlide: 0x100345c0
 */
void Vec3fAdd(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Add one vector to another in place.
 *
 * @param v The vector added to, which receives v + b.
 * @param b The vector added.
 * @ghidraAddress USA: 0x802248c8
 * @ghidraAddress EUR: 0x80225c08
 * @ghidraAddress JPN: 0x802253f8
 * @ghidraAddress BRD3D: 0x1003af70
 * @ghidraAddress BRGlide: 0x100345f0
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
 * @ghidraAddress EUR: 0x80225c3c
 * @ghidraAddress JPN: 0x8022542c
 * @ghidraAddress BRD3D: 0x1003afa0
 * @ghidraAddress BRGlide: 0x10034620
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
 * @ghidraAddress EUR: 0x80225c8c
 * @ghidraAddress JPN: 0x8022547c
 * @ghidraAddress BRD3D: 0x1003afe0
 * @ghidraAddress BRGlide: 0x10034660
 */
void Vec3fMulAdd(Vec3f *out, Vec3f *a, Vec3f *b, f32 scale);

/**
 * Add a scaled vector to another in place.
 *
 * @param v The vector added to, which receives v + b * scale.
 * @param b The vector scaled and added.
 * @param scale The factor for b.
 * @ghidraAddress USA: 0x80224990
 * @ghidraAddress EUR: 0x80225cd0
 * @ghidraAddress JPN: 0x802254c0
 * @ghidraAddress BRD3D: 0x1003b020
 * @ghidraAddress BRGlide: 0x100346a0
 */
void Vec3fMulAddInPlace(Vec3f *v, Vec3f *b, f32 scale);

/**
 * The point halfway between two points.
 *
 * @param out The vector that receives the midpoint.
 * @param a The first point.
 * @param b The second point.
 * @ghidraAddress USA: 0x802249d4
 * @ghidraAddress EUR: 0x80225d14
 * @ghidraAddress JPN: 0x80225504
 * @ghidraAddress BRD3D: 0x1003b050
 * @ghidraAddress BRGlide: 0x100346d0
 */
void Vec3fMidpoint(Vec3f *out, Vec3f *a, Vec3f *b);

/**
 * Set a vector to zero.
 *
 * @param v The vector cleared.
 * @ghidraAddress USA: 0x80224a1c
 * @ghidraAddress EUR: 0x80225d5c
 * @ghidraAddress JPN: 0x8022554c
 * @ghidraAddress BRD3D: 0x1003b090
 * @ghidraAddress BRGlide: 0x10034710
 */
void Vec3fClear(Vec3f *v);

/**
 * The distance between two points of the plane.
 *
 * @param a The first point.
 * @param b The second point.
 * @return The distance between the points.
 * @ghidraAddress USA: 0x80224a34
 * @ghidraAddress EUR: 0x80225d74
 * @ghidraAddress JPN: 0x80225564
 * @ghidraAddress BRD3D: 0x1003b0a0
 * @ghidraAddress BRGlide: 0x10034720
 */
#if TGR_PC
long double Vec2fDistance(Vec2f *a, Vec2f *b);
#else
f32 Vec2fDistance(Vec2f *a, Vec2f *b);
#endif

#if TGR_PC
/**
 * The distance between two points, returned in extended precision.
 *
 * @ghidraAddress BRD3D: 0x1003b0e0
 * @ghidraAddress BRGlide: 0x10034760
 */
long double Vec3fDistance(Vec3f *a, Vec3f *b);
#else
/**
 * The distance between two points.
 *
 * @param a The first point.
 * @param b The second point.
 * @return The distance between the points.
 * @ghidraAddress USA: 0x80224a78
 * @ghidraAddress EUR: 0x80225db8
 * @ghidraAddress JPN: 0x802255a8
 */
f32 Vec3fDistance(Vec3f *a, Vec3f *b);
#endif /* TGR_PC */

/**
 * The square of the distance between two points.
 *
 * @param a The first point.
 * @param b The second point.
 * @return The squared distance between the points.
 * @ghidraAddress USA: 0x80224acc
 * @ghidraAddress EUR: 0x80225e0c
 * @ghidraAddress JPN: 0x802255fc
 * @ghidraAddress BRD3D: 0x1003b130
 * @ghidraAddress BRGlide: 0x100347b0
 */
#if TGR_PC
long double Vec3fDistanceSquared(Vec3f *a, Vec3f *b);
#else
f32 Vec3fDistanceSquared(Vec3f *a, Vec3f *b);
#endif

/**
 * The length of a vector.
 *
 * @param v The vector measured.
 * @return The length of the vector.
 * @ghidraAddress USA: 0x80224b08
 * @ghidraAddress EUR: 0x80225e48
 * @ghidraAddress JPN: 0x80225638
 * @ghidraAddress BRD3D: 0x1003b170
 * @ghidraAddress BRGlide: 0x100347f0
 */
#if TGR_PC
long double Vec3fLength(Vec3f *v);
#else
f32 Vec3fLength(Vec3f *v);
#endif

/**
 * The length of a vector of the plane.
 *
 * @param v The vector measured.
 * @return The length of the vector.
 * @ghidraAddress USA: 0x80224b48
 * @ghidraAddress EUR: 0x80225e88
 * @ghidraAddress JPN: 0x80225678
 * @ghidraAddress BRD3D: 0x1003b1c0
 * @ghidraAddress BRGlide: 0x10034840
 */
#if TGR_PC
long double Vec2fLength(Vec2f *v);
#else
f32 Vec2fLength(Vec2f *v);
#endif

/**
 * Rotate a vector by an angle about a unit axis, clockwise as seen from the tip of the axis.
 *
 * @param v The vector rotated in place.
 * @param angle The angle of the rotation, in radians.
 * @param axis The unit axis of the rotation.
 * @ghidraAddress USA: 0x80224b7c
 * @ghidraAddress EUR: 0x80225ebc
 * @ghidraAddress JPN: 0x802256ac
 */
void Vec3fRotateAroundAxis(Vec3f *v, f32 angle, Vec3f *axis);

/**
 * Transform a point by a matrix and divide by w.
 *
 * @param out The vector that receives the transformed point.
 * @param in The point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224d00
 * @ghidraAddress EUR: 0x80226040
 * @ghidraAddress JPN: 0x80225830
 * @ghidraAddress BRD3D: 0x1003b1f0
 * @ghidraAddress BRGlide: 0x10034870
 */
void Mtx4x4ProjectPoint(Vec3f *out, Vec3f *in, f32 m[4][4]);

/**
 * Transform a point by a matrix into four components.
 *
 * @param out The x, y, z, and w of the transformed point.
 * @param in The point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224ddc
 * @ghidraAddress EUR: 0x8022611c
 * @ghidraAddress JPN: 0x8022590c
 * @ghidraAddress BRD3D: 0x1003b2a0
 * @ghidraAddress BRGlide: 0x10034920
 */
void Vec3fTransformByMatrix(f32 out[4], const Vec3f *in, f32 m[4][4]);

/**
 * Transform a point of short integers by a matrix into four components.
 *
 * @param out The x, y, z, and w of the transformed point.
 * @param in The x, y, and z of the point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224ea8
 * @ghidraAddress EUR: 0x802261e8
 * @ghidraAddress JPN: 0x802259d8
 */
void Vec3sTransformToVec4f(f32 out[4], s16 in[3], f32 m[4][4]);

/**
 * Transform a point by a matrix, divide by w, and retain x and y.
 *
 * @param out The vector that receives the x and y of the transformed point.
 * @param in The point transformed.
 * @param m The matrix, applied to a row vector with its translation in the last row.
 * @ghidraAddress USA: 0x80224f90
 * @ghidraAddress EUR: 0x802262d0
 * @ghidraAddress JPN: 0x80225ac0
 */
void ProjectVec3fToScreen(Vec2f *out, Vec3f *in, f32 m[4][4]);

/**
 * Transform a direction by a matrix without its translation, and divide by w.
 *
 * @param out The vector that receives the transformed direction.
 * @param in The direction transformed.
 * @param m The matrix, applied to a row vector.
 * @ghidraAddress USA: 0x80225038
 * @ghidraAddress EUR: 0x80226378
 * @ghidraAddress JPN: 0x80225b68
 * @ghidraAddress BRD3D: 0x1003b340
 * @ghidraAddress BRGlide: 0x100349c0
 */
void Mtx4x4TransformNoTranslation(Vec3f *out, Vec3f *in, f32 m[4][4]);

/**
 * Transform a direction by the 3 by 3 part of a matrix.
 *
 * @param out The vector that receives the transformed direction.
 * @param in The direction transformed.
 * @param m The matrix, applied to a row vector.
 * @ghidraAddress USA: 0x802250fc
 * @ghidraAddress EUR: 0x8022643c
 * @ghidraAddress JPN: 0x80225c2c
 * @ghidraAddress BRD3D: 0x1003b3f0
 * @ghidraAddress BRGlide: 0x10034a70
 */
void Mtx3x3TransformVec(Vec3f *out, Vec3f *in, f32 m[4][4]);

/**
 * Multiply two matrices. The result may replace either of them.
 *
 * @param out The matrix that receives a * b.
 * @param a The matrix on the left.
 * @param b The matrix on the right.
 * @ghidraAddress USA: 0x80225180
 * @ghidraAddress EUR: 0x802264c0
 * @ghidraAddress JPN: 0x80225cb0
 * @ghidraAddress BRD3D: 0x1003b470
 * @ghidraAddress BRGlide: 0x10034af0
 */
void Mtx4x4Multiply(f32 out[4][4], f32 a[4][4], f32 b[4][4]);

/**
 * Transpose a matrix but its diagonal, which is copied unchanged into the result.
 *
 * @param out The matrix that receives the transpose.
 * @param in The matrix transposed.
 * @ghidraAddress USA: 0x802252ec
 * @ghidraAddress EUR: 0x8022662c
 * @ghidraAddress JPN: 0x80225e1c
 */
void Mtx4fTranspose(f32 out[4][4], f32 in[4][4]);

/**
 * Copy a matrix.
 *
 * @param out The matrix that receives the copy.
 * @param in The matrix copied.
 * @ghidraAddress USA: 0x80225350
 * @ghidraAddress EUR: 0x80226690
 * @ghidraAddress JPN: 0x80225e80
 * @ghidraAddress BRD3D: 0x100307a0
 * @ghidraAddress BRGlide: 0x10029e50
 */
#if TGR_PC
void CopyMatrix4x4(f32 in[4][4], f32 out[4][4]);
#else
void CopyMatrix4x4(f32 out[4][4], f32 in[4][4]);
#endif

/**
 * Invert an affine matrix. On the N64 a singular matrix is reported on the debug display and
 * `out` is not written. On the PC a singular matrix inverts to the identity.
 *
 * @param out The matrix that receives the inverse.
 * @param in The matrix inverted.
 * @return 1 once inverted. On the N64, 0 for a singular matrix.
 * @ghidraAddress USA: 0x80225370
 * @ghidraAddress EUR: 0x802266b0
 * @ghidraAddress JPN: 0x80225ea0
 * @ghidraAddress BRD3D: 0x1003b4f0
 * @ghidraAddress BRGlide: 0x10034b70
 */
s32 MtxInvert4x4(f32 out[4][4], f32 in[4][4]);

/**
 * Set the last column of a matrix to that of an affine transform: 0, 0, 0 and 1.
 *
 * @param m The matrix completed in place.
 * @ghidraAddress USA: 0x8021eb30
 * @ghidraAddress EUR: 0x8021fe30
 * @ghidraAddress JPN: 0x8021f620
 * @ghidraAddress BRD3D: 0x10075340
 * @ghidraAddress BRGlide: 0x1006e5a0
 */
#if TGR_PC
void __thiscall FinishFrameMatrix(f32 m[4][4]);
#else
void FinishFrameMatrix(f32 m[4][4]);
#endif

/**
 * The angle of a point counterclockwise from the x axis, from 0 to two pi, found by a search of
 * the sine. Unlike atan2() of the C library, it takes x first.
 *
 * @param x The x of the point.
 * @param y The y of the point.
 * @return The angle in radians, or 0 for the origin.
 * @ghidraAddress USA: 0x8022576c
 * @ghidraAddress EUR: 0x80226aac
 * @ghidraAddress JPN: 0x8022629c
 * @ghidraAddress BRD3D: 0x1003b7b0
 * @ghidraAddress BRGlide: 0x10034e30
 */
#if TGR_PC
long double Atan2f(f32 x, f32 y);
#else
f32 Atan2f(f32 x, f32 y);
#endif

/**
 * Whether a point lies within a triangle, on the side of each edge the normal gives.
 *
 * @param point The point tested.
 * @param v0 The first corner of the triangle.
 * @param v1 The second corner of the triangle.
 * @param v2 The third corner of the triangle.
 * @param normal The normal of the triangle.
 * @return #TRUE if the point lies within the triangle, #FALSE otherwise.
 * @ghidraAddress USA: 0x8022591c
 * @ghidraAddress EUR: 0x80226c5c
 * @ghidraAddress JPN: 0x8022644c
 * @ghidraAddress BRD3D: 0x1003b940
 * @ghidraAddress BRGlide: 0x10034fc0
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
 * @return #TRUE if the point passes the test against every edge, #FALSE otherwise.
 * @ghidraAddress USA: 0x80225a40
 * @ghidraAddress EUR: 0x80226d80
 * @ghidraAddress JPN: 0x80226570
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
 * @ghidraAddress EUR: 0x80226ea4
 * @ghidraAddress JPN: 0x80226694
 * @ghidraAddress BRD3D: 0x1003ba70
 * @ghidraAddress BRGlide: 0x100350f0
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
 * @ghidraAddress EUR: 0x8022715c
 * @ghidraAddress JPN: 0x8022694c
 * @ghidraAddress BRD3D: 0x1003bc90
 * @ghidraAddress BRGlide: 0x10035310
 */
s32 SegmentCrossesLine2D(const Vec3f *a, const Vec3f *b, const Vec3f *p, const Vec3f *q);

/**
 * Set the seed of the random numbers.
 *
 * @param seed The new seed.
 * @ghidraAddress USA: 0x80225ed8
 * @ghidraAddress EUR: 0x80227218
 * @ghidraAddress JPN: 0x80226a08
 * @ghidraAddress BRD3D: 0x1003bd40
 * @ghidraAddress BRGlide: 0x100353c0
 */
void SetRngSeed(u32 seed);

/**
 * Advance the random numbers by the multiplier 16807 and retain 27 bits.
 *
 * @return The new seed, from 0 to 0x7ffffff.
 * @ghidraAddress USA: 0x80225ee4
 * @ghidraAddress EUR: 0x80227224
 * @ghidraAddress JPN: 0x80226a14
 * @ghidraAddress BRD3D: 0x1003bd50
 * @ghidraAddress BRGlide: 0x100353d0
 */
u32 AdvanceRng(void);

#if TGR_PC
/**
 * The square of the length of a vector in double precision.
 *
 * @param v The vector.
 * @return The square of its length.
 * @ghidraAddress BRD3D: 0x100305b0
 * @ghidraAddress BRGlide: 0x1001dc40
 */
long double Vec3dLengthSquared(Vec3d *v);

/**
 * The length of a vector in double precision.
 *
 * @param v The vector.
 * @return Its length.
 * @ghidraAddress BRD3D: 0x100305f0
 * @ghidraAddress BRGlide: 0x1001dc80
 */
long double Vec3dLength(Vec3d *v);

/**
 * Scale a vector in double precision to a length of 1 in place, unless its length is 0.
 *
 * @param v The vector.
 * @return The vector.
 * @ghidraAddress BRD3D: 0x10030600
 * @ghidraAddress BRGlide: 0x1001dc90
 */
Vec3d *Vec3dNormalize(Vec3d *v);

/**
 * The dot product of two vectors in double precision.
 *
 * @param a The first vector.
 * @param b The second vector.
 * @return The dot product.
 * @ghidraAddress BRD3D: 0x10030640
 * @ghidraAddress BRGlide: 0x1001dcd0
 */
long double Vec3dDot(Vec3d *a, Vec3d *b);

/**
 * The cross product of two vectors in double precision.
 *
 * @param a The first vector.
 * @param b The second vector.
 * @param out The vector that receives a cross b.
 * @ghidraAddress BRD3D: 0x10030670
 * @ghidraAddress BRGlide: 0x1001dd00
 */
void Vec3dCross(Vec3d *a, Vec3d *b, Vec3d *out);
#endif /* TGR_PC */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VECMATH_H */

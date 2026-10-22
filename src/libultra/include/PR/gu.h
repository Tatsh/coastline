#ifndef PR_GU_H
#define PR_GU_H

/**
 * @file
 * The graphics utility routines for matrices and view transforms, and the trigonometry they use.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

/**
 * Convert a floating-point matrix to the fixed-point form of the RSP.
 *
 * @param mf The source matrix, as f32[4][4] in row-major order.
 * @param m Receives the matrix in s15.16 fixed point, integer halves first and fractions after.
 * @ghidraAddress USA: 0x80260ed0
 * @ghidraAddress EUR: 0x80262360
 * @ghidraAddress JPN: 0x80261e10
 */
void guMtxF2L(float mf[4][4], Mtx *m);

/**
 * Convert a fixed-point matrix to floating point.
 *
 * @param mf Receives the matrix as f32[4][4].
 * @param m The source matrix in s15.16 fixed point.
 * @ghidraAddress USA: 0x80261088
 * @ghidraAddress EUR: 0x80262518
 * @ghidraAddress JPN: 0x80261fc8
 */
void guMtxL2F(float mf[4][4], Mtx *m);

/**
 * Set a floating-point matrix to the identity.
 *
 * @param mf The f32[4][4] matrix set.
 * @ghidraAddress USA: 0x80260fd0
 * @ghidraAddress EUR: 0x80262460
 * @ghidraAddress JPN: 0x80261f10
 */
void guMtxIdentF(float mf[4][4]);

/**
 * Set a fixed-point matrix to the identity.
 *
 * @param m The fixed-point matrix set.
 * @ghidraAddress USA: 0x80261058
 * @ghidraAddress EUR: 0x802624e8
 * @ghidraAddress JPN: 0x80261f98
 */
void guMtxIdent(Mtx *m);

/**
 * Multiply two floating-point matrices. The result may alias either operand.
 *
 * @param mf The left operand, as f32[4][4].
 * @param nf The right operand, as f32[4][4].
 * @param res Receives the product mf times nf.
 * @ghidraAddress USA: 0x80260940
 * @ghidraAddress EUR: 0x80261e40
 * @ghidraAddress JPN: 0x802618b0
 */
void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]);

/**
 * Multiply two fixed-point matrices.
 *
 * @param m The left operand, in s15.16 fixed point.
 * @param n The right operand, in s15.16 fixed point.
 * @param res Receives the product m times n.
 * @ghidraAddress USA: 0x80267470
 * @ghidraAddress EUR: 0x80268c60
 * @ghidraAddress JPN: 0x80268710
 */
void guMtxCatL(Mtx *m, Mtx *n, Mtx *res);

/**
 * Transform a point by a floating-point matrix.
 *
 * @param mf The transform, as f32[4][4].
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @param z The Z coordinate of the point.
 * @param ox Receives the transformed X coordinate.
 * @param oy Receives the transformed Y coordinate.
 * @param oz Receives the transformed Z coordinate.
 * @ghidraAddress USA: 0x802608a0
 * @ghidraAddress EUR: 0x80261da0
 * @ghidraAddress JPN: 0x80261810
 */
void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox, float *oy, float *oz);

/**
 * Transform a point by a fixed-point matrix.
 *
 * @param m The transform, in s15.16 fixed point.
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @param z The Z coordinate of the point.
 * @param ox Receives the transformed X coordinate.
 * @param oy Receives the transformed Y coordinate.
 * @param oz Receives the transformed Z coordinate.
 * @ghidraAddress USA: 0x80267410
 * @ghidraAddress EUR: 0x80268c00
 * @ghidraAddress JPN: 0x802686b0
 */
void guMtxXFML(Mtx *m, float x, float y, float z, float *ox, float *oy, float *oz);

/**
 * Build a floating-point scale matrix.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param x The scale along X.
 * @param y The scale along Y.
 * @param z The scale along Z.
 * @ghidraAddress USA: 0x80260800
 * @ghidraAddress EUR: 0x80261d00
 * @ghidraAddress JPN: 0x80261770
 */
void guScaleF(float mf[4][4], float x, float y, float z);

/**
 * Build a fixed-point scale matrix.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param x The scale along X.
 * @param y The scale along Y.
 * @param z The scale along Z.
 * @ghidraAddress USA: 0x80260854
 * @ghidraAddress EUR: 0x80261d54
 * @ghidraAddress JPN: 0x802617c4
 */
void guScale(Mtx *m, float x, float y, float z);

/**
 * Build a floating-point translation matrix.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param x The translation along X.
 * @param y The translation along Y.
 * @param z The translation along Z.
 * @ghidraAddress USA: 0x80260e30
 * @ghidraAddress EUR: 0x802622c0
 * @ghidraAddress JPN: 0x80261d70
 */
void guTranslateF(float mf[4][4], float x, float y, float z);

/**
 * Build a fixed-point translation matrix.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param x The translation along X.
 * @param y The translation along Y.
 * @param z The translation along Z.
 * @ghidraAddress USA: 0x80260e78
 * @ghidraAddress EUR: 0x80262308
 * @ghidraAddress JPN: 0x80261db8
 */
void guTranslate(Mtx *m, float x, float y, float z);

/**
 * Build a floating-point matrix that rotates by a degrees about an axis.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param a The angle of the rotation in degrees.
 * @param x The X component of the axis, normalised before use.
 * @param y The Y component of the axis.
 * @param z The Z component of the axis.
 * @ghidraAddress USA: 0x80260bc0
 * @ghidraAddress EUR: 0x80262050
 * @ghidraAddress JPN: 0x80261b00
 */
void guRotateF(float mf[4][4], float a, float x, float y, float z);

/**
 * Build a fixed-point matrix that rotates by a degrees about an axis.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param a The angle of the rotation in degrees.
 * @param x The X component of the axis, normalised before use.
 * @param y The Y component of the axis.
 * @param z The Z component of the axis.
 * @ghidraAddress USA: 0x80260d54
 * @ghidraAddress EUR: 0x802621e4
 * @ghidraAddress JPN: 0x80261c94
 */
void guRotate(Mtx *m, float a, float x, float y, float z);

/**
 * Scale a vector to unit length in place.
 *
 * @param x The X component, replaced by the normalised value.
 * @param y The Y component, replaced by the normalised value.
 * @param z The Z component, replaced by the normalised value.
 * @ghidraAddress USA: 0x802686c0
 * @ghidraAddress EUR: 0x80269d30
 * @ghidraAddress JPN: 0x802697e0
 */
void guNormalize(float *x, float *y, float *z);

/**
 * Build a floating-point perspective projection.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param perspNorm Receives the factor that normalises the projected W, unless it is NULL.
 * @param fovy The vertical field of view in degrees.
 * @param aspect The ratio of the width of the view to its height.
 * @param near The distance to the near clipping plane.
 * @param far The distance to the far clipping plane.
 * @param scale The factor applied to every element of the matrix.
 * @ghidraAddress USA: 0x80265180
 * @ghidraAddress EUR: 0x80266910
 * @ghidraAddress JPN: 0x80266410
 */
void guPerspectiveF(
    float mf[4][4], u16 *perspNorm, float fovy, float aspect, float near, float far, float scale);

/**
 * Build a fixed-point perspective projection.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param perspNorm Receives the factor that normalises the projected W, unless it is NULL.
 * @param fovy The vertical field of view in degrees.
 * @param aspect The ratio of the width of the view to its height.
 * @param near The distance to the near clipping plane.
 * @param far The distance to the far clipping plane.
 * @param scale The factor applied to every element of the matrix.
 * @ghidraAddress USA: 0x802653b0
 * @ghidraAddress EUR: 0x80266b40
 * @ghidraAddress JPN: 0x80266640
 */
void guPerspective(
    Mtx *m, u16 *perspNorm, float fovy, float aspect, float near, float far, float scale);

/**
 * Build a floating-point viewing matrix from the eye, the point it looks at and the up vector.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param xEye The X coordinate of the eye.
 * @param yEye The Y coordinate of the eye.
 * @param zEye The Z coordinate of the eye.
 * @param xAt The X coordinate of the point looked at.
 * @param yAt The Y coordinate of the point looked at.
 * @param zAt The Z coordinate of the point looked at.
 * @param xUp The X component of the up vector.
 * @param yUp The Y component of the up vector.
 * @param zUp The Z component of the up vector.
 * @ghidraAddress USA: 0x80264e50
 * @ghidraAddress EUR: 0x802665e0
 * @ghidraAddress JPN: 0x802660e0
 */
void guLookAtF(float mf[4][4],
               float xEye,
               float yEye,
               float zEye,
               float xAt,
               float yAt,
               float zAt,
               float xUp,
               float yUp,
               float zUp);

/**
 * Build a fixed-point viewing matrix.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param xEye The X coordinate of the eye.
 * @param yEye The Y coordinate of the eye.
 * @param zEye The Z coordinate of the eye.
 * @param xAt The X coordinate of the point looked at.
 * @param yAt The Y coordinate of the point looked at.
 * @param zAt The Z coordinate of the point looked at.
 * @param xUp The X component of the up vector.
 * @param yUp The Y component of the up vector.
 * @param zUp The Z component of the up vector.
 * @ghidraAddress USA: 0x80265108
 * @ghidraAddress EUR: 0x80266898
 * @ghidraAddress JPN: 0x80266398
 */
void guLookAt(Mtx *m,
              float xEye,
              float yEye,
              float zEye,
              float xAt,
              float yAt,
              float zAt,
              float xUp,
              float yUp,
              float zUp);

/**
 * Build a floating-point viewing matrix and the directions for reflection mapping.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param l Receives the right and up vectors of the view as the two reflection directions.
 * @param xEye The X coordinate of the eye.
 * @param yEye The Y coordinate of the eye.
 * @param zEye The Z coordinate of the eye.
 * @param xAt The X coordinate of the point looked at.
 * @param yAt The Y coordinate of the point looked at.
 * @param zAt The Z coordinate of the point looked at.
 * @param xUp The X component of the up vector.
 * @param yUp The Y component of the up vector.
 * @param zUp The Z component of the up vector.
 * @ghidraAddress USA: 0x80261150
 * @ghidraAddress EUR: 0x802625e0
 * @ghidraAddress JPN: 0x80262080
 */
void guLookAtReflectF(float mf[4][4],
                      LookAt *l,
                      float xEye,
                      float yEye,
                      float zEye,
                      float xAt,
                      float yAt,
                      float zAt,
                      float xUp,
                      float yUp,
                      float zUp);

/**
 * Build a fixed-point viewing matrix and the directions for reflection mapping.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param l Receives the right and up vectors of the view as the two reflection directions.
 * @param xEye The X coordinate of the eye.
 * @param yEye The Y coordinate of the eye.
 * @param zEye The Z coordinate of the eye.
 * @param xAt The X coordinate of the point looked at.
 * @param yAt The Y coordinate of the point looked at.
 * @param zAt The Z coordinate of the point looked at.
 * @param xUp The X component of the up vector.
 * @param yUp The Y component of the up vector.
 * @param zUp The Z component of the up vector.
 * @ghidraAddress USA: 0x80261590
 * @ghidraAddress EUR: 0x80262a20
 * @ghidraAddress JPN: 0x802624c0
 */
void guLookAtReflect(Mtx *m,
                     LookAt *l,
                     float xEye,
                     float yEye,
                     float zEye,
                     float xAt,
                     float yAt,
                     float zAt,
                     float xUp,
                     float yUp,
                     float zUp);

/**
 * Build a floating-point viewing matrix, the reflection directions and the positions of the
 * highlights of two lights on a texture of the given size.
 *
 * @param mf Receives the matrix, as f32[4][4].
 * @param l Receives the right and up vectors of the view as the two reflection directions.
 * @param h Receives the texture coordinates of the two highlights, in units of 1/4 texel.
 * @param xEye The X coordinate of the eye.
 * @param yEye The Y coordinate of the eye.
 * @param zEye The Z coordinate of the eye.
 * @param xAt The X coordinate of the point looked at.
 * @param yAt The Y coordinate of the point looked at.
 * @param zAt The Z coordinate of the point looked at.
 * @param xUp The X component of the up vector.
 * @param yUp The Y component of the up vector.
 * @param zUp The Z component of the up vector.
 * @param xl1 The X component of the direction of the first light.
 * @param yl1 The Y component of the direction of the first light.
 * @param zl1 The Z component of the direction of the first light.
 * @param xl2 The X component of the direction of the second light.
 * @param yl2 The Y component of the direction of the second light.
 * @param zl2 The Z component of the direction of the second light.
 * @param twidth The width of the highlight texture in texels.
 * @param theight The height of the highlight texture in texels.
 * @ghidraAddress USA: 0x80266be0
 * @ghidraAddress EUR: 0x802683d0
 * @ghidraAddress JPN: 0x80267e80
 */
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
                     int theight);

/**
 * Build a fixed-point viewing matrix with reflection directions and highlights.
 *
 * @param m Receives the matrix in s15.16 fixed point.
 * @param l Receives the right and up vectors of the view as the two reflection directions.
 * @param h Receives the texture coordinates of the two highlights, in units of 1/4 texel.
 * @param xEye The X coordinate of the eye.
 * @param yEye The Y coordinate of the eye.
 * @param zEye The Z coordinate of the eye.
 * @param xAt The X coordinate of the point looked at.
 * @param yAt The Y coordinate of the point looked at.
 * @param zAt The Z coordinate of the point looked at.
 * @param xUp The X component of the up vector.
 * @param yUp The Y component of the up vector.
 * @param zUp The Z component of the up vector.
 * @param xl1 The X component of the direction of the first light.
 * @param yl1 The Y component of the direction of the first light.
 * @param zl1 The Z component of the direction of the first light.
 * @param xl2 The X component of the direction of the second light.
 * @param yl2 The Y component of the direction of the second light.
 * @param zl2 The Z component of the direction of the second light.
 * @param twidth The width of the highlight texture in texels.
 * @param theight The height of the highlight texture in texels.
 * @ghidraAddress USA: 0x8026734c
 * @ghidraAddress EUR: 0x80268b3c
 * @ghidraAddress JPN: 0x802685ec
 */
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
                    int theight);

/**
 * Return the sine of an angle in radians.
 *
 * @param x The angle in radians.
 * @return The sine of the angle, from -1 to 1.
 * @ghidraAddress USA: 0x80261610
 * @ghidraAddress EUR: 0x80262aa0
 * @ghidraAddress JPN: 0x80262540
 */
float sinf(float x);

/**
 * Return the cosine of an angle in radians.
 *
 * @param x The angle in radians.
 * @return The cosine of the angle, from -1 to 1.
 * @ghidraAddress USA: 0x802617d0
 * @ghidraAddress EUR: 0x80262c60
 * @ghidraAddress JPN: 0x80262700
 */
float cosf(float x);

/**
 * Return the square root.
 *
 * @param x The value, at least 0.
 * @return The square root of the value.
 * @ghidraAddress USA: 0x80261140
 * @ghidraAddress EUR: 0x802625d0
 * @ghidraAddress JPN: 0x80267c60
 */
float sqrtf(float x);

/**
 * Return the sine of an angle where 0x10000 is a full turn, in s1.15 fixed point.
 *
 * @param x The angle, where 0x10000 is a full turn and the low 4 bits are ignored.
 * @return The sine of the angle from -0x7fff to 0x7fff, where 0x7fff is 1.
 * @ghidraAddress USA: 0x80264420
 * @ghidraAddress EUR: 0x80265ba0
 * @ghidraAddress JPN: 0x80265640
 */
s16 sins(u16 x);

#endif

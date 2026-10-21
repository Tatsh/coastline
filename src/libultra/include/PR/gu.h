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
 * @ghidraAddress USA: 0x80260ed0
 */
void guMtxF2L(float mf[4][4], Mtx *m);

/**
 * Convert a fixed-point matrix to floating point.
 *
 * @ghidraAddress USA: 0x80261088
 */
void guMtxL2F(float mf[4][4], Mtx *m);

/**
 * Set a floating-point matrix to the identity.
 *
 * @ghidraAddress USA: 0x80260fd0
 */
void guMtxIdentF(float mf[4][4]);

/**
 * Set a fixed-point matrix to the identity.
 *
 * @ghidraAddress USA: 0x80261058
 */
void guMtxIdent(Mtx *m);

/**
 * Multiply two floating-point matrices. The result may alias either operand.
 *
 * @ghidraAddress USA: 0x80260940
 */
void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]);

/**
 * Multiply two fixed-point matrices.
 *
 * @ghidraAddress USA: 0x80267470
 */
void guMtxCatL(Mtx *m, Mtx *n, Mtx *res);

/**
 * Transform a point by a floating-point matrix.
 *
 * @ghidraAddress USA: 0x802608a0
 */
void guMtxXFMF(float mf[4][4], float x, float y, float z, float *ox, float *oy, float *oz);

/**
 * Transform a point by a fixed-point matrix.
 *
 * @ghidraAddress USA: 0x80267410
 */
void guMtxXFML(Mtx *m, float x, float y, float z, float *ox, float *oy, float *oz);

/**
 * Build a floating-point scale matrix.
 *
 * @ghidraAddress USA: 0x80260800
 */
void guScaleF(float mf[4][4], float x, float y, float z);

/**
 * Build a fixed-point scale matrix.
 *
 * @ghidraAddress USA: 0x80260854
 */
void guScale(Mtx *m, float x, float y, float z);

/**
 * Build a floating-point translation matrix.
 *
 * @ghidraAddress USA: 0x80260e30
 */
void guTranslateF(float mf[4][4], float x, float y, float z);

/**
 * Build a fixed-point translation matrix.
 *
 * @ghidraAddress USA: 0x80260e78
 */
void guTranslate(Mtx *m, float x, float y, float z);

/**
 * Build a floating-point matrix that rotates by a degrees about an axis.
 *
 * @ghidraAddress USA: 0x80260bc0
 */
void guRotateF(float mf[4][4], float a, float x, float y, float z);

/**
 * Build a fixed-point matrix that rotates by a degrees about an axis.
 *
 * @ghidraAddress USA: 0x80260d54
 */
void guRotate(Mtx *m, float a, float x, float y, float z);

/**
 * Scale a vector to unit length in place.
 *
 * @ghidraAddress USA: 0x802686c0
 */
void guNormalize(float *x, float *y, float *z);

/**
 * Build a floating-point perspective projection.
 *
 * @param perspNorm Receives the factor that normalises the projected W, unless it is NULL.
 * @ghidraAddress USA: 0x80265180
 */
void guPerspectiveF(
    float mf[4][4], u16 *perspNorm, float fovy, float aspect, float near, float far, float scale);

/**
 * Build a fixed-point perspective projection.
 *
 * @ghidraAddress USA: 0x802653b0
 */
void guPerspective(
    Mtx *m, u16 *perspNorm, float fovy, float aspect, float near, float far, float scale);

/**
 * Build a floating-point viewing matrix from the eye, the point it looks at and the up vector.
 *
 * @ghidraAddress USA: 0x80264e50
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
 * @ghidraAddress USA: 0x80265108
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
 * @ghidraAddress USA: 0x80261150
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
 * @ghidraAddress USA: 0x80261590
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
 * @ghidraAddress USA: 0x80266be0
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
 * @ghidraAddress USA: 0x8026734c
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
 * @ghidraAddress USA: 0x80261610
 */
float sinf(float x);

/**
 * Return the cosine of an angle in radians.
 *
 * @ghidraAddress USA: 0x802617d0
 */
float cosf(float x);

/**
 * Return the square root.
 *
 * @ghidraAddress USA: 0x80261140
 */
float sqrtf(float x);

/**
 * Return the sine of an angle where 0x10000 is a full turn, in s1.15 fixed point.
 *
 * @ghidraAddress USA: 0x80264420
 */
s16 sins(u16 x);

#endif

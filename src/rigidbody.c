#include "rigidbody.h"

#if TGR_PLATFORM == TGR_PLATFORM_N64
#include <PR/gu.h>
#include <PR/os.h>
#else
#include <math.h>
#include <stddef.h>
#endif

/* The inertia of a box about an axis is this share of its mass times the squares of the sides. */
#define BOX_INERTIA_SHARE 0.0833333358f

/* The steering angle a body starts with, about 10 degrees. */
#define DEFAULT_STEER_ANGLE 0.174f

#if TGR_PC
/* The libraries do not round a value on the x87 stack until the sum it joins is stored. */
#define BODY_INTERMEDIATE(value) ((long double)(value))
#define BODY_PRODUCT(a, b) ((long double)(a) * (b))
/* The libraries round an accumulated product only when the sum is stored. */
#define BODY_ACCUMULATE(sum, a, b) ((sum) = (f32)(BODY_PRODUCT(a, b) + (sum)))
#else
#define BODY_INTERMEDIATE(value) (value)
#define BODY_PRODUCT(a, b) ((a) * (b))
#define BODY_ACCUMULATE(sum, a, b) ((sum) += (a) * (b))
#endif

void StubPhysics1(void *data) {
}

void StubPhysics2(void *data) {
}

void MatrixToQuaternion(f32 m[4][4], Quaternion *q) {
    if (0.0f <= m[0][0]) {
        if (0.0f <= m[1][1] + m[2][2]) {
#if TGR_PC
            q->w = m[0][0] + (BODY_INTERMEDIATE(m[1][1]) + m[2][2]) + 1.0f;
#else
            q->w = 1.0f + m[0][0] + m[1][1] + m[2][2];
#endif
            q->x = m[1][2] - m[2][1];
            q->y = m[2][0] - m[0][2];
            q->z = m[0][1] - m[1][0];
        } else {
            q->w = m[1][2] - m[2][1];
            q->x = BODY_INTERMEDIATE(1.0f) + m[0][0] - m[1][1] - m[2][2];
            q->y = m[0][1] + m[1][0];
            q->z = m[2][0] + m[0][2];
        }
#if TGR_PC
    } else if (0.0f <= m[1][1] - m[2][2]) {
#else
    } else if (m[2][2] <= m[1][1]) {
#endif
        q->w = m[2][0] - m[0][2];
        q->x = m[0][1] + m[1][0];
        q->y = BODY_INTERMEDIATE(1.0f) - m[0][0] + m[1][1] - m[2][2];
        q->z = m[1][2] + m[2][1];
    } else {
        q->w = m[0][1] - m[1][0];
        q->x = m[2][0] + m[0][2];
        q->y = m[1][2] + m[2][1];
#if TGR_PC
        q->z = m[2][2] - (BODY_INTERMEDIATE(m[0][0]) + m[1][1] - 1.0f);
#else
        q->z = 1.0f - m[0][0] - m[1][1] + m[2][2];
#endif
    }
    QuaternionNormalize(q);
}

void QuaternionMultiply(Quaternion *out, Quaternion *a, Quaternion *b) {
    f32 bw = b->w, bx = b->x, by = b->y, bz = b->z;
    f32 aw = a->w, ax = a->x, ay = a->y, az = a->z;

    out->z =
        BODY_PRODUCT(az, bw) - BODY_PRODUCT(ay, bx) + BODY_PRODUCT(ax, by) + BODY_PRODUCT(aw, bz);
    out->y =
        BODY_PRODUCT(ay, bw) + BODY_PRODUCT(az, bx) + BODY_PRODUCT(aw, by) - BODY_PRODUCT(ax, bz);
    out->x =
        BODY_PRODUCT(ax, bw) + BODY_PRODUCT(aw, bx) - BODY_PRODUCT(az, by) + BODY_PRODUCT(ay, bz);
    out->w =
        BODY_PRODUCT(aw, bw) - BODY_PRODUCT(ax, bx) - BODY_PRODUCT(ay, by) - BODY_PRODUCT(az, bz);
}

void QuaternionNormalize(Quaternion *q) {
#if TGR_PC
    f32 square = BODY_PRODUCT(q->x, q->x) + BODY_PRODUCT(q->y, q->y) + BODY_PRODUCT(q->z, q->z) +
                 BODY_PRODUCT(q->w, q->w);
    /* The sqrtf of the libraries returns the root unrounded. */
    f32 inverse = 1.0f / sqrtl(square);
#else
    f32 inverse = 1.0f / sqrtf(q->z * q->z + (q->w * q->w + q->x * q->x + q->y * q->y));
#endif

    q->w *= inverse;
    q->x *= inverse;
    q->y *= inverse;
    q->z *= inverse;
}

void Vec3fNormalizeFast(Vec3f *v) {
#if TGR_PC
    long double inverse = 1.0f / Vec3fLength(v);
    f32 rounded = (f32)inverse;

    v->x *= rounded;
    v->y = (f32)((long double)v->y * inverse);
    v->z *= rounded;
#else
    f32 inverse = 1.0f / sqrtf(v->z * v->z + (v->x * v->x + v->y * v->y));

    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
#endif /* TGR_PC */
}

void ComputeDeltaQuaternion(RigidBodyState *state) {
    f32 half[3];
    /* The original reads the orientation as an array of w, x, y, and z. */
    f32 *o;

    half[0] = state->angularVelocity.x * 0.5f;
    half[1] = state->angularVelocity.y * 0.5f;
    half[2] = state->angularVelocity.z * 0.5f;
    o = &state->orientation.w;
    state->spin.w =
        BODY_PRODUCT(-half[0], o[1]) - BODY_PRODUCT(half[1], o[2]) - BODY_PRODUCT(half[2], o[3]);
    state->spin.x =
        BODY_PRODUCT(o[0], half[0]) + BODY_PRODUCT(half[1], o[3]) - BODY_PRODUCT(half[2], o[2]);
    state->spin.y =
        BODY_PRODUCT(o[0], half[1]) + BODY_PRODUCT(half[2], o[1]) - BODY_PRODUCT(half[0], o[3]);
    state->spin.z =
        BODY_PRODUCT(o[0], half[2]) + BODY_PRODUCT(half[0], o[2]) - BODY_PRODUCT(half[1], o[1]);
}

void ApplyRigidBodyImpulse(RigidBodyState *state, RigidBody *body, f32 dt) {
#if TGR_PC
    /* The libraries round the product for z alone, where the x87 stack is full. */
    state->velocity.x = BODY_PRODUCT(body->force.x, dt) + state->velocity.x;
    state->velocity.y = BODY_PRODUCT(body->force.y, dt) + state->velocity.y;
    state->velocity.z = (f32)(body->force.z * dt) + state->velocity.z;
    state->angularVelocity.x = BODY_PRODUCT(body->torque.x, dt) + state->angularVelocity.x;
    state->angularVelocity.y = BODY_PRODUCT(body->torque.y, dt) + state->angularVelocity.y;
    state->angularVelocity.z = BODY_PRODUCT(body->torque.z, dt) + state->angularVelocity.z;
#else
    f32 linear[3], angular[3];

    linear[0] = body->force.x * dt;
    linear[1] = body->force.y * dt;
    linear[2] = body->force.z * dt;
    angular[0] = body->torque.x * dt;
    angular[1] = body->torque.y * dt;
    angular[2] = body->torque.z * dt;
    state->velocity.x = linear[0] + state->velocity.x;
    state->velocity.y = linear[1] + state->velocity.y;
    state->velocity.z = linear[2] + state->velocity.z;
    state->angularVelocity.x = angular[0] + state->angularVelocity.x;
    state->angularVelocity.y = angular[1] + state->angularVelocity.y;
    state->angularVelocity.z = angular[2] + state->angularVelocity.z;
#endif
}

void QuaternionToMatrix(f32 out[4][4], RigidBodyState *state) {
    Quaternion *q = &state->orientation;
    f32 ww, xx, yy, zz;
    f32 xy2, wz2, xz2, wy2, yz2, wx2;

    ww = q->w * q->w;
    xx = q->x * q->x;
    yy = q->y * q->y;
    zz = q->z * q->z;
    xy2 = q->y * (q->x + q->x);
    wz2 = q->z * (q->w + q->w);
    xz2 = q->z * (q->x + q->x);
    wy2 = q->y * (q->w + q->w);
    yz2 = q->z * (q->y + q->y);
    wx2 = q->x * (q->w + q->w);
#if TGR_PC
    /* The libraries round the squares and ww - xx, but not the other sums or three products. */
    out[0][0] = (long double)ww + xx - yy - zz;
    out[0][1] = (long double)q->z * (q->w + q->w) + xy2;
    out[0][3] = 0.0f;
    out[0][2] = (long double)q->z * (q->x + q->x) - wy2;
    out[1][0] = xy2 - wz2;
    out[1][1] = (long double)(ww - xx) + yy - zz;
    out[1][3] = 0.0f;
    out[1][2] = (long double)q->x * (q->w + q->w) + yz2;
    out[2][3] = 0.0f;
    out[2][1] = yz2 - wx2;
    out[2][0] = xz2 + wy2;
    out[2][2] = (long double)(ww - xx) - yy + zz;
#else
    out[0][0] = ww + xx - yy - zz;
    out[0][1] = xy2 + wz2;
    out[0][3] = 0.0f;
    out[0][2] = xz2 - wy2;
    out[1][0] = xy2 - wz2;
    out[1][1] = ww - xx + yy - zz;
    out[1][3] = 0.0f;
    out[1][2] = yz2 + wx2;
    out[2][3] = 0.0f;
    out[2][1] = yz2 - wx2;
    out[2][0] = xz2 + wy2;
    out[2][2] = ww - xx - yy + zz;
#endif
    out[3][0] = state->position.x;
    out[3][1] = state->position.y;
    out[3][3] = 1.0f;
    out[3][2] = state->position.z;
    StubPhysics2(out);
}

void IntegrateRigidBodyState(RigidBodyState *out, RigidBodyState *in, f32 dt) {
#if TGR_PC
    /* The libraries round the products for position.z, orientation.x, and orientation.z alone. */
    out->position.x = BODY_PRODUCT(in->velocity.x, dt) + in->position.x;
    out->position.y = BODY_PRODUCT(in->velocity.y, dt) + in->position.y;
    out->position.z = (f32)(in->velocity.z * dt) + in->position.z;
    out->velocity.x = in->velocity.x;
    out->velocity.y = in->velocity.y;
    out->velocity.z = in->velocity.z;
    out->orientation.w = BODY_PRODUCT(in->spin.w, dt) + in->orientation.w;
    out->orientation.x = (f32)(in->spin.x * dt) + in->orientation.x;
    out->orientation.y = BODY_PRODUCT(in->spin.y, dt) + in->orientation.y;
    out->orientation.z = (f32)(in->spin.z * dt) + in->orientation.z;
#else
    f32 move[3];
    f32 turn[4];

    move[0] = in->velocity.x * dt;
    move[1] = in->velocity.y * dt;
    move[2] = in->velocity.z * dt;
    out->position.x = move[0] + in->position.x;
    out->position.y = move[1] + in->position.y;
    out->position.z = move[2] + in->position.z;
    out->velocity.x = in->velocity.x;
    out->velocity.y = in->velocity.y;
    out->velocity.z = in->velocity.z;

    turn[0] = in->spin.w * dt;
    turn[1] = in->spin.x * dt;
    turn[2] = in->spin.y * dt;
    turn[3] = in->spin.z * dt;
    out->orientation.w = turn[0] + in->orientation.w;
    out->orientation.x = turn[1] + in->orientation.x;
    out->orientation.y = turn[2] + in->orientation.y;
    out->orientation.z = turn[3] + in->orientation.z;
#endif
    QuaternionNormalize(&out->orientation);

    out->angularVelocity.x = in->angularVelocity.x;
    out->angularVelocity.y = in->angularVelocity.y;
    out->angularVelocity.z = in->angularVelocity.z;
    out->spin.w = in->spin.w;
    out->spin.x = in->spin.x;
    out->spin.y = in->spin.y;
    out->spin.z = in->spin.z;
}

void SetRigidBodyForce(
    RigidBodyForce *force, f32 x, f32 y, f32 z, f32 pointX, f32 pointY, f32 pointZ, s32 frame) {
    force->frame = frame;
    force->vector.x = x;
    force->vector.y = y;
    force->vector.z = z;
    force->point.x = pointX;
    force->point.y = pointY;
    force->point.z = pointZ;
}

/* Multiply a vector by the rows of a matrix, the sums building up in the result. */
#define MTX_ROWS_MUL_VEC3F(out, m, v, i, k)                                                        \
    for (i = 0; i < 3; i++) {                                                                      \
        (&(out)->x)[i] = 0.0f;                                                                     \
        for (k = 0; k < 3; k++) {                                                                  \
            BODY_ACCUMULATE((&(out)->x)[i], (m)[i][k], (&(v)->x)[k]);                              \
        }                                                                                          \
    }

void Mtx4fInverseRotate(Vec3f *out, f32 m[4][4], Vec3f *v) {
    s32 i, k;

    MTX_ROWS_MUL_VEC3F(out, m, v, i, k);
}

/* Rotate a vector by the 3 by 3 part of a 4 by 4 matrix, the sums building up in the result. */
#define MTX3X3_MUL_VEC3F(out, m, v, i, k)                                                          \
    for (i = 0; i < 3; i++) {                                                                      \
        (&(out)->x)[i] = 0.0f;                                                                     \
        for (k = 0; k < 3; k++) {                                                                  \
            BODY_ACCUMULATE((&(out)->x)[i], (m)[k][i], (&(v)->x)[k]);                              \
        }                                                                                          \
    }

void Mtx3x3MulVec3f(Vec3f *out, f32 m[4][4], Vec3f *v) {
    s32 i, k;

    MTX3X3_MUL_VEC3F(out, m, v, i, k);
}

void Mtx4x4TransformPoint(Vec3f *out, f32 m[4][4], Vec3f *v) {
    s32 i, k;

    MTX3X3_MUL_VEC3F(out, m, v, i, k);
    out->x += m[3][0];
    out->y += m[3][1];
    out->z += m[3][2];
}

void Mtx3fMulVec3f(Vec3f *out, f32 m[3][3], Vec3f *v) {
    s32 i, k;

    MTX_ROWS_MUL_VEC3F(out, m, v, i, k);
}

/* The routines under TGR_PLATFORM_N64 alone are absent from the PC libraries. */
#if TGR_PLATFORM == TGR_PLATFORM_N64
void ComputeRigidBodyAcceleration(RigidBody *body) {
    Vec3f local, angular;

    body->force.x *= 1.0f / body->mass;
    body->force.y *= 1.0f / body->mass;
    body->force.z *= 1.0f / body->mass;
    if (body->kind == RIGID_BODY_NO_ROTATION) {
        return;
    }
    /* The torque into the frame of the body, through the inverse inertia, and back. */
    Mtx4fInverseRotate(&local, body->orientation, &body->torque);
    Mtx3fMulVec3f(&angular, body->inverseInertia, &local);
    Mtx3x3MulVec3f(&body->torque, body->orientation, &angular);
}

/* The original adds an uninitialised stack vector for a force of another frame. */
void ApplySingleForceWithDebug(RigidBody *body, RigidBodyForce *force) {
    Vec3f arm;
#ifdef ENABLE_PATCHES
    /* A force of another frame adds nothing. */
    f32 vector[3] = {0.0f, 0.0f, 0.0f};
#else
    f32 vector[3];
#endif /* ENABLE_PATCHES */
    f32 torque[3];

    switch (force->frame) {
    case RIGID_BODY_FORCE_WORLD:
        vector[0] = force->vector.x;
        vector[1] = force->vector.y;
        vector[2] = force->vector.z;
        break;
    case RIGID_BODY_FORCE_BODY:
        /* The original keeps the force as an array of three and rotates it in place. */
        Mtx3x3MulVec3f((Vec3f *)vector, body->orientation, &force->vector);
        break;
    }
    body->force.x = vector[0] + body->force.x;
    body->force.y = vector[1] + body->force.y;
    body->force.z = vector[2] + body->force.z;

    if (body->kind != RIGID_BODY_NO_ROTATION) {
        Mtx3x3MulVec3f(&arm, body->orientation, &force->point);
        torque[0] = arm.y * vector[2] - vector[1] * arm.z;
        torque[1] = arm.z * vector[0] - vector[2] * arm.x;
        torque[2] = arm.x * vector[1] - vector[0] * arm.y;
        body->torque.x = torque[0] + body->torque.x;
        body->torque.y = torque[1] + body->torque.y;
        body->torque.z = torque[2] + body->torque.z;
    }

    osSyncPrintf("Force = %10.4f, %10.4f, %10.4f, %10.4f, %10.4f, %10.4f\n",
                 force->vector.x,
                 force->vector.y,
                 force->vector.z,
                 force->point.x,
                 force->point.y,
                 force->point.z);
    osSyncPrintf("F/T = %10.4f, %10.4f, %10.4f, %10.4f, %10.4f, %10.4f\n",
                 body->force.x,
                 body->force.y,
                 body->force.z,
                 body->torque.x,
                 body->torque.y,
                 body->torque.z);
}

void ApplyAllForcesToBody(RigidBody *body) {
    RigidBodyForce *force;

    for (force = body->forces; force != NULL; force = force->next) {
        ApplySingleForceWithDebug(body, force);
    }
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

void InitRigidBody(RigidBody *body) {
#if !TGR_PC
    f32 boxShare;
    f32 xx;
#else
    long double xx;
#endif
    f32 yy, zz;
    s32 i, j;

    body->unreconstructed0 = 0;
    body->wheels[0] = NULL;
    body->wheels[1] = NULL;
    body->wheels[2] = NULL;
    body->wheels[3] = NULL;
    body->unreconstructed14 = 0;
    body->contactFrames = 0;
    body->ground = NULL;
    body->spinRate = 0.0f;
    body->steerAngle = DEFAULT_STEER_ANGLE;
    body->radius = 0.5f;
    body->drive = 0.0f;
    body->brake = 0.0f;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            body->inertia[i][j] = i == j ? 1.0f : 0.0f;
        }
    }

    if (body->kind == RIGID_BODY_BOX || body->kind == RIGID_BODY_BOX_1) {
        zz = body->size.z * body->size.z;
        yy = body->size.y * body->size.y;
#if TGR_PC
        /* The libraries scale each sum by the mass before the share. Only the square of x stays
         * on the x87 stack unrounded. */
        xx = BODY_PRODUCT(body->size.x, body->size.x);
        body->inertia[0][0] = (f32)((BODY_INTERMEDIATE(zz) + yy) * body->mass * BOX_INERTIA_SHARE);
        body->inertia[1][1] = (f32)((xx + zz) * body->mass * BOX_INERTIA_SHARE);
        body->inertia[2][2] = (f32)((xx + yy) * body->mass * BOX_INERTIA_SHARE);
#else
        boxShare = BOX_INERTIA_SHARE * body->mass;
        body->inertia[0][0] = (zz + yy) * boxShare;
        xx = body->size.x * body->size.x;
        body->inertia[1][1] = (zz + xx) * boxShare;
        body->inertia[2][2] = (yy + xx) * boxShare;
#endif
        StubPhysics1(body->inertia);
    }
    if (body->kind != RIGID_BODY_NO_ROTATION) {
        body->inverseInertia[0][0] = 1.0f / body->inertia[0][0];
        body->inverseInertia[1][1] = 1.0f / body->inertia[1][1];
        body->inverseInertia[2][2] = 1.0f / body->inertia[2][2];
    }
    body->rollAngle = 0.0f;
    body->suspension = 0.0f;
}

void BuildSkewSymmetricMatrix(f32 out[3][3], Vec3f *v) {
    out[2][2] = 0.0f;
    out[1][1] = 0.0f;
    out[0][0] = 0.0f;
    out[0][1] = -v->z;
    out[0][2] = v->y;
    out[1][0] = v->z;
    out[1][2] = -v->x;
    out[2][0] = -v->y;
    out[2][1] = v->x;
}

void TransposeCopyMatrix3x3(f32 transpose[3][3], f32 copy[3][3], f32 m[4][4]) {
    s32 i, j;
    f32 value;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            value = m[i][j];
            transpose[j][i] = value;
            copy[i][j] = value;
        }
    }
}

void Transpose4x4Into3x3(f32 out[3][3], f32 m[4][4]) {
    s32 i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            out[j][i] = m[i][j];
        }
    }
}

void CopyMatrix4x4To3x3(f32 out[3][3], f32 m[4][4]) {
    s32 i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            out[i][j] = m[i][j];
        }
    }
}

void Mtx3fMultiply(f32 out[3][3], f32 a[3][3], f32 b[3][3]) {
    s32 i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
#if TGR_PC && TGR_RENDERER == TGR_RENDERER_GLIDE
            /* BRGlide adds the third product before the second. */
            out[i][j] = (f32)(BODY_PRODUCT(a[i][0], b[0][j]) + BODY_PRODUCT(a[i][2], b[2][j]) +
                              BODY_PRODUCT(a[i][1], b[1][j]));
#else
            out[i][j] = (f32)(BODY_PRODUCT(a[i][0], b[0][j]) + BODY_PRODUCT(a[i][1], b[1][j]) +
                              BODY_PRODUCT(a[i][2], b[2][j]));
#endif
        }
    }
}

void SubtractMatrix3x3(f32 out[3][3], f32 a[3][3], f32 b[3][3]) {
    s32 i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            out[i][j] = a[i][j] - b[i][j];
        }
    }
}

void MtxBuildScaledInverse(f32 m[4][4], f32 out[4][4], Vec3f *scale) {
    s32 i, j;
    Vec3f negated;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            out[i][j] = m[j][i];
            out[i][j] *= (&scale->x)[j];
        }
        out[i][3] = 0.0f;
    }
    out[3][3] = 1.0f;
    negated.x = -m[3][0];
    negated.y = -m[3][1];
    negated.z = -m[3][2];
    Mtx3x3MulVec3f((Vec3f *)out[3], out, &negated);
}

#if TGR_PC
void SolveLinearSystem3x3(Vec3f *out, f32 a[3][3], Vec3f *b) {
    /* The libraries store one cofactor and the products of the first sum to the stack, and keep
     * the other products, the determinant, and its inverse on the x87 stack. */
    f32 square = a[1][1] * a[2][2];
    f32 cofactor = (f32)(BODY_PRODUCT(a[2][1], a[1][2]) - square);
    long double p0122 = BODY_PRODUCT(a[0][1], a[2][2]);
    long double p0221 = BODY_PRODUCT(a[0][2], a[2][1]);
    f32 p0112 = a[0][1] * a[1][2];
    long double p0211 = BODY_PRODUCT(a[0][2], a[1][1]);
    long double inverse = 1.0f / (p0122 * a[1][0] + BODY_PRODUCT(a[0][0], cofactor) -
                                  p0221 * a[1][0] - BODY_PRODUCT(p0112, a[2][0]) + p0211 * a[2][0]);

    out->x = (f32)((BODY_PRODUCT(b->x, cofactor) + BODY_PRODUCT((f32)p0122, b->y) -
                    BODY_PRODUCT((f32)p0221, b->y) - BODY_PRODUCT(p0112, b->z) +
                    BODY_PRODUCT((f32)p0211, b->z)) *
                   inverse);
    out->y =
        (f32) - (((BODY_PRODUCT(a[2][0], a[1][2]) - BODY_PRODUCT(a[1][0], a[2][2])) * b->x +
                  BODY_PRODUCT(a[0][0], a[2][2]) * b->y - BODY_PRODUCT(a[2][0], a[0][2]) * b->y -
                  BODY_PRODUCT(a[0][0], a[1][2]) * b->z + BODY_PRODUCT(a[1][0], a[0][2]) * b->z) *
                 inverse);
    out->z = (f32)(((BODY_PRODUCT(a[2][0], a[1][1]) - BODY_PRODUCT(a[1][0], a[2][1])) * b->x +
                    BODY_PRODUCT(a[0][0], a[2][1]) * b->y - BODY_PRODUCT(a[2][0], a[0][1]) * b->y -
                    BODY_PRODUCT(a[0][0], a[1][1]) * b->z + BODY_PRODUCT(a[1][0], a[0][1]) * b->z) *
                   inverse);
}
#else
/* A cofactor sum of Cramer's rule over the column b and six products of the other columns. */
static f32 CramerSum(f32 b0, f32 b1, f32 b2, f32 p0, f32 p1, f32 p2, f32 p3, f32 p4, f32 p5) {
    return b2 * p5 + (-b0 * p0 + b0 * p1 + b1 * p2 - b1 * p3 - b2 * p4);
}

void SolveLinearSystem3x3(Vec3f *out, f32 a[3][3], Vec3f *b) {
    f32 inverse;

    /* The original stores the determinant in the slot of its inverse before dividing. */
    inverse = CramerSum(a[0][0],
                        a[1][0],
                        a[2][0],
                        a[2][2] * a[1][1],
                        a[2][1] * a[1][2],
                        a[2][2] * a[0][1],
                        a[2][1] * a[0][2],
                        a[1][2] * a[0][1],
                        a[1][1] * a[0][2]);
    inverse = 1.0f / inverse;
    out->x = CramerSum(b->x,
                       b->y,
                       b->z,
                       a[2][2] * a[1][1],
                       a[2][1] * a[1][2],
                       a[2][2] * a[0][1],
                       a[2][1] * a[0][2],
                       a[1][2] * a[0][1],
                       a[1][1] * a[0][2]) *
             inverse;
    out->y = -CramerSum(b->x,
                        b->y,
                        b->z,
                        a[2][2] * a[1][0],
                        a[2][0] * a[1][2],
                        a[2][2] * a[0][0],
                        a[2][0] * a[0][2],
                        a[1][2] * a[0][0],
                        a[1][0] * a[0][2]) *
             inverse;
    out->z = CramerSum(b->x,
                       b->y,
                       b->z,
                       a[2][1] * a[1][0],
                       a[2][0] * a[1][1],
                       a[2][1] * a[0][0],
                       a[2][0] * a[0][1],
                       a[1][1] * a[0][0],
                       a[1][0] * a[0][1]) *
             inverse;
}
#endif /* TGR_PC */

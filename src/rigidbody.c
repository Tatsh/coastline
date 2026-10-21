#include "rigidbody.h"

#include <PR/gu.h>
#include <PR/os.h>

/* The inertia of a box about an axis is this share of its mass times the squares of the sides. */
#define BOX_INERTIA_SHARE 0.0833333358f

/* The steering angle a body starts with, about 10 degrees. */
#define DEFAULT_STEER_ANGLE 0.174f

void StubPhysics1(void *data) {
}

void StubPhysics2(void *data) {
}

void QuaternionMultiply(Quaternion *out, Quaternion *a, Quaternion *b) {
    f32 bw = b->w, bx = b->x, by = b->y, bz = b->z;
    f32 aw = a->w, ax = a->x, ay = a->y, az = a->z;

    out->z = az * bw - ay * bx + ax * by + aw * bz;
    out->y = ay * bw + az * bx + aw * by - ax * bz;
    out->x = ax * bw + aw * bx - az * by + ay * bz;
    out->w = aw * bw - ax * bx - ay * by - az * bz;
}

void QuaternionNormalize(Quaternion *q) {
    f32 inverse = 1.0f / sqrtf(q->z * q->z + (q->w * q->w + q->x * q->x + q->y * q->y));

    q->w *= inverse;
    q->x *= inverse;
    q->y *= inverse;
    q->z *= inverse;
}

void Vec3fNormalizeFast(Vec3f *v) {
    f32 inverse = 1.0f / sqrtf(v->z * v->z + (v->x * v->x + v->y * v->y));

    v->x *= inverse;
    v->y *= inverse;
    v->z *= inverse;
}

void ComputeDeltaQuaternion(RigidBodyState *state) {
    f32 half[3];

    half[0] = state->angularVelocity.x * 0.5f;
    half[1] = state->angularVelocity.y * 0.5f;
    half[2] = state->angularVelocity.z * 0.5f;
    state->spin.w = -half[0] * state->orientation.x - half[1] * state->orientation.y -
                    state->orientation.z * half[2];
    state->spin.x = state->orientation.w * half[0] + half[1] * state->orientation.z -
                    state->orientation.y * half[2];
    state->spin.y = state->orientation.w * half[1] + half[2] * state->orientation.x -
                    state->orientation.z * half[0];
    state->spin.z = state->orientation.w * half[2] + half[0] * state->orientation.y -
                    state->orientation.x * half[1];
}

void ApplyRigidBodyImpulse(RigidBodyState *state, RigidBody *body, f32 dt) {
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
    out[3][0] = state->position.x;
    out[3][1] = state->position.y;
    out[3][3] = 1.0f;
    out[3][2] = state->position.z;
    StubPhysics2(out);
}

void IntegrateRigidBodyState(RigidBodyState *out, RigidBodyState *in, f32 dt) {
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
            (&(out)->x)[i] += (m)[i][k] * (&(v)->x)[k];                                            \
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
            (&(out)->x)[i] += (m)[k][i] * (&(v)->x)[k];                                            \
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
void ApplySingleForceWithDebug(RigidBody *body, RigidBodyForce *force) {
    Vec3f vector, arm, torque;

    if (force->frame == RIGID_BODY_FORCE_WORLD) {
        vector = force->vector;
    } else if (force->frame == RIGID_BODY_FORCE_BODY) {
        Mtx3x3MulVec3f(&vector, body->orientation, &force->vector);
    }
    body->force.x = vector.x + body->force.x;
    body->force.y = vector.y + body->force.y;
    body->force.z = vector.z + body->force.z;

    if (body->kind != RIGID_BODY_NO_ROTATION) {
        Mtx3x3MulVec3f(&arm, body->orientation, &force->point);
        torque.x = arm.y * vector.z - vector.y * arm.z;
        torque.y = arm.z * vector.x - vector.z * arm.x;
        torque.z = arm.x * vector.y - vector.x * arm.y;
        body->torque.x = torque.x + body->torque.x;
        body->torque.y = torque.y + body->torque.y;
        body->torque.z = torque.z + body->torque.z;
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
#pragma GCC diagnostic pop

void ApplyAllForcesToBody(RigidBody *body) {
    RigidBodyForce *force;

    for (force = body->forces; force != NULL; force = force->next) {
        ApplySingleForceWithDebug(body, force);
    }
}

void InitRigidBody(RigidBody *body) {
    f32 boxShare, xx, yy, zz;
    s32 i, j;

    body->unreconstructed0 = 0;
    for (i = 0; i < 4; i++) {
        body->wheels[i] = NULL;
    }
    body->unreconstructed14 = 0;
    body->contactFrames = 0;
    body->ground = NULL;
    body->spinRate = 0.0f;
    body->drive = 0.0f;
    body->brake = 0.0f;
    body->steerAngle = DEFAULT_STEER_ANGLE;
    body->radius = 0.5f;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            body->inertia[i][j] = i == j ? 1.0f : 0.0f;
        }
    }

    if (body->kind == RIGID_BODY_BOX || body->kind == RIGID_BODY_BOX_1) {
        zz = body->size.z * body->size.z;
        yy = body->size.y * body->size.y;
        boxShare = BOX_INERTIA_SHARE * body->mass;
        body->inertia[0][0] = (zz + yy) * boxShare;
        xx = body->size.x * body->size.x;
        body->inertia[1][1] = (zz + xx) * boxShare;
        body->inertia[2][2] = (yy + xx) * boxShare;
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
            out[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
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
    Vec3f negated;
    s32 i, j;

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

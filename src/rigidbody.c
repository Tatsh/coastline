#include "rigidbody.h"

#include <PR/gu.h>
#include <PR/os.h>

/* The inertia of a box about an axis is this share of its mass times the squares of the sides. */
#define BOX_INERTIA_SHARE 0.0833333358f

/* The friction a body starts with. */
#define DEFAULT_FRICTION 0.174f

void StubPhysics1(void *data) {
}

void StubPhysics2(void *data) {
}

void QuaternionMultiply(Quaternion *out, Quaternion *a, Quaternion *b) {
    f32 aw = a->w, ax = a->x, ay = a->y, az = a->z;
    f32 bw = b->w, bx = b->x, by = b->y, bz = b->z;

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
    f32 hx = state->angularVelocity.x * 0.5f;
    f32 hy = state->angularVelocity.y * 0.5f;
    f32 hz = state->angularVelocity.z * 0.5f;
    f32 w = state->orientation.w, x = state->orientation.x;
    f32 y = state->orientation.y, z = state->orientation.z;

    state->spin.w = -hx * x - hy * y - z * hz;
    state->spin.x = w * hx + hy * z - y * hz;
    state->spin.y = w * hy + hz * x - z * hx;
    state->spin.z = w * hz + hx * y - x * hy;
}

void ApplyRigidBodyImpulse(RigidBodyState *state, RigidBody *body, f32 dt) {
    Vec3f linear, angular;

    linear.x = body->force.x * dt;
    linear.y = body->force.y * dt;
    linear.z = body->force.z * dt;
    angular.x = body->torque.x * dt;
    angular.y = body->torque.y * dt;
    angular.z = body->torque.z * dt;
    state->velocity.x = linear.x + state->velocity.x;
    state->velocity.y = linear.y + state->velocity.y;
    state->velocity.z = linear.z + state->velocity.z;
    state->angularVelocity.x = angular.x + state->angularVelocity.x;
    state->angularVelocity.y = angular.y + state->angularVelocity.y;
    state->angularVelocity.z = angular.z + state->angularVelocity.z;
}

void QuaternionToMatrix(f32 out[4][4], RigidBodyState *state) {
    f32 w = state->orientation.w, x = state->orientation.x;
    f32 y = state->orientation.y, z = state->orientation.z;
    f32 ww = w * w, xx = x * x, yy = y * y, zz = z * z;
    f32 w2 = w + w, x2 = x + x, y2 = y + y;
    f32 xy2 = y * x2, wz2 = z * w2, xz2 = z * x2, wy2 = y * w2, yz2 = z * y2, wx2 = x * w2;
    f32 sum = ww + xx, difference = ww - xx;

    out[0][0] = sum - yy - zz;
    out[0][1] = xy2 + wz2;
    out[0][3] = 0.0f;
    out[0][2] = xz2 - wy2;
    out[1][0] = xy2 - wz2;
    out[1][1] = difference + yy - zz;
    out[1][3] = 0.0f;
    out[1][2] = yz2 + wx2;
    out[2][3] = 0.0f;
    out[2][1] = yz2 - wx2;
    out[2][0] = xz2 + wy2;
    out[2][2] = difference - yy + zz;
    out[3][0] = state->position.x;
    out[3][1] = state->position.y;
    out[3][3] = 1.0f;
    out[3][2] = state->position.z;
    StubPhysics2(out);
}

void IntegrateRigidBodyState(RigidBodyState *out, RigidBodyState *in, f32 dt) {
    Vec3f move;
    Quaternion turn;

    move.x = in->velocity.x * dt;
    move.y = in->velocity.y * dt;
    move.z = in->velocity.z * dt;
    out->position.x = move.x + in->position.x;
    out->position.y = move.y + in->position.y;
    out->position.z = move.z + in->position.z;
    out->velocity = in->velocity;

    turn.w = in->spin.w * dt;
    turn.x = in->spin.x * dt;
    turn.y = in->spin.y * dt;
    turn.z = in->spin.z * dt;
    out->orientation.w = turn.w + in->orientation.w;
    out->orientation.x = turn.x + in->orientation.x;
    out->orientation.y = turn.y + in->orientation.y;
    out->orientation.z = turn.z + in->orientation.z;
    QuaternionNormalize(&out->orientation);

    out->angularVelocity = in->angularVelocity;
    out->spin = in->spin;
}

void SetRigidBodyForce(
    RigidBodyForce *force, f32 x, f32 y, f32 z, f32 pointX, f32 pointY, f32 pointZ, s32 frame) {
    force->vector.x = x;
    force->frame = frame;
    force->vector.y = y;
    force->vector.z = z;
    force->point.x = pointX;
    force->point.y = pointY;
    force->point.z = pointZ;
}

void Mtx4fInverseRotate(Vec3f *out, f32 m[4][4], Vec3f *v) {
    f32 *result = &out->x;
    f32 *in = &v->x;
    s32 i, k;

    /* The sums build up in the result, as in the original. */
    for (i = 0; i < 3; i++) {
        result[i] = 0.0f;
        for (k = 0; k < 3; k++) {
            result[i] += m[i][k] * in[k];
        }
    }
}

void Mtx3x3MulVec3f(Vec3f *out, f32 m[4][4], Vec3f *v) {
    f32 *result = &out->x;
    f32 *in = &v->x;
    s32 i, k;

    for (i = 0; i < 3; i++) {
        result[i] = 0.0f;
        for (k = 0; k < 3; k++) {
            result[i] += m[k][i] * in[k];
        }
    }
}

void Mtx4x4TransformPoint(Vec3f *out, f32 m[4][4], Vec3f *v) {
    Mtx3x3MulVec3f(out, m, v);
    out->x += m[3][0];
    out->y += m[3][1];
    out->z += m[3][2];
}

void Mtx3fMulVec3f(Vec3f *out, f32 m[3][3], Vec3f *v) {
    f32 *result = &out->x;
    f32 *in = &v->x;
    s32 i, k;

    for (i = 0; i < 3; i++) {
        result[i] = 0.0f;
        for (k = 0; k < 3; k++) {
            result[i] += m[i][k] * in[k];
        }
    }
}

void ComputeRigidBodyAcceleration(RigidBody *body) {
    f32 inverseMass = 1.0f / body->mass;
    Vec3f local, angular;

    body->force.x *= inverseMass;
    body->force.y *= inverseMass;
    body->force.z *= inverseMass;
    if (body->kind == RIGID_BODY_NO_ROTATION) {
        return;
    }
    /* The torque into the frame of the body, through the inverse inertia, and back. */
    Mtx4fInverseRotate(&local, body->orientation, &body->torque);
    Mtx3fMulVec3f(&angular, body->inverseInertia, &local);
    Mtx3x3MulVec3f(&body->torque, body->orientation, &angular);
}

void ApplySingleForceWithDebug(RigidBody *body, RigidBodyForce *force) {
    Vec3f vector, arm, torque;

    if (force->frame == RIGID_BODY_FORCE_WORLD) {
        vector = force->vector;
    } else if (force->frame == RIGID_BODY_FORCE_BODY) {
        Mtx3x3MulVec3f(&vector, body->orientation, &force->vector);
    } else {
        /* The original adds whatever its stack holds for another frame. */
        vector.x = vector.y = vector.z = 0.0f;
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

void ApplyAllForcesToBody(RigidBody *body) {
    RigidBodyForce *force;

    for (force = body->forces; force != NULL; force = force->next) {
        ApplySingleForceWithDebug(body, force);
    }
}

void InitRigidBody(RigidBody *body) {
    f32 *words = (f32 *)body->unreconstructed0;
    f32 boxShare, xx, yy, zz;
    s32 i, j;

    for (i = 0; i < 6; i++) {
        words[i] = 0.0f;
    }
    body->resetWord1B4 = 0;
    body->resetWord19C = 0;
    body->resetFloat1C4 = 0.0f;
    body->resetFloat1CC = 0.0f;
    body->resetFloat1D0 = 0.0f;
    body->friction = DEFAULT_FRICTION;
    body->resetFloat1C8 = 0.5f;
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
    body->resetFloat1D4 = 0.0f;
    body->resetFloat1D8 = 0.0f;
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
            out[i][j] = b[2][j] * a[i][2] + (a[i][0] * b[0][j] + a[i][1] * b[1][j]);
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
    f32 *factor = &scale->x;
    Vec3f negated;
    s32 i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            out[i][j] = m[j][i];
            out[i][j] *= factor[j];
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

    inverse = 1.0f / CramerSum(a[0][0],
                               a[1][0],
                               a[2][0],
                               a[2][2] * a[1][1],
                               a[2][1] * a[1][2],
                               a[2][2] * a[0][1],
                               a[2][1] * a[0][2],
                               a[1][2] * a[0][1],
                               a[1][1] * a[0][2]);
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

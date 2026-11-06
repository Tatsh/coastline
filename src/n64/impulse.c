#include "common/physics.h"
#include "common/rigidbody.h"
#include "common/vecmath.h"

s32 ApplyCollisionImpulse(RigidBody *b, Vec3f *pN, Vec3f *pDir, s32 flag, f32 rest) {
    float Rt[3][3];
    float R[3][3];
    float skew[3][3];
    float D[3][3];
    float K[3][3];
    float tmp[3][3];
    float W[3][3];
    float vc[3];
    float spare[2] TGR_MATCHING_UNUSED; /* declared, never used: the frame holds it */
    float x;                            /* the dead NaN test's value */
    float J[3];
    float nb[3];
    float dw[3];
    float jm[3];
    float tt[3];
    float tn[3];
    Vec3f zero TGR_MATCHING_UNUSED; /* initialised, never used */
    float dd;
    float r;
    int i;
    int j;

    zero = g_vecUnitZ;
    vc[0] = pN->x;
    vc[1] = pN->y;
    vc[2] = pN->z;
    TransposeCopyMatrix3x3(Rt, R, b->orientation);
    Mtx3fMulVec3f((Vec3f *)nb, Rt, (Vec3f *)vc);
    vc[0] = b->stepStates[1].angularVelocity.y * nb[2] - b->stepStates[1].angularVelocity.z * nb[1];
    vc[1] = b->stepStates[1].angularVelocity.z * nb[0] - b->stepStates[1].angularVelocity.x * nb[2];
    vc[2] = b->stepStates[1].angularVelocity.x * nb[1] - b->stepStates[1].angularVelocity.y * nb[0];
    vc[0] = vc[0] + b->stepStates[1].velocity.x;
    vc[1] = vc[1] + b->stepStates[1].velocity.y;
    vc[2] = vc[2] + b->stepStates[1].velocity.z;
    if (pDir->x * vc[0] + pDir->y * vc[1] + pDir->z * vc[2] >= 0.0f) {
        return 0;
    }
    BuildSkewSymmetricMatrix(skew, (Vec3f *)nb);
    Mtx3fMultiply(tmp, b->inverseInertia, R);
    Mtx3fMultiply(W, Rt, tmp);
    Mtx3fMultiply(K, W, skew);
    Mtx3fMultiply(tmp, skew, K);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (i == j) {
                D[i][j] = 1.0f;
            } else {
                D[i][j] = 0.0f;
            }
        }
    }
    D[0][0] = D[1][1] = D[2][2] = 1.0f / b->mass;
    SubtractMatrix3x3(K, D, tmp);
    dd = pDir->x * vc[0] + pDir->y * vc[1] + pDir->z * vc[2];
    tn[0] = pDir->x * dd;
    tn[1] = pDir->y * dd;
    tn[2] = pDir->z * dd;
    if (dd < 0.0f) {
        dd = -dd;
    }
    if (dd > 27.0f) {
        dd = 27.0f;
    }
    b->impactSpeed = dd;
    if (b->impactSpeed < 10) {
        b->impactSpeed = 0;
    } else {
        b->impactPoint.x = g_vCollisionContact.axis[0];
        b->impactPoint.y = g_vCollisionContact.axis[1];
        b->impactPoint.z = g_vCollisionContact.axis[2];
    }
    if (b->framesSinceImpact > 10 && rest < g_fConst0p0001) {
        b->impactVolume = b->impactVolume < (unsigned char)(127.0f * dd / 27.0f + 128.0f) ?
                              (unsigned char)(127.0f * dd / 27.0f + 128.0f) :
                              b->impactVolume;
        tn[0] = tn[0] * g_fConst0p9_3;
        tn[1] = tn[1] * g_fConst0p9_3;
        tn[2] = tn[2] * g_fConst0p9_3;
        vc[0] = vc[0] * g_fConst0p9_3;
        vc[1] = vc[1] * g_fConst0p9_3;
        vc[2] = vc[2] * g_fConst0p9_3;
    }
    dd = tt[2];
    tt[0] = vc[0] - tn[0];
    tt[1] = vc[1] - tn[1];
    tt[2] = vc[2] - tn[2];
    if (flag) {
        x = tt[2] * rest;
        if (x != x) {
        }
        tt[0] = tt[0] * g_fConst0p2;
        tt[1] = tt[1] * g_fConst0p2;
        tt[2] = tt[2] * g_fConst0p2;
    } else {
        tt[0] = 0.0f;
        tt[1] = 0.0f;
        tt[2] = 0.0f;
    }
    vc[0] = tt[0] + tn[0];
    vc[1] = tt[1] + tn[1];
    vc[2] = tt[2] + tn[2];
    SolveLinearSystem3x3((Vec3f *)J, K, (Vec3f *)vc);
    jm[0] = J[0] * (1.0f / b->mass);
    jm[1] = J[1] * (1.0f / b->mass);
    jm[2] = J[2] * (1.0f / b->mass);
    vc[0] = nb[1] * J[2] - nb[2] * J[1];
    vc[1] = nb[2] * J[0] - nb[0] * J[2];
    vc[2] = nb[0] * J[1] - nb[1] * J[0];
    Mtx3fMulVec3f((Vec3f *)dw, W, (Vec3f *)vc);
    r = g_fConst1p05 + rest;
    jm[0] = jm[0] * r;
    jm[1] = jm[1] * r;
    jm[2] = jm[2] * r;
    dw[0] = dw[0] * r;
    dw[1] = dw[1] * r;
    dw[2] = dw[2] * r;
    b->stepStates[1].velocity.x = b->stepStates[1].velocity.x - jm[0];
    b->stepStates[1].velocity.y = b->stepStates[1].velocity.y - jm[1];
    b->stepStates[1].velocity.z = b->stepStates[1].velocity.z - jm[2];
    b->stepStates[1].angularVelocity.x = b->stepStates[1].angularVelocity.x - dw[0];
    b->stepStates[1].angularVelocity.y = b->stepStates[1].angularVelocity.y - dw[1];
    b->stepStates[1].angularVelocity.z = b->stepStates[1].angularVelocity.z - dw[2];
    return 1;
}

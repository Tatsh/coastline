#include <PR/os.h>

#include "common/game.h"
#include "common/physics.h"

#define CURRENT_ORIENTATION body->stepStates[1].orientation
#define SAVED_ORIENTATION body->stepStates[0].orientation

s32 CollideBodyWithTriangles(RigidBody *body, f32 box[4][4]) {
    CollisionTriangleNode *node;
    f32 v[9];
    f32 nrm[3];
    f32 e1[3];
    f32 e2[3];
    TrackTriangle *triangle;
    f32 planeD;
    s32 ret;
    s16 cnt;
    f32 sign[3];
    s32 flag;
    s32 spin;
    s32 sgn;
    s32 r;
    f32 d;
    /* These unused locals preserve the original IDO frame layout. */
    s32 unused;
    f32 dp[3];
    s32 pad[2];

    ret = 0;
    cnt = 0;
    for (node = g_pCollisionTriangleList; node != 0; node = node->next) {
        triangle = node->triangle;
        Mtx4x4TransformPoint((Vec3f *)&v[0], box, triangle->vertices[0]);
        Mtx4x4TransformPoint((Vec3f *)&v[3], box, triangle->vertices[1]);
        Mtx4x4TransformPoint((Vec3f *)&v[6], box, triangle->vertices[2]);
        e1[0] = v[3] - v[0];
        e1[1] = v[4] - v[1];
        e1[2] = v[5] - v[2];
        e2[0] = v[6] - v[0];
        e2[1] = v[7] - v[1];
        e2[2] = v[8] - v[2];
        nrm[0] = e1[1] * e2[2] - e1[2] * e2[1];
        nrm[1] = e1[2] * e2[0] - e1[0] * e2[2];
        nrm[2] = e1[0] * e2[1] - e1[1] * e2[0];
        if (IsTriangleInUnitCube((Vec3f *)v, (Vec3f *)nrm) == 0) {
            continue;
        }
        Vec3fNormalizeFast((Vec3f *)nrm);
#if TGR_REGION == TGR_REGION_JP
        planeD = (nrm[0] * v[0] + nrm[1] * v[1]) + nrm[2] * v[2];
        flag = 1;
        g_dwCollisionContactType = 0;
        cnt++;
#else
        flag = 1;
        g_dwCollisionContactType = 0;
        cnt++;
        planeD = (nrm[0] * v[0] + nrm[1] * v[1]) + nrm[2] * v[2];
#endif
#if TGR_REGION != TGR_REGION_EU
#if TGR_REGION == TGR_REGION_JP
        if (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType != 0) {
#else
        if (g_dwGameMode == GAME_MODE_DEMO) {
#endif
            if ((nrm[0] < 0.0f ? -nrm[0] : nrm[0]) > 0.999f ||
                (nrm[1] < 0.0f ? -nrm[1] : nrm[1]) > 0.999f ||
                (nrm[2] < 0.0f ? -nrm[2] : nrm[2]) > 0.999f) {
                g_dwCollisionContactType = 1;
                osSyncPrintf("Wank CT1 case\n");
                spin = 1;
            } else if ((planeD < 0.0f ? -planeD : planeD) < 0.5f) {
                osSyncPrintf("Triangle Edge to CubeFace\n");
                spin = 0;
                g_dwCollisionContactType = 2;
            }
        }
#endif
        osSyncPrintf("Cube Edge to Triangle Face\n");
        e1[0] = nrm[0] * planeD;
        e1[1] = nrm[1] * planeD;
        e1[2] = nrm[2] * planeD;
        sgn = e1[0] < 0 ? -1 : 1;
        sign[0] = sgn * 0.5f;
        sgn = e1[1] < 0 ? -1 : 1;
        sign[1] = sgn * 0.5f;
        sgn = e1[2] < 0 ? -1 : 1;
        sign[2] = sgn * 0.5f;
        g_vCollisionContact.axis[0] = sign[0] * body->boxSize.x;
        g_vCollisionContact.axis[1] = sign[1] * body->boxSize.y;
        g_vCollisionContact.axis[2] = sign[2] * body->boxSize.z + body->boxCenterZ;
        g_pCollisionTriangle = triangle;
        ComputeCollisionPenetration(body, (Vec3f *)nrm, planeD, (Vec3f *)sign, (Vec3f *)v);
        if (body->orientation[2][2] > 0.5f) {
            flag = 0;
        }
        if (flag) {
            osSyncPrintf("Resistive collision %10.3f\n", body->orientation[2][2]);
        }
        if (g_dwCollisionContactType != 1) {
            r = ApplyCollisionImpulse(
                body, &g_vCollisionContact.vector, &g_pCollisionTriangle->normal, flag, 0.0f);
        } else {
            r = BounceOffPlane(body, &g_pCollisionTriangle->normal, flag, spin);
        }
        if (r != 0) {
            ret = 1;
            dp[0] = body->stepStates[1].position.x - body->stepStates[0].position.x;
            dp[1] = body->stepStates[1].position.y - body->stepStates[0].position.y;
            dp[2] = body->stepStates[1].position.z - body->stepStates[0].position.z;
            d = ((dp[0] * triangle->normal.x + dp[1] * triangle->normal.y) +
                 dp[2] * ((f32 *)&triangle->normal)[2]) *
                1.1;
            dp[0] = triangle->normal.x * d;
            dp[1] = triangle->normal.y * d;
            dp[2] = triangle->normal.z * d;
            body->stepStates[1].position.x = body->stepStates[1].position.x - dp[0];
            body->stepStates[1].position.y = body->stepStates[1].position.y - dp[1];
            body->stepStates[1].position.z = body->stepStates[1].position.z - dp[2];
            body->stepStates[1].orientation.w = body->stepStates[0].orientation.w;
            /* clang-format off */
            CURRENT_ORIENTATION.x = SAVED_ORIENTATION.x; CURRENT_ORIENTATION.y = SAVED_ORIENTATION.y;
            /* clang-format on */
            ComputeDeltaQuaternion(&body->stepStates[1]);
            QuaternionToMatrix(body->orientation, &body->stepStates[1]);
        }
    }
    if (cnt == 0) {
        if (body->framesSinceImpact < 40) {
            body->framesSinceImpact++;
        }
    } else {
        body->framesSinceImpact = 0;
    }
    return ret;
}

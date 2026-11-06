#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/physics.h"
#include "common/rigidbody.h"

#define ABS(x) ((x) < 0.0f ? -(x) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SIGN(x) ((x) == 0.0f ? 0.0 : ((x) > 0 ? 1.0 : -1.0))

s32 TipBalancedVehicle(RigidBody *body) {
    s32 count;
    RigidBody *wheel;
    f32 *unusedNormal TGR_MATCHING_UNUSED;
    Vec3f points[2];
    f32 tilt;
    f32 nearest;

    QuaternionToMatrix(body->orientation, &body->stepStates[0]);
    nearest = 100.0f;
    count = 0;
#ifdef ENABLE_PATCHES
    wheel = body->wheels[0];
#endif
    if (body->wheels[0]->contactFrames != 0) {
        count++;
        wheel = body->wheels[0];
        points[1].x = SIGN(body->wheels[0]->state.position.x) * (body->boxSize.x * 0.5f);
        points[1].y = SIGN(wheel->state.position.y) * (body->boxSize.y * 0.5f);
        points[1].z = -body->boxSize.z * 0.5f + body->boxCenterZ;
        Mtx4x4TransformPoint(&points[0], body->orientation, &points[1]);
        tilt = ABS(PlaneDistance(&wheel->groundNormal, wheel->groundOffset, &points[0]));
        nearest = MIN(nearest, tilt);
    }
    if (body->wheels[1]->contactFrames != 0) {
        count++;
        wheel = body->wheels[1];
        points[1].x = SIGN(body->wheels[1]->state.position.x) * (body->boxSize.x * 0.5f);
        points[1].y = SIGN(wheel->state.position.y) * (body->boxSize.y * 0.5f);
        points[1].z = -body->boxSize.z * 0.5f + body->boxCenterZ;
        Mtx4x4TransformPoint(&points[0], body->orientation, &points[1]);
        tilt = ABS(PlaneDistance(&wheel->groundNormal, wheel->groundOffset, &points[0]));
        nearest = MIN(nearest, tilt);
    }
    if (body->wheels[2]->contactFrames != 0) {
        count++;
        wheel = body->wheels[2];
        points[1].x = SIGN(body->wheels[2]->state.position.x) * (body->boxSize.x * 0.5f);
        points[1].y = SIGN(wheel->state.position.y) * (body->boxSize.y * 0.5f);
        points[1].z = -body->boxSize.z * 0.5f + body->boxCenterZ;
        Mtx4x4TransformPoint(&points[0], body->orientation, &points[1]);
        tilt = ABS(PlaneDistance(&wheel->groundNormal, wheel->groundOffset, &points[0]));
        nearest = MIN(nearest, tilt);
    }
    if (body->wheels[3]->contactFrames != 0) {
        count++;
        wheel = body->wheels[3];
        points[1].x = SIGN(body->wheels[3]->state.position.x) * (body->boxSize.x * 0.5f);
        points[1].y = SIGN(wheel->state.position.y) * (body->boxSize.y * 0.5f);
        points[1].z = -body->boxSize.z * 0.5f + body->boxCenterZ;
        Mtx4x4TransformPoint(&points[0], body->orientation, &points[1]);
        tilt = ABS(PlaneDistance(&wheel->groundNormal, wheel->groundOffset, &points[0]));
        nearest = MIN(nearest, tilt);
    }
    if (count > 2 || count < 1) {
        return 0;
    }
    osSyncPrintf("-- Stand Dist = %f\n", nearest);
    if (nearest > 0.6) {
        return 0;
    }
    GetBodyPointVelocity(&points[0], body, &points[1]);
    /* The original array access fixes IDO's load order for consecutive normal components. */
    tilt = points[0].x * (&wheel->groundNormal.x)[0] + points[0].y * (&wheel->groundNormal.x)[1] +
           points[0].z * (&wheel->groundNormal.x)[2];
    osSyncPrintf("-- Stand Vel = %f\n", tilt);
    if (ABS(tilt) > 0.1f) {
        return 0;
    }
    tilt = body->orientation[0][0] * (&body->groundNormal.x)[0] +
           body->orientation[0][1] * (&body->groundNormal.x)[1] +
           body->orientation[0][2] * (&body->groundNormal.x)[2];
    if (tilt > 0.0f) {
        points[1].y = 0.1f;
        points[1].x = 0.0f;
        points[1].z = 0.0f;
    } else {
        points[1].x = 0.0f;
        points[1].y = -0.1f;
        points[1].z = 0.0f;
    }
    osSyncPrintf("-- Stand Point = %f\n", tilt);
    Mtx3x3MulVec3f(&points[0], body->orientation, &points[1]);
    body->stepStates[0].angularVelocity.x = body->stepStates[0].angularVelocity.x + points[0].x;
    body->stepStates[0].angularVelocity.y = body->stepStates[0].angularVelocity.y + points[0].y;
    body->stepStates[0].angularVelocity.z = body->stepStates[0].angularVelocity.z + points[0].z;
    body->stepStates[0].angularVelocity.x = body->stepStates[0].angularVelocity.x + points[0].x;
    body->stepStates[0].angularVelocity.y = body->stepStates[0].angularVelocity.y + points[0].y;
    body->stepStates[0].angularVelocity.z = body->stepStates[0].angularVelocity.z + points[0].z;
    ComputeDeltaQuaternion(&body->stepStates[0]);
    return 1;
}

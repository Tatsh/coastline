#include <PR/gu.h>

#include "common/physics.h"
#include "common/vehiclemodel.h"
#include "common/weather.h"
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) == 0 ? 0.0 : ((x) > 0 ? 1.0 : -1.0))
#define SIGN_FLOAT(x) ((x) == 0 ? 0.0 : ((x) > 0.0f ? 1.0 : -1.0))
#define SIGN_BODY(x) ((x) == 0.0f ? 0. : ((x) > 0.0f ? 1. : -1.))
#define g_afSurfaceGripFlat ((f32 *)g_afSurfaceGrip)
#define g_afSurfaceMaxLoadFlat ((f32 *)g_afSurfaceMaxLoad)
#define g_afSurfaceSlipLoadFlat ((f32 *)g_afSurfaceSlipLoad)
#define LEAN_STEP 0.26666668f

/* IDO needs array-indexed vectors and reserved locals for the original register allocation. */
void ApplySideGrip(
    RigidBody *body, f32 dt, f32 *rearLoad, f32 *frontLoad, u8 *rearSliding, u8 *frontSliding) {
    float tmpB[3];
    float tmpA[3];
    float pt[3];
    float vB[3];
    float vA[3];
    float w[3];
#ifdef __sgi
    int frameReserve[4];
#endif
    double k;
    int n;
    float grip;
    int idx;
    float sv[3];
    float side;
    int ran;
    unsigned char sA;
    unsigned char sB;
    unsigned char sC;
    unsigned char sD;
    float lo;
    float slipF;
    float slipR;
    float g;
    short row;
#ifdef __sgi
    float frameReserveFloat;
#endif
    float hold;
    float s;
    float v;
    float m4;
    float sp;
    float t;

#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    dt = 0.033333335f;
#endif
    ran = 0;
    side = 0.0f;
    n = (s32)body->wheels[0]->ground;
    if (n == 0) {
        body->wheels[0]->contactFrames = 0;
    }
    n = (s32)body->wheels[1]->ground;
    if (n == 0) {
        body->wheels[1]->contactFrames = 0;
    }
    n = (s32)body->wheels[2]->ground;
    if (n == 0) {
        body->wheels[2]->contactFrames = 0;
    }
    n = (s32)body->wheels[3]->ground;
    if (n == 0) {
        body->wheels[3]->contactFrames = 0;
    }
    k = 2.0;
    slipF = -ABS(body->wheels[2]->brake) * SIGN_FLOAT(body->wheels[2]->spinRate) * k;
    slipF /= body->wheels[2]->radius;
    slipR = -ABS(body->wheels[0]->brake) * SIGN_FLOAT(body->wheels[0]->spinRate) * k;
    slipR /= body->wheels[0]->radius;
    m4 = body->mass / 4;
    slipF /= m4;
    slipR /= m4;
    slipF *= dt * dt;
    slipR *= dt * dt;
    if (ABS(slipF) > 1.0f) {
        slipF = SIGN_FLOAT(slipF) * 1.5f;
    }
    if (ABS(slipR) > 1.0f) {
        slipR = SIGN(slipR) * 1.5f;
    }
    pt[1] = pt[2] = 0.0f;
    pt[0] = ((f32 *)&body->wheels[0]->state.position)[0];
    GetBodyPointVelocity((Vec3f *)tmpA, body, (Vec3f *)pt);
    Mtx4fInverseRotate((Vec3f *)vA, body->orientation, (Vec3f *)tmpA);
    sA = body->wheels[0]->surface;
    sB = body->wheels[1]->surface;
    sC = body->wheels[2]->surface;
    sD = body->wheels[3]->surface;
    row = g_dwCurrentWeather - 1;
    if (row >= 3 || row < 0) {
        row = 0;
    }
    body->skidding = 0;
    row <<= 3;
    if ((body->wheels[0]->contactFrames != 0 || body->wheels[1]->contactFrames != 0) &&
        (body->wheels[2]->contactFrames != 0 || body->wheels[3]->contactFrames != 0)) {
        idx = ((sA + sB + 1) >> 1) + row;
        s = ABS(vA[1]) * body->mass / dt + ABS(*rearLoad);
        ran = 1;
        hold = 8000.0f;
        if (ABS(slipF) > 0.0001) {
            s += 10000.0f * (ABS(slipR) > 0.0001);
        } else {
            s += 100000.0f * (ABS(slipR) > 0.0001);
        }
        if (*rearSliding != 0) {
            hold = 5600.0f;
        }
        *rearSliding = 0;
        grip = g_afSurfaceGripFlat[idx];
        grip -= 0.002 * (body->tireType - 1);
        lo = g_afSurfaceSlipLoadFlat[idx];
        v = g_afSurfaceMaxLoadFlat[idx];
        g = s;
        if (v < s) {
            g = v;
        }
        if (g < lo) {
            g = lo;
        }
        g = lo / g * 20.0f * grip;
        if (body->wheels[2]->steerAngle == 0) {
            g = 1.5 * g;
        }
        if (ABS(g) > 1.0f) {
            g = 1;
        }
        if (s < hold) {
            side = vA[1] = 0.0f;
        } else {
            sp = sqrtf(((f32 *)&body->state.velocity)[0] * ((f32 *)&body->state.velocity)[0] +
                       ((f32 *)&body->state.velocity)[1] * ((f32 *)&body->state.velocity)[1] +
                       ((f32 *)&body->state.velocity)[2] * ((f32 *)&body->state.velocity)[2]);
            if (sp < 27.0f) {
                t = (27.0f - sp) * 0.1f / 27.0f;
                if (g < t) {
                    g = t;
                }
            }
            vA[1] = vA[1] - vA[1] * g;
            side = vA[1] * g;
            *rearSliding = 1;
        }
#ifdef __sgi
        {
            float fwd[3] = {1.0f, 0.0f, 0.0f};
        }
#endif
        {
            if (ABS(vA[0]) > 1.0f) {
                if (ABS(vA[1] / vA[0]) > 0.25f) {
                    body->skidding = 0x80;
                }
            } else if (ABS(vA[1]) > 1.0f) {
                body->skidding = 0x80;
            } else {
                body->skidding = 0;
            }
        }
        t = vA[0];
        vA[0] = vA[0] - slipR;
        if (ABS(vA[0]) > 1e-05f) {
            if (SIGN_FLOAT(vA[0]) != SIGN_FLOAT(t)) {
                vA[0] = 0.f;
            }
        }
    } else {
        *rearSliding = 0;
    }
    pt[0] = ((f32 *)&body->wheels[2]->state.position)[0];
    GetBodyPointVelocity((Vec3f *)tmpB, body, (Vec3f *)pt);
    Mtx4fInverseRotate((Vec3f *)vB, body->orientation, (Vec3f *)tmpB);
    if ((body->wheels[2]->contactFrames != 0 || body->wheels[3]->contactFrames != 0) &&
        (body->wheels[0]->contactFrames != 0 || body->wheels[1]->contactFrames != 0)) {
        float d;
        float save[3];
        float lat[3];
#ifdef __sgi
        int frontFrameReserve[8];
#endif

        ran = 1;
        pt[0] = cosf(body->wheels[2]->steerAngle);
        pt[1] = sinf(body->wheels[2]->steerAngle);
        pt[2] = 0.f;
        d = pt[0] * vB[0] + pt[1] * vB[1] + pt[2] * vB[2];
        save[0] = vB[0];
        save[1] = vB[1];
        save[2] = vB[2];
        vB[0] = pt[0] * d;
        vB[1] = pt[1] * d;
        vB[2] = pt[2] * d;
        lat[0] = save[0] - vB[0];
        lat[1] = save[1] - vB[1];
        lat[2] = save[2] - vB[2];
        sp = sqrtf(lat[0] * lat[0] + lat[1] * lat[1] + lat[2] * lat[2]);
        v = ABS(*frontLoad) + sp * body->mass / dt;
        g = 8000.0f;
        v += 10000.0f * (ABS(slipF) > 0.0001);
        if (*frontSliding != 0) {
            g = g * 0.7;
        }
        *frontSliding = 0;
        if (g < v) {
            idx = ((sC + sD + 1) >> 1) + row;
            grip = g_afSurfaceGripFlat[idx];
            grip -= 0.002f * (body->tireType - 1);
            lo = g_afSurfaceSlipLoadFlat[idx];
            s = g_afSurfaceMaxLoadFlat[idx];
            g = v;
            if (s < v) {
                g = s;
            }
            if (g < lo) {
                g = lo;
            }
            g = lo / g * 20.0f * grip;
            if (body->wheels[2]->steerAngle == 0.0f) {
                g = g * 1.5;
            }
            if (ABS(g) > 1.0f) {
                g = 1.0f;
            }
            sp = sqrtf(((f32 *)&body->state.velocity)[0] * ((f32 *)&body->state.velocity)[0] +
                       ((f32 *)&body->state.velocity)[1] * ((f32 *)&body->state.velocity)[1] +
                       ((f32 *)&body->state.velocity)[2] * ((f32 *)&body->state.velocity)[2]);
            if (sp < 27.0f) {
                t = (27.0f - sp) * 0.1f / 27.0f;
                if (g < t) {
                    g = t;
                }
            }
            lat[0] = lat[0] * g;
            lat[1] = lat[1] * g;
            lat[2] = lat[2] * g;
            vB[0] = save[0] - lat[0];
            vB[1] = save[1] - lat[1];
            vB[2] = save[2] - lat[2];
            *frontSliding = 1;
        }
    }
    if (ran != 0) {
        sv[0] = (vB[0] + vA[0]) / 2;
        sv[2] = (vA[1] - vB[1]) / (((f32 *)&body->wheels[0]->state.position)[0] -
                                   ((f32 *)&body->wheels[2]->state.position)[0]);
        sv[1] = vA[1] - ((f32 *)&body->wheels[0]->state.position)[0] * sv[2];
        Mtx4fInverseRotate((Vec3f *)w, body->orientation, &body->state.angularVelocity);
        w[2] = sv[2];
        Mtx3x3MulVec3f(&body->state.angularVelocity, body->orientation, (Vec3f *)w);
        Mtx4fInverseRotate((Vec3f *)w, body->orientation, &body->state.velocity);
        w[0] = sv[0];
        w[1] = sv[1];
        Mtx3x3MulVec3f(&body->state.velocity, body->orientation, (Vec3f *)w);
    }
    if (ABS(side) > 0.5f) {
        side = SIGN_BODY(side) * 0.5;
    }
    side /= 0.5f;
    side *= -4.0f;
    if (ABS(body->rollAngle - side) < LEAN_STEP) {
        body->rollAngle = side;
    } else if (body->rollAngle < side) {
        body->rollAngle = body->rollAngle + LEAN_STEP;
    } else {
        body->rollAngle = body->rollAngle - LEAN_STEP;
    }
}

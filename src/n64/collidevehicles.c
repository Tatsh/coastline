#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/physics.h"
#include "common/racerslot.h"
#include "common/rigidbody.h"
#include "common/vehicle.h"
#include "common/vehiclecount.h"

void CollideVehicles(void) {
    Vec3f d;
    int i;
    int j;
    RigidBodyState *pa;
    RigidBodyState *pb;
    float s;
    float mA[3][3];
    float mB[3][3];
    float mR[3][3];
    Vec3f dd;
    Vec3f t;
    Vec3f sd;
    float imp[3];

    for (i = 0; i < g_dwNumVehicles; i++) {
        if (g_aRacerSlots[i].vehicle != 0 && g_aRacerSlots[i].vehicle->ghostMode != 2) {
            g_aRacerSlots[i].vehicle->body.framesSinceVehicleImpact++;
            pa = &g_aRacerSlots[i].vehicle->body.state;
            for (j = i + 1; j < g_dwNumVehicles; j++) {
                if (g_aRacerSlots[j].vehicle != 0 && g_aRacerSlots[j].vehicle->ghostMode != 2) {
                    pb = &g_aRacerSlots[j].vehicle->body.state;
                    d.x = pa->position.x - pb->position.x;
                    d.y = pa->position.y - pb->position.y;
                    d.z = pa->position.z - pb->position.z;
#ifdef ENABLE_PATCHES
                    if (sqrtf(d.z * d.z + (d.x * d.x + d.y * d.y)) < 5.75f) {
#else
                    if (sqrtf(d.z * d.z + (d.x * d.x + d.y * d.y)) < 5.0f) {
#endif
                        Vec3f ext = {2.5f, 1.0f, 1.0f};
                        float x;

                        Transpose4x4Into3x3(mB, g_aRacerSlots[j].vehicle->body.orientation);
                        CopyMatrix4x4To3x3(mA, g_aRacerSlots[i].vehicle->body.orientation);
                        Mtx3fMultiply(mR, mB, mA);
                        dd.x = pa->position.x - pb->position.x;
                        dd.y = pa->position.y - pb->position.y;
                        dd.z = pa->position.z - pb->position.z;
                        Mtx3fMulVec3f(&t, mA, &dd);
                        if (TestObbIntersection(mR, &t, &ext, &ext) == 0) {
#ifdef ENABLE_PATCHES
                            continue;
#else
                            return;
#endif
                        }
                        Vec3fNormalizeFast(&d);
                        s = 0.0f;
                        s += d.z * pa->velocity.z + (pa->velocity.x * d.x + pa->velocity.y * d.y);
                        x = s;
                        s += pb->velocity.x * d.x + pb->velocity.y * d.y + pb->velocity.z * d.z;
                        s *= 0.5f;
                        imp[0] = d.x * s;
                        imp[1] = d.y * s;
                        imp[2] = d.z * s;
                        if (s < x) {
                            x = -(s - x);
                        } else {
                            x = s - x;
                        }
                        if (x > 27.0f) {
                            x = 27.0f;
                        }
                        if (g_aRacerSlots[i].vehicle->body.framesSinceVehicleImpact > 40) {
                            g_aRacerSlots[j].vehicle->body.vehicleImpactVolume =
                                g_aRacerSlots[i].vehicle->body.vehicleImpactVolume =
                                    127.0f * x / 27.0f + 128.0f;
                        }
                        g_aRacerSlots[i].vehicle->body.framesSinceVehicleImpact = 0;
                        sd.x = d.x * -1.0f;
                        sd.y = d.y * -1.0f;
                        sd.z = d.z * -1.0f;
                        t.x = sd.x * 2.5f;
                        t.y = sd.y * 2.5f;
                        t.z = sd.z * 2.5f;
                        Mtx4fInverseRotate(&dd, g_aRacerSlots[i].vehicle->body.orientation, &t);
                        pa->velocity.x = pa->velocity.x - imp[0];
                        pa->velocity.y = pa->velocity.y - imp[1];
                        pa->velocity.z = pa->velocity.z - imp[2];
                        memcpy(&g_aRacerSlots[i].vehicle->body.stepStates[1],
                               &g_aRacerSlots[i].vehicle->body.state,
                               sizeof(RigidBodyState));
                        ApplyCollisionImpulse(&g_aRacerSlots[i].vehicle->body, &dd, &d, 0, 0.45f);
                        memcpy(&g_aRacerSlots[i].vehicle->body.state,
                               &g_aRacerSlots[i].vehicle->body.stepStates[1],
                               sizeof(RigidBodyState));
                        g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.x =
                            g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.x + imp[0];
                        g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.y =
                            g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.y + imp[1];
                        g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.z =
                            g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.z + imp[2];
                        g_aRacerSlots[i].vehicle->body.stepStates[0].velocity.x =
                            g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.x;
                        g_aRacerSlots[i].vehicle->body.stepStates[0].velocity.y =
                            g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.y;
                        g_aRacerSlots[i].vehicle->body.stepStates[0].velocity.z =
                            g_aRacerSlots[i].vehicle->body.stepStates[1].velocity.z;
                        pa->velocity.x = imp[0] + pa->velocity.x;
                        pa->velocity.y = imp[1] + pa->velocity.y;
                        pa->velocity.z = imp[2] + pa->velocity.z;
                        t.x = d.x * 2.5f;
                        t.y = d.y * 2.5f;
                        t.z = d.z * 2.5f;
                        Mtx4fInverseRotate(&dd, g_aRacerSlots[j].vehicle->body.orientation, &t);
                        pb->velocity.x = pb->velocity.x - imp[0];
                        pb->velocity.y = pb->velocity.y - imp[1];
                        pb->velocity.z = pb->velocity.z - imp[2];
                        memcpy(&g_aRacerSlots[j].vehicle->body.stepStates[1],
                               &g_aRacerSlots[j].vehicle->body.state,
                               sizeof(RigidBodyState));
                        ApplyCollisionImpulse(&g_aRacerSlots[j].vehicle->body, &dd, &sd, 0, 0.45f);
                        memcpy(&g_aRacerSlots[j].vehicle->body.state,
                               &g_aRacerSlots[j].vehicle->body.stepStates[1],
                               sizeof(RigidBodyState));
                        g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.x =
                            g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.x + imp[0];
                        g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.y =
                            g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.y + imp[1];
                        g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.z =
                            g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.z + imp[2];
                        g_aRacerSlots[j].vehicle->body.stepStates[0].velocity.x =
                            g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.x;
                        g_aRacerSlots[j].vehicle->body.stepStates[0].velocity.y =
                            g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.y;
                        g_aRacerSlots[j].vehicle->body.stepStates[0].velocity.z =
                            g_aRacerSlots[j].vehicle->body.stepStates[1].velocity.z;
                        pb->velocity.x = imp[0] + pb->velocity.x;
                        pb->velocity.y = imp[1] + pb->velocity.y;
                        pb->velocity.z = imp[2] + pb->velocity.z;
                    }
                }
            }
        }
    }
}

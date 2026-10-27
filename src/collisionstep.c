#include "physics.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <string.h>

#include "screen.h"
#define osSyncPrintf(...) NullStub()
#define COLLISION_PRODUCT(a, b) ((long double)(a) * (b))
#define COLLISION_LESS(a, b) (!((a) >= (b)))
TrackTriangle *g_pCollisionTriangle;
#else
#include <PR/os.h>
#include <PR/os_libc.h>
#define COLLISION_PRODUCT(a, b) ((a) * (b))
#define COLLISION_LESS(a, b) ((a) < (b))
#endif

#include "game.h"
#include "timing.h"
#include "trianglebox.h"
#include "vehicle.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define AddTimingMarker(...) NullStub()
#endif

#define ABS(x) ((x) < 0.0f ? -(x) : (x))

/* The steps of the collisions in a frame. */
#define COLLISION_STEPS 4

/* The frames a vehicle counts down from while it is upside down. */
#define UPRIGHT_FRAMES 35

/* The height of the body above which a collision does not hold it. */
#define UPRIGHT_LIMIT 0.5f

/* The original passes the spin flag to the bounce without setting it outside the demonstration. */
#pragma GCC diagnostic push
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#pragma GCC diagnostic ignored "-Wuninitialized"
/* The races of the demonstration that sort their contacts with the track: every one in the USA
 * release, the ones under the credits in the Japanese release, and none in the European release. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define SORTS_CONTACTS() (g_dwGameMode == GAME_MODE_DEMO)
#elif TGR_REGION == TGR_REGION_JP
#define SORTS_CONTACTS() (g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType)
#else
#define SORTS_CONTACTS() (g_dwGameMode == GAME_MODE_DEMO)
#endif

s32 CollideBodyWithTriangles(RigidBody *body, f32 box[4][4]) {
    CollisionTriangleNode *node;
    TrackTriangle *triangle;
    Vec3f corners[3], normal, along, half, back;
    f32 distance;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highPush, highBack[3];
#else
    f32 push;
#endif
    s32 resist, spin, moved;
    s32 hit = 0;
    s16 hits = 0;

    for (node = g_pCollisionTriangleList; node != NULL; node = node->next) {
        triangle = node->triangle;
        TriangleInBox(corners, &normal, box, triangle);
        if (!IsTriangleInUnitCube(corners, &normal)) {
            continue;
        }
        Vec3fNormalizeFast(&normal);
        resist = 1;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        distance = (COLLISION_PRODUCT(normal.x, corners[0].x) +
                    COLLISION_PRODUCT(normal.y, corners[0].y)) +
                   COLLISION_PRODUCT(normal.z, corners[0].z);
#else
        distance = corners[0].z * normal.z + (normal.x * corners[0].x + normal.y * corners[0].y);
#endif
        g_dwCollisionContactType = 0;
        hits++;
#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION != TGR_REGION_EU
        if (SORTS_CONTACTS()) {
            if (ABS(normal.x) > 0.999f || ABS(normal.y) > 0.999f || ABS(normal.z) > 0.999f) {
                g_dwCollisionContactType = 1;
                spin = 1;
                osSyncPrintf("Wank CT1 case\n");
            } else if (COLLISION_LESS(ABS(distance), 0.5f)) {
                osSyncPrintf("Triangle Edge to CubeFace\n");
                spin = 0;
                g_dwCollisionContactType = 2;
            }
        }
#endif
        osSyncPrintf("Cube Edge to Triangle Face\n");
        /* The corner of the box towards the triangle. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        along.z = normal.z * distance;
        half.x = (f32)(COLLISION_LESS(COLLISION_PRODUCT(normal.x, distance), 0.0f) ? -1 : 1) * 0.5f;
        half.y = (f32)(COLLISION_LESS(COLLISION_PRODUCT(normal.y, distance), 0.0f) ? -1 : 1) * 0.5f;
        half.z = (f32)(COLLISION_LESS(along.z, 0.0f) ? -1 : 1) * 0.5f;
#else
        along.x = normal.x * distance;
        along.y = normal.y * distance;
        along.z = normal.z * distance;
        half.x = (f32)(along.x < 0.0f ? -1 : 1) * 0.5f;
        half.y = (f32)(along.y < 0.0f ? -1 : 1) * 0.5f;
        half.z = (f32)(along.z < 0.0f ? -1 : 1) * 0.5f;
#endif
        g_vCollisionContact.x = half.x * body->boxSize.x;
        g_vCollisionContact.y = half.y * body->boxSize.y;
        g_vCollisionContact.z = half.z * body->boxSize.z + body->boxCenterZ;
        g_pCollisionTriangle = triangle;
        ComputeCollisionPenetration(body, &normal, distance, &half, corners);
        if (UPRIGHT_LIMIT < body->orientation[2][2]) {
            resist = 0;
        }
        if (resist) {
            osSyncPrintf("Resistive collision %10.3f\n", (f64)body->orientation[2][2]);
        }
        if (g_dwCollisionContactType == 1) {
            moved = BounceOffPlane(body, &g_pCollisionTriangle->normal, resist, spin);
        } else {
            moved = ApplyCollisionImpulse(
                body, &g_vCollisionContact, &g_pCollisionTriangle->normal, resist, 0.0f);
        }
        if (moved) {
            hit = 1;
            /* Move the step back out along the normal by a tenth more than it moved in. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            highBack[0] =
                (long double)body->stepStates[1].position.x - body->stepStates[0].position.x;
            highBack[1] =
                (long double)body->stepStates[1].position.y - body->stepStates[0].position.y;
            highBack[2] =
                (long double)body->stepStates[1].position.z - body->stepStates[0].position.z;
            highPush = (triangle->normal.x * highBack[0] +
                        (triangle->normal.y * highBack[1] + triangle->normal.z * highBack[2])) *
                       1.1f;
            back.y = triangle->normal.y * highPush;
            back.z = triangle->normal.z * highPush;
            body->stepStates[1].position.x =
                body->stepStates[1].position.x - triangle->normal.x * highPush;
            body->stepStates[1].position.y = body->stepStates[1].position.y - back.y;
            body->stepStates[1].position.z = body->stepStates[1].position.z - back.z;
#else
            back.x = body->stepStates[1].position.x - body->stepStates[0].position.x;
            back.y = body->stepStates[1].position.y - body->stepStates[0].position.y;
            back.z = body->stepStates[1].position.z - body->stepStates[0].position.z;
            push = (f32)((f64)(triangle->normal.z * back.z +
                               (back.x * triangle->normal.x + back.y * triangle->normal.y)) *
                         1.1);
            back.x = triangle->normal.x * push;
            back.y = triangle->normal.y * push;
            back.z = triangle->normal.z * push;
            body->stepStates[1].position.x = body->stepStates[1].position.x - back.x;
            body->stepStates[1].position.y = body->stepStates[1].position.y - back.y;
            body->stepStates[1].position.z = body->stepStates[1].position.z - back.z;
#endif
            /* The original takes back three of the four parts of the orientation. */
            body->stepStates[1].orientation.w = body->stepStates[0].orientation.w;
            body->stepStates[1].orientation.y = body->stepStates[0].orientation.y;
            body->stepStates[1].orientation.x = body->stepStates[0].orientation.x;
            ComputeDeltaQuaternion(&body->stepStates[1]);
            QuaternionToMatrix(body->orientation, &body->stepStates[1]);
        }
    }
    if (hits) {
        body->framesSinceImpact = 0;
    } else if (body->framesSinceImpact < 40) {
        body->framesSinceImpact++;
    }
    return hit;
}
#pragma GCC diagnostic pop

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void StepVehicleCollisions(Vehicle *vehicle, RigidBody *body) {
#else
void StepVehicleCollisions(RigidBody *body) {
#endif
    f32 box[4][4];
    Vec3f scale;
    /* The original reserves stack for twelve bytes of locals it does not use. */
    f32 unused[3];
    f32 remaining, step;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highRemaining, dx, dy, dz, displacement;
    f32 storedDx, storedDz;
#endif
    /* The count of the steps is a variable, and the original divides by it at run time. */
    f32 steps;

    (void)unused;
    g_pCollisionTriangleList = NULL;
    g_pNextCollisionTriangleNode = g_aCollisionTriangleNodes;
    g_vCollisionContact.x = 0.0f;
    g_vCollisionContact.y = 0.0f;
    g_vCollisionContact.z = 0.0f;
    g_vCollisionPenetration.x = 0.0f;
    g_vCollisionPenetration.y = 0.0f;
    g_vCollisionPenetration.z = 0.0f;
    g_dwCollisionContactType = 0;
    g_pCollisionTriangle = NULL;
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    /* The triangles near the body, in a box ten times its size. */
    scale.y = 0.1f;
    scale.x = 0.1f;
    scale.z = 0.1f;
    MtxBuildScaledInverse(body->orientation, box, &scale);
    CollectTrianglesInBox(body, box);
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    scale.x = 1.0f / body->boxSize.x;
    scale.y = 1.0f / body->boxSize.y;
    scale.z = 1.0f / body->boxSize.z;
    steps = COLLISION_STEPS;
    remaining = 1.0f / 30.0f;
    if (0.002f < remaining) {
        step = 1.0f / 30.0f / steps;
        do {
            if (TipBalancedVehicle(body)) {
                osSyncPrintf("Standing on it's F'in Nose damnit\n");
            }
            CollideVehicles();
            IntegrateRigidBodyState(&body->stepStates[1], &body->stepStates[0], step);
            QuaternionToMatrix(body->orientation, &body->stepStates[1]);
            MtxBuildScaledInverse(body->orientation, box, &scale);
            box[3][2] = box[3][2] - body->boxCenterZ;
            if (CollideBodyWithTriangles(body, box)) {
                ComputeDeltaQuaternion(&body->stepStates[1]);
                QuaternionToMatrix(body->orientation, &body->stepStates[1]);
            }
            memcpy(&body->stepStates[0], &body->stepStates[1], sizeof(RigidBodyState));
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            highRemaining = (long double)remaining - step;
            remaining = highRemaining;
        } while (0.002f < highRemaining);
#else
            remaining = remaining - step;
        } while (0.002f < remaining);
#endif
    }
    QuaternionToMatrix(body->orientation, &body->stepStates[1]);
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    memcpy(&body->stepStates[0], &body->stepStates[1], sizeof(RigidBodyState));
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (vehicle->driver != AiVehicleControllerCallback) {
        if (COLLISION_LESS(body->orientation[2][2], UPRIGHT_LIMIT)) {
            if (body->uprightFrames < 0) {
                body->uprightFrames = -1;
            }
            dx = (long double)body->orientation[3][0] - vehicle->previousCollisionPosition.x;
            dy = (long double)body->orientation[3][1] - vehicle->previousCollisionPosition.y;
            dz = (long double)body->orientation[3][2] - vehicle->previousCollisionPosition.z;
            storedDx = dx;
            storedDz = dz;
            displacement = (dx * storedDx + dy * dy) + dz * storedDz;
            if (COLLISION_LESS(displacement, 1.0f)) {
                body->uprightFrames--;
            }
        } else {
            body->uprightFrames = 120;
        }
    } else
#endif
    {
        if (COLLISION_LESS(body->orientation[2][2], UPRIGHT_LIMIT)) {
            if (body->uprightFrames < 0) {
                body->uprightFrames = -1;
            }
            body->uprightFrames--;
        } else {
            body->uprightFrames = UPRIGHT_FRAMES;
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    vehicle->previousCollisionPosition.x = body->orientation[3][0];
    vehicle->previousCollisionPosition.y = body->orientation[3][1];
    vehicle->previousCollisionPosition.z = body->orientation[3][2];
#endif
}

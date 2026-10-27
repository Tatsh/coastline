#include "game.h"
#include "physics.h"
#include "physicsimpact.h"
#include "racerslotaccess.h"
#include "timing.h"
#include "trianglebox.h"
#include "vehicle.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <math.h>
#include <string.h>

#include "platform/win/settings.h"
#include "screen.h"
#else
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN

#define osSyncPrintf(...) NullStub()
#define COLLISION_PRODUCT(a, b) ((long double)(a) * (b))
#define COLLISION_LESS(a, b) (!((a) >= (b)))
TrackTriangle *g_pCollisionTriangle;
#else
#define COLLISION_PRODUCT(a, b) ((a) * (b))
#define COLLISION_LESS(a, b) ((a) < (b))
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define AddTimingMarker(...) NullStub()
#endif

#define ABS(x) ((x) < 0.0f ? -(x) : (x))

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define COLLISION_DISTANCE_TYPE long double
#define COLLISION_SIGN(x) (!((x) < 0.0f) && !((x) > 0.0f) ? 0.0L : ((x) > 0.0f ? 1.0L : -1.0L))
#define COLLISION_NEAREST(a, b) ((a) >= (b))
#else
#define COLLISION_DISTANCE_TYPE f32
#define SIGN(x) ((x) == 0.0f ? 0.0 : (0.0f < (x) ? 1.0 : -1.0))
#define COLLISION_NEAREST(a, b) (!((a) < (b)))
#endif

/* The distance of a point from the ground of a wheel, either side of it. */
static inline COLLISION_DISTANCE_TYPE GroundDistance(RigidBody *wheel, Vec3f *point) {
    if (COLLISION_LESS(PlaneDistance(&wheel->groundNormal, wheel->groundOffset, point), 0.0f)) {
        return -PlaneDistance(&wheel->groundNormal, wheel->groundOffset, point);
    }
    return PlaneDistance(&wheel->groundNormal, wheel->groundOffset, point);
}

/* A touching wheel initialises the final corner and wheel used after the loop. */
#pragma GCC diagnostic push
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#pragma GCC diagnostic ignored "-Wuninitialized"
s32 TipBalancedVehicle(RigidBody *body) {
    RigidBody *wheel;
    Vec3f corner, point, velocity, nudge;
    f32 nearest, along;
    COLLISION_DISTANCE_TYPE distance, tilt;
    s32 touching, i;
    Vec3f *spin;

    QuaternionToMatrix(body->orientation, &body->stepStates[0]);
    nearest = 100.0f;
    touching = 0;
    /* The nearest of the corners of the box over the wheels on the ground to their ground. */
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        if (!body->wheels[i]->contactFrames) {
            continue;
        }
        touching++;
        wheel = body->wheels[i];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        corner.x = (f32)(COLLISION_SIGN(wheel->state.position.x) *
                         COLLISION_PRODUCT(body->boxSize.x, 0.5f));
        corner.y = (f32)(COLLISION_SIGN(wheel->state.position.y) *
                         COLLISION_PRODUCT(body->boxSize.y, 0.5f));
        corner.z = body->boxCenterZ - COLLISION_PRODUCT(body->boxSize.z, 0.5f);
#else
        corner.x = (f32)(SIGN(wheel->state.position.x) * (f64)(body->boxSize.x * 0.5f));
        corner.y = (f32)(SIGN(wheel->state.position.y) * (f64)(body->boxSize.y * 0.5f));
        corner.z = -body->boxSize.z * 0.5f + body->boxCenterZ;
#endif
        Mtx4x4TransformPoint(&point, body->orientation, &corner);
        distance = GroundDistance(wheel, &point);
        if (COLLISION_NEAREST(nearest, distance)) {
            nearest = distance;
        }
    }
    if (touching >= 3 || touching <= 0) {
        return 0;
    }
    osSyncPrintf("-- Stand Dist = %f\n", (f64)nearest);
    if ((f64)nearest > 0.6) {
        return 0;
    }
    GetBodyPointVelocity(&velocity, body, &corner);
    along = COLLISION_PRODUCT(wheel->groundNormal.z, velocity.z) +
            (COLLISION_PRODUCT(velocity.x, wheel->groundNormal.x) +
             COLLISION_PRODUCT(velocity.y, wheel->groundNormal.y));
    osSyncPrintf("-- Stand Vel = %f\n", (f64)along);
    if (ABS(along) > 0.1f) {
        return 0;
    }
    /* Roll it towards the way its nose leans across the ground. */
    tilt = COLLISION_PRODUCT(body->groundNormal.z, body->orientation[0][2]) +
           (COLLISION_PRODUCT(body->orientation[0][0], body->groundNormal.x) +
            COLLISION_PRODUCT(body->orientation[0][1], body->groundNormal.y));
    if (0.0f < tilt) {
        nudge.x = 0.0f;
        nudge.y = 0.1f;
        nudge.z = 0.0f;
    } else {
        nudge.x = 0.0f;
        nudge.y = -0.1f;
        nudge.z = 0.0f;
    }
    osSyncPrintf("-- Stand Point = %f\n", (f64)tilt);
    Mtx3x3MulVec3f(&point, body->orientation, &nudge);
    spin = &body->stepStates[0].angularVelocity;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    spin->x = spin->x - COLLISION_PRODUCT(point.x, -2.0f);
    spin->y = spin->y - COLLISION_PRODUCT(point.y, -2.0f);
    spin->z = spin->z - COLLISION_PRODUCT(point.z, -2.0f);
#else
    for (i = 0; i < 2; i++) {
        spin->x = point.x + spin->x;
        spin->y = point.y + spin->y;
        spin->z = point.z + spin->z;
    }
#endif
    ComputeDeltaQuaternion(&body->stepStates[0]);
    return 1;
}
#pragma GCC diagnostic pop

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* The x87 reduction order differs between face axes and each group of edge axes. */
#define OBB_ABS(x) (!((x) >= 0.0f) ? -(x) : (x))
#define OBB_LEQ(a, b) (!((a) > (b)))
#define OBB_SUM3(base, a, b, c) ((((c) + (b)) + (a)) + (base))
#define OBB_DOT0(a, b, c) (((c) + (a)) + (b))
#define OBB_DOT1(a, b, c) (((a) + (c)) + (b))
#define OBB_SUM4_X(a, b, c, d) ((((a) + (b)) + (d)) + (c))
#define OBB_SUM4_YZ(a, b, c, d) ((((b) + (d)) + (c)) + (a))
#else
#define OBB_ABS(x) ABS(x)
#define OBB_LEQ(a, b) ((a) <= (b))
#define OBB_SUM3(base, a, b, c) ((((base) + (a)) + (b)) + (c))
#define OBB_DOT0(a, b, c) (((a) + (b)) + (c))
#define OBB_DOT1(a, b, c) (((a) + (b)) + (c))
#define OBB_SUM4_X(a, b, c, d) ((((a) + (b)) + (c)) + (d))
#define OBB_SUM4_YZ(a, b, c, d) ((((a) + (b)) + (c)) + (d))
#endif

s32 TestObbIntersection(f32 rotation[3][3], Vec3f *offset, Vec3f *halfA, Vec3f *halfB) {
    f32 r[3][3];
    f32(*m)[3] = rotation;
    f32 tx = offset->x, ty = offset->y, tz = offset->z;
    f32 ax = halfA->x, ay = halfA->y, az = halfA->z;
    f32 bx = halfB->x, by = halfB->y, bz = halfB->z;
    s32 overlap = 1;

    r[0][0] = OBB_ABS(m[0][0]);
    r[0][1] = OBB_ABS(m[0][1]);
    r[0][2] = OBB_ABS(m[0][2]);
    r[1][0] = OBB_ABS(m[1][0]);
    r[1][1] = OBB_ABS(m[1][1]);
    r[1][2] = OBB_ABS(m[1][2]);
    r[2][0] = OBB_ABS(m[2][0]);
    r[2][1] = OBB_ABS(m[2][1]);
    r[2][2] = OBB_ABS(m[2][2]);
    /* The axes of the two boxes, in the order of the original. */
    overlap &= OBB_LEQ(OBB_ABS(tx),
                       OBB_SUM3(ax,
                                COLLISION_PRODUCT(bx, r[0][0]),
                                COLLISION_PRODUCT(by, r[0][1]),
                                COLLISION_PRODUCT(r[0][2], bz)));
    overlap &= OBB_LEQ(OBB_ABS(OBB_DOT0(COLLISION_PRODUCT(tx, m[0][0]),
                                        COLLISION_PRODUCT(ty, m[1][0]),
                                        COLLISION_PRODUCT(m[2][0], tz))),
                       OBB_SUM3(bx,
                                COLLISION_PRODUCT(ax, r[0][0]),
                                COLLISION_PRODUCT(ay, r[1][0]),
                                COLLISION_PRODUCT(r[2][0], az)));
    overlap &= OBB_LEQ(OBB_ABS(ty),
                       OBB_SUM3(ay,
                                COLLISION_PRODUCT(bx, r[1][0]),
                                COLLISION_PRODUCT(by, r[1][1]),
                                COLLISION_PRODUCT(r[1][2], bz)));
    overlap &= OBB_LEQ(OBB_ABS(tz),
                       OBB_SUM3(az,
                                COLLISION_PRODUCT(bx, r[2][0]),
                                COLLISION_PRODUCT(by, r[2][1]),
                                COLLISION_PRODUCT(r[2][2], bz)));
    overlap &= OBB_LEQ(OBB_ABS(OBB_DOT1(COLLISION_PRODUCT(tx, m[0][1]),
                                        COLLISION_PRODUCT(ty, m[1][1]),
                                        COLLISION_PRODUCT(m[2][1], tz))),
                       OBB_SUM3(by,
                                COLLISION_PRODUCT(ax, r[0][1]),
                                COLLISION_PRODUCT(ay, r[1][1]),
                                COLLISION_PRODUCT(r[2][1], az)));
    overlap &= OBB_LEQ(OBB_ABS(OBB_DOT1(COLLISION_PRODUCT(tx, m[0][2]),
                                        COLLISION_PRODUCT(ty, m[1][2]),
                                        COLLISION_PRODUCT(m[2][2], tz))),
                       OBB_SUM3(bz,
                                COLLISION_PRODUCT(ax, r[0][2]),
                                COLLISION_PRODUCT(ay, r[1][2]),
                                COLLISION_PRODUCT(r[2][2], az)));
    /* The products of their edges. */
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(tz, m[1][0]) - COLLISION_PRODUCT(m[2][0], ty)),
                       OBB_SUM4_X(COLLISION_PRODUCT(ay, r[2][0]),
                                  COLLISION_PRODUCT(az, r[1][0]),
                                  COLLISION_PRODUCT(by, r[0][2]),
                                  COLLISION_PRODUCT(bz, r[0][1])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(tz, m[1][1]) - COLLISION_PRODUCT(m[2][1], ty)),
                       OBB_SUM4_X(COLLISION_PRODUCT(ay, r[2][1]),
                                  COLLISION_PRODUCT(az, r[1][1]),
                                  COLLISION_PRODUCT(bx, r[0][2]),
                                  COLLISION_PRODUCT(bz, r[0][0])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(tz, m[1][2]) - COLLISION_PRODUCT(m[2][2], ty)),
                       OBB_SUM4_X(COLLISION_PRODUCT(ay, r[2][2]),
                                  COLLISION_PRODUCT(az, r[1][2]),
                                  COLLISION_PRODUCT(bx, r[0][1]),
                                  COLLISION_PRODUCT(by, r[0][0])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(tx, m[2][0]) - COLLISION_PRODUCT(m[0][0], tz)),
                       OBB_SUM4_YZ(COLLISION_PRODUCT(ax, r[2][0]),
                                   COLLISION_PRODUCT(az, r[0][0]),
                                   COLLISION_PRODUCT(by, r[1][2]),
                                   COLLISION_PRODUCT(bz, r[1][1])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(tx, m[2][1]) - COLLISION_PRODUCT(m[0][1], tz)),
                       OBB_SUM4_YZ(COLLISION_PRODUCT(ax, r[2][1]),
                                   COLLISION_PRODUCT(az, r[0][1]),
                                   COLLISION_PRODUCT(bx, r[1][2]),
                                   COLLISION_PRODUCT(bz, r[1][0])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(tx, m[2][2]) - COLLISION_PRODUCT(m[0][2], tz)),
                       OBB_SUM4_YZ(COLLISION_PRODUCT(ax, r[2][2]),
                                   COLLISION_PRODUCT(az, r[0][2]),
                                   COLLISION_PRODUCT(bx, r[1][1]),
                                   COLLISION_PRODUCT(by, r[1][0])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(ty, m[0][0]) - COLLISION_PRODUCT(m[1][0], tx)),
                       OBB_SUM4_YZ(COLLISION_PRODUCT(ax, r[1][0]),
                                   COLLISION_PRODUCT(ay, r[0][0]),
                                   COLLISION_PRODUCT(by, r[2][2]),
                                   COLLISION_PRODUCT(bz, r[2][1])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(ty, m[0][1]) - COLLISION_PRODUCT(m[1][1], tx)),
                       OBB_SUM4_YZ(COLLISION_PRODUCT(ax, r[1][1]),
                                   COLLISION_PRODUCT(ay, r[0][1]),
                                   COLLISION_PRODUCT(bx, r[2][2]),
                                   COLLISION_PRODUCT(bz, r[2][0])));
    overlap &= OBB_LEQ(OBB_ABS(COLLISION_PRODUCT(ty, m[0][2]) - COLLISION_PRODUCT(m[1][2], tx)),
                       OBB_SUM4_YZ(COLLISION_PRODUCT(ax, r[1][2]),
                                   COLLISION_PRODUCT(ay, r[0][2]),
                                   COLLISION_PRODUCT(bx, r[2][1]),
                                   COLLISION_PRODUCT(by, r[2][0])));
    return overlap;
}

/* Share collision speed before applying the impulse. */
static inline void
PushVehicle(RacerSlot *slot, RigidBodyState *state, Vec3f *shared, Vec3f *towards, Vec3f *normal) {
    Vec3f arm, local;

    /* The original reads the vehicle from its slot afresh at each use. */
    arm.x = towards->x * 2.5f;
    arm.y = towards->y * 2.5f;
    arm.z = towards->z * 2.5f;
    Mtx4fInverseRotate(&local, VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).orientation, &arm);
    state->velocity.x = state->velocity.x - shared->x;
    state->velocity.y = state->velocity.y - shared->y;
    state->velocity.z = state->velocity.z - shared->z;
    memcpy(&VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1],
           &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).state,
           sizeof(RigidBodyState));
    ApplyCollisionImpulse(
        &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body), &local, normal, 0, 0.45f);
    memcpy(&VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).state,
           &VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1],
           sizeof(RigidBodyState));
    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.x =
        shared->x + VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.x;
    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.y =
        shared->y + VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.y;
    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.z =
        shared->z + VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.z;
    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[0].velocity.x =
        VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.x;
    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[0].velocity.y =
        VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.y;
    VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[0].velocity.z =
        VEHICLE_GET(RACER_SLOT_GET(slot, Vehicle), Body).stepStates[1].velocity.z;
    state->velocity.x = shared->x + state->velocity.x;
    state->velocity.y = shared->y + state->velocity.y;
    state->velocity.z = shared->z + state->velocity.z;
}

void CollideVehicles(void) {
    RigidBodyState *sa, *sb;
    Vec3f apart, offset, local, extents, shared, back;
    f32 transposeB[3][3], rotationA[3][3], relative[3][3];
    COLLISION_DISTANCE_TYPE speedA, speedB, mean, closing;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double highApartZ;
    f32 square;
#endif
    u8 volume;
    s32 i, j;

    /* The original reads each vehicle from its slot afresh at each use. */
    for (i = 0; i < g_dwNumVehicles; i++) {
        if (RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle) == NULL ||
            VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), GhostMode) == VEHICLE_GHOST) {
            continue;
        }
        VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body).framesSinceVehicleImpact++;
        sa = &VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body).state;
        for (j = i + 1; j < g_dwNumVehicles; j++) {
            if (RACER_SLOT_GET(&(g_aRacerSlots[j]), Vehicle) == NULL ||
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[j]), Vehicle), GhostMode) ==
                    VEHICLE_GHOST) {
                continue;
            }
            sb = &VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[j]), Vehicle), Body).state;
            apart.x = sa->position.x - sb->position.x;
            apart.y = sa->position.y - sb->position.y;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            highApartZ = (long double)sa->position.z - sb->position.z;
            apart.z = highApartZ;
            square = highApartZ * apart.z +
                     (COLLISION_PRODUCT(apart.x, apart.x) + COLLISION_PRODUCT(apart.y, apart.y));
            if (!COLLISION_LESS(sqrtl(square), 5.0f)) {
#else
            apart.z = sa->position.z - sb->position.z;
            if (!(sqrtf(apart.z * apart.z + (apart.x * apart.x + apart.y * apart.y)) < 5.0f)) {
#endif
                continue;
            }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            extents.x = 2.5f;
            extents.y = 1.0f;
            extents.z = 1.0f;
#else
            extents = g_vVehicleObbHalfExtents;
#endif
            Transpose4x4Into3x3(
                transposeB,
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[j]), Vehicle), Body).orientation);
            CopyMatrix4x4To3x3(
                rotationA,
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body).orientation);
            Mtx3fMultiply(relative, transposeB, rotationA);
            offset.x = sa->position.x - sb->position.x;
            offset.y = sa->position.y - sb->position.y;
            offset.z = sa->position.z - sb->position.z;
            Mtx3fMulVec3f(&local, rotationA, &offset);
            if (!TestObbIntersection(relative, &local, &extents, &extents)) {
                return;
            }
            /* They share the mean of their speeds along the line between them. */
            Vec3fNormalizeFast(&apart);
            speedA = COLLISION_PRODUCT(apart.z, sa->velocity.z) +
                     (COLLISION_PRODUCT(sa->velocity.x, apart.x) +
                      COLLISION_PRODUCT(sa->velocity.y, apart.y));
            speedB = OBB_DOT1(COLLISION_PRODUCT(sb->velocity.x, apart.x),
                              COLLISION_PRODUCT(sb->velocity.y, apart.y),
                              COLLISION_PRODUCT(sb->velocity.z, apart.z));
            mean = (speedA + speedB) * 0.5f;
            shared.x = apart.x * mean;
            shared.y = apart.y * mean;
            shared.z = apart.z * mean;
            closing = COLLISION_LESS(mean, speedA) ? -(mean - speedA) : mean - speedA;
            if (MAX_IMPACT_SPEED < closing) {
                closing = MAX_IMPACT_SPEED;
            }
            if (VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body)
                    .framesSinceVehicleImpact >= 41) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                volume = (u32)(128.0L - closing * -4.70370388031005859375f);
#else
                volume = (u32)(127.0f * closing / MAX_IMPACT_SPEED + 128.0f);
#endif
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body)
                    .vehicleImpactVolume = volume;
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[j]), Vehicle), Body)
                    .vehicleImpactVolume = volume;
            }
            VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body)
                .framesSinceVehicleImpact = 0;
            back.x = apart.x * -1.0f;
            back.y = apart.y * -1.0f;
            back.z = apart.z * -1.0f;
            PushVehicle(&g_aRacerSlots[i], sa, &shared, &back, &apart);
            PushVehicle(&g_aRacerSlots[j], sb, &shared, &apart, &back);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            if (g_dwNetworkPlayMode) {
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body)
                    .stepStates[0]
                    .velocity.x = sa->velocity.x;
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body)
                    .stepStates[0]
                    .velocity.y = sa->velocity.y;
                VEHICLE_GET(RACER_SLOT_GET(&(g_aRacerSlots[i]), Vehicle), Body)
                    .stepStates[0]
                    .velocity.z = sa->velocity.z;
            }
#endif
        }
    }
}

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
    if (VEHICLE_GET(vehicle, Driver) != AiVehicleControllerCallback) {
        if (COLLISION_LESS(body->orientation[2][2], UPRIGHT_LIMIT)) {
            if (body->uprightFrames < 0) {
                body->uprightFrames = -1;
            }
            dx = (long double)body->orientation[3][0] -
                 VEHICLE_GET(vehicle, PreviousCollisionPosition).x;
            dy = (long double)body->orientation[3][1] -
                 VEHICLE_GET(vehicle, PreviousCollisionPosition).y;
            dz = (long double)body->orientation[3][2] -
                 VEHICLE_GET(vehicle, PreviousCollisionPosition).z;
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
    VEHICLE_GET(vehicle, PreviousCollisionPosition).x = body->orientation[3][0];
    VEHICLE_GET(vehicle, PreviousCollisionPosition).y = body->orientation[3][1];
    VEHICLE_GET(vehicle, PreviousCollisionPosition).z = body->orientation[3][2];
#endif
}

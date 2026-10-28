/* Host check of src/physics.c, linked with src/rigidbody.c for the arithmetic of the bodies. */
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "bodyforces.c"
#include "collisionstep.c"
#include "physics.c"
#include "sidegrip.c"

/* The globals the file reads and writes. */
TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];
u16 g_awTriCacheSectorId[TRI_CACHE_SLOTS], g_awTriCacheTriCount[TRI_CACHE_SLOTS];
u32 g_adwTriCacheLruStamp[TRI_CACHE_SLOTS], g_dwTriCacheLruCounter;
CollisionTriangleNode g_aCollisionTriangleNodes[COLLISION_TRIANGLE_NODES];
CollisionTriangleNode *g_pCollisionTriangleList, *g_pNextCollisionTriangleNode;
s32 g_dwCollisionContactType, g_bTrackCollisionFrameToggle;
TrackTriangle *g_pCollisionTriangle;
f32 g_afSurfaceGrip[GRIP_WEATHERS][TRACK_SURFACES],
    g_afSurfaceMaxLoad[GRIP_WEATHERS][TRACK_SURFACES],
    g_afSurfaceSlipLoad[GRIP_WEATHERS][TRACK_SURFACES];
Vec3f g_vecUnitX = {1, 0, 0}, g_vecUnitY = {0, 1, 0}, g_vecUnitZ = {0, 0, 1},
      g_vecNegUnitZ = {0, 0, -1};
Vec3f g_vVehicleObbHalfExtents = {2.5f, 1.0f, 1.0f};
Vec3f g_vCollisionContact, g_vCollisionPenetration;
f32 g_fCollisionContactW;
u32 g_dwCurrentWeather;
s32 g_dwGameMode;
TrackHeader g_LoadedTrack;
RacerSlot g_aRacerSlots[RACER_SLOTS];
s32 g_dwNumVehicles;

static int prints, markers;
void osSyncPrintf(const char *format, ...) {
    prints++;
}
void AddTimingMarker(s32 bar, s32 r, s32 g, s32 b, s32 a) {
    markers++;
}

/* A grid of one cell that lists the triangles of the track. */
static u16 cellTriangles[8];
static s32 cellCount, lookups;
u32 LookupTrackGridCell(f32 x, f32 y) {
    lookups++;
    return cellCount ? (u32)cellCount << 16 : 0;
}
u16 NextGridCellTriangle(TrackGridCursor *cursor) {
    if (cursor->count == 0) {
        return 0;
    }
    cursor->count--;
    return cellTriangles[cursor->start++];
}

static int Near(f32 a, f32 b, f32 tolerance) {
    return fabsf(a - b) <= tolerance;
}

static u32 Bits(f32 f) {
    union {
        f32 f;
        u32 u;
    } v;
    v.f = f;
    return v.u;
}

/* A track of one flat square at z 0 from -50 to 50, of triangles 1 and 2. */
static Vec3f trackVertices[4] = {{-50, -50, 0}, {50, -50, 0}, {50, 50, 0}, {-50, 50, 0}};
static TrackSurface trackFaces[3] = {{{0, 0, 0}, 0}, {{0, 1, 2}, 0}, {{0, 2, 3}, 0}};
static u8 trackFlags[3] = {0, 0x0B, 0x04};

static void SetUpTrack(void) {
    g_LoadedTrack.vertices = trackVertices;
    g_LoadedTrack.surfaces = trackFaces;
    g_LoadedTrack.triangleFlags = trackFlags;
    cellTriangles[0] = 1;
    cellTriangles[1] = 2;
    cellCount = 2;
}

/* A vehicle at rest: a body and its wheels, upright, with identity frames. */
static Vehicle vehicles[2];

static void IdentityState(RigidBody *body, f32 x, f32 y, f32 z) {
    body->state.position.x = x;
    body->state.position.y = y;
    body->state.position.z = z;
    body->state.velocity.x = body->state.velocity.y = body->state.velocity.z = 0;
    body->state.orientation.w = 1;
    body->state.orientation.x = body->state.orientation.y = body->state.orientation.z = 0;
    body->state.angularVelocity.x = body->state.angularVelocity.y = body->state.angularVelocity.z =
        0;
    QuaternionToMatrix(body->orientation, &body->state);
}

static void SetUpVehicle(Vehicle *v, f32 x) {
    RigidBody *body = &v->body;
    s32 i;

    __builtin_memset(v, 0, sizeof(*v));
    body->kind = RIGID_BODY_BOX_1;
    body->size.x = 3.5f;
    body->size.y = 2.0f;
    body->size.z = 1.5f;
    body->mass = 1000.0f;
    InitRigidBody(body);
    IdentityState(body, x, 0, 2);
    body->boxSize.x = 4.0f;
    body->boxSize.y = 2.0f;
    body->boxSize.z = 1.0f;
    body->boxCenterZ = 0.5f;
    body->springRate = 100000.0f;
    body->damping = 2000.0f;
    body->tireType = 1;
    body->wheels[0] = &v->wheelBodies[0];
    body->wheels[1] = &v->wheelBodies[2];
    body->wheels[2] = &v->wheelBodies[1];
    body->wheels[3] = &v->wheelBodies[3];
    for (i = 0; i < 4; i++) {
        v->wheelBodies[i].kind = RIGID_BODY_NO_ROTATION;
        InitRigidBody(&v->wheelBodies[i]);
        IdentityState(&v->wheelBodies[i], (i & 1) ? 1.5f : -1.5f, (i & 2) ? -1.0f : 1.0f, -0.3f);
        v->wheelBodies[i].forces = &v->forces[VEHICLE_FORCE_WHEEL_0 + i];
    }
    for (i = 0; i < 4; i++) {
        SetRigidBodyForce(&v->forces[i], 0, 0, 0, i & 1 ? 1.5f : -1.5f, i & 2 ? 1 : -1, 0, 1);
    }
    v->forces[0].next = &v->forces[2];
    v->forces[2].next = &v->forces[1];
    v->forces[1].next = &v->forces[3];
    v->forces[3].next = NULL;
    body->forces = &v->forces[0];
}

static void TestCube(void) {
    Vec3f inside[3] = {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}};
    Vec3f beyond[3] = {{1, 0, 0}, {2, 0, 0}, {1, 2, 0}};
    Vec3f slicing[3] = {{-5, -5, 0.1f}, {5, -5, 0.1f}, {0, 5, 0.1f}};
    Vec3f corner[3] = {{1.2f, 1.2f, -5}, {1.2f, 1.2f, 5}, {5, 5, 0}};
    Vec3f normal = {0, 0, 100};
    Vec3f through = {-2, 0, 0}, past = {2, 0, 0}, high = {-2, 0, 2}, high2 = {2, 0, 2};
    Vec3f point = {0, 0, 0.1f}, outside = {9, 9, 0.1f};

    assert(ClassifyTriangleInUnitCube(inside) == 1);
    assert(ClassifyTriangleInUnitCube(beyond) == 0);
    assert(ClassifyTriangleInUnitCube(slicing) == -1);
    assert(IsTriangleInUnitCube(slicing, &normal) == 1);
    /* Beyond an edge of the cube though no face holds all three corners. */
    assert(ClassifyTriangleInUnitCube(corner) == 0);
    assert(DoesSegmentCrossUnitCube(&through, &past) == 1);
    assert(DoesSegmentCrossUnitCube(&high, &high2) == 0);
    assert(CountTriangleWinding(slicing, &normal, &point) != 0);
    assert(CountTriangleWinding(slicing, &normal, &outside) == 0);
}

static void TestCache(void) {
    TrackTriangle *t;
    s16 slot;

    SetUpTrack();
    ResetCollisionCaches();
    /* After the reset, cell 0 reads as cached and empty. */
    lookups = 0;
    slot = BuildTrackTriangleCache(10, 10);
    assert(g_awTriCacheTriCount[slot] == 0 && lookups == 0);
    slot = BuildTrackTriangleCache(40, 40);
    assert(g_awTriCacheTriCount[slot] == 2 && g_awTriCacheSectorId[slot] == 65 && lookups == 1);
    t = &g_aTriCacheTriangles[slot][0];
    assert(t->index == 1 && t->surface == 3 && t->vertices[1] == &trackVertices[1]);
    assert(Near(t->normal.z, 1, 1e-3f) && Near(t->normal.x, 0, 1e-6f) && t->planeOffset == 0);
    assert(g_aTriCacheTriangles[slot][1].surface == 4);
    /* The same cell comes from the cache, a new one fills the oldest slot. */
    assert(BuildTrackTriangleCache(50, 50) == slot && lookups == 1);
    assert(BuildTrackTriangleCache(70, 40) != slot && lookups == 2);
    assert(g_awTriCacheSectorId[BuildTrackTriangleCache(70, 40)] == 66);
    assert(g_awTriCacheSectorId[BuildTrackTriangleCache(10, 40)] == 64);
    assert(InvalidateTriCacheSectorAt(10, 40) == 1 && InvalidateTriCacheSectorAt(10, 40) == 0);
    /* A negative cell is never found, as the original compares it unsigned. */
    BuildTrackTriangleCache(-40, 10);
    assert(InvalidateTriCacheSectorAt(-40, 10) == 0);
}

static void TestRay(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *wheel;
    Vec3f hit = {-10, 10, 0}, miss = {10, -10, 0};

    SetUpTrack();
    ResetCollisionCaches();
    SetUpVehicle(v, 40);
    v->body.state.position.y = 40;
    /* Planes two units away or more are not looked at. */
    QuaternionToMatrix(v->body.orientation, &v->body.state);
    wheel = v->body.wheels[0];
    assert(CastWheelRay(&v->body, wheel) == 100.0f && wheel->ground == NULL);
    v->body.state.position.z = 1.5f;
    QuaternionToMatrix(v->body.orientation, &v->body.state);
    assert(Near(CastWheelRay(&v->body, wheel), 1.5f, 1e-4f));
    /* The wheel is over the second triangle, above the diagonal. */
    assert(wheel->ground != NULL && wheel->surface == 4 && Near(wheel->groundNormal.z, 1, 1e-3f));
    assert(IsPointInTrackTriangle(wheel->ground, &hit) == 1);
    assert(IsPointInTrackTriangle(wheel->ground, &miss) == 0);
    /* A unit and a half down is beyond the reach of the suspension. */
    UpdateWheelHeights(&v->body);
    assert(Near(wheel->suspension, -1.5f, 1e-4f) && wheel->state.position.z == -0.4f);
    v->body.state.position.z = 0.25f;
    QuaternionToMatrix(v->body.orientation, &v->body.state);
    UpdateWheelHeights(&v->body);
    assert(Near(wheel->state.position.z, -0.25f, 1e-4f));
}

static void TestSuspension(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body;
    s32 i;

    SetUpVehicle(v, 0);
    for (i = 0; i < 4; i++) {
        body->wheels[i]->suspension = -0.2f;
    }
    body->wheels[3]->suspension = -0.5f;
    ComputeSpringForces(body);
    /* Compressed a tenth of a unit, the spring pushes by its square. */
    assert(Near(v->forces[0].vector.z, 0.1f * 0.1f * 100000.0f, 1.0f));
    assert(body->wheels[0]->contactFrames == 1 && body->landed == 0x80);
    assert(body->wheels[3]->contactFrames == 0 && v->forces[3].vector.z == 0);
    body->landed = 0;
    ComputeSpringForces(body);
    assert(body->wheels[0]->contactFrames == 2 && body->landed == 0);
    /* The dampers resist the body moving down onto a wheel on the ground. */
    body->state.velocity.z = 2.0f;
    ComputeDampingForces(body);
    assert(Near(v->forces[0].vector.z, 2.0f * 2000.0f, 1e-2f) && v->forces[3].vector.z == 0);
    body->state.velocity.z = -2.0f;
    ComputeDampingForces(body);
    assert(v->forces[0].vector.z == 0);
}

static void TestForces(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body;
    RigidBodyForce gravity, drag;

    SetUpVehicle(v, 0);
    SetRigidBodyForce(&gravity, 0, 0, -9810.0f, 1, 0, 0, RIGID_BODY_FORCE_WORLD);
    v->forces[3].next = &gravity;
    ComputeVehicleForces(body);
    /* Gravity at an arm along x turns the body about y. */
    assert(Near(body->force.z, -9.81f, 1e-3f) && body->torque.y != 0);
    /* The drag, and the more of it on the slowing surface but for snow. */
    body->state.velocity.x = 10;
    ComputeDragForce(body, &drag);
    assert(drag.vector.x == -1100.0f);
    body->wheels[2]->surface = TRACK_SURFACE_SLOW;
    ComputeDragForce(body, &drag);
    assert(drag.vector.x == -3300.0f);
    g_dwCurrentWeather = WEATHER_SNOW;
    ComputeDragForce(body, &drag);
    assert(drag.vector.x == -1100.0f);
    g_dwCurrentWeather = 0;
}

static void TestImpulses(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body;
    Vec3f up = {0, 0, 1}, middle = {0, 0, 0};

    SetUpVehicle(v, 0);
    /* Into a plane, the step bounces back by a twentieth, and a tenth goes to the damping. */
    body->stepStates[1].velocity.z = -10;
    body->framesSinceImpact = 20;
    assert(BounceOffPlane(body, &up, 0, 0) == 1);
    assert(Near(body->stepStates[1].velocity.z, 0.45f, 1e-4f));
    assert(body->impactSpeed == 10 && body->impactVolume == ImpactVolume(10));
    assert(BounceOffPlane(body, &up, 0, 0) == 0);
    /* At the middle the impulse is the mass times the speed into the surface. */
    body->stepStates[1].velocity.z = -10;
    body->stepStates[1].angularVelocity.x = 0;
    body->framesSinceImpact = 0;
    assert(ApplyCollisionImpulse(body, &middle, &up, 0, 0.0f) == 1);
    assert(Near(body->stepStates[1].velocity.z, 0.5f, 1e-3f));
    assert(ApplyCollisionImpulse(body, &middle, &up, 0, 0.0f) == 0);
}

static void TestObb(void) {
    f32 identity[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    f32 turned[3][3] = {{0.7071068f, -0.7071068f, 0}, {0.7071068f, 0.7071068f, 0}, {0, 0, 1}};
    Vec3f half = {2.5f, 1, 1}, near = {4.9f, 0, 0}, far = {5.1f, 0, 0}, side = {0, 2.1f, 0};
    Vec3f diagonal = {2.5f, 2.6f, 0};

    assert(TestObbIntersection(identity, &near, &half, &half) == 1);
    assert(TestObbIntersection(identity, &far, &half, &half) == 0);
    assert(TestObbIntersection(identity, &side, &half, &half) == 0);
    assert(TestObbIntersection(turned, &diagonal, &half, &half) == 1);
}

static void TestVehicles(void) {
    Vehicle *a = &vehicles[0], *b = &vehicles[1];

    SetUpVehicle(a, 0);
    SetUpVehicle(b, 4);
    g_aRacerSlots[0].vehicle = a;
    g_aRacerSlots[1].vehicle = b;
    g_dwNumVehicles = 2;
    /* B drives into A along x; they share their speed, and the hit is heard. */
    b->body.state.velocity.x = -10;
    a->body.framesSinceVehicleImpact = 50;
    CollideVehicles();
    assert(a->body.vehicleImpactVolume == b->body.vehicleImpactVolume &&
           a->body.vehicleImpactVolume == ImpactVolume(5));
    assert(a->body.framesSinceVehicleImpact == 0);
    assert(a->body.state.velocity.x < 0 && b->body.state.velocity.x > -10);
    /* A ghost does not collide. */
    b->body.state.velocity.x = -10;
    b->ghostMode = VEHICLE_GHOST;
    CollideVehicles();
    assert(b->body.state.velocity.x == -10 && a->body.framesSinceVehicleImpact == 1);
    /* Apart, nothing happens. */
    b->ghostMode = 0;
    SetUpVehicle(b, 20);
    b->body.state.velocity.x = -10;
    CollideVehicles();
    assert(b->body.state.velocity.x == -10);
    g_dwNumVehicles = 0;
}

static void TestGrip(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body;
    f32 rearLoad = 0, frontLoad = 0;
    u8 rearSliding = 0, frontSliding = 0;
    s32 i, w, s;

    for (w = 0; w < GRIP_WEATHERS; w++) {
        for (s = 0; s < TRACK_SURFACES; s++) {
            g_afSurfaceGrip[w][s] = 0.08f;
            g_afSurfaceMaxLoad[w][s] = 90000;
            g_afSurfaceSlipLoad[w][s] = 3000;
        }
    }
    SetUpVehicle(v, 0);
    for (i = 0; i < 4; i++) {
        body->wheels[i]->contactFrames = 5;
        body->wheels[i]->ground = &g_aTriCacheTriangles[0][0];
        body->wheels[i]->steerAngle = 0;
    }
    /* A slow slide sideways is held. */
    body->state.velocity.x = 10;
    body->state.velocity.y = 0.001f;
    ApplySideGrip(body, 1.0f / 30, &rearLoad, &frontLoad, &rearSliding, &frontSliding);
    assert(Near(body->state.velocity.y, 0, 1e-4f) && Near(body->state.velocity.x, 10, 1e-3f));
    assert(!rearSliding && !frontSliding && body->rollAngle == 0);
    /* A hard one slides, and the body leans into it. */
    body->state.velocity.y = 8;
    ApplySideGrip(body, 1.0f / 30, &rearLoad, &frontLoad, &rearSliding, &frontSliding);
    assert(rearSliding && frontSliding && body->skidding == 0x80);
    assert(body->state.velocity.y > 0 && body->state.velocity.y < 8);
    assert(Near(fabsf(body->rollAngle), LEAN_STEP, 1e-6f));
    /* Off the ground nothing grips. */
    for (i = 0; i < 4; i++) {
        body->wheels[i]->ground = NULL;
    }
    body->state.velocity.y = 8;
    ApplySideGrip(body, 1.0f / 30, &rearLoad, &frontLoad, &rearSliding, &frontSliding);
    assert(body->state.velocity.y == 8 && !rearSliding);
}

static void TestDrive(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body, *wheel;
    f32 load = 0;
    u8 sliding = 0;
    s32 i;

    SetUpVehicle(v, 0);
    for (i = 0; i < 4; i++) {
        body->wheels[i]->contactFrames = 5;
        body->wheels[i]->ground = &g_aTriCacheTriangles[0][0];
        body->wheels[i]->groundNormal.z = 1;
    }
    wheel = body->wheels[0];
    wheel->drive = -1000;
    wheel->forces->vector.x = 0;
    DriveWheel(body, wheel, &load, &sliding, 1.0f / 30);
    /* The drive pushes the body along x, and loads the tyres. */
    assert(wheel->forces->vector.x != 0 && load == -1000 / 0.5f / 2);
    assert(wheel->rollAngle >= 0 && wheel->rollAngle < 360);
    /* In the air the drive spins the wheel up to its limit. */
    body->wheels[1]->contactFrames = 0;
    wheel->drive = -100000;
    for (i = 0; i < 10; i++) {
        DriveWheel(body, wheel, &load, &sliding, 1.0f / 30);
    }
    assert(wheel->spinRate == -300.0f);
    /* A steep ground is not driven on. */
    wheel->groundNormal.z = 0.5f;
    wheel->spinRate = 0;
    DriveWheel(body, wheel, &load, &sliding, 1.0f / 30);
    assert(wheel->spinRate == 0);
}

static void TestBalance(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body;
    s32 i;

    SetUpVehicle(v, 0);
    body->stepStates[0] = body->state;
    for (i = 0; i < 4; i++) {
        body->wheels[i]->groundNormal.z = 1;
        body->wheels[i]->groundOffset = 0;
    }
    /* On two wheels, still, near the ground: rolled off balance. */
    body->stepStates[0].position.z = 0.1f;
    body->wheels[0]->contactFrames = 3;
    body->wheels[2]->contactFrames = 3;
    body->groundNormal.x = 1;
    prints = 0;
    assert(TipBalancedVehicle(body) == 1 && prints == 3);
    assert(Near(body->stepStates[0].angularVelocity.y, 0.2f, 1e-5f));
    /* On three, or far from the ground, it is left alone. */
    body->wheels[1]->contactFrames = 3;
    assert(TipBalancedVehicle(body) == 0);
    body->wheels[1]->contactFrames = 0;
    body->stepStates[0].position.z = 5;
    assert(TipBalancedVehicle(body) == 0);
}

static void TestSteps(void) {
    Vehicle *v = &vehicles[0];
    RigidBody *body = &v->body;

    SetUpTrack();
    ResetCollisionCaches();
    SetUpVehicle(v, 0);
    body->stepStates[0] = body->state;
    body->stepStates[0].velocity.x = 3;
    body->uprightFrames = 0;
    g_dwNumVehicles = 0;
    markers = 0;
    StepVehicleCollisions(body);
    /* Four steps carry it a frame along, clear of the ground. */
    assert(Near(body->stepStates[0].position.x, 0.1f, 1e-4f) && markers == 3);
    assert(body->uprightFrames == UPRIGHT_FRAMES);
    /* Upside down it counts down. */
    body->stepStates[0].orientation.w = 0;
    body->stepStates[0].orientation.x = 1;
    body->stepStates[0].velocity.x = 0;
    StepVehicleCollisions(body);
    assert(body->uprightFrames == UPRIGHT_FRAMES - 1);
    body->uprightFrames = -5;
    StepVehicleCollisions(body);
    assert(body->uprightFrames == -2);
    /* Dropped onto the ground, its box is held out of it and it stops falling. */
    SetUpVehicle(v, 40);
    body->state.position.y = 40;
    body->state.position.z = 0.05f;
    QuaternionToMatrix(body->orientation, &body->state);
    body->stepStates[0] = body->state;
    body->stepStates[0].velocity.z = -3;
    body->framesSinceImpact = 50;
    StepVehicleCollisions(body);
    fprintf(stderr,
            "drop: z %f vz %f since %u speed %u\n",
            body->stepStates[0].position.z,
            body->stepStates[0].velocity.z,
            body->framesSinceImpact,
            body->impactSpeed);
    assert(body->framesSinceImpact == 0 && body->stepStates[0].velocity.z > -3.0f);
    assert(body->stepStates[0].position.z > 0.05f - 0.1f);
}

int main(void) {
    /* The constants that are not plain decimals. */
    assert(Bits(LEAN_STEP) == 0x3E888889 && Bits(WHEEL_WEIGHT_SHARE) == 0x403C5A1E);
    assert(Bits(0.999f) == 0x3F7FBE77 && Bits(1e-05f) == 0x3727C5AC && Bits(0.0001f) == 0x38D1B717);
    assert(Bits(57.29577637f) == 0x42652EE0 && Bits(1.0f / 30.0f) == 0x3D088889);
    assert(Bits(-0.3f) == 0xBE99999A && Bits(0.45f) == 0x3EE66666 && Bits(1.05f) == 0x3F866666);
    assert(Near(PlaneDistance(&g_vecUnitZ, 2, &g_vecUnitZ), 3, 0));
    TestCube();
    TestCache();
    TestRay();
    TestSuspension();
    TestForces();
    TestImpulses();
    TestObb();
    TestVehicles();
    TestGrip();
    TestDrive();
    TestBalance();
    TestSteps();
    puts("physics ok");
    return 0;
}

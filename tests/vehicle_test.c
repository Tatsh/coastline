/* Host check of src/n64/vehicle.c with the physics, model loading and cache in the assembly
 * stubbed. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "common/vehiclephysics.c"
#include "common/vehiclereset.c"
#include "common/vehiclestatus.c"
#include "vehicle.c"

TrackHeader g_LoadedTrack;
Vehicle g_aVehicles[MAX_VEHICLES];
CarModel g_aCarModels[CAR_MODELS];
CarPhysicsSpec g_aCarPhysicsSpecs[CAR_MODELS];
VehicleModel g_aVehicleModels[2];
s32 g_adwVehicleModelSlotCars[2];
u8 g_aDmaWorkBuffers[0x10000];
PlayerInputState g_aPlayerInputState[4];
u32 g_dwNumLocalPlayers = 1, g_dwSelectedTrackId;
s32 g_dwGameMode;
s32 g_dwTotalLaps = 3, g_dwNumViewports = 1;
f32 g_fFrameDeltaTime = 0.5f;
ViewportRect g_aViewportRects[4];
TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];
u16 g_awTriCacheSectorId[TRI_CACHE_SLOTS], g_awTriCacheTriCount[TRI_CACHE_SLOTS];
u32 g_adwTriCacheLruStamp[TRI_CACHE_SLOTS], g_dwTriCacheLruCounter;
f32 g_fSteerLock, g_fSteerRate, g_fSteerGain, g_fSteerLockLoss;
s16 g_wSteerGainSpeedShare;

static s32 calls, cameraUpdates, materialColor[3], fixups, loads[3], transferred;
static void *initBuffer;

/* The physics the frame of a vehicle runs, logged in the order it runs. */
static char physicsLog[64];
static s32 physicsCount, drives;
static void Log(char c) {
    physicsLog[physicsCount++] = c;
}
void ComputeSpringForces(RigidBody *b) {
    Log('s');
    calls |= 2;
}
void DriveWheel(RigidBody *b, RigidBody *w, f32 *load, u8 *sliding, f32 dt) {
    Log('w');
    drives++;
    assert(dt == 1.0f / 30.0f);
}
void ComputeVehicleForces(RigidBody *b) {
    Log('f');
}
void ApplySideGrip(RigidBody *b, f32 dt, f32 *rl, f32 *fl, u8 *rs, u8 *fs) {
    Log('g');
    assert(rl == &g_aVehicles[0].rearLoad && fs == &g_aVehicles[0].frontSliding);
}
void ComputeDragForce(RigidBody *b, RigidBodyForce *f) {
    Log('d');
    assert(f == &g_aVehicles[0].forces[VEHICLE_FORCE_DRAG]);
}
void ComputeDampingForces(RigidBody *b) {
    Log('a');
}
void StepVehicleCollisions(RigidBody *b) {
    Log('c');
}
void UpdateWheelHeights(RigidBody *b) {
    Log('h');
}
void AddTimingMarker(s32 bar, s32 r, s32 g, s32 bl, s32 a) {
}

/* The turns the wheel matrices are built from, and a plain product of matrices. */
static f32 rotations[16][4];
static s32 rotationCount;
void guRotateF(float m[4][4], float a, float x, float y, float z) {
    s32 i, j;

    if (rotationCount < 16) {
        rotations[rotationCount][0] = a;
        rotations[rotationCount][1] = x;
        rotations[rotationCount][2] = y;
        rotations[rotationCount][3] = z;
    }
    rotationCount++;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            m[i][j] = i == j;
        }
    }
    calls |= 4;
}
void guMtxCatF(float a[4][4], float b[4][4], float out[4][4]) {
    f32 r[4][4];
    s32 i, j, k;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            r[i][j] = 0;
            for (k = 0; k < 4; k++) {
                r[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    __builtin_memcpy(out, r, sizeof(r));
}
void UpdateCameraSystem(Vehicle *v) {
    cameraUpdates++;
}
void SetMaterialColor(VehicleModel *m, s32 r, s32 g, s32 b) {
    materialColor[0] = r;
    materialColor[1] = g;
    materialColor[2] = b;
}
void FixupVehicleModelPointers(VehicleModel *m) {
    fixups++;
}
void LoadCar(s32 slot, s32 car, void *source) {
    loads[0] = slot;
    loads[1] = car;
    loads[2] = source != NULL;
}
u32 DmaReadAssetSize(u32 rom) {
    return rom + 1;
}
void InitDmaRequest(DecompressJob *job, u8 *buffer) {
    job->dst = NULL;
    job->buffer = buffer;
    initBuffer = buffer;
    job->remaining = 2;
}
u32 DecompressFromRom(void *dst, u32 rom, DecompressJob *job) {
    job->remaining--;
    return 0;
}
void TransferFlags(PlayerInputState *s, u32 m) {
    transferred = m;
    s->buttons &= ~m;
}
void guMtxIdent(Mtx *m) {
    calls |= 16;
}
void osSyncPrintf(const char *f, ...) {
}
void SetDebugMessage(const char *message) {
}
u32 g_dwRandomSeed;

static int Near(f32 a, f32 b) {
    return fabsf(a - b) < 1.0e-5f;
}

static Vec3f vertexA[3] = {{-100.0f, -100.0f, 0.0f}, {100.0f, -100.0f, 0.0f}, {0.0f, 100.0f, 0.0f}};
static Vec3f vertexB[3] = {{-100.0f, -100.0f, 1.2f}, {100.0f, -100.0f, 1.2f}, {0.0f, 100.0f, 1.2f}};

int main(void) {
    static struct {
        TrackSection section;
        TrackPoint points[4];
    } route;
    static TrackSection *sections[1];
    static TrackSurface surfaces[8];
    static u16 pool[16], starts[8];
    Vehicle *v = &g_aVehicles[0];
    Quaternion q;
    f32 m[4][4];
    Vec3f point = {0.0f, 0.0f, 0.5f}, normal;
    u16 ground[RAY_INSTANCES], nearby[RAY_INSTANCES];
    s32 done, count, surface, i;
    f32 distance;
    u32 bits;

    /* The constants of the original. */
#define BITS(f) (memcpy(&bits, &(f32){f}, 4), bits)
    assert(BITS(FLY_TURN_RATE) == 0x40490FDB && BITS(FLY_TURN_SCALE) == 0x3ECCCCCD);
    assert(BITS(FRONT_BRAKE_DRIVE_LIMIT) == 0x38D1B717 && BITS(SPEED_TO_DISPLAY) == 0x400F5C29);
    assert(BITS(HALF_PI) == 0x3FC90FDB && BITS(DEFAULT_CAMERA_FOV) == 0x3F060A92);

    /* The frame of an angle, and its quaternion back from the matrix by both branches. */
    FinishFrameMatrix(m);
    assert(m[0][3] == 0.0f && m[1][3] == 0.0f && m[2][3] == 0.0f && m[3][3] == 1.0f);
    InitVehicleOrientationFromAngle(v, 0.7f);
    assert(Near(v->frame.forward.x, cosf(0.7f)) && Near(v->frame.side.y, cosf(0.7f)) &&
           v->frame.up.z == 1.0f);
    assert(Near(v->body.state.orientation.w, cosf(0.35f)) &&
           Near(v->body.state.orientation.z, sinf(0.35f)));
    assert(v->body.stepStates[0].orientation.z == v->body.state.orientation.z);
    FinishFrameMatrix(v->matrix);
    MatrixToQuaternion(v->matrix, &q);
    assert(Near(q.w, cosf(0.35f)) && Near(q.z, sinf(0.35f)) && q.x == 0.0f);
    InitVehicleOrientationFromAngle(v, 2.5f);
    MatrixToQuaternion(v->matrix, &q);
    assert(Near(q.w, cosf(1.25f)) && Near(q.z, sinf(1.25f)));
    SetVehicleFrameMatrix(v, v->matrix);
    assert(Near(v->body.stepStates[1].orientation.w, cosf(1.25f)));

    /* The setters of position, velocity and turn rate fill every state. */
    SetVehiclePosition(v, 1.0f, 2.0f, 3.0f);
    assert(v->frame.position.y == 2.0f && v->sprayOrigin.z == 3.0f &&
           v->body.stepStates[0].position.x == 1.0f);
    SetVehicleVelocity(v, 4.0f, 5.0f, 6.0f);
    assert(v->body.state.velocity.x == 4.0f && v->body.stepStates[1].velocity.z == 6.0f &&
           v->placedVelocity.y == 5.0f);
    SetVehicleTurnRate(v, 7.0f, 8.0f, 9.0f);
    assert(v->body.state.angularVelocity.z == 9.0f &&
           v->body.stepStates[0].angularVelocity.x == 7.0f);
    v->body.state.orientation = (Quaternion){1.0f, 0.0f, 0.0f, 0.0f};
    RotateVehicleByEuler(v, 0.4f, 0.0f, 0.0f);
    assert(Near(v->body.state.orientation.w, cosf(0.2f)) &&
           Near(v->body.state.orientation.z, sinf(0.2f)));

    /* A straight route along x, a point every 10 units, the finish 100 away. */
    for (i = 0; i < 4; i++) {
        route.points[i].left = (Vec3f){i * 10.0f, 5.0f, 0.0f};
        route.points[i].right = (Vec3f){i * 10.0f, -5.0f, 0.0f};
        route.points[i].position = (Vec3f){i * 10.0f, 0.0f, 0.0f};
        route.points[i].distance = 100.0f - i * 10.0f;
    }
    route.section.pointCount = 3;
    route.section.gridMaxX = route.section.gridMaxY = 63;
    sections[0] = &route.section;
    g_LoadedTrack.rootSection = &route.section;
    g_LoadedTrack.sections = sections;
    g_LoadedTrack.sectionCount = 1;
    g_LoadedTrack.maxX = 2048.0f;
    v->section = &route.section;
    v->pointIndex = 0;
    v->frame.position = (Vec3f){12.0f, 1.0f, 0.0f};
    v->previousPosition = v->frame.position;
    v->distance = 0.0f;
    assert(UpdateVehicleTrackPosition(v) == 1);
    assert(v->pointIndex == 0 && Near(v->distance, 12.0f) && v->trackDirection.x == 1.0f);
    v->previousPosition = (Vec3f){5.0f, 0.0f, 0.0f};
    v->frame.position = (Vec3f){15.0f, 0.0f, 0.0f};
    v->distance = 25.0f;
    assert(UpdateVehicleTrackPosition(v) == 1);
    assert(v->pointIndex == 1 && Near(v->distance, 15.0f));
    route.section.flags = TRACK_SECTION_ALTERNATE;
    v->index = 1;
    assert(UpdateVehicleTrackPosition(v) == 0);
    v->index = 0;
    route.section.flags = 0;
    v->frame.position = (Vec3f){100.0f, 3000.0f, 0.0f};
    v->previousPosition = v->frame.position;
    UpdateVehicleGridPosition(v);
    assert(v->gridX == 3 && v->gridY == 63);

    /* The clocks and the messages. */
    static RacerSlot racer;
    v->racer = &racer;
    v->totalTime = v->lapTime = 0.0f;
    v->timeLeft = 0.25f;
    g_dwGameMode = GAME_MODE_ARCADE;
    UpdateRaceTimers(v);
    assert(v->totalTime == 0.5f && v->lapTime == 0.5f && v->timeLeft == 0.0f);
    g_dwGameMode = GAME_MODE_PRACTICE;
    UpdateRaceTimers(v);
    assert(v->totalTime == 0.5f && v->lapTime == 1.0f);
    racer.status = 2;
    UpdateRaceTimers(v);
    assert(v->lapTime == 1.0f);
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    v->message = "A";
    v->messageTime = 0.4f;
    v->subMessage = "B";
    v->subMessageTime = 1.0f;
    UpdateHudMessageTimers(v);
    assert(v->message == NULL && v->messageTime == 0.0f && v->subMessageTime == 1.0f);
    UpdateHudMessageTimers(v);
    assert(v->subMessage != NULL && v->subMessageTime == 0.5f);

    /* Facing against the track: the warning shows while bit 4 of the frames is set, from 48. */
    v->lap = 0;
    v->trackDirection = (Vec3f){1.0f, 0.0f, 0.0f};
    v->frame.forward = (Vec3f){-1.0f, 0.0f, 0.0f};
    v->message = NULL;
    v->wrongWayFrames = 0;
    for (i = 0; i < 47; i++) {
        CheckWrongWayWarning(v);
    }
    assert(v->message == NULL);
    CheckWrongWayWarning(v);
    assert(v->message == sWrongWay && v->messageTime == 0.25f);
    for (i = 0; i < 15; i++) {
        CheckWrongWayWarning(v);
    }
    assert(v->wrongWayFrames == 63 && v->message == sWrongWay);
    CheckWrongWayWarning(v);
    assert(v->message == NULL);
    v->frame.forward = (Vec3f){1.0f, 0.0f, 0.0f};
    CheckWrongWayWarning(v);
    assert(v->wrongWayFrames == 0);

    /* The ray: two floors, the higher the ground and the lower further down. */
    g_LoadedTrack.surfaces = surfaces;
    g_LoadedTrack.nearbyInstances = pool;
    g_LoadedTrack.triangleNearbyInstances = starts;
    g_LoadedTrack.maxZ = 10.0f;
    surfaces[3].instance = 6;
    surfaces[5].instance = 9;
    starts[3] = 4;
    pool[4] = 11;
    pool[5] = 12;
    starts[5] = 7;
    pool[7] = 21;
    pool[0] = 31;
    /* The ray reads the slot the cache gives its point, which is the first while the cache is
     * empty, and finds it again later by the sector of the point. */
    assert(BuildTrackTriangleCache(point.x, point.y) == 0);
    g_aTriCacheTriangles[0][0] =
        (TrackTriangle){{0.0f, 0.0f, 1.0f}, 0.0f, {&vertexA[0], &vertexA[1], &vertexA[2]}, 3};
    g_aTriCacheTriangles[0][1] =
        (TrackTriangle){{0.0f, 0.0f, 1.0f}, 0.0f, {&vertexB[0], &vertexB[1], &vertexB[2]}, 5};
    g_awTriCacheTriCount[0] = 2;
    assert(CastRayAgainstTrack(
               &point, &normal, &point, ground, &done, nearby, &count, &distance, &surface) == 2);
    assert(Near(point.z, 1.2f) && Near(distance, -0.7f) && normal.z == 1.0f && surface == 5 &&
           done == 1);
    assert(ground[0] == 10 && ground[1] == 7 && count == 3);
    assert(nearby[0] == 21 && nearby[1] == 11 && nearby[2] == 12);
    point.z = -5.0f;
    assert(CastRayAgainstTrack(
               &point, &normal, &point, ground, &done, nearby, &count, &distance, &surface) == 0);
    assert(point.z == -5.0f && ground[0] == 10 && count == 1 && nearby[0] == 31);

    /* A frame on the ground with all-wheel drive and braking, one with rear-wheel drive, then
     * one flying. */
    static RigidBody wheelBodies[4];
    static RacerSlot racerSlot;
    for (i = 0; i < 4; i++) {
        v->body.wheels[i] = &wheelBodies[i];
    }
    v->input = &g_aPlayerInputState[0];
    v->racer = &racerSlot;
    v->flyMode = 0;
    /* An automatic gearbox in first, an engine of a torque of 1, full stick to the left. */
    v->transmission = 1;
    v->gear = 1;
    v->gearCount = 5;
    v->torqueCurve[3] = 1.0f;
    g_aPlayerInputState[0].controllerLayout = 4;
    g_aPlayerInputState[0].steering = -1.0f;
    g_aPlayerInputState[0].buttons = INPUT_ACCELERATE | INPUT_BRAKE;
    v->rearWheelDrive = 0;
    v->wheelBodies[1].contactFrames = 1;
    v->body.state.velocity = (Vec3f){3.0f, 4.0f, 0.0f};
    v->index = 0;
    g_aViewportRects[0].vehicleIndex = 0;
    v->frame.position = (Vec3f){0.0f, 0.0f, 0.5f};
    calls = 0;
    UpdateVehiclePhysicsFrame(v);
    /* The frame of the vehicle ends as the matrix of its body, the lean being 0. */
    assert(calls == 6 && cameraUpdates == 1 && v->groundHits == 2);
    assert(v->frame.position.z == v->body.orientation[3][2] &&
           v->matrix[0][0] == v->body.orientation[0][0]);
    assert(physicsCount == 12 && __builtin_memcmp(physicsLog, "swwwwfgdafch", 12) == 0);
    assert(wheelBodies[0].steerAngle == 0.0f && Near(wheelBodies[3].steerAngle, 0.17453292f));
    fprintf(stderr,
            "engine: throttle %f brake %f rpm %f gear %d\n",
            v->throttle,
            v->brake,
            v->engineRpm,
            v->gear);
    /* From rest the revolutions rise no more than 400 a frame towards the least of 900. */
    assert(v->throttle == 7.0f && v->brake == -140000.0f && v->engineRpm == 400.0f);
    assert(wheelBodies[0].drive == 0.0f && wheelBodies[2].drive == -7.0f &&
           wheelBodies[0].brake == 140000.0f);
    assert(wheelBodies[2].brake == 0.0f && Near(v->speedKph, 5.0f * 2.24f));
    /* The first frame of a vehicle drives its wheels; one that skips the drive does not. */
    v->skipDrive = 1;
    drives = 0;
    UpdateVehiclePhysicsFrame(v);
    assert(drives == 0 && v->skipDrive == 0);
    v->rearWheelDrive = 1;
    g_aPlayerInputState[0].buttons = INPUT_ACCELERATE | INPUT_REVERSE_TOGGLE;
    UpdateVehiclePhysicsFrame(v);
    assert(v->gear == 1 && v->throttle == -7.0f && v->brake == 0.0f);
    assert(wheelBodies[0].drive == 14.0f && wheelBodies[2].drive == 0.0f);
    /* The stick through the dead zone steers towards a share of the lock, a step a frame. */
    g_aPlayerInputState[0].controllerLayout = 0;
    g_aPlayerInputState[0].steering = 0.5f;
    g_aPlayerInputState[0].buttons = 0;
    v->steeringMode = 0;
    v->steerAngle = 0.0f;
    v->steerReturn = 0;
    v->body.state.velocity = (Vec3f){10.0f, 0.0f, 0.0f};
    QuaternionToMatrix(v->body.orientation, &v->body.state);
    UpdateVehicleEngineAndGearbox(v);
    assert(v->steerAngle == -0.01f && v->steerReturn == 0 && g_fSteerLock == 14.0f);
    UpdateVehicleEngineAndGearbox(v);
    assert(Near(v->steerAngle, -0.02f));
    /* Let go, it springs back to the middle at once. */
    g_aPlayerInputState[0].steering = 0.0f;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->steerAngle == 0.0f && v->steerReturn == 1);
    /* A hard push takes the whole lock, which narrows with the speed. */
    g_aPlayerInputState[0].steering = 1.0f;
    v->steeringMode = 2;
    for (i = 0; i < 20; i++) {
        UpdateVehicleEngineAndGearbox(v);
    }
    assert(Near(v->steerAngle, -(17.0f - 10.0f / 90.0f * 14.0f) * 3.141592741f / 180.0f));
    /* By hand, a gear at a press, not up while reversing. */
    v->transmission = 0;
    v->gear = 2;
    g_aPlayerInputState[0].buttons = INPUT_GEAR_DOWN;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->gear == 1 && transferred == INPUT_GEAR_DOWN &&
           !(g_aPlayerInputState[0].buttons & INPUT_GEAR_DOWN));
    g_aPlayerInputState[0].buttons = INPUT_GEAR_UP | INPUT_REVERSE;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->gear == 1);
    /* The automatic box changes down below 4000 and up above 6000. */
    v->transmission = 1;
    v->gear = 3;
    v->engineRpm = 3000.0f;
    g_aPlayerInputState[0].buttons = 0;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->gear == 2);
    v->engineRpm = 7000.0f;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->gear == 3);
    /* In arcade the second gets a push by its gap to the first, up to 30. */
    g_dwGameMode = GAME_MODE_ARCADE;
    v->racePosition = 1;
    v->distance = 0.0f;
    g_aVehicles[1].distance = 100.0f;
    g_aPlayerInputState[0].buttons = INPUT_ACCELERATE;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->throttle == (1.0f + 30.0f) * 7.0f);
    g_aVehicles[1].distance = 40.0f;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->throttle == (1.0f + 2.0f) * 7.0f);
    g_aVehicles[1].distance = 30.0f;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->throttle == 1.0f * 7.0f);
    g_dwGameMode = 0;
    /* A held racer is in neutral, where the throttle only revs the engine. */
    racerSlot.status = RACER_HELD;
    v->engineRpm = 2000.0f;
    UpdateVehicleEngineAndGearbox(v);
    assert(v->gear == 0 && v->throttle == 0.0f && v->engineRpm == 2300.0f);
    racerSlot.status = 0;
    v->flyMode = 1;
    v->accelerator = 1.0f;
    v->steering = 0.0f;
    v->pitchInput = 0.0f;
    v->rollInput = 0.0f;
    v->frame.forward = (Vec3f){1.0f, 0.0f, 0.0f};
    v->frame.position = (Vec3f){0.0f, 0.0f, 0.0f};
    g_aPlayerInputState[0].buttons = INPUT_FLY_LEVEL;
    calls = 0;
    UpdateVehiclePhysicsFrame(v);
    assert(v->frame.position.x == 50.0f && v->body.state.position.x == 50.0f &&
           v->engineRpm == 0.0f);
    assert(transferred == INPUT_FLY_LEVEL && v->frame.up.z == 1.0f && calls == 4);
    assert(v->body.state.velocity.x == 0.0f && v->body.state.angularVelocity.y == 0.0f);

    /* The reset for a car, and the models. */
    g_aCarPhysicsSpecs[2] = (CarPhysicsSpec){{0.0f, 3.23f, 2.1f, 1.46f, 1.11f, 0.85f, 0.6f},
                                             {1.0f, 2.0f, 3.0f, 4.0f},
                                             4.4f,
                                             5,
                                             1,
                                             {3.5f, 2.0f, 0.8f},
                                             0.7f,
                                             1};
    v->carModel = 2;
    v->chosenTransmission = 7;
    v->cameras[VEHICLE_CAMERA_4].fov = 9.0f;
    ResetVehicleState(v);
    assert(v->gearRatios[1] == 3.23f && v->torqueCurve[3] == 4.0f && v->finalDrive == 4.4f &&
           v->gearCount == 5);
    assert(v->rearWheelDrive == 1 && v->transmission == 7 && v->body.boxCenterZ == 0.7f &&
           v->tableIndex == 1);
    assert(v->body.boxSize.x == 3.5f && v->body.boxSize.z == 0.8f);
    assert(v->activeCamera == &v->cameras[0] && v->model == NULL && v->section == NULL);
    assert(Near(v->cameras[VEHICLE_CAMERA_MIRROR].fov, 0.5235988f) &&
           v->cameras[VEHICLE_CAMERA_4].fov == 9.0f);
    assert(v->wheelMatrices[2][3][3] == 1.0f && v->cameras[0].positionW == 1.0f);
    v->color[0] = 0xF8;
    v->color[1] = 0x10;
    v->color[2] = 0x08;
    g_aVehicleModels[1].wheels[1] = (Vec3f){1.4f, 0.8f, 0.0f};
    g_aVehicleModels[1].wheels[2] = (Vec3f){-1.3f, 0.75f, 0.0f};
    v->suspensionStiffness = 2;
    v->tireType = 3;
    InstallVehicleModel(v, 1, 4);
    assert(loads[0] == 1 && loads[1] == 4 && loads[2] == 0 && v->model == &g_aVehicleModels[1]);
    assert(materialColor[0] == 31 && materialColor[1] == 2 && materialColor[2] == 1);
    /* The physics of the model: the body, its wheels, and the lists of the forces. */
    assert(v->skipDrive == 1 && v->gear == 0 && v->body.mass == 1000.0f &&
           v->body.kind == RIGID_BODY_BOX_1);
    assert(v->body.state.position.z == 2.0f && v->body.springRate == 448000.0f &&
           v->body.damping == -3000.0f);
    assert(v->body.tireType == 3 && v->body.wheels[1] == &v->wheelBodies[2] &&
           v->body.wheels[2] == &v->wheelBodies[1]);
    assert(v->wheelBodies[2].state.position.x == -1.3f &&
           v->wheelBodies[2].state.position.y == -0.75f);
    assert(v->wheelBodies[3].state.position.x == 1.4f &&
           v->wheelBodies[3].state.position.y == -0.8f);
    assert(v->wheelBodies[1].state.position.z == -0.1f &&
           v->wheelBodies[1].kind == RIGID_BODY_NO_ROTATION);
    assert(v->body.forces == &v->forces[VEHICLE_FORCE_SPRING_0]);
    assert(v->forces[VEHICLE_FORCE_SPRING_0].next == &v->forces[VEHICLE_FORCE_SPRING_2]);
    assert(v->forces[VEHICLE_FORCE_SPRING_3].next == &v->forces[VEHICLE_FORCE_GRAVITY]);
    assert(v->forces[VEHICLE_FORCE_GRAVITY].next == NULL &&
           v->forces[VEHICLE_FORCE_GRAVITY].vector.z == -15450.75f);
    assert(v->forces[VEHICLE_FORCE_SPRING_2].point.y == 1.0f &&
           v->forces[VEHICLE_FORCE_SPRING_1].point.x == 1.5f);
    assert(v->wheelBodies[1].forces == &v->forces[VEHICLE_FORCE_WHEEL_1]);
    assert(v->forces[VEHICLE_FORCE_WHEEL_1].next == &v->forces[VEHICLE_FORCE_FRONT_WHEEL_GRAVITY]);
    assert(v->forces[VEHICLE_FORCE_WHEEL_2].next == &v->forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY]);
    assert(v->forces[VEHICLE_FORCE_REAR_WHEEL_GRAVITY].vector.z == -(10.5f * 9.81f));
    assert(v->forces[VEHICLE_FORCE_DAMPER_3].next == &v->forces[VEHICLE_FORCE_DRAG]);
    assert(v->forces[VEHICLE_FORCE_DRAG].next == NULL &&
           v->forces[VEHICLE_FORCE_DRAG].frame == RIGID_BODY_FORCE_WORLD);
    /* The wheels are drawn a little below where they hang, the front ones steered twice over. */
    v->body.rollAngle = 3.0f;
    v->wheelBodies[1].rollAngle = 20.0f;
    v->wheelBodies[1].steerAngle = 0.1f;
    rotationCount = 0;
    TransformAllWheelMatrices(v);
    assert(rotationCount == 7 && rotations[0][0] == 3.0f && rotations[0][1] == 1.0f);
    assert(rotations[3][0] == 20.0f && rotations[3][2] == 1.0f && rotations[4][3] == 1.0f);
    assert(Near(rotations[4][0], 0.1f * 114.5915527f));
    assert(Near(v->wheelMatrices[1][3][0], 1.4f) &&
           Near(v->wheelMatrices[1][3][2], 2.0f - 0.1f + 0.254f));
    assert(Near(v->wheelMatrices[3][3][1], -0.75f) &&
           Near(v->wheelBodies[1].orientation[3][2], -0.1f));
    static VehicleModelLoad load;
    g_aCarModels[4].modelRom = 0x1000;
    LoadVehicleModel(&load, 1, 4, 1);
    assert(g_aCarModels[4].modelSize == 0x1001 && initBuffer == &g_aDmaWorkBuffers[0x8000]);
    assert(load.slot == 1 && load.car == 4 && load.job.remaining == 1 &&
           !IsVehicleModelLoaded(&load));
    assert(PollVehicleModelDma(&load) == 0 && g_adwVehicleModelSlotCars[1] == 4 && fixups == 1);
    assert(IsVehicleModelLoaded(&load) && PollVehicleModelDma(&load) == 0 && fixups == 1);
    calls = 0;
    BindPlayerInputToVehicle(&g_aVehicles[2]);
    assert(g_aVehicles[2].input == &g_aPlayerInputState[2] && calls == 16);
    puts("vehicle ok");
    return 0;
}

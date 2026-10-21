/* Host check of src/effects.c against the words and arithmetic of the disassembly of 3C130.s. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "effects.c"

Gfx *gDisplayListPtr;
static Gfx gfx[512];
ViewportRect g_aViewportRects[4] = {{10, 20, 100, 50, 0}};
s32 g_dwCurrentSplitScreenIndex, g_dwTrackMirrored, g_dwViewportMirrored;
CameraFrame *g_pActiveCamera;
static CameraFrame camera;
f32 g_afMvpMatrix[4][4], g_afScratchMatrix[4][4], g_afPerspectiveMatrix[4][4];
static Mtx projection;
Mtx *g_pProjectionMtx = &projection;
u32 g_dwTextureFilterMode = 0x2000, g_dwColorDitherMode = 0x40, g_dwAlphaDitherMode = 0x10;
s32 g_dwViewportScaleZ = 511, g_dwViewportTransZ = 511;
f32 g_fFrameDeltaTime = 1.0f / 30.0f;
u32 g_dwRacePauseState, g_dwSelectedTrackId, g_dwSnowMode, g_dwRainMode, g_dwNightMode;
TrackHeader g_LoadedTrack;
static TrackInstance instances[4];
Vec3f g_vWindVelocity;
u8 g_aParticleTexture[64 * 64], g_aTireSprayTexture[32 * 64 / 2];
RacerSlot g_aRacerSlots[4];
s32 g_dwNumVehicles, g_dwNumActiveVehicles;
Vehicle g_aVehicles[MAX_VEHICLES];
Particle g_aParticlePool[PARTICLES];
u16 g_wParticleFreeListHead, g_wExhaustParticleListHead, g_wTireSmokeListHead;
u16 g_wPavedTireSmokeListHead;
f32 g_afBillboardMatrix[4][4];
u32 g_dwRandomSeed;

void SetDebugMessage(const char *message) {
}

/* The matrix routines of libultra, recorded. */
static Mtx *translated, *catResult;
static f32 translation[3], scaled[3];
void guTranslate(Mtx *m, float x, float y, float z) {
    translated = m;
    translation[0] = x;
    translation[1] = y;
    translation[2] = z;
}
void guScale(Mtx *m, float x, float y, float z) {
    scaled[0] = x;
    scaled[1] = y;
    scaled[2] = z;
}
void guMtxCatL(Mtx *m, Mtx *n, Mtx *res) {
    catResult = res;
}

static void Reset(void) {
    gDisplayListPtr = gfx;
}
static u32 Count(void) {
    return (u32)(gDisplayListPtr - gfx);
}

static void Word(u32 i, u32 w0, u32 w1) {
    if (gfx[i].words.w0 != w0 || gfx[i].words.w1 != w1) {
        fprintf(stderr,
                "word %u: %08x %08x, expected %08x %08x\n",
                i,
                gfx[i].words.w0,
                gfx[i].words.w1,
                w0,
                w1);
    }
    assert(gfx[i].words.w0 == w0 && gfx[i].words.w1 == w1);
}

static void Word0(u32 i, u32 w0) {
    if (gfx[i].words.w0 != w0) {
        fprintf(stderr, "word %u: %08x, expected %08x\n", i, gfx[i].words.w0, w0);
    }
    assert(gfx[i].words.w0 == w0);
}

/* The common start of the particle passes, from RenderParticleEffects and its sibling. */
static void ParticleTexture(void) {
    Word(0, 0xE7000000, 0);
    Word(1, 0xBA001402, 0);
    Word(2, 0xBB000001, 0xFFFFFFFF);
    Word(3, 0xBA000C02, 0x2000);
    Word(4, 0xFCFF97FF, 0xFF2DFEFF);
    Word0(5, 0xFD900000);
    Word(6, 0xF5900000, 0x07018060);
    Word(7, 0xE6000000, 0);
    Word(8, 0xF3000000, 0x077FF100);
    Word(9, 0xE7000000, 0);
    Word(10, 0xF5881000, 0x00018060);
    Word(11, 0xF2000000, 0x000FC0FC);
    Word(12, 0xBA000E02, 0);
    Word(13, 0xBA001301, 0);
}

static void RestoredModes(u32 i) {
    Word(i, 0xE7000000, 0);
    Word(i + 1, 0xBA001301, 0x80000);
    Word(i + 2, 0xB9000201, 0);
    Word(i + 3, 0xBA000602, 0x40);
    Word(i + 4, 0xBA000402, 0x10);
    Word(i + 5, 0xB9000002, 1);
}

static void Identity(f32 m[4][4]) {
    bzero(m, 64);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

static Vehicle *Racer(void) {
    Vehicle *v = &g_aVehicles[0];

    bzero(g_aVehicles, sizeof(g_aVehicles));
    bzero(g_aRacerSlots, sizeof(g_aRacerSlots));
    g_aRacerSlots[1].vehicle = v;
    g_dwNumVehicles = 2;
    v->frame.forward.x = 1.0f;
    v->frame.side.y = 1.0f;
    v->frame.up.z = 1.0f;
    return v;
}

int main(void) {
    Vehicle *v;
    Particle *p;
    Vtx *head;
    s32 i, s;
    u16 index;

    /* The depth through the viewport, in the 32nds of the depth buffer. */
    assert(ComputeScreenDepthZ(0.5f, 1.0f) == (s32)((0.5f * 511.0f + 511.0f) * 32.0f));

    /* The free list runs through the whole pool from 1, the last link cleared. */
    g_dwNumActiveVehicles = 3;
    g_aVehicles[2].exhaustTimer = g_aVehicles[3].exhaustTimer = 7.0f;
    g_wExhaustParticleListHead = g_wTireSmokeListHead = g_wPavedTireSmokeListHead = 9;
    InitParticleSystem();
    assert(g_wParticleFreeListHead == 1 && g_aParticlePool[PARTICLES - 1].next == 0);
    for (i = 1; i < PARTICLES - 1; i++) {
        assert(g_aParticlePool[i].next == i + 1);
    }
    assert(g_aVehicles[2].exhaustTimer == 0.0f && g_aVehicles[3].exhaustTimer == 7.0f);
    assert(!g_wExhaustParticleListHead && !g_wTireSmokeListHead && !g_wPavedTireSmokeListHead);

    /* The exhaust waits for its timer, then blows back from behind the third wheel. */
    v = Racer();
    v->wheelFrames[2].position.x = -2.0f;
    v->wheelFrames[2].position.y = 1.0f;
    v->engineRpm = 1000.0f;
    g_dwRandomSeed = 1;
    g_fFrameDeltaTime = 0.05f;
    SpawnExhaustParticle(v);
    assert(v->exhaustTimer > 0.0f && v->exhaustTimer <= 0.25f && g_wExhaustParticleListHead == 0);
    g_fFrameDeltaTime = 0.5f;
    SpawnExhaustParticle(v);
    assert(v->exhaustTimer == 0.0f && g_wExhaustParticleListHead == 1);
    assert(g_wParticleFreeListHead == 2 && g_aParticlePool[1].next == 0);
    p = &g_aParticlePool[1];
    assert(p->velocity.x == -2.5f && p->velocity.y == 0.0f);
    assert(v->exhaustPosition.x == -3.0f && v->exhaustPosition.y == 1.2f &&
           v->exhaustPosition.z == 0.2f);
    assert(p->size == 1.1f * 0.15f && p->alpha == 255);
    assert(p->fade == (u8)(u32)(1.0f / 1.1f * 255.0f));
    /* The puff lies between the exhaust and where the last puff left, nearer the exhaust. */
    assert(p->position.x >= -3.0f && p->position.x <= 0.0f);

    /* The exhaust drifts with the wind and its faded velocity, and frees itself when faint. */
    g_vWindVelocity.x = 0.25f;
    p->position.x = p->position.y = p->position.z = 0.0f;
    p->velocity.x = 2.0f;
    p->velocity.z = 0.0f;
    p->alpha = 255;
    p->fade = 128;
    p->size = 1.0f;
    g_fFrameDeltaTime = 0.5f;
    UpdateExhaustParticles();
    assert(p->position.x == 0.25f + 2.0f * (128.0f * 255.0f / 65280.0f) * 0.5f);
    assert(p->position.z == 0.8f * 0.5f && p->size == 1.15f);
    assert(p->fade == (u8)(s32)(5.7375f / (1.15f * 1.15f)) && g_wExhaustParticleListHead == 1);
    p->fade = 7;
    UpdateExhaustParticles();
    assert(g_wExhaustParticleListHead == 0 && g_wParticleFreeListHead == 1 && p->next == 2);
    g_vWindVelocity.x = 0.0f;

    /* The tyre smoke is freed when it falls too fast, from either list. */
    g_wPavedTireSmokeListHead = 1;
    g_wParticleFreeListHead = 2;
    p->next = 0;
    p->alpha = p->fade = 255;
    p->size = 1.0f;
    p->velocity.z = -29.0f;
    UpdateTireSmoke();
    assert(g_wPavedTireSmokeListHead == 0 && g_wParticleFreeListHead == 1);
    assert(p->velocity.z == -29.0f - 0.5f * 19.62f && p->fade == (u8)(s32)(102.0f / 1.35f));

    /* The smoke rises from a skidding wheel on a surface that smokes. */
    InitParticleSystem();
    v = Racer();
    v->body.skidding = 0x80;
    v->speedKph = 100.0f;
    v->wheelBodies[3].contactFrames = 1;
    v->wheelBodies[3].surface = TRACK_SURFACE_PAVED;
    v->wheelBodies[1].contactFrames = 1;
    v->wheelBodies[1].surface = 1;
    v->smokeTimers[0] = v->smokeTimers[1] = 0.74f;
    v->smokeTimers[2] = 0.7f;
    v->wheelBodies[0].contactFrames = 1;
    v->wheelBodies[0].surface = TRACK_SURFACE_SLOW;
    v->wheelFrames[0].position.x = 5.0f;
    v->groundDistance = 1.0f;
    g_fFrameDeltaTime = 1.0f / 30.0f;
    SpawnTireSmoke(v);
    assert(g_wPavedTireSmokeListHead == 1 && g_wTireSmokeListHead == 2);
    assert(v->smokeTimers[0] == 0.0f && v->smokeTimers[1] == 0.0f);
    /* The third wheel is on a surface that does not smoke, and only its timer resets. */
    assert(v->smokeTimers[2] == 0.0f && g_wParticleFreeListHead == 3);
    p = &g_aParticlePool[1];
    assert(p->size == 0.4f && p->fade == 25);
    assert(p->alpha == (u8)(s32)(239.0f * (1.0f - 50.0f / 150.0f) * 0.7f + 16.0f));
    assert(v->smokePositions[0].x == 4.0f && v->smokePositions[0].z == 0.5f);
    assert(p->velocity.y > 0.0f && g_aParticlePool[2].velocity.y < 0.0f);
    v->body.skidding = 0;
    v->smokeTimers[3] = 0.74f;
    SpawnTireSmoke(v);
    assert(v->smokeTimers[3] == 0.74f);

    /* A wheel on grass throws a new segment, its bottom edge at half the speed of the top. */
    v = Racer();
    g_dwSnowMode = g_dwRainMode = 0;
    g_dwSelectedTrackId = 0;
    g_LoadedTrack.instances = instances;
    v->wheelBodies[3].contactFrames = 1;
    v->wheelBodies[3].surface = TRACK_SURFACE_SLOW;
    v->wheelFrames[0].position.x = 2.0f;
    v->wheelFrames[0].position.z = 1.0f;
    v->sprayOrigin.x = 1.0f;
    v->placedVelocity.x = 10.0f;
    v->speedKph = 50.0f;
    v->sprayPhases[0] = 0.7f;
    v->sprayPositions[0].x = 100.0f;
    EmitTireSpray(v);
    assert(v->sprayAdvanced[0] && !v->sprayAdvanced[1]);
    assert(v->sprayFramesLeft[0] == SPRAY_FRAMES && v->spraySurfaces[0] == TRACK_SURFACE_SLOW);
    assert(v->sprayPhases[0] == 0.0f && v->spraySegmentSurfaces[0][0] == TRACK_SURFACE_SLOW);
    assert(v->sprayVelocities[0][0][0][0] == v->sprayVelocities[0][0][1][0] >> 1);
    assert(v->sprayVelocities[0][0][0][2] == 0 && v->sprayVelocities[0][0][1][2] > 0);
    assert(v->sprayVelocities[0][0][1][0] == (s16)(s32)(0.3f * 10.0f * 127.0f));
    head = v->sprayVertices[0][0];
    assert(head[1].v.ob[0] == 127 && head[1].v.ob[1] == (s16)(s32)(0.15f * 127.0f));
    assert(head[1].v.ob[2] == (s16)(s32)(0.75f * 127.0f));
    assert(head[0].v.ob[0] == head[1].v.ob[0] && head[0].v.ob[2] == head[1].v.ob[2]);
    assert(v->sprayPositions[0].x == 2.0f);
    /* A wheel off the ground and out of its frames throws from the wheel, still. */
    assert(v->spraySegmentSurfaces[1][0] == 0 && v->sprayVertices[1][0][0].v.ob[0] == -127);

    /* Between segments the spray holds still, and the frames count down off the ground. */
    v->wheelBodies[3].contactFrames = 0;
    v->sprayVelocities[0][0][1][0] = 77;
    EmitTireSpray(v);
    assert(!v->sprayAdvanced[0] && v->sprayFramesLeft[0] == 2.0f);
    assert(v->sprayVelocities[0][0][1][0] == 77);

    /* Water on the paved road in the rain begins a segment every frame; not under a shelter. */
    v->wheelBodies[3].contactFrames = 1;
    v->wheelBodies[3].surface = TRACK_SURFACE_PAVED;
    v->sprayPhases[0] = 0.75f;
    g_dwRainMode = 1;
    EmitTireSpray(v);
    assert(v->sprayPhases[0] == 7.75f &&
           v->sprayVelocities[0][0][0][0] == v->sprayVelocities[0][0][1][0]);
    v->groundInstanceCount = 1;
    instances[0].flags = TRACK_INSTANCE_SHELTER;
    v->sprayPhases[0] = 0.75f;
    EmitTireSpray(v);
    assert(v->sprayPhases[0] != 7.75f && v->sprayVelocities[0][0][1][0] == 0);
    v->groundInstanceCount = 0;
    g_dwRainMode = 0;
    /* In the snow, only two tracks show spray. */
    g_dwSnowMode = 1;
    v->sprayPhases[0] = 0.0f;
    EmitTireSpray(v);
    assert(v->sprayPhases[0] == 0.0f);
    g_dwSelectedTrackId = 7;
    EmitTireSpray(v);
    assert(v->sprayPhases[0] != 0.0f);
    g_dwSnowMode = 0;
    g_dwSelectedTrackId = 0;

    /* The spray ages: a new segment is coloured, older ones shift back and fade out. */
    v = Racer();
    v->sprayAdvanced[0] = TRUE;
    for (i = 0; i < SPRAY_SEGMENTS; i++) {
        v->spraySegmentSurfaces[0][i] = i == 0 ? TRACK_SURFACE_SLOW : 0;
        v->sprayVertices[0][i][0].v.ob[0] = (s16)(i * 10);
    }
    v->sprayVelocities[0][2][1][2] = 300;
    v->sprayVelocities[0][3][1][0] = 4096;
    v->sprayVelocities[0][3][0][1] = -4096;
    v->sprayVelocities[0][3][1][2] = -4096;
    g_fFrameDeltaTime = 0.5f;
    MoveTireSpray();
    assert(translated == &v->sprayMtx && catResult == &v->sprayMtx);
    assert(translation[0] == 0.0f && scaled[0] == 1.0f / 127.0f);
    assert(v->sprayOrigin.x == 0.0f);
    head = v->sprayVertices[0][0];
    assert(head[0].v.cn[0] == 0x30 && head[0].v.cn[1] == 0x18 && head[1].v.cn[0] == 0x80);
    /* The new segment is the first and second, the rest moved back by one. */
    assert(v->spraySegmentSurfaces[0][1] == TRACK_SURFACE_SLOW);
    assert(v->spraySegmentSurfaces[0][2] == TRACK_SURFACE_SLOW);
    assert(v->sprayVertices[0][2][0].v.cn[0] == 0x30 && v->sprayVertices[0][3][0].v.ob[0] == 20);
    assert(v->sprayVertices[0][1][0].v.cn[3] == 255 && v->sprayVertices[0][4][1].v.cn[3] == 255);
    assert(v->sprayVertices[0][5][0].v.cn[3] == 191 && v->sprayVertices[0][7][1].v.cn[3] == 63);
    assert(v->sprayVertices[0][8][0].v.cn[3] == 0);
    assert(v->sprayVertices[0][3][0].v.tc[0] == 0x10 && v->sprayVertices[0][3][1].v.tc[0] == 0x410);
    assert(v->sprayVertices[0][6][0].v.tc[1] == 6 << 11);
    /* The fourth segment moved by the velocity of the third before it, half a second of it. */
    assert(v->sprayVertices[0][4][1].v.ob[0] == 2048 && v->sprayVertices[0][4][0].v.ob[1] == -2048);
    /* Its top edge fell below the bottom one and was held there. */
    assert(v->sprayVertices[0][4][1].v.ob[2] == 0 && v->sprayVelocities[0][4][1][2] == -4096);
    /* The third segment before it rose, and keeps falling by gravity. */
    assert(v->sprayVertices[0][3][1].v.ob[2] == 150);
    assert(v->sprayVelocities[0][3][1][2] == 300 - (s16)(s32)(9.81f * 0.5f * 127.0f));

    /* The origin moves to the vehicle, and every segment but the oldest moves back with it. */
    v = Racer();
    v->sprayOrigin.x = 2.0f;
    for (i = 0; i < SPRAY_SEGMENTS; i++) {
        v->spraySegmentSurfaces[0][i] = 0;
    }
    v->sprayVertices[0][8][0].v.ob[0] = 5;
    v->sprayVertices[0][7][0].v.ob[0] = 5;
    MoveTireSpray();
    assert(v->sprayOrigin.x == 2.0f - (f32)254 * (1.0f / 127.0f));
    assert(v->sprayVertices[0][7][0].v.ob[0] == 5 + 254 && v->sprayVertices[0][8][0].v.ob[0] == 5);
    /* At night and in the rain the colours of grass darken or wash out. */
    g_dwNightMode = 1;
    v->sprayAdvanced[1] = TRUE;
    v->spraySegmentSurfaces[1][0] = TRACK_SURFACE_SLOW;
    MoveTireSpray();
    assert(v->sprayVertices[1][0][0].v.cn[0] == 0x18 && v->sprayVertices[1][0][1].v.cn[2] == 0x20);
    g_dwNightMode = 0;
    g_dwRainMode = 1;
    v->spraySegmentSurfaces[1][0] = TRACK_SURFACE_SLOW;
    MoveTireSpray();
    assert(v->sprayVertices[1][0][0].v.cn[0] == 0x38 && v->sprayVertices[1][0][1].v.cn[2] == 0x40);
    /* A surface that throws nothing keeps the last colours. */
    v->spraySegmentSurfaces[1][0] = 1;
    v->sprayVertices[1][1][0].v.cn[0] = 0x99;
    MoveTireSpray();
    assert(v->spraySegmentSurfaces[1][1] == -1 && v->sprayVertices[1][1][0].v.cn[0] == 0x99);
    assert(v->sprayVertices[1][1][0].v.cn[3] == 0);
    g_dwRainMode = 0;
    g_dwRacePauseState = 1;
    v->sprayOrigin.x = 9.0f;
    MoveTireSpray();
    assert(v->sprayOrigin.x == 9.0f);
    g_dwRacePauseState = 0;

    /* The spray: its modes, then a quad for each pair of segments that still moves. */
    v = Racer();
    v->sprayVelocities[0][2][1][1] = 5;
    Reset();
    DrawTireSpray();
    Word(0, 0xE7000000, 0);
    Word(1, 0xBA001402, 0x00100000);
    Word(2, 0xFCFF99FF, 0xFFFE7E38);
    Word(3, 0xB900031D, 0x0C184A50);
    Word(4, 0xB7000000, 0x4);
    Word(5, 0xB6000000, 0x23000);
    Word0(6, 0x01030040);
    Word0(7, 0xFD900000);
    Word(8, 0xF5900000, 0x07018050);
    Word(9, 0xE6000000, 0);
    Word(10, 0xF3000000, 0x071FF400);
    Word(11, 0xE7000000, 0);
    Word(12, 0xF5800400, 0x00018050);
    Word(13, 0xF2000000, 0x0007C0FC);
    Word(14, 0xF2002002, 0x0007E0FE);
    Word(15, 0xBB000001, 0xFFFFFFFF);
    Word0(16, 0x01060040);
    Word0(17, 0x040081FF);
    /* The second and third segments meet the moving one, as the quads from vertex 0 and 4. */
    Word(18, 0xB1000802, 0x0002080A);
    Word(19, 0xB108100A, 0x000A1012);
    Word0(20, 0x040081FF);
    Word0(23, 0xBD000000);
    Word(24, 0xB7000000, 0x2000);
    assert(Count() == 25);
    g_dwSnowMode = 1;
    Reset();
    DrawTireSpray();
    assert(Count() == 0);
    g_dwSelectedTrackId = 2;
    DrawTireSpray();
    assert(Count() == 25);
    g_dwSnowMode = 0;
    g_dwSelectedTrackId = 0;

    /* A puff a unit before the camera, in the middle of a view of 100 by 50 at 10, 20. */
    bzero(g_aParticlePool, sizeof(g_aParticlePool));
    Identity(g_afBillboardMatrix);
    p = &g_aParticlePool[1];
    p->position.z = 0.5f;
    p->size = 0.1f;
    p->alpha = 200;
    p->fade = 128;
    p->next = 2;
    g_aParticlePool[2].position.x = -1.2f;
    g_aParticlePool[2].position.z = 0.5f;
    g_aParticlePool[2].size = 0.1f;
    g_aParticlePool[2].next = 3;
    /* Behind the near plane, and past the edge of the screen. */
    g_aParticlePool[3].position.z = 1.0f;
    g_aParticlePool[3].size = 0.1f;
    g_aParticlePool[3].next = 4;
    g_aParticlePool[4].position.x = 1.5f;
    g_aParticlePool[4].size = 0.1f;
    g_aParticlePool[4].next = 0;
    Reset();
    RenderBillboardParticles(1, 0x1A0, 0x20, 0x30);
    if (Count() != 12) {
        for (i = 0; i < (s32)Count(); i++) {
            fprintf(stderr, "%d: %08x %08x\n", i, gfx[i].words.w0, gfx[i].words.w1);
        }
    }
    assert(Count() == 12);
    Word(0, 0xE7000000, 0);
    Word(1, 0xFA00FFFF, 0xA0203000 | (200 * 128) >> 8);
    Word(2, 0xEE000000, (u32)((s32)(0.5f * 16352.0f) + 0x3FE0) << 16);
    Word(3, 0xE4000000 | 300 << 12 | 260, 180 << 12 | 100);
    Word(4, 0xB4000000, 0x7E0);
    Word(5, 0xB3000000, (u32)(129024 / 60) << 16 | ((u32)(-129024 / 80) & 0xFFFF));
    /* Off the left of the screen, the rectangle starts at the edge and its texture further in. */
    s = (s32)(-1.2f * 200.0f) + 240 - 60;
    Word(9, 0xE4000000 | (s + 120) << 12 | 260, 0 << 12 | 100);
    Word(10, 0xB4000000, (u32)(-(((s16)s * (129024 / 60)) >> 7)) << 16 | 0x7E0);
    /* Mirrored, the puff moves to the other side of the middle. */
    g_dwViewportMirrored = 1;
    g_aParticlePool[1].next = 0;
    g_aParticlePool[1].position.x = 0.1f;
    Reset();
    RenderBillboardParticles(1, 0, 0, 0);
    assert(Count() == 6);
    Word0(3, 0xE4000000 | (240 - (s32)(0.1f * 200.0f) + 60) << 12 | 260);
    g_dwViewportMirrored = 0;
    /* Above the top of the screen, the texture starts further down it. */
    g_aParticlePool[1].position.x = 0.0f;
    g_aParticlePool[1].position.y = -1.3f;
    Reset();
    RenderBillboardParticles(1, 0, 0, 0);
    i = (s32)(-1.3f * 100.0f) + 180 - 80;
    assert(i < 0);
    Word(3, 0xE4000000 | 300 << 12 | (i + 160), 180 << 12 | 0);
    Word(4, 0xB4000000, 0x7E0 - (((s16)i * (s16)(-129024 / 80)) >> 7));

    /* The smoke: the texture, the billboard matrix, and the colours of the track. */
    g_pActiveCamera = &camera;
    Identity((f32(*)[4]) & camera);
    Identity(g_afPerspectiveMatrix);
    g_wTireSmokeListHead = g_wPavedTireSmokeListHead = 0;
    Reset();
    RenderTireSmoke();
    ParticleTexture();
    Word(14, 0xB9000201, 4);
    Word(15, 0xBA000602, 0xC0);
    Word(16, 0xBA000402, 0xC0);
    Word(17, 0xB900031D, 0x0F0A0233);
    Word(18, 0xF9000000, 0);
    RestoredModes(19);
    assert(Count() == 25);
    assert(g_afBillboardMatrix[0][2] == -1.0f && g_afBillboardMatrix[1][0] == -1.0f);
    assert(g_afBillboardMatrix[2][1] == -1.0f && g_afBillboardMatrix[3][3] == 1.0f);
    assert(g_afScratchMatrix[0][2] == -1.0f && g_afBillboardMatrix[0][0] == 0.0f);
    g_dwNightMode = 1;
    Reset();
    RenderTireSmoke();
    Word(15, 0xBA000602, 0xC0);
    RestoredModes(16);
    assert(Count() == 22);
    g_dwNightMode = 0;
    /* The colours of the smoke of tracks 4 and 9. */
    g_wTireSmokeListHead = 1;
    g_aParticlePool[1].position.y = 0.0f;
    g_aParticlePool[1].alpha = 255;
    g_aParticlePool[1].fade = 255;
    g_dwSelectedTrackId = 9;
    Reset();
    RenderTireSmoke();
    Word(20, 0xFA00FFFF, 0x705838FE);
    g_dwSelectedTrackId = 6;
    Reset();
    RenderTireSmoke();
    Word(20, 0xFA00FFFF, 0x605438FE);
    g_dwSelectedTrackId = 0;
    Reset();
    RenderTireSmoke();
    Word(20, 0xFA00FFFF, 0xA08860FE);
    g_wTireSmokeListHead = 0;

    /* The exhaust shows in the snow, then the spray follows. */
    g_dwSnowMode = 1;
    g_dwSelectedTrackId = 2;
    g_wExhaustParticleListHead = 1;
    Racer();
    Reset();
    RenderExhaustAndTireSpray();
    ParticleTexture();
    Word(14, 0xB9000201, 4);
    Word(15, 0xBA000602, 0xC0);
    Word(16, 0xBA000402, 0x80);
    Word(17, 0xB900031D, 0x00504B50);
    Word(19, 0xFA00FFFF, 0xE0E0FFFE);
    RestoredModes(24);
    Word(30, 0xE7000000, 0);
    Word(31, 0xBA001402, 0x00100000);
    g_dwSnowMode = 0;
    g_dwSelectedTrackId = 0;
    Reset();
    RenderExhaustAndTireSpray();
    RestoredModes(16);
    Word(23, 0xBA001402, 0x00100000);

    /* The effects of the racers by the weather. */
    InitParticleSystem();
    g_wParticleFreeListHead = 0;
    v = Racer();
    v->engineRpm = 6000.0f;
    g_fFrameDeltaTime = 1.0f;
    g_dwSnowMode = 1;
    g_dwSelectedTrackId = 2;
    UpdateAllVehicleEffects();
    /* The first call rebuilt the emptied pool before the puff was taken from it. */
    assert(g_wExhaustParticleListHead == 1);
    g_dwSnowMode = 0;
    v->body.skidding = 1;
    v->speedKph = 100.0f;
    v->wheelBodies[3].contactFrames = 1;
    v->wheelBodies[3].surface = 1;
    v->smokeTimers[0] = 0.76f;
    UpdateAllVehicleEffects();
    /* Later calls do not rebuild the pool, and the exhaust list is not emptied. */
    assert(g_wTireSmokeListHead != 0 && g_wExhaustParticleListHead == 1);
    index = g_wTireSmokeListHead;
    g_dwNightMode = 1;
    v->smokeTimers[0] = 0.76f;
    UpdateAllVehicleEffects();
    assert(g_wTireSmokeListHead == index && v->smokeTimers[0] == 0.76f);
    g_dwNightMode = 0;

    puts("effects_check: ok");
    return 0;
}

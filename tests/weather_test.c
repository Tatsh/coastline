/* Host check of src/weather.c with the random numbers and the matrix library stubbed. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "weather.c"

Gfx *gDisplayListPtr;
u32 g_dwTextureFilterMode = 0x2000;
u32 g_dwSnowMode, g_dwRainMode;
s32 g_dwLightningFlashCounter, g_dwRainParticleCount, g_nLightningBoltDepth,
    g_bRainParticlesInitialized, g_dwNumViewports = 1, g_dwCurrentSplitScreenIndex,
                                 g_dwTrackMirrored, g_dwViewportMirrored, g_dwCameraRayHitCount;
f32 g_fWindAngle, g_fWindStrength, g_fRainCameraSpeed, g_fRainCameraSpeedScale,
    g_fLightningFlashTimer, g_fFrameDeltaTime;
f32 g_afMvpMatrix[4][4], g_afPerspectiveMatrix[4][4], g_afScratchMatrix[4][4];
u8 g_aParticleTexture[0x1000];
s16 g_awRainParticlePos[WEATHER_VIEWS][RAIN_PARTICLE_SLOTS][3];
Vec3f g_avRainPrevCameraPos[WEATHER_VIEWS], g_vWindVelocity, g_avRainVelocity[WEATHER_VIEWS],
    g_avLightningBolt[LIGHTNING_BOLT_POINTS];
u8 g_abPrecipitationTextures[WEATHER_VIEWS][RAIN_STREAK_TEXTURE_SIDE * RAIN_STREAK_TEXTURE_SIDE];
u16 g_awCameraRayHitInstances[4];
TrackHeader g_LoadedTrack;
Vehicle g_aVehicles[MAX_VEHICLES];
CameraFrame *g_pActiveCamera;

static u32 queue[64];
static u32 queued, taken;

u32 AdvanceRng(void) {
    return taken < queued ? queue[taken++] : 0;
}
static void Queue(const u32 *values, u32 n) {
    memcpy(queue, values, n * sizeof(u32));
    queued = n;
    taken = 0;
}
void Vec3fMulAdd(Vec3f *out, Vec3f *a, Vec3f *b, f32 s) {
    out->x = a->x + b->x * s;
    out->y = a->y + b->y * s;
    out->z = a->z + b->z * s;
}
void Vec3fScaleInPlace(Vec3f *v, f32 s) {
    v->x *= s;
    v->y *= s;
    v->z *= s;
}
void Vec3fAddInPlace(Vec3f *v, Vec3f *b) {
    v->x += b->x;
    v->y += b->y;
    v->z += b->z;
}
void Vec3fNormalize(Vec3f *v) {
    f32 l = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
    Vec3fScaleInPlace(v, 1.0f / l);
}
void Mtx4x4TransformNoTranslation(Vec3f *out, Vec3f *in, f32 m[4][4]) {
    f32 x = in->x, y = in->y, z = in->z;
    f32 w = 1.0f / (m[3][3] + (x * m[0][3] + y * m[1][3] + z * m[2][3]));
    out->x = (m[2][0] * z + (x * m[0][0] + y * m[1][0])) * w;
    out->y = (m[2][1] * z + (x * m[0][1] + y * m[1][1])) * w;
    out->z = (m[2][2] * z + (x * m[0][2] + y * m[1][2])) * w;
}
void Mtx4x4Multiply(f32 out[4][4], f32 a[4][4], f32 b[4][4]) {
    f32 p[4][4];
    int r, c, k;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            for (p[r][c] = 0, k = 0; k < 4; k++) {
                p[r][c] += a[r][k] * b[k][c];
            }
        }
    }
    memcpy(out, p, sizeof(p));
}
/* Only the identity is inverted here. */
s32 MtxInvert4x4(f32 out[4][4], f32 in[4][4]) {
    memcpy(out, in, sizeof(f32[4][4]));
    return 1;
}
void MemFill64Aligned(void *dst, u32 size, u16 value) {
    memset(dst, value, size);
}
void AddTimingMarker(s32 bar, s32 r, s32 g, s32 b, s32 a) {
}

static u32 Bits(f32 f) {
    u32 b;
    memcpy(&b, &f, 4);
    return b;
}

int main(void) {
    static Gfx list[64];
    static CameraFrame frame;
    static TrackInstance instances[2];
    const u32 snowSetup[][2] = {
        {0xE7000000, 0},
        {0xBA001402, 0},
        {0xBB000001, 0xFFFFFFFF},
        {0xBA000C02, 0x2000},
        {0xFCFFFFFF, 0xFFFDF2F9},
        {0xB900031D, 0x00504240},
        {0xFD900000, (u32)(uintptr_t)g_aParticleTexture},
        {0xF5900000, 0x07018060},
        {0xE6000000, 0},
        {0xF3000000, 0x077FF100},
        {0xE7000000, 0},
        {0xF5881000, 0x00018060},
        {0xF2000000, 0x000FC0FC},
        {0xBA000E02, 0},
        {0xBA001301, 0},
        {0xFA00FFFF, 0xE0E0FFFF},
        {0xE432C22E, 0x0032021C},
        {0xB4000000, 0x000007E0},
        {0xB3000000, 0x5400C800},
        {0xE7000000, 0},
        {0xBA001301, 0x00080000},
    };
    u32 i, n = sizeof(snowSetup) / sizeof(snowSetup[0]);
    f32 scale;
    s32 moveX;
    u8 *streak;

    /* The constants of the original. */
    assert(Bits(RANDOM_TO_TWO) == 0x38000080 && Bits(WIND_TURN) == 0x40C90FDB);
    assert(Bits(RAIN_MIN_CAMERA_SPEED) == 0x3E8E38E3 && Bits(RAIN_CAMERA_SPEED_MUL) == 0x40666667);
    assert(Bits(PARTICLE_SCALE) == 0x38800100 && Bits(PARTICLE_NEAR) == 0x3DCCCCCD);

    /* A strike, its flash and the wait for the next. */
    g_fFrameDeltaTime = 0.5f;
    g_LoadedTrack.minZ = 300.0f;
    g_dwLightningFlashCounter = -1;
    Queue((const u32[]){0x80}, 1);
    UpdateLightningFlash();
    assert(g_dwLightningFlashCounter == -1);
    Queue((const u32[]){0x7F, 100, 0x10200}, 3);
    UpdateLightningFlash();
    assert(g_dwLightningFlashCounter == 3 && g_fLightningFlashTimer == 0.0f);
    assert(g_avLightningBolt[0].x == 100.0f && g_avLightningBolt[0].y == 512.0f &&
           g_avLightningBolt[0].z == 300.0f);
    UpdateLightningFlash();
    assert(g_dwLightningFlashCounter == 2 && g_fLightningFlashTimer == 171.5f);
    g_dwLightningFlashCounter = 0;
    g_fLightningFlashTimer = 1900.0f;
    UpdateLightningFlash();
    assert(g_dwLightningFlashCounter == -1);

    /* The division of the bolt. */
    memset(g_avLightningBolt, 0, sizeof(g_avLightningBolt));
    g_avLightningBolt[4] = (Vec3f){8.0f, 16.0f, 32.0f};
    Queue((const u32[]){3, 10, 0, 7, 0, 0}, 6);
    SubdivideLightningBolt(2, 2);
    assert(g_avLightningBolt[2].x == -0.5f && g_avLightningBolt[2].y == 10.5f &&
           g_avLightningBolt[2].z == 16.0f);
    assert(g_avLightningBolt[1].x == -3.75f && g_avLightningBolt[1].y == 8.75f &&
           g_avLightningBolt[1].z == 8.0f);
    assert(g_avLightningBolt[3].x == 0.25f && g_avLightningBolt[3].y == 9.75f &&
           g_avLightningBolt[3].z == 24.0f);
    assert(g_nLightningBoltDepth == 0);

    /* Snow behind a camera that moved a unit: wind, speed limit, fall and drift. */
    frame.forward.x = 1.0f;
    frame.side.y = 1.0f;
    frame.up.z = 1.0f;
    frame.positionW = 1.0f;
    frame.position.x = 10.0f;
    g_aVehicles[0].activeCamera = &frame;
    g_avRainPrevCameraPos[0] = (Vec3f){12.0f, 0.0f, 0.0f};
    g_bRainParticlesInitialized = TRUE;
    g_dwSnowMode = 1;
    g_fWindAngle = 1.5f;
    g_fWindStrength = 0.75f;
    Queue((const u32[]){0, 0, 0xFFFF, 0, 1, 0, 2}, 7);
    UpdatePrecipitationParticles();
    assert(g_fWindAngle == 1.0f && g_fWindStrength == 0.5f && g_vWindVelocity.z == 0.0f);
    assert(g_vWindVelocity.x == cosf(1.0f) * 0.25f && g_vWindVelocity.y == sinf(1.0f) * 0.25f);
    assert(g_dwRainParticleCount == 512);
    scale = sqrtf(2.0f * 3.600000143f) * 0.2777777612f / 2.0f;
    assert(g_fRainCameraSpeedScale == scale && g_fRainCameraSpeed == 2.0f * scale);
    assert(g_avRainPrevCameraPos[0].x == 13.0f);
    moveX = (s32)((double)scale * 16383.5);
    assert(g_awRainParticlePos[0][0][0] == moveX && g_awRainParticlePos[0][0][1] == -2047);
    assert(g_awRainParticlePos[0][1][0] == moveX + 2047 && g_awRainParticlePos[0][1][1] == 0);
    assert(g_awRainParticlePos[0][2][0] == moveX - 2047 && g_awRainParticlePos[0][2][1] == 0);
    assert(g_awRainParticlePos[0][3][0] == moveX + 2047 && g_awRainParticlePos[0][3][1] == 2047);
    assert(g_awRainParticlePos[0][0][2] == 4095 && g_awRainParticlePos[0][511][2] == 4095);
    assert(g_awRainParticlePos[0][512][2] == 0);

    /* A camera under a shelter draws nothing. */
    g_LoadedTrack.instances = instances;
    instances[1].flags = TRACK_INSTANCE_SHELTER;
    g_awCameraRayHitInstances[0] = 1;
    g_dwCameraRayHitCount = 1;
    gDisplayListPtr = list;
    RenderPrecipitationEffect();
    assert(gDisplayListPtr == list);
    g_dwCameraRayHitCount = 0;

    /* One snowflake on the screen and one above it, through identity matrices. */
    g_pActiveCamera = &frame;
    frame.position.x = 0.0f;
    for (i = 0; i < 4; i++) {
        g_afPerspectiveMatrix[i][i] = 1.0f;
    }
    g_dwRainParticleCount = 2;
    memset(g_awRainParticlePos, 0, sizeof(g_awRainParticlePos));
    g_awRainParticlePos[0][0][1] = 8192;
    g_awRainParticlePos[0][0][2] = 4096;
    g_awRainParticlePos[0][1][1] = -32768;
    gDisplayListPtr = list;
    RenderPrecipitationEffect();
    assert((u32)(gDisplayListPtr - list) == n);
    for (i = 0; i < n; i++) {
        if (list[i].words.w0 != snowSetup[i][0] || list[i].words.w1 != snowSetup[i][1]) {
            printf("%u: %08X %08X\n", i, list[i].words.w0, list[i].words.w1);
            return 1;
        }
    }

    /* The streak of rain falling along the first axis of the view. */
    g_dwSnowMode = 0;
    g_dwRainMode = 1;
    g_dwRainParticleCount = 0;
    g_avRainVelocity[0] = (Vec3f){0.0f, 1.0f, 0.0f};
    memset(g_abPrecipitationTextures, 0xAA, sizeof(g_abPrecipitationTextures));
    gDisplayListPtr = list;
    RenderPrecipitationEffect();
    assert(list[6].words.w1 == (u32)(uintptr_t)g_abPrecipitationTextures[0]);
    assert(list[15].words.w1 == 0x788088FF);
    streak = &g_abPrecipitationTextures[0][32 * 64];
    assert(streak[60] == 0xFF && streak[62] == 0xFF && streak[63] == 0 && streak[8] == 0);
    assert(streak[10] == 0x11 && streak[12] == 0x22 && streak[5] == 0 && streak[4] == 0);
    assert(streak[-2 * 64 + 59] == 0xFF && streak[-2 * 64 + 58] == 0 && streak[-3 * 64 + 60] == 0);
    assert(g_abPrecipitationTextures[1][0] == 0xAA);
    puts("weather ok");
    return 0;
}

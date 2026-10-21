/* Test of src/vehiclerender.c against the branches and words of the disassembly of 30EF0.s. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "vehiclerender.c"

char *getenv(const char *name);

/* Words that may hold anything, and the words of the tile of the highlight, known after drawing. */
#define ANY 0xDEADBEEF
#define HILITE_W0 0x11111111
#define HILITE_W1 0x22222222

Gfx *gDisplayListPtr;
static Gfx gfx[2048];
f32 g_afMvpMatrix[4][4], g_afScratchMatrix[4][4], g_afViewProjMatrix[4][4];
u32 g_dwTextureFilterMode = 0x2000, g_dwColorDitherMode = 0x40;
s32 g_dwTrackMirrored, g_dwViewportMirrored, g_dwNumViewports = 1;
u32 g_dwNightMode, g_dwRainMode, g_dwSnowMode, g_dwWeatherActive;
CameraFrame *g_pActiveCamera;
Vehicle *g_pActiveCameraVehicle;
static Mtx projection;
Mtx *g_pProjectionMtx = &projection;
u16 g_wPerspNorm;
f32 g_afLightDirection[3] = {0.0f, 0.0f, 1.0f};
Lights1 g_DefaultLights;
s32 g_adwVehicleVisible[MAX_VEHICLES];
Gfx g_aClipRatioDisplayLists[CLIP_RATIOS][5];
u8 g_bDirLightR = 0x80, g_bDirLightG = 0x60, g_bDirLightB = 0x40;
u8 g_bAmbientLightR = 0x20, g_bAmbientLightG = 0x30, g_bAmbientLightB = 0x50;
s32 g_dwVehicleLodBias, g_dwVehicleCullingDisabled, g_dwStatVehicleTriangles;
u8 g_bFogR = 0x11, g_bFogG = 0x22, g_bFogB = 0x33, g_bFogIntensity = 0x40;
s32 g_dwVehicleViewModeIndex;
f32 g_fHeadlightGlare;
TrackHeader g_LoadedTrack;
Vehicle g_aVehicles[MAX_VEHICLES];
VehicleModel g_aVehicleModels[2];
s32 g_adwVehicleModelSlotCars[2];
CarModel g_aCarModels[CAR_MODELS];
u32 g_dwRandomSeed;

VehicleModel *g_pCurrentVehicleModelData;
f32 g_mtxVehicleModelScaled[4][4];
u32 g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags;
s32 g_dwVehicleFogAlpha;
s32 g_adwVehicleInViewRange[MAX_VEHICLES], g_anVehicleLod[MAX_VEHICLES];
Mtx *g_apVehicleModelMatrix[MAX_VEHICLES], *g_apVehicleLightMatrix[MAX_VEHICLES];
Lights1 g_aVehicleLights[MAX_VEHICLES];
Vec3f g_vecVehicleLightDir, g_vecVehicleHiliteDir;
u16 g_awVehicleShadePalette[16];
s32 g_bForceVehicleAltVariant, g_bVehicleEnvMapLinearTexGen, g_bPaintShopPreviewActive;
u8 g_aVehicleHighlightTexture[2048], g_aVehicleNightHighlightTexture[2048];
u8 *g_apVehicleReflectionTextures[4];
u16 *g_apVehicleReflectionTluts[4];
u8 g_abVehicleReflectionTexSelect[4][2] = {{0, 1}};
s32 g_dwGameFrameCount;
Vtx g_aSpeedometerNeedleVertices[32];
Mtx g_SpeedometerNeedleMtx;
Gfx g_aSpeedometerNeedleDisplayList[1], g_aTachometerDisplayList[1], g_aTrackShadowDisplayList[1];

void SetDebugMessage(const char *message) {
}

static int visible = 1;
s32 IsPointVisible(f32 x, f32 y) {
    return visible;
}
f32 ComputeFogFactorAtPoint(Vec3f *point) {
    return 0.25f;
}
void NormalizeLightMatrix(f32 m[4][4]) {
}
static int tiles;
void DrawTexturedTile(s32 index, ModelTexture *textures) {
    tiles |= 1 << index;
}
static DisplayListBlock blocks[64];
static int blockCount;
DisplayListBlock *AllocDisplayListBlock(void) {
    return &blocks[blockCount++ & 63];
}
static Hilite hiliteBlock;
Hilite *AllocHiliteBlock(void) {
    return &hiliteBlock;
}
LookAt *AllocLightBlock(void) {
    static LookAt l[2];
    static int i;
    return &l[i++ & 1];
}

static u32 Word(int i) {
    return ((u32 *)gfx)[i];
}

/** Find each pair of words in order in the display list, ANY matching any second word. */
static void ExpectSequence(const u32 *pairs, int count) {
    int n = (int)(gDisplayListPtr - gfx) * 2, at = 0, i;

    for (i = 0; i < count; i += 2) {
        while (at < n &&
               !(Word(at) == pairs[i] && (pairs[i + 1] == ANY || Word(at + 1) == pairs[i + 1]))) {
            at += 2;
        }
        if (at >= n) {
            fprintf(stderr, "missing %08X %08X (pair %d)\n", pairs[i], pairs[i + 1], i / 2);
            assert(0);
        }
        at += 2;
    }
}

static void Dump(void) {
    int i, n = (int)(gDisplayListPtr - gfx) * 2;

    if (getenv("DUMP") != NULL) {
        for (i = 0; i < n; i += 2) {
            fprintf(stderr, "%4d %08X %08X\n", i, Word(i), Word(i + 1));
        }
    }
}

static void CheckDeform(void) {
    static Vtx vertices[2] = {{{{0x100, 0x23, 0x10}, 0, {0, 0}, {0, 0, 0, 0}}},
                              {{{0x10, 0x23, 0x10}, 0, {0, 0}, {0, 0, 0, 0}}}};
    static Gfx list[2];
    Vehicle *v = &g_aVehicles[0];
    Vec3f blow = {2.0f, 0.0f, 0.5f};

    list[0].words.w0 = (G_VTX << 24) | (2 << 10) | 0x1F;
    list[0].words.w1 = (u32)(unsigned long)vertices;
    list[1].words.w0 = (u32)G_ENDDL << 24;
    v->model = &g_aVehicleModels[0];
    v->model->displayLists[0][0] = list;
    DeformVehicleMeshByImpact(v, &blow, 8);
    assert(v->dents[0] == 32 && v->dentJitter == 1);
    assert(vertices[0].v.ob[0] == 0xF8 && vertices[0].v.ob[1] == 0x28 &&
           vertices[0].v.ob[2] == 0xF);
    assert(vertices[1].v.ob[0] == 0x10 && vertices[1].v.ob[1] == 0x23);
    /* A side dented deeply enough dents no more. */
    v->dents[0] = MAX_DENT;
    DeformVehicleMeshByImpact(v, &blow, 8);
    assert(vertices[0].v.ob[0] == 0xF8 && v->dentJitter == 1);
    /* A blow from behind, on side 4: the jitter -3 is moved up to -2, so x is -0x202 and y 0x11.
     * dentX -32 by y: (1 - 12) * -32 >> 5 = 11; dentY -16 by x, bit 7 set: (4 - 14) * -16 >> 5 = 5;
     * dentZ doubled to 8 by x + y = 0xFE0F: (15 - 8) * 8 >> 6 = 0. */
    blow.x = -2.0f;
    v->dents[4] = 0;
    vertices[0].v.ob[0] = -0x200;
    vertices[0].v.ob[1] = 0x13;
    vertices[0].v.ob[2] = 0x20;
    v->dentJitter = 4;
    DeformVehicleMeshByImpact(v, &blow, 8);
    assert(v->dents[4] == 32 && v->dentJitter == -3);
    assert(vertices[0].v.ob[0] == -0x200 + 11 && vertices[0].v.ob[1] == 0x13 + 5);
    assert(vertices[0].v.ob[2] == 0x20);
    v->model->displayLists[0][0] = NULL;
}

static void CheckVisibility(void) {
    Vehicle *v = &g_aVehicles[1], *camera = &g_aVehicles[0];

    v->index = 1;
    camera->index = 0;
    g_pActiveCameraVehicle = camera;
    g_pActiveCamera = &camera->cameras[VEHICLE_CAMERA_CHASE];
    visible = 0;
    CheckVehicleVisibility(v);
    assert(!g_adwVehicleInViewRange[1] && v->fogFactor == 0.25f);
    visible = 1;
    v->ghostMode = VEHICLE_GHOST;
    CheckVehicleVisibility(v);
    assert(g_adwVehicleInViewRange[1] && !g_adwVehicleVisible[1]);
    v->ghostMode = 0;
    /* The own car from the cockpit is in range but not visible. */
    camera->activeCamera = &camera->cameras[VEHICLE_CAMERA_COCKPIT];
    CheckVehicleVisibility(camera);
    assert(g_adwVehicleInViewRange[0] && !g_adwVehicleVisible[0]);
}

static void CheckModel(void) {
    static Gfx body[1], textured[1], highlighted[1], wheel[1];
    static ModelTexture textures[8];
    static u16 headPalette[16], rearPalette[16];
    static PlayerInputState input;
    Vehicle *v = &g_aVehicles[1], *camera = &g_aVehicles[0];
    VehicleModel *m = &g_aVehicleModels[1];
    u32 clip = (u32)(unsigned long)g_aClipRatioDisplayLists[0];
    u32 shade = (u32)(unsigned long)g_awVehicleShadePalette;
    unsigned i;
    u32 expected[] = {0x01060040,
                      ANY,
                      0x01030040,
                      (u32)(unsigned long)&projection,
                      0x06000000,
                      clip,
                      0xBC000002,
                      0x80000040,
                      0x03860010,
                      (u32)(unsigned long)&g_DefaultLights.l[0],
                      0x03880010,
                      (u32)(unsigned long)&g_DefaultLights,
                      0xE7000000,
                      0,
                      0xBA001001,
                      0x00010000,
                      0xB7000000,
                      0x00020205,
                      0xB7000000,
                      0x2000,
                      0xB6000000,
                      0x1000,
                      0xBA001402,
                      0x00100000,
                      0xB900031D,
                      0x0C192038,
                      0xFC127FFF,
                      0xFFFFF238,
                      0xBA000C02,
                      0x2000,
                      0xBA001001,
                      0,
                      0xB6000000,
                      0x000C0000,
                      0xBC00000A,
                      0x80604000,
                      0xBC00040A,
                      0x80604000,
                      0xBC00200A,
                      0x20305000,
                      0xBC00240A,
                      0x20305000,
                      0x039E0010,
                      ANY,
                      0x03980010,
                      ANY,
                      0xFD100000,
                      shade,
                      0xE8000000,
                      0,
                      0xF50001E0,
                      0x07000000,
                      0xE6000000,
                      0,
                      0xF0000000,
                      0x0703C000,
                      0xE7000000,
                      0,
                      0x03840010,
                      ANY,
                      0x03820010,
                      ANY,
                      /* The textured parts. */
                      0xBB000001,
                      0xFFFFFFFF,
                      0xB6000000,
                      0x000C0000,
                      0xE8000000,
                      0,
                      0xF5100000,
                      0x07000000,
                      0xF50001F0,
                      0x06000000,
                      0xF5000100,
                      0x05000000,
                      0xB900031D,
                      0x0C192038,
                      0xFC127FFF,
                      0xFFFFF238,
                      0xBA000E02,
                      0,
                      0xFC1219FF,
                      0xFFFFFE38,
                      0xB6000000,
                      0x00040000,
                      0xBC00000A,
                      0x80604000,
                      0xBA000C02,
                      0x2000,
                      0x06000000,
                      (u32)(unsigned long)textured,
                      /* The highlight. */
                      0xE7000000,
                      0,
                      0xBA001402,
                      0,
                      0xB7000000,
                      0x00040000,
                      0xBB000001,
                      0x0F800F80,
                      0xFA000000,
                      0xFFFFCCFF,
                      0xFCFFFFFF,
                      0xFFFDF2F9,
                      0xB900031D,
                      0x00504F50,
                      0xE8000000,
                      0,
                      0xBA000E02,
                      0,
                      0xFD900000,
                      (u32)(unsigned long)g_aVehicleHighlightTexture,
                      0xF5900000,
                      0x07018060,
                      0xE6000000,
                      0,
                      0xF3000000,
                      0x073FF200,
                      0xE7000000,
                      0,
                      0xF5800800,
                      0x00018060,
                      0xF2000000,
                      0x000FC0FC,
                      0xBA000602,
                      0xC0,
                      HILITE_W0,
                      HILITE_W1,
                      0x06000000,
                      (u32)(unsigned long)highlighted,
                      0xBA000602,
                      0x40,
                      /* The reflection. */
                      0xE7000000,
                      0,
                      0xBA001402,
                      0x00100000,
                      0xB7000000,
                      0x00040000,
                      0xBB000001,
                      0x08001000,
                      0xFC167E2C,
                      0x55FEF379,
                      0xFB000000,
                      0x112233FF,
                      0xB900031D,
                      0x0C192038,
                      0xE8000000,
                      0,
                      0xBA000E02,
                      0,
                      0xFD500000,
                      ANY,
                      0xF5500000,
                      0x0701FC5F,
                      0xE6000000,
                      0,
                      0xF3000000,
                      0x073FF400,
                      0xE7000000,
                      0,
                      0xF5400400,
                      0x0001FC5F,
                      0xF2000000,
                      0x0007C1FC,
                      0xE6000000,
                      0,
                      0xFD100000,
                      ANY,
                      0xE8000000,
                      0,
                      0xF5000100,
                      0x07000000,
                      0xE6000000,
                      0,
                      0xF0000000,
                      0x0703C000,
                      0xE7000000,
                      0,
                      0xBA000E02,
                      0x8000,
                      0xF2FEB002,
                      0x000671FE,
                      0xE7000000,
                      0,
                      0xBC00000A,
                      0,
                      0x03840010,
                      ANY,
                      0x06000000,
                      (u32)(unsigned long)body,
                      0xBD000000,
                      0,
                      0xB6000000,
                      0x00040000,
                      0xBA000E02,
                      0,
                      /* The wheels, then the end. */
                      0xFC127FFF,
                      0xFFFFF238,
                      0xB900031D,
                      0x0C192230,
                      0x01060040,
                      ANY,
                      0x039E0010,
                      ANY,
                      0x06000000,
                      (u32)(unsigned long)wheel,
                      0xBD000000,
                      0,
                      0xE7000000,
                      0,
                      0xBA001402,
                      0,
                      0xFC121824,
                      0xFF33FFFF};

    m->textures = textures;
    m->headlightTexture = 2;
    m->rearLightTexture = 4;
    textures[2].palette = headPalette;
    textures[4].palette = rearPalette;
    m->displayLists[0][VEHICLE_DL_BODY] = body;
    m->displayLists[0][VEHICLE_DL_TEXTURED] = textured;
    m->displayLists[0][VEHICLE_DL_HIGHLIGHTED] = highlighted;
    m->wheelDisplayList = wheel;
    m->triangleCount = 123;
    v->model = m;
    v->index = 1;
    v->input = &input;
    v->throttle = -1.0f;
    input.buttons = 0x40000;
    v->frame.position.x = 5.0f;
    v->frame.up.z = 1.0f;
    v->frame.forward.x = 1.0f;
    camera->input = &input;
    camera->cameraHeading = 0.5f;
    camera->activeCamera = &camera->cameras[VEHICLE_CAMERA_CHASE];
    g_pActiveCamera = &camera->cameras[VEHICLE_CAMERA_CHASE];
    g_pActiveCamera->position.z = 2.0f;
    g_pActiveCamera->forward.y = 1.0f;
    g_adwVehicleInViewRange[1] = TRUE;
    gDisplayListPtr = gfx;
    tiles = 0;
    RenderVehicleModel(v, 0);
    Dump();
    /* The tile of the highlight sits at its first highlight, 64 texels wide and high. */
    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        if (expected[i] == HILITE_W0) {
            expected[i] =
                0xF2000000 | ((hiliteBlock.h.x1 & 0xFFF) << 12) | (hiliteBlock.h.y1 & 0xFFF);
        } else if (expected[i] == HILITE_W1) {
            expected[i] =
                (((252 + hiliteBlock.h.x1) & 0xFFF) << 12) | ((252 + hiliteBlock.h.y1) & 0xFFF);
        }
    }
    ExpectSequence(expected, sizeof(expected) / sizeof(expected[0]));
    /* The camera is above the car, so the underside is not drawn; the near details are. */
    assert(tiles == 1 << NEAR_DETAIL_TEXTURE && g_anVehicleLod[1] == 0);
    assert(g_dwStatVehicleTriangles == 123);
    /* Unlit, braking and reversing lights. */
    assert(headPalette[12] == 1 && headPalette[15] == 1);
    assert(rearPalette[15] == 0x9001 && rearPalette[14] == 0xC000 && rearPalette[9] == 0xF904);
    assert(rearPalette[13] == 0xC000 && rearPalette[6] == 0xFFFE && rearPalette[11] == 0xB5AC);

    /* A far car at night in two views: coarser, with the light dimmed by the distance. */
    g_dwNumViewports = 2;
    g_dwNightMode = TRUE;
    v->frame.position.x = 90.0f;
    input.buttons = 0;
    v->throttle = 1.0f;
    gDisplayListPtr = gfx;
    RenderVehicleModel(v, 0);
    assert(g_anVehicleLod[1] == 2 && rearPalette[15] == 0x7000 && rearPalette[13] == 0x7000);
    assert(rearPalette[8] == 0x9082 && rearPalette[6] == 0xC631 && headPalette[12] == 0);
    {
        const u32 night[] = {0xBC00000A, 0x0E0A0700, 0xBC00200A, 0x20305000};
        ExpectSequence(night, 4);
    }
    g_dwNumViewports = 1;
    g_dwNightMode = FALSE;
}

static void CheckWheels(void) {
    static Gfx wheel[1], snow[1];
    Vehicle *v = &g_aVehicles[2];
    VehicleModel *m = &g_aVehicleModels[0];
    const u32 expected[] = {
        0xE7000000, 0,          0xBA001402, 0x00100000, 0xB900031D, 0x0C184A50,
        0xFB000000, 0x00000080, 0xFC127FFF, 0xFF17F23F, 0x01060040, ANY,
        0x039E0010, ANY,        0x03980010, ANY,        0xBB000001, 0xFFFFFFFF,
        0xB6000000, 0x000C0000, 0xE8000000, 0,          0x06000000, (u32)(unsigned long)snow,
        0xBD000000, 0};
    int i;

    m->wheelDisplayList = wheel;
    m->snowWheelDisplayList = snow;
    v->model = m;
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        m->wheels[i].x = (f32)i;
    }
    v->frame.position.x = 10.0f;
    v->frame.forward.x = 1.0f;
    v->frame.side.y = 1.0f;
    v->frame.up.z = 1.0f;
    v->frame.positionW = 1.0f;
    TransformWheelPositions(2);
    assert(v->wheelFrames[3].position.x == 13.0f && v->wheelFrames[3].forward.x == 1.0f);
    v->ghostMode = VEHICLE_GHOST;
    g_dwVehicleFogAlpha = 0x80;
    g_dwVehicleOtherModeLBlend = BLEND_PLAIN;
    g_dwSnowMode = TRUE;
    gDisplayListPtr = gfx;
    RenderWheelModels(v);
    ExpectSequence(expected, sizeof(expected) / sizeof(expected[0]));
    /* The matrix of the lighting is forced from its physical address. */
    {
        int n = (int)(gDisplayListPtr - gfx) * 2;
        u32 base = (u32)(unsigned long)blocks, physical;

        for (i = 0; i < n; i += 2) {
            if (Word(i) == 0x039E0010) {
                physical = Word(i + 1) - 0x80000000u;
                assert(physical >= base && physical < base + sizeof(blocks));
                assert(Word(i + 3) == Word(i + 1) + 0x10);
                break;
            }
        }
        assert(i < n);
    }
    assert((int)(gDisplayListPtr - gfx) == VEHICLE_WHEELS * 18);
    g_dwSnowMode = FALSE;
}

static void CheckBeams(void) {
    Vehicle *v = &g_aVehicles[1], *camera = &g_aVehicles[0];

    g_dwRainMode = TRUE;
    g_adwVehicleVisible[1] = TRUE;
    v->ghostMode = 0;
    g_pActiveCameraVehicle = camera;
    g_pActiveCamera = &camera->cameras[VEHICLE_CAMERA_CHASE];
    /* The car faces the camera, which looks back along it. */
    v->frame.position.x = 0.0f;
    v->frame.forward.x = 1.0f;
    v->frame.up.z = 1.0f;
    g_pActiveCamera->position.x = 3.0f;
    g_pActiveCamera->position.y = 0.0f;
    g_pActiveCamera->position.z = 0.0f;
    g_pActiveCamera->forward.x = -1.0f;
    g_pActiveCamera->forward.y = 0.0f;
    g_fHeadlightGlare = 0.0f;
    gDisplayListPtr = gfx;
    RenderVehicleHeadlightBeams(v);
    /* glare 1: 750 * 0.05 / 4 */
    assert(fabsf(g_fHeadlightGlare - 750.0f * 0.05f / 4.0f) < 1e-3f);
    g_dwRainMode = FALSE;
}

int main(void) {
    CheckDeform();
    CheckVisibility();
    CheckModel();
    CheckWheels();
    CheckBeams();
    puts("vehiclerender_check: ok");
    return 0;
}

/*
 * Host check of src/trackrender.c against the words and arithmetic of the disassembly of 34E10.s.
 * Build with -no-pie so that the globals, which the display lists hold as 32-bit words, sit below
 * 4 GB.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "trackrender.c"
#include "visibleinstances.c"

Gfx *gDisplayListPtr;
static Gfx gfx[512];
ViewportRect g_aViewportRects[4] = {{10, 20, 100, 50, 0}};
s32 g_dwCurrentSplitScreenIndex, g_dwTrackMirrored, g_dwViewportMirrored, g_dwNumViewports = 1;
CameraFrame *g_pActiveCamera;
Vehicle *g_pActiveCameraVehicle;
f32 g_afMvpMatrix[4][4], g_afScratchMatrix[4][4], g_afPerspectiveMatrix[4][4];
f32 g_afViewProjMatrix[4][4];
static Mtx projection;
Mtx *g_pProjectionMtx = &projection;
Mtx g_mtxIdentity;
u16 g_wPerspNorm = 0x1234;
f32 gFrustumFar = 128.0f;
u32 g_dwTextureFilterMode = 0x2000, g_dwColorDitherMode = 0x40, g_dwAlphaDitherMode = 0x10;
s32 g_dwStatVerticesDrawn, g_dwStatTrianglesDrawn, g_dwStatPolygonsDrawn;
s32 g_bTrackTextureLod, g_bSmoothShading;
u32 g_dwDirLightColor = 0x80604000, g_dwAmbientLightColor = 0x20202000;
Lights1 g_DefaultLights;
f32 g_afLightDirection[3] = {0.0f, 0.0f, 1.0f};
u32 g_adwLightRampColors[4] = {0x11111100, 0x22222200, 0x33333300, 0x44444400};
s32 g_adwVehicleVisible[MAX_VEHICLES];
u32 g_dwNightMode, g_dwRainMode, g_dwSnowMode, g_dwWeatherActive, g_dwSelectedTrackId;
s32 g_dwGameMode;
u8 g_bFogR = 0x11, g_bFogG = 0x22, g_bFogB = 0x33;
u8 g_aParticleTexture[64 * 64];
s32 g_dwFrameBufferIndex;
TrackHeader g_LoadedTrack;
static TrackInstance instances[16];
static u16 gridInstances[16], gridInstanceStarts[GRID_CELLS * GRID_CELLS + 1];
u16 g_awVisibleInstanceList[64];
s32 g_nOpaqueVisibleInstanceCount, g_nDebugHiddenInstanceIndex, g_dwAirplaneInstance;
s32 g_dwVisibleGridMinRow, g_dwVisibleGridMaxRow;
s32 g_adwVisibleGridRowMinCol[GRID_CELLS], g_adwVisibleGridRowMaxCol[GRID_CELLS];
s32 g_dwCameraRayHitCount;
u16 g_awCameraRayHitInstances[8];
RacerSlot g_aRacerSlots[RACER_SLOTS];
s32 g_dwNumVehicles, g_dwNumActiveVehicles;
Vehicle g_aVehicles[MAX_VEHICLES];
static PlayerInputState inputs[MAX_VEHICLES];
static PlayerProfile profile;
PlayerProfile *g_pActiveProfile = &profile;
static VehicleModel model;
u32 g_dwRandomSeed;

Gfx g_aShadowDisplayLists[2][SHADOW_COMMANDS];
Vtx g_aShadowVertices[2][SHADOW_VERTICES];
Gfx *g_pShadowDisplayListWritePtr, *g_pShadowDisplayListStart;
Vtx *g_pShadowVertexWritePtr, *g_pShadowVertexStart;
s32 g_bShadowCullDisabled;
VisibleCell g_aVisibleCellTuples[VISIBLE_CELLS + 1];
s32 g_bVisibleCellListOverflow, g_nVisibleInstanceCount;
u8 g_abInstanceVisibilityMask[16];
s32 g_nVisibleCellIdxNear, g_nVisibleCellIdxMid, g_nVisibleCellIdxFar;
s32 g_nTrackTransparentStartIdx, g_nTrackTransparentStartIdxNear, g_nTrackTransparentStartIdxFar;
s32 g_nDeferredInstanceLastIdx;
u32 g_dwInstanceHideFlags;
u16 g_wActiveInstanceHideMask;
s32 g_bDebugHideInstance;
u32 g_dwTrackOtherModeLBlend, g_dwTrackOtherModeLFlags;
s32 g_nNearbyVehicleCount, g_anNearbyVehicleIdx[MAX_VEHICLES];
f32 g_afNearbyVehicleDot[MAX_VEHICLES];
Vec3f g_vVehicleVisibilityDelta, g_vTrackLightDirection, g_vShadowForward;
f32 g_fViewMatrixMaxAbs;
LookAt *g_pTrackLookAt;
Lights1 g_aTrackLights[4];
s32 g_nTrackLightBufferIndex = 3;

/* The source display lists and vertices of the placed models. */
static Gfx sourceList[16], plainList[1];
static Vtx sourceVertices[64];

/* Stubs of the routines the file calls, which record what they were given. */
static s32 scissors, fogs, viewports, debugSteps, prints, blocksUsed;
static s32 scissorArgs[4];
static const char *debugMessage;
static DisplayListBlock blocks[16];
static LookAt lookAt;
static f32 lookEye[3], converted[16][4][4];
static s32 conversions;

void SetScissorRect(s32 x, s32 y, s32 width, s32 height) {
    scissorArgs[0] = x;
    scissorArgs[1] = y;
    scissorArgs[2] = width;
    scissorArgs[3] = height;
    scissors++;
    gDisplayListWords(gDisplayListPtr++, 0xED000000, (u32)(x << 16 | y));
}
void SetupFog(void) {
    fogs++;
    gDisplayListWords(gDisplayListPtr++, 0xF8000000, 0);
}
void RestoreCurrentViewport(void) {
    viewports++;
    gDisplayListWords(gDisplayListPtr++, 0x03800010, 0);
}
void StepDebugHiddenInstance(void) {
    debugSteps++;
}
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
}
DisplayListBlock *AllocDisplayListBlock(void) {
    return &blocks[blocksUsed++];
}
LookAt *AllocLightBlock(void) {
    return &lookAt;
}
void guMtxF2L(float mf[4][4], Mtx *m) {
    s32 r, c;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            converted[conversions][r][c] = mf[r][c];
        }
    }
    conversions++;
}
void guLookAtReflectF(float mf[4][4],
                      LookAt *l,
                      float xEye,
                      float yEye,
                      float zEye,
                      float xAt,
                      float yAt,
                      float zAt,
                      float xUp,
                      float yUp,
                      float zUp) {
    lookEye[0] = xEye;
    lookEye[1] = yEye;
    lookEye[2] = zEye;
    assert(l == &lookAt && zUp == 1.0f && xAt == 0.0f && yUp == 0.0f);
}
void CopyVec4(void *dst, const void *src) {
    s32 i;

    for (i = 0; i < 4; i++) {
        ((u32 *)dst)[i] = ((const u32 *)src)[i];
    }
}
/* Each word holds the value in the byte order of the console. */
void MemFill64Aligned(void *dst, u32 size, s32 value) {
    u32 i;

    for (i = 0; i < size; i++) {
        ((u8 *)dst)[i] = (u32)value >> (24 - 8 * (i & 3));
    }
}
void QuickSort(void *base, u32 count, u32 size, CompareFunc compare) {
    VisibleCell *cells = base, swap;
    u32 i, j;

    assert(size == sizeof(VisibleCell));
    for (i = 1; i < count; i++) {
        for (j = i; j > 0 && compare(&cells[j - 1], &cells[j]) > 0; j--) {
            swap = cells[j];
            cells[j] = cells[j - 1];
            cells[j - 1] = swap;
        }
    }
}
s32 IsPointVisible(f32 x, f32 y) {
    return 1;
}
u32 LookupInstanceGridCell(s32 column, s32 row) {
    s32 cell = column + row * GRID_CELLS;

    return (u32)(gridInstanceStarts[cell + 1] - gridInstanceStarts[cell]) << 16 |
           gridInstanceStarts[cell];
}
u16 NextGridCellInstance(TrackGridCursor *cursor) {
    if (cursor->count == 0) {
        return 0;
    }
    cursor->count--;
    return gridInstances[cursor->start++];
}
void SetDebugMessage(const char *message) {
    debugMessage = message;
}
void osSyncPrintf(const char *fmt, ...) {
    prints++;
}
s32 FloatToInt(float value) {
    return (s32)value;
}

static void Reset(void) {
    gDisplayListPtr = gfx;
}
static u32 Count(void) {
    return (u32)(gDisplayListPtr - gfx);
}

static void Word(Gfx *list, u32 i, u32 w0, u32 w1) {
    if (list[i].words.w0 != w0 || list[i].words.w1 != w1) {
        fprintf(stderr,
                "word %u: %08x %08x, expected %08x %08x\n",
                i,
                list[i].words.w0,
                list[i].words.w1,
                w0,
                w1);
    }
    assert(list[i].words.w0 == w0 && list[i].words.w1 == w1);
}

static void Identity(f32 m[4][4]) {
    s32 r, c;

    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            m[r][c] = r == c ? 1.0f : 0.0f;
        }
    }
}

static void SetVertex(s32 i, s16 x, s16 y) {
    sourceVertices[i].v.ob[0] = x;
    sourceVertices[i].v.ob[1] = y;
    sourceVertices[i].v.ob[2] = 0;
    sourceVertices[i].v.cn[3] = (u8)(0x40 + i);
}

int main(void) {
    s16 low[2] = {-1, -1}, high[2] = {-1, -1};
    VisibleCell a = {0, 0, 3}, b = {0, 0, 5};
    Vehicle *v;
    Gfx *shadow;
    f32 scale;
    s32 i;

    g_LoadedTrack.instances = instances;
    g_LoadedTrack.gridInstances = gridInstances;
    g_LoadedTrack.gridInstanceStarts = gridInstanceStarts;
    g_LoadedTrack.instanceCount = 16;
    for (i = 0; i < MAX_VEHICLES; i++) {
        g_aVehicles[i].index = i;
        g_aVehicles[i].input = &inputs[i];
        g_aVehicles[i].model = &model;
    }

    /* A point and a size to the corners of their rectangle on the screen. */
    Identity(g_afViewProjMatrix);
    {
        Vec3f point = {0.5f, 0.25f, 0.0f};

        ProjectWorldToScreenRect(&point, 1, low, high);
        assert(low[0] == 35 && low[1] == 63 && high[0] == 135 && high[1] == 14);
        g_dwViewportMirrored = 1;
        ProjectWorldToScreenRect(&point, 1, low, high);
        assert(low[0] == -15 && high[0] == 85 && low[1] == 63);
        g_dwViewportMirrored = 0;
        g_afViewProjMatrix[3][3] = 0.0f;
        low[0] = 99;
        ProjectWorldToScreenRect(&point, 1, low, high);
        assert(low[0] == 99);
        g_afViewProjMatrix[3][3] = 1.0f;
    }

    assert(CompareVisibleCellDistance(&a, &b) == -1 && CompareVisibleCellDistance(&b, &a) == 1);
    assert(CompareVisibleCellDistance(&a, &a) == 0);

    g_dwFrameBufferIndex = 1;
    ResetShadowBuffers();
    assert(g_pShadowDisplayListWritePtr == g_aShadowDisplayLists[1]);
    assert(g_pShadowDisplayListStart == g_aShadowDisplayLists[1]);
    assert(g_pShadowVertexWritePtr == g_aShadowVertices[1] &&
           g_pShadowVertexStart == g_aShadowVertices[1]);

    /* A placed model no vehicle casts a shadow on is drawn as it is, and counted once. */
    v = &g_aVehicles[0];
    g_pActiveCameraVehicle = &g_aVehicles[2];
    g_pActiveCamera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_CHASE];
    instances[1].displayList = plainList;
    instances[1].triangleCount = 7;
    instances[1].vertexCount = 9;
    instances[1].polygonCount = 3;
    Reset();
    RenderTrackInstance(1, 0, 0);
    assert(Count() == 3);
    Word(gfx, 0, 0xBB001001, 0xFFFFFFFF);
    Word(gfx, 1, 0xE8000000, 0);
    Word(gfx, 2, 0x06000000, (u32)plainList);
    assert(g_dwStatTrianglesDrawn == 7 && g_dwStatVerticesDrawn == 9 && g_dwStatPolygonsDrawn == 3);
    /* In the mirror, or on a model that takes no shadows, it is drawn as it is too. */
    g_pActiveCamera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_MIRROR];
    Reset();
    RenderTrackInstance(1, 1, 0);
    assert(Count() == 3);
    g_pActiveCamera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_CHASE];
    instances[1].flags = TRACK_INSTANCE_NO_SHADOW;
    Reset();
    RenderTrackInstance(1, 1, 0);
    assert(Count() == 3);
    instances[1].flags = 0;

    /* A shadow: the vertices on the texture, the triangles off it left out. */
    Identity(instances[2].matrix);
    instances[2].displayList = sourceList;
    instances[2].triangleCount = 1;
    SetVertex(0, -600, 0);
    SetVertex(1, -700, 10);
    SetVertex(2, -800, 20);
    SetVertex(3, 0, 0);
    gSPVertex(&sourceList[0], sourceVertices, 4, 0);
    gSP2Triangles(&sourceList[1], 0, 1, 2, 0, 1, 2, 3, 0);
    gDisplayListWords(&sourceList[2], 0xBF000000, 0 << 17 | 1 << 9 | 3 << 1);
    gDisplayListWords(&sourceList[3], 0xBF000000, 0 << 17 | 1 << 9 | 2 << 1);
    gDisplayListWords(&sourceList[4], 0xFC000000, 0);
    gSPVertex(&sourceList[5], sourceVertices, 2, 0);
    gSPEndDisplayList(&sourceList[6]);
    v->frame.forward.x = 1.0f;
    v->frame.side.y = 1.0f;
    v->frame.up.z = 1.0f;
    v->fogFactor = 0.5f;
    v->shadowCorners[0][0] = 12;
    v->shadowCorners[0][1] = 60;
    v->shadowCorners[1][0] = 90;
    v->shadowCorners[1][1] = 25;
    model.wheels[0].x = 235.52f;
    model.wheels[0].y = 161.28f;
    g_dwTrackOtherModeLBlend = 0x0C080000;
    g_dwTrackOtherModeLFlags = 0x00112038;
    g_dwStatTrianglesDrawn = 0;
    Reset();
    shadow = g_pShadowDisplayListWritePtr;
    RenderTrackInstance(2, 1, 1);
    assert(Count() == 32 && g_dwStatTrianglesDrawn == 3);
    Word(gfx, 0, 0xBB001001, 0xFFFFFFFF);
    Word(gfx, 2, 0xFA001700, 0xFF0000FF);
    Word(gfx, 3, 0x06000000, (u32)sourceList);
    Word(gfx, 5, 0xBA001402, 0);
    Word(gfx, 6, 0xFD900000, (u32)g_aParticleTexture);
    Word(gfx, 7, 0xF5900000, 0x07018060);
    Word(gfx, 9, 0xF3000000, 0x077FF100);
    Word(gfx, 10, 0xF5881000, 0x0009BE6F);
    Word(gfx, 11, 0xF2000000, 0x000FC0FC);
    Word(gfx, 12, 0xBB000001, 0xFFFFFFFF);
    Word(gfx, 13, 0xBA001001, 0);
    Word(gfx, 15, 0xB900031D, 0x00504B50);
    Word(gfx, 16, 0xB9000002, 1);
    Word(gfx, 17, 0xF9000000, 8);
    Word(gfx, 18, 0xFC567EAC, 0xFFFFF3F9);
    Word(gfx, 19, 0xB6000000, 0x00070004);
    Word(gfx, 20, 0xBA000602, 0xC0);
    Word(gfx, 21, 0x06000000, (u32)shadow);
    Word(gfx, 23, 0xED000000, 10 << 16 | 20);
    Word(gfx, 24, 0xBA000602, 0x40);
    Word(gfx, 26, 0xB7000000, 0x00020004);
    Word(gfx, 29, 0xBA001402, 0x00100000);
    Word(gfx, 30, 0xB900031D, 0x0C192038);
    Word(gfx, 31, 0xFC26A004, 0x1FFC93F8);
    /* The list of the shadow: the colour of the fog, the scissor of the vehicle, the load. */
    Word(shadow, 0, 0xE7000000, 0);
    Word(shadow, 1, 0xFB000000, 0x1122337F);
    Word(shadow, 2, 0xED000000, 12 << 16 | 25);
    assert(scissorArgs[2] == 78 && scissorArgs[3] == 35);
    Word(shadow, 3, sourceList[0].words.w0, (u32)g_aShadowVertices[1]);
    /* The first triangle is off the texture and the second is drawn alone. */
    Word(shadow, 4, 0xBF000000, sourceList[1].words.w1 & 0xFFFFFF);
    Word(shadow, 5, 0xBF000000, sourceList[2].words.w1);
    /* The triangle off the texture, the other command, and the unused load are dropped. */
    Word(shadow, 6, 0xB8000000, 0);
    assert(g_pShadowDisplayListWritePtr == shadow + 7);
    assert(g_pShadowVertexWritePtr == g_aShadowVertices[1] + 4);
    assert(g_aShadowVertices[1][0].v.tc[0] == -600 + 512 && g_aShadowVertices[1][0].v.tc[1] == 512);
    assert(g_aShadowVertices[1][3].v.tc[0] == 512 && g_aShadowVertices[1][1].v.tc[1] == 522);
    assert(g_aShadowVertices[1][2].v.cn[3] == 0x42);
    /* Seen from its own cockpit, a vehicle casts no shadow; with the culling off, all is kept. */
    g_pActiveCameraVehicle = &g_aVehicles[0];
    g_pActiveCamera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_COCKPIT];
    ResetShadowBuffers();
    shadow = g_pShadowDisplayListWritePtr;
    Reset();
    RenderTrackInstance(2, 1, 0);
    Word(gfx, 15, 0xB900031D, 0x00504F50);
    Word(shadow, 0, 0xB8000000, 0);
    g_pActiveCamera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_CHASE];
    g_bShadowCullDisabled = TRUE;
    ResetShadowBuffers();
    shadow = g_pShadowDisplayListWritePtr;
    Reset();
    RenderTrackInstance(2, 1, 0);
    assert(Count() == 31);
    Word(shadow, 2, sourceList[0].words.w0, (u32)g_aShadowVertices[1]);
    Word(shadow, 3, sourceList[1].words.w0, sourceList[1].words.w1);
    Word(shadow, 4, sourceList[2].words.w0, sourceList[2].words.w1);
    Word(shadow, 5, sourceList[3].words.w0, sourceList[3].words.w1);
    Word(shadow, 6, 0xB8000000, 0);
    g_bShadowCullDisabled = FALSE;
    /* A load of too many vertices is reported. */
    gSPVertex(&sourceList[0], sourceVertices, 33, 0);
    ResetShadowBuffers();
    Reset();
    RenderTrackInstance(2, 1, 0);
    assert(debugMessage != NULL);
    gSPVertex(&sourceList[0], sourceVertices, 4, 0);

    /* The cells in view, the nearest first, and their models, each once. */
    g_pActiveCameraVehicle = &g_aVehicles[2];
    g_pActiveCamera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_CHASE];
    g_pActiveCamera->forward.x = 1.0f;
    g_pActiveCamera->position.x = 40.0f;
    g_pActiveCamera->position.y = 8.0f;
    g_dwVisibleGridMinRow = 0;
    g_dwVisibleGridMaxRow = 1;
    g_adwVisibleGridRowMinCol[0] = 0;
    g_adwVisibleGridRowMaxCol[0] = 1;
    g_adwVisibleGridRowMinCol[1] = 0;
    g_adwVisibleGridRowMaxCol[1] = 0;
    /* Cell (0, 0) holds model 5, cell (1, 0) model 3, cell (0, 1) models 4, 3, 8, and 9. */
    gridInstances[0] = 5;
    gridInstances[1] = 3;
    gridInstances[2] = 4;
    gridInstances[3] = 3;
    gridInstances[4] = 8;
    gridInstances[5] = 9;
    for (i = 0; i <= GRID_CELLS * GRID_CELLS; i++) {
        gridInstanceStarts[i] = i == 0 ? 0 : i == 1 ? 1 : i < GRID_CELLS + 1 ? 2 : 6;
    }
    g_dwAirplaneInstance = 6;
    g_dwCameraRayHitCount = 1;
    g_awCameraRayHitInstances[0] = 7;
    instances[7].hidesGroups = 0x4;
    g_DefaultLights.a.l.col[0] = 0x55;
    BuildVisibleInstanceList();
    assert(g_aVisibleCellTuples[0].column == 1 && g_aVisibleCellTuples[0].distance == 0);
    assert(g_aVisibleCellTuples[2].row == 1 && g_aVisibleCellTuples[2].distance == 2);
    assert(g_aVisibleCellTuples[3].column == 0xFF && !g_bVisibleCellListOverflow);
    assert(g_nVisibleInstanceCount == 6);
    assert(g_awVisibleInstanceList[0] == 6 && g_awVisibleInstanceList[1] == 3);
    assert(g_awVisibleInstanceList[2] == 5 && g_awVisibleInstanceList[3] == 4);
    assert(g_awVisibleInstanceList[5] == 9);
    assert(g_nVisibleCellIdxNear == -1 && g_nVisibleCellIdxMid == 3 && g_nVisibleCellIdxFar == 3);
    assert(g_abInstanceVisibilityMask[3] == 0 && g_abInstanceVisibilityMask[10] == 0xFF);
    assert(g_wActiveInstanceHideMask == 0x4);
    assert(lookEye[0] == 50.0f && lookEye[1] == 0.0f && g_pTrackLookAt == &lookAt);
    assert(g_nTrackLightBufferIndex == 0 && g_aTrackLights[0].a.l.col[0] == 0x55);
    assert(g_aTrackLights[0].l[0].l.dir[2] == 120 && g_aTrackLights[0].l[0].l.dir[0] == 0);
    /* Vehicles ahead of the camera mark the models near them with their bit. */
    g_dwNumActiveVehicles = 2;
    g_adwVehicleVisible[1] = TRUE;
    g_aVehicles[1].frame.position.x = 43.0f;
    g_aVehicles[1].nearbyInstanceCount = 2;
    g_aVehicles[1].nearbyInstances[0] = 4;
    g_aVehicles[1].nearbyInstances[1] = 10;
    g_adwVehicleVisible[0] = TRUE;
    g_aVehicles[0].frame.position.x = 41.0f;
    BuildVisibleInstanceList();
    assert(g_nNearbyVehicleCount == 1 && g_anNearbyVehicleIdx[0] == 1);
    assert(g_abInstanceVisibilityMask[4] == 2 && g_abInstanceVisibilityMask[10] == 0xFF);
    g_aVehicles[0].frame.position.x = 45.0f;
    BuildVisibleInstanceList();
    assert(g_nNearbyVehicleCount == 2 && g_anNearbyVehicleIdx[0] == 1 &&
           g_anNearbyVehicleIdx[1] == 0);
    g_adwVehicleVisible[0] = g_adwVehicleVisible[1] = FALSE;
    /* The rectangle of the shadow of each racer, kept to the view. */
    g_dwNumVehicles = 1;
    g_aRacerSlots[0].vehicle = &g_aVehicles[1];
    g_aVehicles[1].frame.position.x = 0.1f;
    BuildVisibleInstanceList();
    assert(g_aVehicles[1].shadowCorners[0][0] == 10 && g_aVehicles[1].shadowCorners[0][1] == 70);
    assert(g_aVehicles[1].shadowCorners[1][0] == 110 && g_aVehicles[1].shadowCorners[1][1] == 20);
    /* At night the light comes from the headlights of the car in view. */
    g_dwNightMode = 1;
    g_pActiveCameraVehicle->frame.forward.x = 1.0f;
    BuildVisibleInstanceList();
    assert(g_nTrackLightBufferIndex == 0 && g_aTrackLights[0].l[0].l.dir[0] == -120);
    g_dwNightMode = 0;

    /* The first pass: the modes of the track, then each model in view. */
    instances[6].flags = TRACK_INSTANCE_UNROTATED;
    Identity(instances[6].matrix);
    instances[6].matrix[0][0] = 2.0f;
    instances[6].matrix[3][0] = 3.0f;
    instances[6].displayList = plainList;
    instances[3].flags = TRACK_INSTANCE_RAMPED | TRACK_INSTANCE_TWO_SIDED | 2;
    instances[3].displayList = plainList;
    instances[5].flags = TRACK_INSTANCE_TRANSLUCENT;
    instances[5].displayList = plainList;
    instances[4].displayList = plainList;
    instances[8].groups = 0x4;
    instances[9].flags = TRACK_INSTANCE_DARK_OR_WET;
    g_dwGameMode = GAME_MODE_ARCADE;
    g_nTrackLightBufferIndex = 3;
    Reset();
    blocksUsed = 0;
    conversions = 0;
    RenderTrackGeometry(FALSE);
    assert(g_dwInstanceHideFlags == TRACK_INSTANCE_DARK_OR_WET && debugSteps == 1);
    assert(fogs == 2 && viewports == 1);
    Word(gfx, 0, 0x01030040, (u32)&projection + 0x80000000);
    Word(gfx, 1, 0x01060040, (u32)&g_mtxIdentity + 0x80000000);
    Word(gfx, 2, 0xBC00000E, 0x1234);
    Word(gfx, 3, 0x03840010, (u32)&lookAt);
    Word(gfx, 4, 0x03820010, (u32)&lookAt + 16);
    Word(gfx, 5, 0xBC000002, 0x80000040);
    Word(gfx, 6, 0x03860010, (u32)&g_aTrackLights[0] + 8);
    Word(gfx, 7, 0x03880010, (u32)&g_aTrackLights[0]);
    Word(gfx, 8, 0xBC00000A, 0x80604000);
    Word(gfx, 9, 0xBC00040A, 0x80604000);
    Word(gfx, 10, 0xBC00200A, 0x20202000);
    Word(gfx, 11, 0xBC00240A, 0x20202000);
    Word(gfx, 12, 0xE7000000, 0);
    Word(gfx, 13, 0xBA001301, 0x80000);
    Word(gfx, 14, 0xBA000903, 0xC00);
    Word(gfx, 15, 0xBA000801, 0);
    Word(gfx, 16, 0xB9000002, 1);
    Word(gfx, 17, 0xBA000602, 0x40);
    Word(gfx, 18, 0xBA000402, 0x10);
    Word(gfx, 19, 0xBA001402, 0);
    Word(gfx, 20, 0xF9000000, 0);
    Word(gfx, 21, 0xBA001402, 0x100000);
    Word(gfx, 22, 0xB900031D, 0x0C192038);
    Word(gfx, 23, 0xFC26A004, 0x1FFC93F8);
    Word(gfx, 24, 0xBA001102, 0);
    Word(gfx, 25, 0xBA001001, 0);
    Word(gfx, 26, 0xBA000E02, 0);
    Word(gfx, 27, 0xBA000C02, 0x2000);
    Word(gfx, 28, 0xBC000006, 0);
    Word(gfx, 29, 0xB6000000, 0x00853200);
    Word(gfx, 30, 0xB7000000, 0x000A2005);
    Word(gfx, 31, 0x03800010, 0);
    Word(gfx, 32, 0x01030040, (u32)&projection + 0x80000000);
    Word(gfx, 33, 0xBC00000E, 0x1234);
    Word(gfx, 34, 0x01060040, (u32)&g_mtxIdentity + 0x80000000);
    Word(gfx, 36, 0x03860010, (u32)&g_aTrackLights[0] + 8);
    Word(gfx, 41, 0xBC00240A, 0x20202000);
    Word(gfx, 42, 0xF8000000, 0);
    Word(gfx, 43, 0xF8000000, 0);
    Word(gfx, 44, 0xF9000000, 0);
    Word(gfx, 45, 0xB6000000, 0x00053200);
    Word(gfx, 46, 0xB7000000, 0x000A2005);
    Word(gfx, 47, 0xE7000000, 0);
    Word(gfx, 51, 0xFA001700, 0xFF0000FF);
    Word(gfx, 52, 0xBA001102, 0);
    Word(gfx, 56, 0xBC000404, 1);
    Word(gfx, 57, 0xBC000C04, 1);
    Word(gfx, 58, 0xBC001404, 0xFFFF);
    Word(gfx, 59, 0xBC001C04, 0xFFFF);
    Word(gfx, 60, 0xBB001001, 0xFFFFFFFF);
    Word(gfx, 61, 0xB6000000, 0x000C0000);
    Word(gfx, 62, 0xE8000000, 0);
    Word(gfx, 63, 0xF5100000, 0x07000000);
    Word(gfx, 64, 0xF50001F0, 0x06000000);
    Word(gfx, 65, 0xF5000100, 0x05000000);
    /* The model of the animation: its matrix forced from the view and projection. */
    Word(gfx, 66, 0x01020040, (u32)&g_mtxIdentity + 0x80000000);
    Word(gfx, 67, 0x039E0010, (u32)&blocks[0]);
    Word(gfx, 70, 0x039C0010, (u32)&blocks[0] + 48);
    scale = 1.999755859375f / (2.0f * 1.0f + 0.375f);
    assert(converted[0][3][0] == 3.0f * scale && converted[0][3][3] == scale);
    assert(converted[0][0][0] == scale * 2.0f && converted[0][2][2] == scale * 2.0f);
    Word(gfx, 71, 0xBB001001, 0xFFFFFFFF);
    /* The model lit by its ramp, from both sides, put back after. */
    Word(gfx, 74, 0x01020040, (u32)&blocks[1] + 0x80000000);
    Word(gfx, 75, 0xBC00000A, 0);
    Word(gfx, 77, 0xBC00200A, 0x33333300);
    Word(gfx, 79, 0xB6000000, 0x3000);
    Word(gfx, 82, 0x06000000, (u32)plainList);
    Word(gfx, 83, 0xBC00000A, 0x80604000);
    Word(gfx, 86, 0xBC00240A, 0x20202000);
    Word(gfx, 87, 0xB7000000, 0x2000);
    /* The translucent model waits, and the far cells clip. */
    Word(gfx, 88, 0xBC000404, 1);
    Word(gfx, 92, 0xB7000000, 0x00800000);
    Word(gfx, 93, 0x01020040, (u32)&blocks[2] + 0x80000000);
    Word(gfx, 96, 0x06000000, (u32)plainList);
    /* The hidden models are left out. */
    Word(gfx, 97, 0x01020040, (u32)&g_mtxIdentity + 0x80000000);
    Word(gfx, 98, 0xB6000000, 0x10000);
    Word(gfx, 99, 0xE7000000, 0);
    assert(Count() == 100);
    assert(g_nDeferredInstanceLastIdx == 0 && g_awVisibleInstanceList[0] == 5);
    assert(g_nTrackTransparentStartIdx == 6 && g_nOpaqueVisibleInstanceCount == 6);
    assert(g_nTrackTransparentStartIdxNear == 0x1000 && g_nTrackTransparentStartIdxFar == 0x1000);
    /* The second pass draws the translucent model. */
    Reset();
    RenderTrackGeometry(TRUE);
    assert(g_nVisibleInstanceCount == 7 && debugSteps == 1);
    Word(gfx, 66, 0x01020040, (u32)&blocks[3] + 0x80000000);
    Word(gfx, 69, 0x06000000, (u32)plainList);
    assert(Count() == 73);
    /* Outside arcade races the arcade models hide, and in fog the blender mixes in the fog. */
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    g_dwWeatherActive = 1;
    g_dwRainMode = 1;
    Reset();
    RenderTrackGeometry(FALSE);
    assert(g_dwInstanceHideFlags == (TRACK_INSTANCE_ARCADE | TRACK_INSTANCE_CLEAR_SKY));
    assert(g_dwTrackOtherModeLBlend == 0xC8000000);
    Word(gfx, 30, 0xB7000000, 0x000B2005);

    puts("trackrender_check: ok");
    return 0;
}

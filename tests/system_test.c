/* Test of src/system.c: the DMA in blocks, the pool of its messages, the clock of the frames, the
 * routines of the screen, and the commands of the frame, the fog, the lights, and the textures
 * against the words of the disassembly. */
#include <assert.h>
#include <stdio.h>

#include "system.c"

OSIoMesg g_aIoMessagePool[32];
s32 g_dwIoMessagePendingCount, g_dwIoMessagePoolIndex;
OSMesgQueue g_DmaMessageQueue;
u32 g_dwDmaAssetSizeBuffer, g_dwDmaReadWordBuffer, g_dwDmaChunkSize;
u32 g_dwFrameTimestamp;
s32 g_dwElapsedTimeMs, g_dwPrevElapsedTimeMs, g_dwFrameDeltaMs, g_bRaceScreenActive;
f32 g_fFrameDeltaTime;
u64 osClockRate = 62500000;
ScreenCallback g_pfnCurrentScreen, g_pfnNextScreen, g_pfnSecondaryScreen;
const char *g_pszDebugMessage;

static u32 count;
static s32 dmaCount, received, dmaSizes[8];
static u32 dmaRoms[8];
static ScreenCallback rearm;
OSMesgQueue g_RdpDoneMsgQueue;

u32 osGetCount(void) {
    return count;
}
s32 osRecvMesg(OSMesgQueue *q, OSMesg *m, s32 flag) {
    received++;
    if (q == &g_RdpDoneMsgQueue && rearm != NULL) {
        g_pfnNextScreen = rearm;
    }
    return 0;
}
s32 osPiStartDma(
    OSIoMesg *mb, s32 pri, s32 direction, u32 devAddr, void *dramAddr, u32 size, OSMesgQueue *mq) {
    dmaRoms[dmaCount] = devAddr;
    dmaSizes[dmaCount++] = size;
    return 0;
}
void osInvalDCache(void *vaddr, s32 nbytes) {
}
void UpdateControllerState(void) {
}
void DebugFrameStubA(void) {
}
void DebugFrameStubB(void) {
}

static s32 calls;
static void Screen(void) {
    calls++;
}

/* The state of the renderer. */
Gfx *gDisplayListPtr, *g_pDisplayListBase;
u32 g_dwTextureTmemAddress, g_dwTextureRenderTile, g_dwTextureFilterMode;
u32 g_dwColorDitherMode, g_dwAlphaDitherMode;
s32 g_dwScreenWidth = 320, g_dwScreenHeight = 240, g_dwScreenScaleShift, g_dwFrameBufferIndex;
u16 g_aDepthBuffer[16], g_aFramebuffer0[0x100], g_aFramebuffer1[0x100], *g_apFramebuffers[2];
s32 g_dwFadeRectLeft, g_dwFadeRectRight = 320, g_dwFadeRectTop, g_dwFadeRectBottom = 240;
s32 g_dwViewportIndex, g_dwTrackMirrored, g_dwViewportMirrored;
s32 g_dwViewportScaleZ, g_dwViewportTransZ;
Vp g_aViewports[VIEWPORTS];
s32 g_dwNumViewports, g_dwDisplayModeChangeCountdown, g_bBilinearFiltering, g_bGbiClipping;
s32 g_dwClipRatioIndex, g_dwViGammaEnabled;
ViewportRect g_aViewportRects[2];
Mtx g_mtxIdentity;
Gfx g_aClipRatioDisplayLists[CLIP_RATIOS][5];
s32 g_dwStatUnused0, g_dwStatUnused1, g_dwStatUnused2, g_dwStatUnused3, g_dwStatVehicleTriangles;
s32 g_dwStatPolygonsDrawn, g_dwStatVerticesDrawn, g_dwStatTrianglesDrawn;

/* The weather and the lights. */
u32 g_dwNightMode, g_dwSnowMode, g_dwRainMode, g_dwWeatherActive, g_dwNumLocalPlayers = 1;
u32 g_dwSelectedTrackId;
u8 g_bFogR, g_bFogG, g_bFogB, g_bFogIntensity;
s32 g_dwLightningFlashCounter;
s32 g_dwFogMinDistance, g_dwFogMaxDistance, g_dwFogFactorOffset, g_dwFogFactorScale;
CameraFrame *g_pActiveCamera;
Vehicle *g_pActiveCameraVehicle;
Vec3f g_avLightningBolt[LIGHTNING_BOLT_POINTS];
TrackHeader g_LoadedTrack;
f32 g_afViewProjMatrix[4][4], g_afLightDirection[3];
u8 g_bDirLightR, g_bDirLightG, g_bDirLightB, g_bAmbientLightR, g_bAmbientLightG, g_bAmbientLightB;
u8 g_abLightRampR[4], g_abLightRampG[4], g_abLightRampB[4];
u32 g_dwDirLightColor, g_dwAmbientLightColor, g_adwLightRampColors[4];

/* The patches of the display lists. */
Gfx g_aDlClearCombinePatch[1][2], g_aDlSnowTintCombines[2];
Gfx g_aaDlTrackRenderModes[RENDER_MODE_ROWS][4], g_aaDlGhostRenderModes[RENDER_MODE_ROWS][4];
Gfx g_aaDlGhostWheelRenderModes[RENDER_MODE_ROWS][4];
s32 g_bDisplayListSnowTint, g_dwNumActiveVehicles;
Vehicle g_aVehicles[MAX_VEHICLES];

/* The textures of the track. */
s32 g_dwStreamedSnowMode;
u32 g_dwTrackTextureRomOffset, g_dwRacePauseState;

/* The end of a frame. */
OSTask g_aGfxTask[2];
u64 g_aRspBootUcode[0x1A], g_aGspUcodeText[0x100], g_abRspUcodeData[0x100], g_aRdpOutput[0x100];
u8 g_abRspDramStackBuffer[0x410];
u64 *g_pRdpOutputBuffer = g_aRdpOutput, *g_pRdpOutputBufferEnd = &g_aRdpOutput[0x100];
s32 g_dwOutputBuffReserve = 8, g_dwDisplayListCommandCount, g_dwFirstFlushDone;
s32 g_dwPeakShadowCommandCount, g_dwPeakShadowVertexCount, g_dwPeakDisplayListCommands;
Gfx *g_pShadowDisplayListWritePtr, *g_pShadowDisplayListStart;
Vtx *g_pShadowVertexWritePtr, *g_pShadowVertexStart;
OSMesgQueue g_RspDoneMsgQueue, g_ViRetraceMesgQueue;
s32 g_bShowDepthBuffer, g_dwAppliedScreenScaleShift;
u32 g_dwFlushDuration, g_dwFlushInterval, g_dwFlushStartTime, g_dwFlushValueSource;
u32 g_dwLastFlushValue, g_dwKeepScreenBlanked, g_dwScreenUnblankDelay, osTvType;
OSViMode g_ViModeNtscLan1, g_ViModeMpalLan1, g_ViModeNtscHaf1, g_ViModeMpalHaf1;

static s32 sinCalls, normalizeCalls, features, viModeSets, tasksStarted;
static void *swapped;
static OSViMode *viMode;

s16 sins(u16 x) {
    sinCalls++;
    return 0;
}
/* These stand in for the linked routines through -Wl,--wrap, so the calls of the other units are
 * counted while the matrix routines of vecmath.c run as built. */
f32 __wrap_Vec2fDistance(Vec2f *a, Vec2f *b) {
    return 300.0f;
}
void __wrap_Vec3fNormalize(Vec3f *v) {
    normalizeCalls++;
}
void __wrap_Vec3fTransformByMatrix(f32 out[4], const Vec3f *in, f32 m[4][4]) {
    out[0] = in->x;
    out[1] = in->y;
    out[2] = in->z;
    out[3] = 2.0f;
}
void AddTimingMarker(s32 bar, s32 red, s32 green, s32 blue, s32 alpha) {
}
void BeginTimingFrame(void) {
}
void __wrap_ResetFrameBlocks(void) {
}
void osViSetSpecialFeatures(u32 func) {
    features |= func;
}
void osWritebackDCacheAll(void) {
}
void osViBlack(u8 active) {
}
void osViSetMode(OSViMode *modep) {
    viMode = modep;
    viModeSets++;
}
void osViSwapBuffer(void *frameBufPtr) {
    swapped = frameBufPtr;
}
void *osViGetCurrentFramebuffer(void) {
    return swapped;
}
void osSpTaskLoad(OSTask *tp) {
}
void osSpTaskStartGo(OSTask *tp) {
    tasksStarted++;
}

static Gfx list[64];

/* The command at an index of the list, as its two words. */
static s32 IsCommand(s32 index, u32 w0, u32 w1) {
    return list[index].words.w0 == w0 && list[index].words.w1 == w1;
}

static void TestMatrices(void) {
    f32 m[4][4] = {{1, -3, 0, 0}, {0, 2, 0, 0}, {0, 0, 1, 0}, {-9, 9, 9, 1}};

    /* The last row does not count, and the magnitude of a negative counts. */
    assert(GetMaxAbsValue12(m) == 3.0f);
    m[0][1] = -1.0f;
    assert(GetMaxAbsValue12(m) == 2.0f);
    /* The whole matrix scales by 1.9997559 / (2 + 0.375). */
    NormalizeLightMatrix(m);
    assert(m[1][1] == 2.0f * (1.9997559f / 2.375f) && m[3][0] == -9.0f * (1.9997559f / 2.375f));
}

static void TestTextureSizes(void) {
    static const s32 sizes[] = {1, 2, 3, 4, 5, 16, 32, 33, 64, 128, 256, 257, 512, 1024};
    static const s32 bits[] = {0, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10};
    s32 i, scale, maskBits;

    for (i = 0; i < (s32)(sizeof(sizes) / sizeof(sizes[0])); i++) {
        scale = 0;
        GetTextureScaleAndMask(sizes[i], &scale, &maskBits);
        assert(maskBits == bits[i] && scale == 0xFFFF);
    }
    /* A side past 1024 is reported, with its size less one, and the mask is not set. */
    maskBits = -1;
    GetTextureScaleAndMask(1025, &scale, &maskBits);
    assert(maskBits == -1 && scale == 0xFFFF);
}

static void TestTexturedTile(void) {
    static u8 image[0x200];
    static u16 palette[16];
    static ModelTexture textures[2];

    textures[1].image = image;
    textures[1].palette = palette;
    textures[1].width = 32;
    textures[1].height = 16;
    textures[1].tileS1 = 31;
    textures[1].tileT1 = 15;
    textures[1].mirrorS = 1;
    textures[1].clampT = 1;
    textures[1].format = TEXTURE_FORMAT_CI4;
    g_dwTextureTmemAddress = 0;
    g_dwTextureRenderTile = 0;
    gDisplayListPtr = list;
    DrawTexturedTile(1, textures);
    assert(gDisplayListPtr == &list[16]);
    assert(IsCommand(0, 0xE8000000, 0) && IsCommand(1, 0xFD100000, (u32)image));
    assert(IsCommand(2, 0xF5100000, 0x07010050) && IsCommand(4, 0xF3000000, 0x0707F000));
    assert(IsCommand(6, 0xFD100000, (u32)palette) && IsCommand(8, 0xF50001F0, 0x07000000));
    assert(IsCommand(10, 0xF0000000, 0x0703C000) && IsCommand(12, 0xBA000E02, 0x8000));
    assert(IsCommand(13, 0xF5400400, 0x00F90150) && IsCommand(14, 0xF2002002, 0x0007E03E));
    assert(IsCommand(15, 0xBB000001, 0xFFFFFFFF));

    /* An 8-bit texture clamps both ways and calls sins() with the result discarded. */
    textures[0] = textures[1];
    textures[0].format = TEXTURE_FORMAT_I8;
    gDisplayListPtr = list;
    DrawTexturedTile(0, textures);
    assert(sinCalls == 1 && gDisplayListPtr == &list[9]);
    assert(IsCommand(4, 0xF3000000, 0x070FF000) && IsCommand(5, 0xBA000E02, 0));
    assert(list[6].words.w0 == 0xF5880800 && (list[6].words.w1 & 0xFFFFF) == 0x00090250);
}

static void TestFills(void) {
    /* The bottom of a cleared rectangle of the depth buffer is taken from x. */
    gDisplayListPtr = list;
    ClearZBufferRect(10, 20, 30, 40);
    assert(IsCommand(3, 0xFF100000 | 319, (u32)g_aDepthBuffer - 0x80000000));
    assert(IsCommand(4, 0xF7000000, 0xFFFCFFFC));
    assert(IsCommand(5, 0xF609C0C4, 0x00028050));

    /* In the high resolution, the lower right corner is doubled twice. */
    g_dwScreenScaleShift = 1;
    gDisplayListPtr = list;
    DrawFilledRect(10, 20, 30, 40, 0xFF, 0, 0);
    assert(IsCommand(3, 0xF7000000, 0xF801F801));
    assert(IsCommand(4, 0xF6000000 | 159 << 14 | 239 << 2, 20 << 14 | 40 << 2));
    g_dwScreenScaleShift = 0;

    gDisplayListPtr = list;
    ClearScreen(0, 0xFF, 0x80);
    assert(IsCommand(3, 0xF7000000, 0x07E107E1) && gDisplayListPtr == &list[7]);
}

static void TestViewports(void) {
    /* The scissor is limited to the rectangle of the fade. */
    gDisplayListPtr = list;
    SetScissorRect(-10, 5, 50, 300);
    assert(IsCommand(1, 0xED000014, 0x000A03C0));

    /* A negative width mirrors the view. */
    gDisplayListPtr = list;
    g_dwViewportIndex = 31;
    SetViewport(10, 20, -100, 50, FALSE);
    assert(g_dwViewportIndex == 0 && g_dwViewportMirrored);
    assert(g_aViewports[0].vp.vscale[0] == -200 && g_aViewports[0].vp.vscale[1] == 100);
    assert(g_aViewports[0].vp.vtrans[0] == 240 && g_aViewports[0].vp.vtrans[1] == 180);
    assert(IsCommand(0, 0x03800010, (u32)&g_aViewports[0]) && g_dwViewportScaleZ == 0x1FF);
    /* A mirrored track mirrors it back. */
    g_dwTrackMirrored = TRUE;
    SetViewport(10, 20, -100, 50, FALSE);
    assert(g_aViewports[1].vp.vscale[0] == 200 && g_dwViewportMirrored);
    g_dwTrackMirrored = FALSE;

    gDisplayListPtr = list;
    SetupViewportAndScissor(0, 0, -100, 100, TRUE);
    assert(g_aViewports[2].vp.vscale[0] == 640 && g_aViewports[2].vp.vtrans[1] == 480);
    assert(!g_dwViewportMirrored && IsCommand(2, 0xED000000, 400 << 12 | 400));
    RestoreCurrentViewport();
    assert(IsCommand(4, 0x03800010, (u32)&g_aViewports[2]));
}

static void TestFog(void) {
    static CameraFrame camera;
    static Vehicle vehicle;
    Vec3f point;
    f32 fog;

    /* Clear weather: the fog spans the whole range. */
    gDisplayListPtr = list;
    SetupFog();
    assert(g_bFogIntensity == 0xFF && g_dwFogFactorScale == 128 && g_dwFogFactorOffset == 128);
    assert(IsCommand(0, 0xBC000008, 0x00800080) && IsCommand(1, 0xF8000000, 0xFF));

    g_dwSnowMode = TRUE;
    gDisplayListPtr = list;
    SetupFog();
    assert(g_dwFogFactorScale == 512 && g_dwFogFactorOffset == -307);
    assert(IsCommand(0, 0xBC000008, 0x0200FECD) && IsCommand(1, 0xF8000000, 0xB8B8D8FF));
    g_dwSnowMode = FALSE;

    /* An odd frame of lightning lights the rain by the distance of the bolt, here 300. */
    g_pActiveCamera = &camera;
    g_dwRainMode = TRUE;
    g_dwLightningFlashCounter = 3;
    gDisplayListPtr = list;
    SetupFog();
    assert(g_bFogR == 132 && g_bFogG == 140 && g_bFogB == 147);
    g_dwRainMode = FALSE;

    /* The fog of the desert turns toward grey past a y of 1024. */
    g_dwWeatherActive = TRUE;
    g_pActiveCameraVehicle = &vehicle;
    vehicle.frame.position.z = 50.0f;
    g_LoadedTrack.maxZ = 100.0f;
    g_LoadedTrack.fogColor[0] = g_LoadedTrack.fogColor[1] = g_LoadedTrack.fogColor[2] = 100;
    camera.position.x = 1000.0f;
    camera.position.y = 2048.0f;
    g_dwSelectedTrackId = TRACK_DESERT;
    gDisplayListPtr = list;
    SetupFog();
    assert(g_bFogIntensity == 127 && g_bFogR == 123 && g_bFogB == 123);
    assert(g_dwFogMinDistance == 980 && g_dwFogMaxDistance == 1000);
    g_dwSelectedTrackId = TRACK_MOUNTAIN;
    SetupFog();
    assert(g_bFogR == 100);

    /* The fog of a point is its depth through the fog factors, out of 255, clamped. */
    point.x = 0.0f;
    point.y = 0.0f;
    point.z = 1.96f;
    fog = ComputeFogFactorAtPoint(&point);
    assert(fog > 0.5f && fog < 0.503f);
    point.z = 100.0f;
    assert(ComputeFogFactorAtPoint(&point) == 1.0f);
    point.z = -100.0f;
    assert(ComputeFogFactorAtPoint(&point) == 0.0f);
    g_dwWeatherActive = FALSE;
    assert(ComputeFogFactorAtPoint(&point) == 0.0f);
}

static void TestLights(void) {
    /* A clear day. */
    SetupWeatherLights();
    assert(normalizeCalls == 1 && g_afLightDirection[2] == 20.0f);
    assert(g_dwDirLightColor == 0xFFFFCC00 && g_dwAmbientLightColor == 0x66667700);
    assert(g_adwLightRampColors[0] == 0x3F3F3300 && g_adwLightRampColors[2] == 0xBEBE9900);

    /* The fog blends toward its colour by its intensity. */
    g_dwWeatherActive = TRUE;
    g_bFogR = g_bFogG = g_bFogB = 0;
    g_bFogIntensity = 0xFF;
    SetupWeatherLights();
    assert(g_dwDirLightColor == 0x7F7F6600 && g_dwAmbientLightColor == 0);

    /* The snow lights by the fog colour and has a fixed ramp. */
    g_dwSnowMode = TRUE;
    g_bFogR = g_bFogG = 0xB8;
    g_bFogB = 0xD8;
    SetupWeatherLights();
    assert(g_dwDirLightColor == 0xEDEDCF00 && g_dwAmbientLightColor == 0x73738700);
    assert(g_adwLightRampColors[0] == 0xD0D0F000 && g_adwLightRampColors[3] == 0xFFFFFF00);

    /* The night does not change the direction and has its own lights. */
    g_dwSnowMode = FALSE;
    g_dwNightMode = TRUE;
    SetupWeatherLights();
    assert(normalizeCalls == 3 && g_dwDirLightColor == 0xDDEEFF00);
    assert(g_adwLightRampColors[1] == 0x44444400);
    g_dwNightMode = FALSE;
    g_dwWeatherActive = FALSE;
}

static void SetCommand(Gfx *command, u32 w0, u32 w1) {
    command->words.w0 = w0;
    command->words.w1 = w1;
}

static void TestDisplayListPatches(void) {
    static Gfx track[6], body[2];
    static TrackInstance instances[2];
    s32 row;

    for (row = 0; row < RENDER_MODE_ROWS; row++) {
        SetCommand(&g_aaDlTrackRenderModes[row][0], 0xB900031D, row);
        SetCommand(&g_aaDlTrackRenderModes[row][1], 0xB900031D, 0x100 + row);
        SetCommand(&g_aaDlTrackRenderModes[row][2], 0xB900031D, 0x200 + row);
        SetCommand(&g_aaDlTrackRenderModes[row][3], 0xB900031D, 0x300 + row);
    }
    SetCommand(&g_aDlClearCombinePatch[0][0], 0xFC127E08, 0xF3FFF2F8);
    SetCommand(&g_aDlClearCombinePatch[0][1], 0xFC26A004, 0x1FFC93F8);
    SetCommand(&g_aDlSnowTintCombines[1], 0xFC50FE04, 0x3FFDF3F8);

    /* In the fog, a mode becomes the mode of the third column, and the translucent rows count. */
    g_dwWeatherActive = TRUE;
    SetCommand(&track[0], 0xB900031D, 1);
    SetCommand(&track[1], 0xFC127E08, 0xF3FFF2F8);
    SetCommand(&track[2], 0xFA000000, 0x11111111);
    SetCommand(&track[3], 0xB8000000, 0);
    SetCommand(&track[4], 0xB900031D, 4);
    assert(!ProcessDisplayList(track, g_aaDlTrackRenderModes));
    assert(track[0].words.w1 == 0x201 && track[1].words.w1 == 0x1FFC93F8);
    assert(track[2].words.w1 == 0x11111111 && track[4].words.w1 == 4);
    assert(!ProcessDisplayList(NULL, g_aaDlTrackRenderModes));

    /* Snow tints the colours after a combine it lists, and the rain takes the last column. */
    g_dwSnowMode = TRUE;
    g_dwRainMode = TRUE;
    g_bDisplayListSnowTint = TRUE;
    SetCommand(&track[0], 0xB900031D, 4);
    SetCommand(&track[1], 0xFC50FE04, 0x3FFDF3F8);
    SetCommand(&track[2], 0xFA000000, 0);
    SetCommand(&track[3], 0xFB000000, 0);
    SetCommand(&track[4], 0xB8000000, 0);
    assert(ProcessDisplayList(track, g_aaDlTrackRenderModes) && !g_bDisplayListSnowTint);
    assert(track[0].words.w1 == 0x304 && track[2].words.w1 == 0x60789000);
    assert(track[3].words.w1 == 0x8C9CA800);
    g_dwSnowMode = FALSE;
    g_dwRainMode = FALSE;

    /* The placed models whose modes are translucent are marked. */
    g_LoadedTrack.instances = instances;
    g_LoadedTrack.instanceCount = 2;
    instances[0].displayList = track;
    instances[1].displayList = body;
    SetCommand(&track[0], 0xB900031D, 5);
    SetCommand(&body[0], 0xB8000000, 0);
    g_dwNumActiveVehicles = 0;
    ProcessAllDisplayLists();
    assert(instances[0].flags & TRACK_INSTANCE_TRANSLUCENT);
    assert(!(instances[1].flags & TRACK_INSTANCE_TRANSLUCENT));
    g_dwWeatherActive = FALSE;
}

static void TestTrackTextures(void) {
    static struct {
        TextureAnimation header;
        TextureAnimationFrame frames[3];
    } animation;
    static u8 image[0x40];
    static u16 palette[16];
    static ModelTexture textures[3];

    /* An animated texture streams the frame of the time, and a still one its palette once the
     * snow mode changes. */
    animation.header.frameCount = 3;
    animation.frames[0].time = 0;
    animation.frames[1].time = 100;
    animation.frames[1].imageRom = 0x1000;
    animation.frames[1].paletteRom = 0x2000;
    animation.frames[2].time = 200;
    textures[0].image = image;
    textures[0].palette = palette;
    textures[0].animated = 1;
    textures[0].size = sizeof(image);
    textures[0].format = TEXTURE_FORMAT_CI4;
    textures[0].source.animation = &animation.header;
    textures[2].image = image;
    textures[2].palette = palette;
    textures[2].source.paletteRomIndex = 0x7123;
    g_LoadedTrack.textures = textures;
    g_LoadedTrack.textureCount = 3;
    g_dwTrackTextureRomOffset = 0x100000;
    g_dwElapsedTimeMs = 350;
    g_dwStreamedSnowMode = -1;
    dmaCount = 0;
    StreamAnimatedTrackTextures();
    assert(dmaCount == 3 && dmaRoms[0] == 0x101000 && dmaSizes[0] == sizeof(image));
    assert(dmaRoms[1] == 0x102000 && dmaSizes[1] == 0x20);
    assert(dmaRoms[2] == 0x100000 + 0x2460 && dmaSizes[2] == 0x200);
    assert(g_dwStreamedSnowMode == 0);

    /* Paused, an animated texture waits, and a still one waits for the snow mode to change. */
    g_dwRacePauseState = 1;
    dmaCount = 0;
    StreamAnimatedTrackTextures();
    assert(dmaCount == 0);
    g_dwRacePauseState = 0;
}

static s32 nextCalls, rearmedCalls, secondaryCalls;
static void Rearmed(void) {
    rearmedCalls++;
}
static void Next(void) {
    nextCalls++;
}
static void Secondary(void) {
    secondaryCalls++;
}

static void TestFrames(void) {
    static Gfx pool[2 * DISPLAY_LIST_COMMANDS];
    OSTask *task;

    /* A frame starts after the reserved commands of its half of the pool. */
    g_pDisplayListBase = pool;
    g_dwFrameBufferIndex = 1;
    g_dwNumViewports = 2;
    g_bBilinearFiltering = TRUE;
    StartLowResFrame();
    assert(g_aViewportRects[1].y == 121 && g_aViewportRects[1].width == 224);
    assert(g_aViewportRects[0].height == 112 && g_dwTextureFilterMode == G_TF_BILERP);
    assert(pool[DISPLAY_LIST_COMMANDS + 64].words.w0 == 0xBC000006);
    assert(pool[DISPLAY_LIST_COMMANDS + 68].words.w0 == 0xFCFFFFFF);
    assert(pool[DISPLAY_LIST_COMMANDS + 68].words.w1 == 0xFFFDF638);
    /* The alpha dither goes to the field of the colour dither. */
    assert(pool[DISPLAY_LIST_COMMANDS + 78].words.w0 == 0xBA000602);
    assert(pool[DISPLAY_LIST_COMMANDS + 79].words.w0 == 0xBA000602);
    assert(pool[DISPLAY_LIST_COMMANDS + 79].words.w1 == 0);
    assert(features == (OS_VI_DITHER_FILTER_ON | OS_VI_DIVOT_ON | OS_VI_GAMMA_OFF));

    /* A change of resolution moves the frame buffers below the first of the low resolution. */
    StartHighResFrame();
    assert(g_dwDisplayModeChangeCountdown == 1 && g_dwScreenScaleShift == 1);
    assert(g_apFramebuffers[1] - g_apFramebuffers[0] == 4 * (g_aFramebuffer1 - g_aFramebuffer0));

    /* The first flush only starts the task. */
    FlushDisplayList();
    task = &g_aGfxTask[1];
    assert(g_dwFirstFlushDone == 1 && tasksStarted == 1 && g_dwFrameBufferIndex == 0);
    assert(task->t.ucode_boot_size == (u8 *)g_aGspUcodeText - (u8 *)g_aRspBootUcode);
    assert(task->t.type == 1 && task->t.flags == 6 && task->t.data_ptr == (u64 *)&pool[6064]);
    assert(task->t.output_buff_size == &g_aRdpOutput[0x100 - 8]);
    assert((u32)task->t.dram_stack % 16 == 0 && task->t.data_size == 28 * 8);
    assert(g_dwDisplayListCommandCount == 28 && g_dwPeakDisplayListCommands == 28);

    /* The second routine is tested, but the next is called in its place, here as another routine
     * sets it while the RDP finishes. */
    g_pfnNextScreen = Next;
    g_pfnSecondaryScreen = Secondary;
    rearm = Rearmed;
    StartHighResFrame();
    FlushDisplayList();
    assert(nextCalls == 1 && rearmedCalls == 1 && secondaryCalls == 0 && !g_pfnNextScreen);
    assert(viModeSets == 1 && viMode == &g_ViModeNtscHaf1 && g_dwAppliedScreenScaleShift == 1);
    assert(swapped == g_apFramebuffers[1] && g_dwFrameBufferIndex == 1);
    g_pfnSecondaryScreen = NULL;
    rearm = NULL;
    g_dwScreenScaleShift = 0;
}

int main(void) {
    static u8 buffer[0x2800];
    s32 i;

    /* A read of 10 KB goes as two blocks of 4 KB and the rest, then waits for all three. */
    DmaRead(buffer, 0x1000, 0x2800);
    assert(dmaCount == 3 && dmaRoms[1] == 0x2000 && dmaSizes[2] == 0x800);
    assert(received == 3 && g_dwIoMessagePendingCount == 0);

    /* With 32 messages in flight, the next waits for one and the count stays at 32. The index goes
     * round from the 3 the read took. */
    received = 0;
    for (i = 0; i < 33; i++) {
        AllocIoMessage();
    }
    assert(g_dwIoMessagePendingCount == 32 && received == 1 && g_dwIoMessagePoolIndex == 4);

    /* The clock counts the CPU counter, here 62500 counts a millisecond... */
    ResetFrameClock();
    count = 1000;
    UpdateFrameTiming();
    count = 1000 + 62500 * 3;
    UpdateFrameTiming();
    assert(g_dwElapsedTimeMs == 3 && g_dwFrameDeltaMs == 3 && g_fFrameDeltaTime == 0.003f);
    /* ...and a race steps 1/30 of a second a frame whatever the counter does. */
    g_bRaceScreenActive = TRUE;
    UpdateFrameTiming();
    assert(g_dwFrameDeltaMs == 33);

    SetCurrentScreenCallback(Screen);
    assert(IsCurrentScreen(Screen) && !IsCurrentScreen(NULL));
    CallCurrentGameMode();
    assert(calls == 1);
    SetDebugMessage("hello");
    assert(g_pszDebugMessage[0] == 'h');

    TestMatrices();
    TestTextureSizes();
    TestTexturedTile();
    TestFills();
    TestViewports();
    TestFog();
    TestLights();
    TestDisplayListPatches();
    TestTrackTextures();
    TestFrames();

    puts("system ok");
    return 0;
}

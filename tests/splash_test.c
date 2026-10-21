/* Host check of src/splash.c with the drawing, the sounds, the fades and the loading logged. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "splash.c"

/* The host's string.h clashes with libultra, so use the builtins. */
#define strlen __builtin_strlen
#define memcpy __builtin_memcpy

static int Same(const char *a, const char *b) {
    return strlen(a) == strlen(b) && !__builtin_memcmp(a, b, strlen(a));
}

static int Near(f32 a, f32 b) {
    return fabsf(a - b) <= 1e-3f * (1.0f + fabsf(b));
}

/* The globals the file reads and writes. */
Gfx *gDisplayListPtr;
static Gfx gfx[512];
f32 g_afMvpMatrix[4][4];
Ambient g_MenuAmbientLight, g_SplashAmbientLight;
Light g_MenuDirectionalLight, g_SplashDirectionalLight;
s32 g_dwDrawModelListNoZBuffer, g_dwKeepCustomClipPlanes, g_dwHudDebugLevel, gControllerPakState;
f32 g_fCameraNearScale, g_fCameraFarDistance, g_fFrameDeltaTime;
s32 g_dwScreenWidth = 320, g_dwScreenHeight = 240;
u16 g_wScreenFrameCount;
u8 g_bControllerWarningType, g_bControllerCheckStarted;
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
Sprite g_texBackground, g_texLogo, g_texButtonA;
s32 g_bSplashStarted, g_bSplashFadingOut, g_bSplashSkipped;
Model *g_pSplashFirstLogo, *g_pSplashProducerLogo, *g_pSplashDeveloperLogo, *g_pSplashTitleModelA,
    *g_pSplashTitleModelB, *g_pSplashTitleModelC, *g_pSplashTitleModelD;
f32 g_fSplashTime;
u32 g_dwSplashSoundALength, g_dwSplashSoundBLength;
s8 *g_pSplashSoundA, *g_pSplashSoundB;
s32 g_dwSplashFirstLogoVoice, g_dwSplashFirstLogoSoundA, g_bSplashFirstLogoSoundB;
s32 g_dwSplashProducerVoice, g_dwSplashProducerSoundA, g_bSplashProducerSoundB;
s32 g_dwSplashDeveloperVoice, g_dwSplashDeveloperSoundA, g_bSplashDeveloperSoundB;

/* The logs. */
static struct {
    Model *model;
    f32 x, y, z, angle, scale;
    s32 noZ;
} draws[16];
static u32 drawCount;
static struct {
    char text[80];
    s32 x, y, size, alpha;
} texts[16];
static u32 textCount;
static s32 textSize, textAlpha;
static struct {
    s16 channel;
    s8 *sample;
    u32 length;
} sounds[8];
static u32 soundCount;
static s16 nextChannel;
static f32 fadeTarget = -1.0f, sfxTarget = -1.0f, musicTarget = -1.0f;
static s32 fadingIn, fadingOut, fadeComplete, fadedToBlack;
static ScreenCallback current, next;
static s32 demoStarted, cleared, transfers, scissor[4], vbBlack, loads;
static u32 heap[0x4000], heapUsed;
static Model models[16];
static u32 modelCount;
static DisplayListBlock blocks[8];
static u32 blockCount;

void AdvanceModelAnimations(Model *model) {
    (void)model;
}
void LoopModelAnimations(Model *model) {
    (void)model;
}
Model *DecompressModel(void *dst, u32 rom) {
    assert(dst != NULL);
    /* The stub keeps the offset of the model on the cartridge in its part count. */
    models[modelCount].partCount = rom;
    return &models[modelCount++];
}
void *AllocHeapAligned(u32 size) {
    void *p = &heap[heapUsed];
    heapUsed += (size + 3) / 4;
    assert(heapUsed <= sizeof(heap) / 4);
    return p;
}
void ResetHeap(void) {
    heapUsed = 0;
}
u32 DmaReadAssetSize(u32 rom) {
    return rom == ROM_SPLASH_SOUND_A ? 100 : 60;
}
u32 DecompressFromRom(void *dst, u32 rom, DecompressJob *job) {
    assert(job == NULL);
    /* Fill the sound and what lies after it, so that the check sees the silence written. */
    __builtin_memset(dst, 0x55, (rom == ROM_SPLASH_SOUND_A ? 100 : 60) + SOUND_PADDING);
    return 0;
}
s16 FindFreeSoundChannel(void) {
    return nextChannel;
}
void InitSoundChannel(s16 channel, s8 *sample, u32 length, u32 loop) {
    assert(loop == 0);
    sounds[soundCount].channel = channel;
    sounds[soundCount].sample = sample;
    sounds[soundCount++].length = length;
}
void DrawModelAt(Model *model, f32 x, f32 y, f32 z, f32 angle, f32 scale) {
    draws[drawCount].model = model;
    draws[drawCount].x = x;
    draws[drawCount].y = y;
    draws[drawCount].z = z;
    draws[drawCount].angle = angle;
    draws[drawCount].scale = scale;
    draws[drawCount++].noZ = g_dwDrawModelListNoZBuffer;
}
void SetTextCustomColors(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f) {
    assert(a == b && b == c && c == d && d == e && e == f);
    textAlpha = a;
}
void SetTextStyleNormal(void) {
}
void SetTextSize(s32 size) {
    textSize = size;
}
void SetTextAlignCenter(void) {
}
void SetTextAlignLeft(void) {
}
void DrawText(const char *text, s32 x, s32 y) {
    memcpy(texts[textCount].text, text, strlen(text) + 1);
    texts[textCount].x = x;
    texts[textCount].y = y;
    texts[textCount].size = textSize;
    texts[textCount++].alpha = textAlpha;
}
void guRotateF(float m[4][4], float a, float x, float y, float z) {
    (void)m, (void)a, (void)x, (void)y, (void)z;
}
void guTranslateF(float m[4][4], float x, float y, float z) {
    assert(x == 160.0f && y == 120.0f && z == 100.0f);
    (void)m;
}
void guMtxF2L(float m[4][4], Mtx *mtx) {
    (void)m, (void)mtx;
}
DisplayListBlock *AllocDisplayListBlock(void) {
    return &blocks[blockCount++ % 8];
}
void SetScissorRect(s32 x, s32 y, s32 w, s32 h) {
    scissor[0] = x;
    scissor[1] = y;
    scissor[2] = w;
    scissor[3] = h;
}
void TransferFlags(PlayerInputState *state, u32 mask) {
    state->buttons &= ~mask;
    transfers++;
}
s32 IsFadingIn(void) {
    return fadingIn;
}
s32 IsFadingOut(void) {
    return fadingOut;
}
s32 IsFadeComplete(void) {
    return fadeComplete;
}
s32 IsFadedToBlack(void) {
    return fadedToBlack;
}
void SetFadeTarget(float target, float step) {
    assert(step == 0.2f);
    fadeTarget = target;
}
void FadeSfxTo(float target, float seconds) {
    assert(seconds == 0.2f);
    sfxTarget = target;
}
void FadeMusicTo(float target, float seconds) {
    assert(seconds == 0.2f);
    musicTarget = target;
}
void ClearFramebuffersForWideMode(void) {
    cleared++;
}
void SetCurrentScreenCallback(ScreenCallback callback) {
    current = callback;
}
void SetNextScreenCallback(ScreenCallback callback) {
    next = callback;
}
void ProcessMainMenu(void) {
}
void StartAttractDemo(void) {
    demoStarted++;
}
void UpdateFrameTiming(void) {
}
void UpdateFadeTransitions(void) {
}
void StartHighResFrame(void) {
}
void ClearDepthBuffer(void) {
}
void ClearScreen(s32 r, s32 g, s32 b) {
    assert(!r && !g && !b);
}
void AddTimingMarker(s32 bar, s32 r, s32 g, s32 b, s32 a) {
    assert(!bar && !r && !g && b == 200 && a == 255);
}
void SetupMenuProjection(f32 w, f32 h) {
    assert(w == 320.0f && h == 240.0f);
}
void SetupProjectionMatrix(f32 w, f32 h) {
    assert(w == 320.0f && h == 240.0f);
}
void SetViewport(s32 x, s32 y, s32 w, s32 h, s32 s) {
    assert(!x && !y && w == 320 && h == 240 && s);
}
void ApplyScreenFade(void) {
}
void DrawTimingBars(void) {
}
void FlushDisplayList(void) {
}
void osViBlack(u8 active) {
    vbBlack += active;
}
void LoadTextureFromRom(Sprite *sprite) {
    (void)sprite;
    loads++;
}
void DrawSpriteTinted(Sprite *s, u8 a, u8 b, u8 c) {
    assert(s == &g_texBackground && a == 192 && !b && c == 64);
}
static s32 spriteX[4], spriteY[4], spriteCount;
void DrawTexturedSprite(Sprite *s, s32 x, s32 y) {
    (void)s;
    spriteX[spriteCount] = x;
    spriteY[spriteCount++] = y;
}

static void Reset(void) {
    drawCount = textCount = soundCount = 0;
    spriteCount = 0;
    gDisplayListPtr = gfx;
    g_dwDrawModelListNoZBuffer = 0;
}

/* Run a frame of the splash screens at a time. */
static void Frame(f32 t) {
    Reset();
    g_fSplashTime = t;
    ProcessSplashScreens();
}

static void Text(u32 i, const char *text, s32 x, s32 y, s32 size) {
    assert(i < textCount);
    assert(Same(texts[i].text, text));
    assert(texts[i].x == x && texts[i].y == y && texts[i].size == size);
}

int main(void) {
    u32 i;
    Gfx *g;

    /* The first frame loads the models in their order and the two padded sounds. */
    g_fFrameDeltaTime = 0.25f;
    fadingIn = 1;
    Reset();
    ProcessSplashScreens();
    assert(g_bSplashStarted && fadeTarget == 1.0f && sfxTarget == 1.0f && musicTarget == 1.0f);
    assert(modelCount == 7);
    assert(g_pSplashDeveloperLogo->partCount == ROM_SPLASH_DEVELOPER_LOGO);
    assert(g_pSplashProducerLogo->partCount == ROM_SPLASH_PRODUCER_LOGO);
    assert(g_pSplashFirstLogo->partCount == ROM_SPLASH_FIRST_LOGO);
    assert(g_pSplashTitleModelA->partCount == ROM_SPLASH_TITLE_A);
    assert(g_pSplashTitleModelB->partCount == ROM_SPLASH_TITLE_B);
    assert(g_pSplashTitleModelD->partCount == ROM_SPLASH_TITLE_D);
    assert(g_pSplashTitleModelC->partCount == ROM_SPLASH_TITLE_C);
    assert(g_dwSplashSoundALength == 100 && g_dwSplashSoundBLength == 60);
    assert(g_pSplashSoundA[99] == 0x55 && g_pSplashSoundB[59] == 0x55);
    for (i = 0; i < SOUND_PADDING; i++) {
        assert(g_pSplashSoundA[100 + i] == 0 && g_pSplashSoundB[60 + i] == 0);
    }
    /* Fading in, the time waits; the first logo flies in from the distance. */
    assert(g_fSplashTime == 0.0f);
    assert(drawCount == 1 && draws[0].model == g_pSplashFirstLogo);
    assert(Near(draws[0].x, -64.0f) && draws[0].y == 0.0f && Near(draws[0].z, -1550.0f));
    assert(Near(draws[0].angle, -PI) && draws[0].scale == 1.0f / 32.0f);
    assert(next == AnimateSplashModels);
    assert(g_dwKeepCustomClipPlanes == 1 && g_fCameraNearScale == 10.0f &&
           g_fCameraFarDistance == 5000.0f);
    /* The lights, the clip ratios and the matrix. */
    g = gfx;
    assert(g[0].words.w0 == 0xBC000002 && g[0].words.w1 == 0x80000040);
    assert(g[1].words.w0 == 0x03860010 && g[1].words.w1 == (u32)&g_SplashDirectionalLight);
    assert(g[2].words.w0 == 0x03880010 && g[2].words.w1 == (u32)&g_SplashAmbientLight);
    assert(g[3].words.w0 == 0xBC000404 && g[3].words.w1 == 6);
    assert(g[5].words.w0 == 0xBC001404 && g[5].words.w1 == 0xFFFA);
    assert(g[7].words.w0 == 0xBC000404 && g[7].words.w1 == 1);
    assert(g[10].words.w0 == 0xBC001C04 && g[10].words.w1 == 0xFFFF);
    assert(g[11].words.w0 == 0x01040040);
    assert(gDisplayListPtr - gfx == 13 && g[12].words.w0 == 0xBD000000 && g[12].words.w1 == 0);

    /* Once faded in, the time runs. */
    fadingIn = 0;
    Frame(0.0f);
    assert(g_fSplashTime == 0.25f && soundCount == 0);
    g_fFrameDeltaTime = 0.0f;

    /* At 1 second both sounds of the first logo have played, at full volume. */
    nextChannel = 3;
    Frame(1.0f);
    assert(soundCount == 2 && sounds[0].sample == g_pSplashSoundA && sounds[0].length == 100);
    assert(sounds[1].sample == g_pSplashSoundB && sounds[1].length == 60);
    assert(g_aSoundEffectChannels[3].volume == 0x200020 &&
           g_aSoundEffectChannels[3].step == 1ULL << 32);
    assert(g_dwSplashFirstLogoSoundA == 1 && g_bSplashFirstLogoSoundB == 1);
    assert(Near(draws[0].x, -32.0f) && Near(draws[0].z, -800.0f));
    assert(Near(draws[0].angle, HALF_PI / 16.0f - PI));
    Frame(1.5f);
    assert(soundCount == 0);

    /* Held, then sliding off with the sound of the leaving, which pans after it. */
    Frame(3.0f);
    assert(drawCount == 1 && draws[0].x == 0.0f && draws[0].z == -50.0f &&
           draws[0].angle == -HALF_PI);
    nextChannel = 4;
    Frame(3.6f);
    assert(draws[0].model == g_pSplashFirstLogo && Near(draws[0].x, 0.1f * 0.1f * 256.0f));
    assert(g_dwSplashFirstLogoVoice == 4 && g_dwSplashFirstLogoSoundA == 2);
    assert(soundCount == 1 && g_aSoundEffectChannels[4].volume == 0x140014);
    Frame(3.75f);
    assert(g_aSoundEffectChannels[4].volume == ((12 << 16) | 28));
    /* The producer logo plays its own sounds elsewhere. */
    nextChannel = 2;
    Frame(5.0f);
    assert(g_aSoundEffectChannels[4].volume == 40 && g_dwSplashProducerSoundA == 1);
    assert(draws[0].model == g_pSplashFirstLogo && draws[1].model == g_pSplashProducerLogo);
    Frame(6.0f);
    for (i = 0; i < drawCount; i++) {
        assert(draws[i].model != g_pSplashFirstLogo);
    }

    /* The caption of the producer fades in as its logo lands. */
    Frame(5.9f);
    Text(0, "Produced by", 160, 70, 10);
    assert(texts[0].alpha == 200 - (s32)((f64)(6.0f - 5.9f) * 255.0 * 4.0));
    Frame(5.0f);
    assert(textCount == 0);
    Frame(7.0f);
    assert(texts[0].alpha == 200);
    Frame(9.95f);
    assert(textCount == 1);
    Text(0, "Developed by", 160, 60, 10);
    assert(texts[0].alpha == 200 - (s32)((f64)(10.0f - 9.95f) * 255.0 * 4.0));

    /* The title: its first part tilted and slowing as it comes, the others after it. */
    Frame(14.5f);
    assert(drawCount == 2 && draws[0].model == g_pSplashTitleModelA && draws[0].noZ);
    assert(Near(draws[0].z, (sqrtf(3.0f) - sqrtf(2.5f)) * 300.0f));
    assert(Near(draws[0].angle, sqrtf(2.5f) * 0.01f - HALF_PI) && draws[0].scale == 0.05f);
    assert(draws[1].model == g_pSplashTitleModelB &&
           Near(draws[1].z, (sqrtf(2.0f) - sqrtf(1.5f)) * 500.0f));
    assert(scissor[2] == 320 && scissor[3] == 240);
    /* The rotations of the tilt, pushed, and popped after the first part. */
    assert(gfx[12].words.w0 == 0xBC000002 && gfx[13].words.w1 == (u32)&g_MenuDirectionalLight);
    assert(gfx[15].words.w0 == 0x01040040 && gfx[16].words.w0 == 0x01000040);
    assert(gfx[17].words.w0 == 0xBD000000 && gfx[18].words.w0 == 0xBD000000);
    assert(gDisplayListPtr - gfx == 19);
    Frame(15.5f);
    assert(drawCount == 4);
    assert(draws[0].model == g_pSplashTitleModelA && draws[0].z == 0.0f && signbit(draws[0].z));
    assert(draws[1].model == g_pSplashTitleModelB && draws[1].z == 0.0f);
    assert(draws[2].model == g_pSplashTitleModelC);
    assert(Near(draws[2].z, (sqrtf(16.2f - 15.2f) - sqrtf(15.5f - 15.2f)) * 500.0f));
    assert(draws[3].model == g_pSplashTitleModelD &&
           Near(draws[3].z, (1.0f - sqrtf(0.5f)) * 500.0f));
    assert(draws[3].scale == 1.0f / 12.0f);

    /* The copyright, and the date of the build while every C button is held. */
    Frame(20.0f);
    assert(textCount == 6 && texts[0].alpha == 200);
    Text(0, "O 1997 Boss Game Studios, Inc.  All Rights Reserved.", 160, 190, 7);
    Text(4, "LICENSED BY NINTENDO", 160, 224, 7);
    Text(5, "c", 78, 190, 3);
    g_aPlayerInputState[0].buttons = 0xF00;
    Frame(20.0f);
    Text(0, "25Aug97 11:46:52", 160, 40, 10);
    g_aPlayerInputState[0].buttons = 0;
    Frame(17.25f);
    assert(texts[0].alpha == (s32)((f64)(17.25f - 17.2f) * 255.0 * 4.0));

    /* A skip before the copyright jumps to it; one after it fades out. */
    fadeTarget = sfxTarget = -1.0f;
    g_aPlayerInputState[1].buttons = INPUT_START;
    Frame(5.0f);
    assert(g_fSplashTime == 17.2f && g_bSplashSkipped && transfers == 1 && fadeTarget == -1.0f);
    assert(!g_aPlayerInputState[1].buttons && !g_bSplashFadingOut);
    g_aPlayerInputState[0].buttons = INPUT_A;
    Frame(18.0f);
    assert(fadeTarget == -1.0f && transfers == 2);
    Frame(19.5f);
    assert(g_bSplashFadingOut && fadeTarget == 0.0f && sfxTarget == 0.0f);
    /* Once skipped, the fade starts by itself after the copyright. */
    g_bSplashFadingOut = 0;
    fadeTarget = -1.0f;
    Frame(19.3f);
    assert(g_bSplashFadingOut && fadeTarget == 0.0f && transfers == 2);
    /* Without a skip it ends by itself. */
    g_bSplashFadingOut = g_bSplashSkipped = 0;
    fadeTarget = -1.0f;
    Frame(24.0f);
    assert(!g_bSplashFadingOut && fadeTarget == -1.0f);
    Frame(24.2f);
    assert(g_bSplashFadingOut && fadeTarget == 0.0f);

    /* Faded out, the sounds stop and the demo starts, or the main menu after a skip. */
    fadingOut = fadeComplete = 1;
    for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
        g_aSoundEffectChannels[i].step = 5;
        g_aSoundEffectChannels[i].volume = 5;
    }
    Frame(25.0f);
    assert(!g_bSplashStarted && demoStarted == 1 && cleared == 1);
    for (i = 0; i < SPLASH_SFX_CHANNELS; i++) {
        assert(!g_aSoundEffectChannels[i].step && !g_aSoundEffectChannels[i].volume);
    }
    assert(g_aSoundEffectChannels[5].step == 5);
    g_bSplashStarted = 1;
    g_bSplashSkipped = 1;
    Frame(25.0f);
    assert(current == ProcessMainMenu && demoStarted == 1);
    fadingOut = fadeComplete = 0;

    /* The controller check: two black frames, its pictures, then the warning. */
    current = NULL;
    fadeTarget = -1.0f;
    g_bControllerWarningType = 2;
    g_texLogo.originX = 30;
    g_texLogo.originY = 12;
    Reset();
    ProcessControllerCheck();
    assert(g_bControllerCheckStarted && vbBlack == 2 && loads == 3 && fadeTarget == 1.0f);
    assert(g_wScreenFrameCount == 1 && textCount == 7);
    Text(0, "%ryWARNING", 160, 41, 30);
    Text(1, "%ywCONTROLLER PAK CONTAINS GAME", 160, 85, 18);
    Text(5, "%ywTO SAVE GAME DATA FROM THIS GAME.", 160, 165, 18);
    Text(6, "%wwPROCEED", 142, 225, 11);
    assert(spriteX[0] == 30 && spriteY[0] == 20 && spriteX[1] == 254 && spriteY[1] == 432);
    assert(gDisplayListPtr - gfx == 1 && gfx[0].words.w0 == 0xE7000000);
    g_bControllerWarningType = 0;
    Reset();
    ProcessControllerCheck();
    assert(textCount == 3);
    Text(1, "%ywCONTROLLER NOT FOUND", 160, 124, 18);
    /* It proceeds by itself after its frames. */
    fadeTarget = -1.0f;
    g_wScreenFrameCount = CONTROLLER_CHECK_FRAMES - 1;
    Reset();
    ProcessControllerCheck();
    assert(fadeTarget == -1.0f);
    Reset();
    ProcessControllerCheck();
    assert(fadeTarget == 0.0f && g_wScreenFrameCount == CONTROLLER_CHECK_FRAMES + 1);
    /* Or on A, and leaves for the main menu once faded. */
    fadeTarget = -1.0f;
    g_aPlayerInputState[0].buttons = INPUT_A;
    fadedToBlack = 1;
    gControllerPakState = 7;
    Reset();
    ProcessControllerCheck();
    assert(fadeTarget == 0.0f && !g_aPlayerInputState[0].buttons);
    assert(!g_bControllerCheckStarted && !gControllerPakState && !g_wScreenFrameCount);
    assert(current == ProcessMainMenu && cleared == 3);
    puts("splash ok");
    return 0;
}

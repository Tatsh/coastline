/* Host check of src/hud.c with the drawing of text and rectangles and the DMA logged. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

/* libultra declares memcpy() and strlen(), and the host's string.h clashes with it. */
#include "hud.c"

static int Same(const char *a, const char *b) {
    return strlen(a) == strlen(b) && !__builtin_memcmp(a, b, strlen(a));
}

Gfx *gDisplayListPtr;
u32 g_dwNightMode, g_dwRacePauseState;
s32 g_dwGameMode;
s32 g_dwCurrentSplitScreenIndex, g_dwNumViewports = 1, g_dwFrameBufferIndex, g_dwTotalLaps = 3;
OptionSettings g_OptionSettings;
u32 g_dwNumLocalPlayers = 1;
s32 g_adwDashboardLoadedKey[2] = {-1, -1}, g_adwDashboardLoadedGear[2] = {-1, -1};
static u8 facePixels[0x1000], cellPixels[0x1000];
Texture g_aDashboardTextures[2] = {{0, 0, facePixels, 64, 64, G_IM_FMT_CI, G_IM_SIZ_8b}};
Texture g_aDashboardGearTextures[2] = {{0, 0, cellPixels, 64, 64, G_IM_FMT_CI, G_IM_SIZ_8b}};
u16 g_aawDashboardPalettes[2][256], g_awDashboardBarPalette[256];
Vtx g_aGaugeNeedleVerts[2][2][4];
char g_szHudSpeedTextBuf[16];
ViewportRect g_aViewportRects[4] = {{0, 0, 320, 240, 0}};
CarModel g_aCarModels[CAR_MODELS];
Vehicle g_aVehicles[MAX_VEHICLES];
Vehicle *g_pActiveCameraVehicle = &g_aVehicles[0];
CameraFrame *g_pActiveCamera;

static struct {
    char text[32];
    s32 x, y, size, align;
} texts[16];
static u32 textCount;
static s32 textSize, textAlign, shadow;
static struct {
    void *dst;
    u32 rom, size;
} dmas[8];
static u32 dmaCount;
static struct {
    Texture *t;
    s32 x, y, w, h, blend;
} rects[4];
static u32 rectCount;
static s32 scissor[4];

void DrawText(const char *text, s32 x, s32 y) {
    memcpy(texts[textCount].text, text, strlen(text) + 1);
    texts[textCount].x = x;
    texts[textCount].y = y;
    texts[textCount].size = textSize;
    texts[textCount++].align = textAlign;
}
void SetTextSize(s32 size) {
    textSize = size;
}
void SetTextAlignLeft(void) {
    textAlign = TEXT_ALIGN_LEFT;
}
void SetTextAlignRight(void) {
    textAlign = TEXT_ALIGN_RIGHT;
}
void SetTextAlignCenter(void) {
    textAlign = TEXT_ALIGN_CENTER;
}
void SetTextStyleNormal(void) {
}
void EnableTextShadow(void) {
    shadow = 1;
}
void DisableTextShadow(void) {
    shadow = 0;
}
void SetTextCustomColors(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f) {
    assert(a == 0xFF && b == 0xF0 && c == 0x7D && d == 0xFF && e == 0x78 && f == 0);
}
s32 MeasureTextWidth(const char *text, s32 size) {
    return (s32)strlen(text) * size / 2;
}
void DmaRead(void *dst, u32 rom, s32 size) {
    dmas[dmaCount].dst = dst;
    dmas[dmaCount].rom = rom;
    dmas[dmaCount++].size = size;
}
void DrawTexturedRect(Texture *t,
                      s32 x,
                      s32 y,
                      s32 w,
                      s32 h,
                      s32 pr,
                      s32 pg,
                      s32 pb,
                      s32 pa,
                      s32 er,
                      s32 eg,
                      s32 eb,
                      s32 ea,
                      s32 blend) {
    assert(pr == 0 && pg == 0 && pb == 0 && pa == 0xFF && er == 0xFF && ea == 0xFF);
    rects[rectCount].t = t;
    rects[rectCount].x = x;
    rects[rectCount].y = y;
    rects[rectCount].w = w;
    rects[rectCount].h = h;
    rects[rectCount++].blend = blend;
}
void SetScissorRect(s32 x, s32 y, s32 w, s32 h) {
    scissor[0] = x;
    scissor[1] = y;
    scissor[2] = w;
    scissor[3] = h;
}
u32 AdvanceRng(void) {
    return 0x123;
}

static void Reset(void) {
    textCount = dmaCount = rectCount = 0;
}
static void Text(u32 i, const char *text, s32 x, s32 y, s32 size) {
    if (!Same(texts[i].text, text) || texts[i].x != x || texts[i].y != y || texts[i].size != size) {
        printf("%u: \"%s\" %d %d %d\n", i, texts[i].text, texts[i].x, texts[i].y, texts[i].size);
        assert(0);
    }
}

int main(void) {
    static Gfx list[64];
    const u32 needle[][2] = {
        {0xE7000000, 0},
        {0xFD100000, (u32)(uintptr_t)g_aawDashboardPalettes[0]},
        {0xE8000000, 0},
        {0xF5000100, 0x07000000},
        {0xE6000000, 0},
        {0xF0000000, 0x073FC000},
        {0xE7000000, 0},
        {0xE7000000, 0},
        {0xBA001402, 0},
        {0xFCFFFFFF, 0xFFFE793C},
        {0xB900031D, 0x00552048},
        {0xB6000000, 0x00033000},
        {0xB7000000, 0x00000004},
        {0x0400103F, (u32)(uintptr_t)g_aGaugeNeedleVerts[0][0]},
        {0xB1000204, 0x00000406},
        {0xE7000000, 0},
    };
    Vehicle *v = &g_aVehicles[0];
    CarModel *car = &g_aCarModels[1];
    f32 rpm, angle;
    u32 i;

    /* A needle dashboard loaded for the first time, then drawn again without loading. */
    car->dashboardRom = 0x10000;
    car->dashboardWidth = car->dashboardHeight = 64;
    car->gearX = 27;
    car->gearY = 45;
    car->gearWidth = car->gearHeight = 8;
    car->tachometer = TACHOMETER_NEEDLE;
    car->needleX = car->needleY = 32;
    car->needleFullAngle = -0.5236f;
    car->needleRestAngle = 3.665f;
    v->carModel = 1;
    v->throttle = 10.0f;
    v->gear = 2;
    v->engineRpm = 4000.0f;
    gDisplayListPtr = list;
    RenderDashboardGauges();
    assert(dmaCount == 3);
    assert(dmas[0].dst == facePixels && dmas[0].rom == 0x10400 && dmas[0].size == 0x1000);
    assert(dmas[1].dst == g_aawDashboardPalettes[0] && dmas[1].rom == 0x10000 &&
           dmas[1].size == 0x200);
    assert(dmas[2].dst == cellPixels && dmas[2].rom == 0x10400 + 0x1000 + 64 * 3 &&
           dmas[2].size == 64);
    assert(g_adwDashboardLoadedKey[0] == 1 && g_adwDashboardLoadedGear[0] == 3);
    assert(rectCount == 2 && rects[0].t == &g_aDashboardTextures[0] && rects[0].x == 232 &&
           rects[0].y == 172 && rects[0].w == 64 && rects[0].h == 64);
    assert(rects[1].t == &g_aDashboardGearTextures[0] && rects[1].x == 259 && rects[1].y == 217 &&
           rects[1].w == 8 && rects[1].h == 8);
    assert((u32)(gDisplayListPtr - list) == sizeof(needle) / sizeof(needle[0]));
    for (i = 0; i < sizeof(needle) / sizeof(needle[0]); i++) {
        if (list[i].words.w0 != needle[i][0] || list[i].words.w1 != needle[i][1]) {
            printf("%u: %08X %08X\n", i, list[i].words.w0, list[i].words.w1);
            return 1;
        }
    }
    rpm = 35.0f + 4000.0f;
    angle = 3.665f - rpm * (3.665f - -0.5236f) / 8000.0f;
    assert(g_aGaugeNeedleVerts[0][0][0].v.ob[0] == (s32)(cosf(angle - 0.05f) * 20.0f + 264.0f));
    assert(g_aGaugeNeedleVerts[0][0][0].v.ob[1] == (s32)(sinf(angle - 0.05f) * 20.0f + 36.0f));
    assert(g_aGaugeNeedleVerts[0][0][3].v.ob[0] == (s32)(cosf(angle - 0.3f) * 7.0f + 264.0f));
    assert(g_aGaugeNeedleVerts[0][0][2].v.cn[1] == 0xFF &&
           g_aGaugeNeedleVerts[0][0][2].v.cn[0] == 0);
    Reset();
    gDisplayListPtr = list;
    RenderDashboardGauges();
    assert(dmaCount == 0);
    v->throttle = -1.0f;
    Reset();
    RenderDashboardGauges();
    assert(dmaCount == 1 && dmas[0].rom == 0x10400 + 0x1000 && g_adwDashboardLoadedGear[0] == 0);

    /* A bar whose unlit segments dim, at night and paused. */
    car = &g_aCarModels[2];
    car->dashboardRom = 0x20000;
    car->dashboardWidth = car->dashboardHeight = 64;
    car->tachometer = TACHOMETER_BAR_DIMMED;
    car->barSegments = 10;
    v->carModel = 2;
    g_dwNightMode = 1;
    g_dwRacePauseState = 1;
    for (i = 0; i < 256; i++) {
        g_awDashboardBarPalette[i] = 0xFFFF;
        g_aawDashboardPalettes[0][i] = 0x1234;
    }
    Reset();
    gDisplayListPtr = list;
    RenderDashboardGauges();
    assert(dmaCount == 4 && dmas[1].rom == 0x20200);
    assert(dmas[3].dst == &g_awDashboardBarPalette[244] && dmas[3].rom == 0x20200 + 488 &&
           dmas[3].size == 24);
    assert(g_aawDashboardPalettes[0][245] == 0x1234 && g_aawDashboardPalettes[0][246] == 0xFFFF);
    assert(g_aawDashboardPalettes[0][250] == 0xFFFF && g_aawDashboardPalettes[0][251] == 0x18C7);
    assert(g_aawDashboardPalettes[0][255] == 0x18C7);
    car->tachometer = TACHOMETER_BAR_HIDDEN;
    Reset();
    RenderDashboardGauges();
    assert(g_aawDashboardPalettes[0][250] == 0xFFFF && g_aawDashboardPalettes[0][251] == 0xFFFE);
    g_dwRacePauseState = 0;
    g_dwNightMode = 0;

    /* The times of each mode. */
    Reset();
    DrawLabelWithTime("A", "%ww", 83.456f, 10, 20);
    Text(0, "%ww1'23\"45", 10, 35, 0);
    Text(1, "A", 10, 20, 0);
    v->totalTime = 61.0f;
    v->lapTime = 1.5f;
    v->bestLapTime = 30.0f;
    v->timeLeft = 9.0f;
    Reset();
    DrawRaceTimerHud();
    assert(textCount == 4 && texts[0].align == TEXT_ALIGN_RIGHT);
    Text(1, "%15TOTAL TIME", 296, 20, 15);
    Text(2, "%ww0'01\"50", 296, 65, 15);
    Text(3, "%15LAP TIME", 296, 50, 15);
    g_dwGameMode = GAME_MODE_ARCADE;
    Reset();
    DrawRaceTimerHud();
    Text(3, "%15TIME LEFT", 296, 50, 15);
    v->lap = 3;
    Reset();
    DrawRaceTimerHud();
    Text(3, "%15BEST LAP", 296, 50, 15);
    g_dwGameMode = GAME_MODE_PRACTICE;
    g_dwNumViewports = 2;
    Reset();
    DrawRaceTimerHud();
    assert(textCount == 2);
    Text(1, "%15LAP TIME", 296, 20, 15);
    g_dwNumViewports = 1;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;

    /* The lap and the place. */
    v->lap = 1;
    v->racePosition = 0;
    Reset();
    DrawLapCounterHud();
    assert(textCount == 3 && !shadow);
    Text(0, "%y1LAP 2/3", 16, 20, 15);
    Text(1, "1", 13, 227, 40);
    Text(2, "st", 14 - 3 + 20 + 3, 213, 20);
    g_pActiveCamera = &v->cameras[VEHICLE_CAMERA_BUMPER];
    g_dwNumViewports = 2;
    g_aViewportRects[0].height = 120;
    v->racePosition = 5;
    Reset();
    DrawLapCounterHud();
    assert(textCount == 2);
    Text(0, "6", 14, 108, 26);
    Text(1, "th", 14 + 0 + 13 + 3, 98, 13);
    g_aViewportRects[0].height = 240;
    g_dwNumViewports = 1;
    g_pActiveCamera = NULL;

    /* The messages. */
    v->message = "GO";
    Reset();
    DrawHudMessage();
    Text(0, "GO", 160, 87, 30);
    v->message = NULL;
    v->subMessage = "WAIT";
    Reset();
    DrawHudMessage();
    Text(0, "WAIT", 160, 85, 20);
    v->subMessage = NULL;

    /* The speed, in miles an hour, beside the dashboard. */
    g_OptionSettings.speedInMph = 1;
    v->speedKph = 160.9344f;
    Reset();
    DrawSpeedAndHudOverlay();
    assert(scissor[0] == 8 && scissor[1] == 0 && scissor[2] == 312 && scissor[3] == 240);
    Text(textCount - 2, "%yw100", 266, 169, 20);
    Text(textCount - 1, "%wwmph", 266, 169, 15);
    assert(texts[textCount - 1].align == TEXT_ALIGN_LEFT);
    g_OptionSettings.speedInMph = 0;
    v->speedKph = -5.0f;
    g_pActiveCamera = &v->cameras[VEHICLE_CAMERA_BUMPER];
    Reset();
    DrawSpeedAndHudOverlay();
    Text(textCount - 1, "%yw0", 293, 233, 20);
    puts("hud ok");
    return 0;
}

/* Host check of RenderSkybox() in src/sky.c against the command words of the disassembly. */
#include <assert.h>
#include <stdio.h>

#include "input.h"
#include "render.h"
#include "sky.h"
#include "track.h"
#include "util.h"
#include "weather.h"

Gfx *gDisplayListPtr;
Mtx *g_pProjectionMtx;
f32 g_afMvpMatrix[4][4];
u32 g_dwTextureFilterMode = 0x2000;
s32 g_dwTrackMirrored, g_dwViewportMirrored, g_dwCurrentSplitScreenIndex, g_dwSkyboxDisabled;
CameraFrame *g_pActiveCamera;
ViewportRect g_aViewportRects[2];
u8 g_bFogR = 1, g_bFogG = 2, g_bFogB = 3, g_bFogIntensity = 4;
s32 g_dwLightningFlashCounter;
u32 g_dwWeatherActive = 1, g_dwNightMode, g_dwSnowMode, g_dwRainMode, g_dwNumLocalPlayers = 1;
TrackHeader g_LoadedTrack;
static DisplayListBlock block;
DisplayListBlock *AllocDisplayListBlock(void) {
    return &block;
}
void guTranslateF(float mf[4][4], float x, float y, float z) {
}
void guMtxF2L(float mf[4][4], Mtx *m) {
}
void DrawFilledRect(s32 x0, s32 y0, s32 x1, s32 y1, s32 r, s32 g, s32 b) {
}

static const u32 expected[][2] = {
    {0xBC000404, 1},          {0xBC000C04, 1},          {0xBC001404, 0xFFFF},
    {0xBC001C04, 0xFFFF},     {0x01030040, 0},          {0x01040040, 0},
    {0xE7000000, 0},          {0xBA001402, 0},          {0xFC167E2C, 0x55FEF379},
    {0xFB000000, 0x01020304}, {0xB900031D, 0x0F0A4200}, {0xBA000C02, 0x2000},
    {0xB6000000, 0x000F0205}, {0xB7000000, 0x2000},     {0xB6000000, 0x1000},
    {0xBA001001, 0},          {0xBB000001, 0xFFFFFFFF}, {0xB6000000, 0x000C0000},
    {0xE8000000, 0},          {0xF5100000, 0x07000000}, {0xF50001F0, 0x06000000},
    {0xF5000100, 0x05000000}, {0x06000000, 0x1234},     {0xE7000000, 0},
    {0xBA001402, 0},          {0xB7000000, 0x00020205}, {0xBD000000, 0},
};

int main(void) {
    static Gfx list[64];
    static CameraFrame camera;
    u32 i, n = sizeof(expected) / sizeof(expected[0]);

    g_pActiveCamera = &camera;
    g_LoadedTrack.skyDisplayList = (Gfx *)0x1234;
    gDisplayListPtr = list;
    RenderSkybox();
    assert((u32)(gDisplayListPtr - list) == n);
    for (i = 0; i < n; i++) {
        /* The two matrix addresses depend on the host. */
        if (i == 4 || i == 5) {
            assert(list[i].words.w0 == expected[i][0]);
            continue;
        }
        if (list[i].words.w0 != expected[i][0] || list[i].words.w1 != expected[i][1]) {
            printf("%u: %08X %08X\n", i, list[i].words.w0, list[i].words.w1);
            return 1;
        }
    }
    g_dwWeatherActive = 0;
    gDisplayListPtr = list;
    RenderSkybox();
    assert(list[8].words.w0 == 0xFCFFFFFF && list[8].words.w1 == 0xFFFCF87C);
    assert(list[9].words.w0 == 0xB900031D);
    puts("sky ok");
    return 0;
}

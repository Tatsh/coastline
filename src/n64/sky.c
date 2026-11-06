#include "common/sky.h"

#include <PR/R4300.h>
#include <PR/gu.h>

#include "common/render.h"
#include "common/track.h"
#include "common/util.h"
#include "common/weather.h"

#define SKY_COMMAND(a, b)                                                                          \
    {                                                                                              \
        Gfx *command = gDisplayListPtr++;                                                          \
        command->words.w0 = (a);                                                                   \
        command->words.w1 = (b);                                                                   \
    }

void RenderSkybox(void) {
    /* IDO retains this array in the original stack frame. */
    s32 unused[4];
    Mtx *matrix;

    (void)unused;
    gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_1);
    if (g_dwSkyboxDisabled != 0) {
        return;
    }
    if (IsSkyFogFilled()) {
        if (g_dwLightningFlashCounter > 0 && (g_dwLightningFlashCounter & 1)) {
            DrawFilledRect(g_aViewportRects[g_dwCurrentSplitScreenIndex].x,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height,
                           (g_bFogR * 3 + 240) >> 2,
                           (g_bFogG * 3 + 248) >> 2,
                           (g_bFogB * 3 + 255) >> 2);
        } else {
            DrawFilledRect(g_aViewportRects[g_dwCurrentSplitScreenIndex].x,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height,
                           g_bFogR,
                           g_bFogG,
                           g_bFogB);
        }
        return;
    }
    guTranslateF(g_afMvpMatrix,
                 g_pActiveCamera->position.x,
                 g_pActiveCamera->position.y,
                 g_pActiveCamera->position.z * 0.99f);
    /* Keep both statements on one line for IDO's instruction order. */
    /* clang-format off */
    matrix = (Mtx *)AllocDisplayListBlock(); guMtxF2L(g_afMvpMatrix, matrix);
    /* clang-format on */
    gSPMatrix(gDisplayListPtr++,
              (char *)g_pProjectionMtx + 0x80000000,
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(
        gDisplayListPtr++, (char *)matrix + 0x80000000, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    if (g_dwWeatherActive != 0) {
        SKY_COMMAND(0xFC167E2C, 0x55FEF379);
        gDPSetEnvColor(gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, g_bFogIntensity);
    } else {
        SKY_COMMAND(0xFCFFFFFF, 0xFFFCF87C);
    }
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = (_SHIFTL(G_SETOTHERMODE_L, 24, 8) | _SHIFTL(G_MDSFT_RENDERMODE, 8, 8) |
                             _SHIFTL(29, 0, 8));
        command->words.w1 = (u32)(0x0F0A4200 | 0);
    }
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gSPClearGeometryMode(gDisplayListPtr++, 0xF0205);
    gSPSetGeometryMode(gDisplayListPtr++,
                       g_dwTrackMirrored ^ g_dwViewportMirrored ? 0x1000 : 0x2000);
    gSPClearGeometryMode(gDisplayListPtr++,
                         g_dwTrackMirrored ^ g_dwViewportMirrored ? 0x2000 : 0x1000);
    gDPSetTextureLOD(gDisplayListPtr++, 0);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
    gSPClearGeometryMode(gDisplayListPtr++, 0xC0000);
    gDPTileSync(gDisplayListPtr++);
    SKY_COMMAND(0xF5100000, 0x07000000);
    SKY_COMMAND(0xF50001F0, 0x06000000);
    SKY_COMMAND(0xF5000100, 0x05000000);
    gSPDisplayList(gDisplayListPtr++, g_LoadedTrack.skyDisplayList);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    {
        Gfx *command = gDisplayListPtr++;
        command->words.w0 = _SHIFTL(G_SETGEOMETRYMODE, 24, 8);
        command->words.w1 = (u32)0x20205;
    }
    gSPPopMatrix(gDisplayListPtr++, 0);
}

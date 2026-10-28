#include "sky.h"

#include <PR/gu.h>

#include "render.h"
#include "track.h"
#include "util.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/render.h"
#else
#include <PR/R4300.h>
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x106c6608 */
s32 g_dwSkyboxDisabled;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/* The sky sits just inside the far end of the view along the depth of the camera. */
#define SKY_DEPTH_SCALE 0.99f

/* A lightning flash lifts each fog component three quarters of the way towards these. */
#define FLASH_RED 0xF0
#define FLASH_GREEN 0xF8
#define FLASH_BLUE 0xFF

/* The RSP takes physical addresses, which are the KSEG0 addresses less its base. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define TO_RSP_ADDRESS(p) (p)

static inline void WriteSkyCombineMode(void) {
    PackCombineMode(gDisplayListPtr++,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_TEXEL0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_SHADE,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_TEXEL0,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_SHADE);
}

void RenderSkybox(const ViewportRect *viewports) {
    const ViewportRect *rect;
    f32(*mtx)[4];
#else
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))
void RenderSkybox(void) {
    ViewportRect *rect;
    Mtx *mtx;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

    gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_1);
    if (g_dwSkyboxDisabled) {
        return;
    }

    if (IsSkyFogFilled()) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        rect = &viewports[g_dwCurrentSplitScreenIndex];
#else
        rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
#endif
        if (g_dwLightningFlashCounter > 0 && (g_dwLightningFlashCounter & 1)) {
            DrawFilledRect(rect->x,
                           rect->y,
                           rect->width,
                           rect->height,
                           (g_bFogR * 3 + FLASH_RED) >> 2,
                           (g_bFogG * 3 + FLASH_GREEN) >> 2,
                           (g_bFogB * 3 + FLASH_BLUE) >> 2);
        } else {
            DrawFilledRect(rect->x, rect->y, rect->width, rect->height, g_bFogR, g_bFogG, g_bFogB);
        }
        return;
    }

    /* Centre the sky on the camera. */
    guTranslateF(g_afMvpMatrix,
                 g_pActiveCamera->position.x,
                 g_pActiveCamera->position.y,
                 g_pActiveCamera->position.z * SKY_DEPTH_SCALE);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    mtx = AllocDisplayListBlock()->matrix;
    CopyMatrix4x4(g_afMvpMatrix, mtx);
#else
    mtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afMvpMatrix, mtx);
#endif
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gDisplayListPtr++, TO_RSP_ADDRESS(mtx), G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    if (g_dwWeatherActive) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        WriteSkyCombineMode();
#else
        /* Blend the sky towards the fog colour by the intensity of the fog. */
        gDPSetCombineLERP(gDisplayListPtr++,
                          TEXEL0,
                          ENVIRONMENT,
                          ENV_ALPHA,
                          ENVIRONMENT,
                          0,
                          0,
                          0,
                          TEXEL0,
                          TEXEL0,
                          ENVIRONMENT,
                          ENV_ALPHA,
                          ENVIRONMENT,
                          0,
                          0,
                          0,
                          TEXEL0);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        gDPSetEnvColor(gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, g_bFogIntensity);
    } else {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        WriteSkyCombineMode();
#else
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, TEXEL0, 0, 0, 0, SHADE, 0, 0, 0, TEXEL0, 0, 0, 0, SHADE);
#endif
    }
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_FULL | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH | G_FOG | G_LIGHTING |
                             G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwWeatherActive) {
        gSPSetGeometryMode(gDisplayListPtr++, G_FOG);
    }
#endif

    /* Cull the back faces, or the front faces when only one of the track and view is mirrored. */
    gSPSetGeometryMode(gDisplayListPtr++,
                       g_dwTrackMirrored == g_dwViewportMirrored ? G_CULL_BACK : G_CULL_FRONT);
    gSPClearGeometryMode(gDisplayListPtr++,
                         g_dwTrackMirrored == g_dwViewportMirrored ? G_CULL_FRONT : G_CULL_BACK);

    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPTileSync(gDisplayListPtr++);
    gDPSetTile(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
    gSPDisplayList(gDisplayListPtr++, g_LoadedTrack.skyDisplayList);

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPSetGeometryMode(gDisplayListPtr++, G_SHADE | G_SHADING_SMOOTH | G_ZBUFFER | G_LIGHTING);
    gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
}

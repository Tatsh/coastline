#include <PR/gu.h>

#include "audiomix.h"
#include "camera.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "track.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/d3d/renderer.h"
#include "platform/win/displaylist.h"
#include "platform/win/ghost.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#define StubWeatherUpdate NullStub
#define AddTimingMarker(...) NullStub()
#define PRECIPITATION_RENDER_REAL long double
#define PRECIPITATION_RENDER_INTEGER(value) ((s32)(s64)(value))
#if TGR_RENDERER == TGR_RENDERER_D3D
#define PRECIPITATION_STREAK_REAL f32
#define WRITE_PRECIPITATION_TEXEL(dx, dy)                                                          \
    CopyTileColumnScaled(g_dwCurrentSplitScreenIndex, row + (dy), column + (dx), step)
#else
/* BRGlide holds the streak on the x87 stack and never rounds it to a float. */
#define PRECIPITATION_STREAK_REAL long double
#define WRITE_PRECIPITATION_TEXEL(dx, dy) (texel[(dy) * RAIN_STREAK_TEXTURE_SIDE + (dx)] = shade)
#endif /* TGR_RENDERER == TGR_RENDERER_D3D */
static inline void WritePrecipitationRectangle(s32 left, s32 top, s32 width, s32 height) {
    u32 right = ((u32)left + (u32)width) >> 2;
    u32 bottom = ((u32)top + (u32)height) >> 2;
    gDisplayListWords(gDisplayListPtr++,
                      ((u32)GBI_OP_TEXTURE_RECTANGLE << 24) | ((right & 0xfff) << 12) |
                          (bottom & 0xfff),
                      ((((u32)left >> 2) & 0xfff) << 12) | (((u32)top >> 2) & 0xfff));
}
#else
#define PRECIPITATION_RENDER_REAL f32
#define PRECIPITATION_STREAK_REAL f32
#define PRECIPITATION_RENDER_INTEGER(value) ((s32)(value))
#define WRITE_PRECIPITATION_TEXEL(dx, dy) (texel[(dy) * RAIN_STREAK_TEXTURE_SIDE + (dx)] = shade)
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/* The scale from a particle position back to the units of the world. */
#define PARTICLE_SCALE 6.10370189e-05f

/* The streak a view's rain draws: where it starts, the steps it takes and how far each moves. */
#define STREAK_CENTER 32.0f
#define STREAK_START 28.0f
#define STREAK_STEP 3.5f
#define STREAK_STEPS 16
#define STREAK_MARGIN 2

/* The size of a particle a unit away, for snow and rain. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define SNOW_PARTICLE_WIDTH 96.0f
#define SNOW_PARTICLE_HEIGHT 144.0f
#else
#define SNOW_PARTICLE_WIDTH 24.0f
#define SNOW_PARTICLE_HEIGHT 36.0f
#endif
#define RAIN_PARTICLE_SIZE 128.0f

/* The closest a particle is drawn. */
#define PARTICLE_NEAR 0.1f

/* The center of the screen in 10.2 fixed point, and the span from it to an edge in x and y. */
#define SCREEN_CENTER_X 640
#define SCREEN_CENTER_Y 480
#define SCREEN_HALF_WIDTH 640.0f
#define SCREEN_HALF_HEIGHT 480.0f

void RenderPrecipitationEffect(void) {
    Vec3f fall;
    s16 *particle;
#if TGR_PLATFORM == TGR_PLATFORM_N64 ||                                                            \
    (TGR_PLATFORM == TGR_PLATFORM_WIN && TGR_RENDERER == TGR_RENDERER_GLIDE)
    u8 *texel;
    u8 shade;
#endif
    s32 i, step, row, column, sizeX, sizeY, x, y;
    PRECIPITATION_STREAK_REAL u, v, du, dv;
    f32 width, height, halfWidth, px, py, pz, clipX, clipY, clipW, scale;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double updatedU, rawW, rawScale, rawProjected;
    f32 screenHeight;
#endif

    if (!g_dwSnowMode && !g_dwRainMode) {
        return;
    }
    for (i = 0; i < g_dwCameraRayHitCount; i++) {
        if (g_LoadedTrack.instances[g_awCameraRayHitInstances[i]].flags & TRACK_INSTANCE_SHELTER) {
            return;
        }
    }
    particle = g_awRainParticlePos[g_dwCurrentSplitScreenIndex][0];
    if (g_dwRainMode) {
        StubWeatherUpdate();
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    PackCombineMode(gDisplayListPtr++,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_PRIMITIVE,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_TEXEL0,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_PRIMITIVE,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_TEXEL0);
#else
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      TEXEL0,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      TEXEL0);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_FULL | IM_RD |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwRainMode) {
        gDisplayListWords(
            gDisplayListPtr++,
            ((u32)GBI_OP_LOAD_PREPARED_TEXTURE << 24) |
                ((u32)g_adwPrecipitationTextureIds[g_dwCurrentSplitScreenIndex] & 0xffffff),
            (u32)g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]);
        gDisplayListWords(
            gDisplayListPtr++,
            ((u32)GBI_OP_BIND_TEXTURE_ENTRY << 24) |
                ((u32)g_adwPrecipitationTextureIds[g_dwCurrentSplitScreenIndex] & 0xffffff),
            1);
    } else {
        gDisplayListWords(gDisplayListPtr++,
                          ((u32)GBI_OP_BIND_TEXTURE_ENTRY << 24) |
                              (g_dwHudOverlayHalfTextureId & 0xffffff),
                          1);
    }
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 2, 2, (63 << 2) + 2, (63 << 2) + 2);
    gSPClearGeometryMode(gDisplayListPtr++, G_CULL_BOTH);
#else
    gDPSetTextureImage(gDisplayListPtr++,
                       G_IM_FMT_I,
                       G_IM_SIZ_16b,
                       1,
                       g_dwRainMode ? g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex] :
                                      g_aParticleTexture);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_16b,
               0,
               0,
               G_TX_LOADTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               6,
               G_TX_NOLOD);
    gDPLoadSync(gDisplayListPtr++);
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, PARTICLE_LOAD_TEXELS, PARTICLE_LOAD_DXT);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_8b,
               8,
               0,
               G_TX_RENDERTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               6,
               G_TX_NOLOD);
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 63 << 2, 63 << 2);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    if (g_dwSnowMode) {
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xE0, 0xE0, 0xFF, 0xFF);
    } else {
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x78, 0x80, 0x88, 0xFF);
    }

    /* The view without its translation, taking the axes of a particle to those of the world. */
    MtxInvert4x4(g_afMvpMatrix, (f32(*)[4])g_pActiveCamera);
    g_afMvpMatrix[3][2] = 0.0f;
#if TGR_RENDERER == TGR_RENDERER_GLIDE
    g_afMvpMatrix[3][1] = 0.0f;
    g_afMvpMatrix[3][0] = 0.0f;
#endif
    g_afScratchMatrix[0][0] = 0.0f;
    g_afScratchMatrix[0][1] = 0.0f;
    g_afScratchMatrix[0][2] = PARTICLE_SCALE;
    g_afScratchMatrix[0][3] = 0.0f;
    g_afScratchMatrix[1][0] = PARTICLE_SCALE;
    g_afScratchMatrix[1][1] = 0.0f;
    g_afScratchMatrix[1][2] = 0.0f;
    g_afScratchMatrix[1][3] = 0.0f;
    g_afScratchMatrix[2][0] = 0.0f;
    g_afScratchMatrix[2][1] = PARTICLE_SCALE;
    g_afScratchMatrix[2][2] = 0.0f;
    g_afScratchMatrix[2][3] = 0.0f;
    g_afScratchMatrix[3][0] = 0.0f;
    g_afScratchMatrix[3][1] = 0.0f;
    g_afScratchMatrix[3][2] = 0.0f;
    g_afScratchMatrix[3][3] = 1.0f;
#if TGR_RENDERER != TGR_RENDERER_GLIDE
    /* The other two terms of the translation are read back from the one cleared. */
    g_afMvpMatrix[3][0] = g_afMvpMatrix[3][1] = g_afMvpMatrix[3][2];
#endif
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);

    /* A streak along the fall of the rain on the screen, brighter towards its end. */
    if (g_dwRainMode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN && TGR_RENDERER == TGR_RENDERER_D3D
        FillMemoryPatternBytes(g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex],
                               (u32)g_dwAlphaRampStripRowCount << 12,
                               0);
#elif TGR_PLATFORM == TGR_PLATFORM_WIN
        FillMemoryPatternBytes(g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex],
                               sizeof(g_abPrecipitationTextures[0]),
                               0);
#else
        MemFill64Aligned(g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex],
                         sizeof(g_abPrecipitationTextures[0]),
                         0);
#endif
        Mtx4x4TransformNoTranslation(
            &fall, &g_avRainVelocity[g_dwCurrentSplitScreenIndex], g_afMvpMatrix);
        Vec3fNormalize(&fall);
        if (g_dwViewportMirrored != g_dwTrackMirrored) {
            fall.x = -fall.x;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        u = STREAK_CENTER - (long double)fall.x * STREAK_START;
        v = STREAK_CENTER - (long double)fall.y * -STREAK_START;
        du = (long double)fall.x * STREAK_STEP;
        dv = (long double)fall.y * -STREAK_STEP;
#else
        u = STREAK_CENTER - fall.x * STREAK_START;
        v = fall.y * STREAK_START + STREAK_CENTER;
        du = fall.x * STREAK_STEP;
        dv = -fall.y * STREAK_STEP;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        v = (PRECIPITATION_RENDER_REAL)v + dv;
        for (step = 0; step < STREAK_STEPS; step++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            updatedU = (long double)u + du;
            u = updatedU;
#else
            u += du;
#endif
            row = PRECIPITATION_RENDER_INTEGER(v);
            if (row >= STREAK_MARGIN && row < RAIN_STREAK_TEXTURE_SIDE - STREAK_MARGIN) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                column = PRECIPITATION_RENDER_INTEGER(updatedU);
#else
                column = (s32)u;
#endif
                if (column >= STREAK_MARGIN && column < RAIN_STREAK_TEXTURE_SIDE - STREAK_MARGIN) {
                    /* A disc five texels across without its corners, a store for each texel. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN && TGR_RENDERER == TGR_RENDERER_D3D
                    GetPrecipitationTextureDimensions(g_dwCurrentSplitScreenIndex);
#else
                    shade = (step << 4) | step;
                    texel = &g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                                      [row * RAIN_STREAK_TEXTURE_SIDE + column];
#endif
                    WRITE_PRECIPITATION_TEXEL(-1, -2);
                    WRITE_PRECIPITATION_TEXEL(0, -2);
                    WRITE_PRECIPITATION_TEXEL(1, -2);
                    WRITE_PRECIPITATION_TEXEL(-2, -1);
                    WRITE_PRECIPITATION_TEXEL(-1, -1);
                    WRITE_PRECIPITATION_TEXEL(0, -1);
                    WRITE_PRECIPITATION_TEXEL(1, -1);
                    WRITE_PRECIPITATION_TEXEL(2, -1);
                    WRITE_PRECIPITATION_TEXEL(-2, 0);
                    WRITE_PRECIPITATION_TEXEL(-1, 0);
                    WRITE_PRECIPITATION_TEXEL(0, 0);
                    WRITE_PRECIPITATION_TEXEL(1, 0);
                    WRITE_PRECIPITATION_TEXEL(2, 0);
                    WRITE_PRECIPITATION_TEXEL(-2, 1);
                    WRITE_PRECIPITATION_TEXEL(-1, 1);
                    WRITE_PRECIPITATION_TEXEL(0, 1);
                    WRITE_PRECIPITATION_TEXEL(1, 1);
                    WRITE_PRECIPITATION_TEXEL(2, 1);
                    WRITE_PRECIPITATION_TEXEL(-1, 2);
                    WRITE_PRECIPITATION_TEXEL(0, 2);
                    WRITE_PRECIPITATION_TEXEL(1, 2);
                }
            }
            v = (PRECIPITATION_RENDER_REAL)v + dv;
        }
    }

    AddTimingMarker(0, 0x80, 0xFF, 0xFF, 0xFF);
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afPerspectiveMatrix);
    if (g_dwSnowMode) {
        width = SNOW_PARTICLE_WIDTH;
        height = SNOW_PARTICLE_HEIGHT;
    } else {
        width = RAIN_PARTICLE_SIZE;
        height = RAIN_PARTICLE_SIZE;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    halfWidth = (long double)(g_dwViewportMirrored != g_dwTrackMirrored ?
                                  (s32)(0u - (u32)g_dwConfigFramebufferWidth) :
                                  g_dwConfigFramebufferWidth) *
                2.0f;
    screenHeight = (long double)g_dwConfigFramebufferHeight * 2.0f;
#else
    halfWidth = g_dwViewportMirrored != g_dwTrackMirrored ? -SCREEN_HALF_WIDTH : SCREEN_HALF_WIDTH;
#endif

    /* Each particle in front of the camera and on the screen is a rectangle of the texture. */
    for (i = 0; i < g_dwRainParticleCount; i++, particle += 3) {
        px = particle[0];
        py = particle[1];
        pz = particle[2];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        rawW = (((long double)pz * g_afMvpMatrix[2][3] + (long double)py * g_afMvpMatrix[1][3]) +
                (long double)px * g_afMvpMatrix[0][3]) +
               g_afMvpMatrix[3][3];
        clipW = rawW;
        clipX = (((long double)pz * g_afMvpMatrix[2][0] + (long double)py * g_afMvpMatrix[1][0]) +
                 (long double)px * g_afMvpMatrix[0][0]) +
                g_afMvpMatrix[3][0];
        clipY = (((long double)pz * g_afMvpMatrix[2][1] + (long double)py * g_afMvpMatrix[1][1]) +
                 (long double)px * g_afMvpMatrix[0][1]) +
                g_afMvpMatrix[3][1];
        if (!(clipW >= PARTICLE_NEAR)) {
            continue;
        }
        rawScale = 0.5L / clipW;
        scale = rawScale;
        rawProjected = rawScale * clipY;
        clipY = rawProjected;
        if (!(rawProjected >= -1.0f) || clipY > 1.0f) {
            continue;
        }
        rawProjected = (long double)scale * clipX;
        clipX = rawProjected;
        if (!(rawProjected >= -1.0f) || clipX > 1.0f) {
            continue;
        }
        sizeX = PRECIPITATION_RENDER_INTEGER((long double)scale * width);
        if (sizeX < 2) {
            continue;
        }
        sizeY = PRECIPITATION_RENDER_INTEGER((long double)scale * height);
        if (sizeY < 2) {
            continue;
        }
        x = PRECIPITATION_RENDER_INTEGER((long double)clipX * halfWidth);
        y = PRECIPITATION_RENDER_INTEGER((long double)clipY * screenHeight);
        x = (s32)((u32)x + (u32)(g_dwConfigFramebufferWidth / 2) * 4u);
        y = (s32)((u32)y + (u32)(g_dwConfigFramebufferHeight / 2) * 4u);
        WritePrecipitationRectangle(x, y, sizeX, sizeY);
#else
        clipX = g_afMvpMatrix[3][0] +
                (px * g_afMvpMatrix[0][0] + py * g_afMvpMatrix[1][0] + pz * g_afMvpMatrix[2][0]);
        clipY = g_afMvpMatrix[3][1] +
                (px * g_afMvpMatrix[0][1] + py * g_afMvpMatrix[1][1] + pz * g_afMvpMatrix[2][1]);
        clipW = g_afMvpMatrix[3][3] +
                (px * g_afMvpMatrix[0][3] + py * g_afMvpMatrix[1][3] + pz * g_afMvpMatrix[2][3]);
        if (clipW < PARTICLE_NEAR) {
            continue;
        }
        scale = 0.5f / clipW;
        clipY *= scale;
        if (clipY < -1.0f || clipY > 1.0f) {
            continue;
        }
        clipX *= scale;
        if (clipX < -1.0f || clipX > 1.0f) {
            continue;
        }
        sizeX = (s32)(width * scale);
        if (sizeX < 2) {
            continue;
        }
        sizeY = (s32)(height * scale);
        if (sizeY < 2) {
            continue;
        }
        x = (s32)(clipX * halfWidth);
        y = (s32)(clipY * SCREEN_HALF_HEIGHT);
        gSPTextureRectangle(gDisplayListPtr++,
                            x + SCREEN_CENTER_X,
                            y + SCREEN_CENTER_Y,
                            x + sizeX + SCREEN_CENTER_X,
                            y + sizeY + SCREEN_CENTER_Y,
                            G_TX_RENDERTILE,
                            0,
                            PARTICLE_TEXTURE_LAST_T,
                            PARTICLE_TEXTURE_STEPS / sizeX,
                            -PARTICLE_TEXTURE_STEPS / sizeY);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}

#include "weather.h"

#include <PR/gbi.h>
#include <PR/gu.h>

#include "audiomix.h"
#include "camera.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "track.h"
#include "vehicle.h"

/* The scale from a particle position back to the units of the world. */
#define PARTICLE_SCALE 6.10370189e-05f

/* The streak a view's rain draws: where it starts, the steps it takes and how far each moves. */
#define STREAK_CENTER 32.0f
#define STREAK_START 28.0f
#define STREAK_STEP 3.5f
#define STREAK_STEPS 16
#define STREAK_MARGIN 2

/* The size of a particle a unit away, for snow and rain. */
#define SNOW_PARTICLE_WIDTH 24.0f
#define SNOW_PARTICLE_HEIGHT 36.0f
#define RAIN_PARTICLE_SIZE 128.0f

/* The closest a particle is drawn. */
#define PARTICLE_NEAR 0.1f

/* The center of the screen in 10.2 fixed point, and the span from it to an edge in x and y. */
#define SCREEN_CENTER_X 640
#define SCREEN_CENTER_Y 480
#define SCREEN_HALF_WIDTH 640.0f
#define SCREEN_HALF_HEIGHT 480.0f

void SubdivideLightningBolt(s32 index, s32 span) {
    s32 mask = span * 8 - 1;
    Vec3f *point;
    f32 half = (f32)mask * 0.5f;

    g_avLightningBolt[index].x =
        (g_avLightningBolt[index - span].x + g_avLightningBolt[index + span].x) * 0.5f +
        (f32)(s32)(AdvanceRng() & mask) - half;
    g_avLightningBolt[index].y =
        (g_avLightningBolt[index - span].y + g_avLightningBolt[index + span].y) * 0.5f +
        (f32)(s32)(AdvanceRng() & mask) - half;
    point = &g_avLightningBolt[index];
    point->z = (g_avLightningBolt[index - span].z + g_avLightningBolt[index + span].z) * 0.5f;
    span >>= 1;
    if (span != 0) {
        g_nLightningBoltDepth += 4;
        SubdivideLightningBolt(index - span, span);
        SubdivideLightningBolt(index + span, span);
        g_nLightningBoltDepth -= 4;
    }
}

void StubWeatherUpdate(void) {
}

void RenderPrecipitationEffect(void) {
    Vec3f fall;
    s16 *particle;
    u8 *texel;
    u8 shade;
    s32 i, step, row, column, sizeX, sizeY, x, y;
    f32 u, v, du, dv, width, height, halfWidth, px, py, pz, clipX, clipY, clipW, scale;

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
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_FULL | IM_RD |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
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
    if (g_dwSnowMode) {
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0xE0, 0xE0, 0xFF, 0xFF);
    } else {
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, 0x78, 0x80, 0x88, 0xFF);
    }

    /* The view without its translation, taking the axes of a particle to those of the world. */
    MtxInvert4x4(g_afMvpMatrix, (f32(*)[4])g_pActiveCamera);
    g_afMvpMatrix[3][2] = 0.0f;
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
    /* The other two terms of the translation are read back from the one cleared. */
    g_afMvpMatrix[3][0] = g_afMvpMatrix[3][1] = g_afMvpMatrix[3][2];
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);

    /* A streak along the fall of the rain on the screen, brighter towards its end. */
    if (g_dwRainMode) {
        MemFill64Aligned(g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex],
                         sizeof(g_abPrecipitationTextures[0]),
                         0);
        Mtx4x4TransformNoTranslation(
            &fall, &g_avRainVelocity[g_dwCurrentSplitScreenIndex], g_afMvpMatrix);
        Vec3fNormalize(&fall);
        if (g_dwViewportMirrored != g_dwTrackMirrored) {
            fall.x = -fall.x;
        }
        u = STREAK_CENTER - fall.x * STREAK_START;
        v = fall.y * STREAK_START + STREAK_CENTER;
        du = fall.x * STREAK_STEP;
        dv = -fall.y * STREAK_STEP;
        v += dv;
        for (step = 0; step < STREAK_STEPS; step++) {
            u += du;
            row = (s32)v;
            if (row >= STREAK_MARGIN && row < RAIN_STREAK_TEXTURE_SIDE - STREAK_MARGIN) {
                column = (s32)u;
                if (column >= STREAK_MARGIN && column < RAIN_STREAK_TEXTURE_SIDE - STREAK_MARGIN) {
                    /* A disc five texels across without its corners, a store for each texel. */
                    shade = (step << 4) | step;
                    texel = &g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                                      [row * RAIN_STREAK_TEXTURE_SIDE + column];
                    texel[-2 * RAIN_STREAK_TEXTURE_SIDE - 1] = shade;
                    texel[-2 * RAIN_STREAK_TEXTURE_SIDE] = shade;
                    texel[-2 * RAIN_STREAK_TEXTURE_SIDE + 1] = shade;
                    texel[-RAIN_STREAK_TEXTURE_SIDE - 2] = shade;
                    texel[-RAIN_STREAK_TEXTURE_SIDE - 1] = shade;
                    texel[-RAIN_STREAK_TEXTURE_SIDE] = shade;
                    texel[-RAIN_STREAK_TEXTURE_SIDE + 1] = shade;
                    texel[-RAIN_STREAK_TEXTURE_SIDE + 2] = shade;
                    texel[-2] = shade;
                    texel[-1] = shade;
                    texel[0] = shade;
                    texel[1] = shade;
                    texel[2] = shade;
                    texel[RAIN_STREAK_TEXTURE_SIDE - 2] = shade;
                    texel[RAIN_STREAK_TEXTURE_SIDE - 1] = shade;
                    texel[RAIN_STREAK_TEXTURE_SIDE] = shade;
                    texel[RAIN_STREAK_TEXTURE_SIDE + 1] = shade;
                    texel[RAIN_STREAK_TEXTURE_SIDE + 2] = shade;
                    texel[2 * RAIN_STREAK_TEXTURE_SIDE - 1] = shade;
                    texel[2 * RAIN_STREAK_TEXTURE_SIDE] = shade;
                    texel[2 * RAIN_STREAK_TEXTURE_SIDE + 1] = shade;
                }
            }
            v += dv;
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
    halfWidth = g_dwViewportMirrored != g_dwTrackMirrored ? -SCREEN_HALF_WIDTH : SCREEN_HALF_WIDTH;

    /* Each particle in front of the camera and on the screen is a rectangle of the texture. */
    for (i = 0; i < g_dwRainParticleCount; i++, particle += 3) {
        px = particle[0];
        py = particle[1];
        pz = particle[2];
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
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}

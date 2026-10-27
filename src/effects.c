#include "effects.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "race.h"
#include "render.h"
#include "system.h"
#include "track.h"
#include "trackselect.h"
#include "weather.h"

/** The texels of a puff left of or above the screen, from its step a pixel, within its sign. */
static inline s32 OffscreenTexels(s16 offscreen, s16 step) {
    s32 texels;

    if (step < 0) {
        texels = (offscreen * step) >> 7;
        return texels > 0 ? texels : 0;
    }
    texels = (offscreen * step) >> 7;
    return texels < 0 ? texels : 0;
}

/** Clamp a coordinate of the screen, taken to 16 bits, to its left or top edge. */
static inline s32 ClampToScreen(s32 coordinate) {
    s16 clamped = coordinate;

    return clamped > 0 ? clamped : 0;
}

/* Load the particle texture to be drawn as rectangles of the primitive colour. */
#define LoadParticleTexture()                                                                      \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);                                              \
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);                       \
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);                                 \
    gDPSetCombineLERP(gDisplayListPtr++,                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      TEXEL0,                                                                      \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      TEXEL0,                                                                      \
                      0,                                                                           \
                      PRIMITIVE,                                                                   \
                      0);                                                                          \
    gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_I, G_IM_SIZ_16b, 1, g_aParticleTexture);        \
    gDPSetTile(gDisplayListPtr++,                                                                  \
               G_IM_FMT_I,                                                                         \
               G_IM_SIZ_16b,                                                                       \
               0,                                                                                  \
               0,                                                                                  \
               G_TX_LOADTILE,                                                                      \
               0,                                                                                  \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD,                                                                         \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD);                                                                        \
    gDPLoadSync(gDisplayListPtr++);                                                                \
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, PARTICLE_LOAD_TEXELS, PARTICLE_LOAD_DXT); \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetTile(gDisplayListPtr++,                                                                  \
               G_IM_FMT_I,                                                                         \
               G_IM_SIZ_8b,                                                                        \
               8,                                                                                  \
               0,                                                                                  \
               G_TX_RENDERTILE,                                                                    \
               0,                                                                                  \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD,                                                                         \
               G_TX_WRAP,                                                                          \
               6,                                                                                  \
               G_TX_NOLOD);                                                                        \
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 63 << 2, 63 << 2);                    \
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);                                                \
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE)

/* Put back the modes the rest of the frame draws with. */
#define RestoreParticleModes()                                                                     \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);                                             \
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PIXEL);                                              \
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);                                     \
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);                                     \
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD)

s32 ComputeScreenDepthZ(f32 z, f32 w) {
    return (s32)((z / w * (f32)g_dwViewportScaleZ + (f32)g_dwViewportTransZ) * 32.0f);
}

void DrawTireSpray(void) {
    RacerSlot *slot;
    Vehicle *vehicle;
    s16(*velocities)[SPRAY_SEGMENT_VERTICES][3];
    s32 wheel, first;

    if (g_dwSnowMode && g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
        return;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      SHADE,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      0,
                      0,
                      0,
                      COMBINED,
                      0,
                      0,
                      0,
                      COMBINED);
    gDPSetRenderMode(gDisplayListPtr++,
                     GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     Z_CMP | IM_RD | CVG_DST_FULL | ZMODE_XLU | FORCE_BL |
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gSPSetGeometryMode(gDisplayListPtr++, G_SHADE);
    gSPClearGeometryMode(gDisplayListPtr++, G_CULL_FRONT | G_CULL_BACK | G_LIGHTING);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gDPSetTextureImage(gDisplayListPtr++, G_IM_FMT_I, G_IM_SIZ_16b, 1, g_aTireSprayTexture);
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
               5,
               G_TX_NOLOD);
    gDPLoadSync(gDisplayListPtr++);
    gDPLoadBlock(gDisplayListPtr++, G_TX_LOADTILE, 0, 0, 511, 0x400);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTile(gDisplayListPtr++,
               G_IM_FMT_I,
               G_IM_SIZ_4b,
               2,
               0,
               G_TX_RENDERTILE,
               0,
               G_TX_WRAP,
               6,
               G_TX_NOLOD,
               G_TX_WRAP,
               5,
               G_TX_NOLOD);
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, 31 << 2, 63 << 2);
    /* The size is set again half a texel in, which replaces the first. */
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 2, 2, (31 << 2) + 2, (63 << 2) + 2);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);

    for (slot = g_aRacerSlots; slot < &g_aRacerSlots[g_dwNumVehicles]; slot++) {
        vehicle = slot->vehicle;
        if (vehicle == NULL) {
            continue;
        }
        gSPMatrix(gDisplayListPtr++, &vehicle->sprayMtx, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
        for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
            /* The newest segment is not drawn, nor a quad whose two edges have stopped. */
            gSPVertex(gDisplayListPtr++,
                      vehicle->sprayVertices[wheel][1],
                      (SPRAY_SEGMENTS - 1) * SPRAY_SEGMENT_VERTICES,
                      0);
            velocities = &vehicle->sprayVelocities[wheel][1];
            for (first = 0; first < (SPRAY_SEGMENTS - 2) * SPRAY_SEGMENT_VERTICES;
                 first += SPRAY_SEGMENT_VERTICES, velocities++) {
                if (velocities[0][1][0] == 0 && velocities[0][1][1] == 0 &&
                    velocities[0][1][2] == 0 && velocities[1][1][0] == 0 &&
                    velocities[1][1][1] == 0 && velocities[1][1][2] == 0) {
                    continue;
                }
                gSP2Triangles(gDisplayListPtr++,
                              first,
                              first + SPRAY_SEGMENT_VERTICES,
                              first + 1,
                              0,
                              first + 1,
                              first + SPRAY_SEGMENT_VERTICES,
                              first + SPRAY_SEGMENT_VERTICES + 1,
                              0);
            }
        }
        gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    }
    gSPSetGeometryMode(gDisplayListPtr++, G_CULL_BACK);
}

void RenderBillboardParticles(u16 head, s32 red, s32 green, s32 blue) {
    ViewportRect *view;
    Particle *particle;
    f32 clip[4];
    f32 span, scaleX, inverse, size, edge;
    s32 centerX, centerY, height, halfWidth, halfHeight, x, y, top, stepS, stepT, s, t;
    s16 left;
    u16 index;

    view = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    centerX = (view->x * 2 + view->width) * 2;
    centerY = (view->y * 2 + view->height) * 2;
    span = (f32)(view->width * 2);
    height = view->height;
    scaleX = g_dwViewportMirrored != g_dwTrackMirrored ? -span : span;

    for (index = head; index != 0; index = particle->next) {
        particle = &g_aParticlePool[index];
        Vec3fTransformByMatrix(clip, &particle->position, g_afBillboardMatrix);
        if (clip[3] <= 0.001f && clip[3] >= -0.001f) {
            continue;
        }
        inverse = 1.0f / clip[3];
        clip[2] *= inverse;
        if (clip[2] >= 1.0f || clip[2] <= -0.3f) {
            continue;
        }
        clip[0] *= inverse;
        size = particle->size * (4.0f * inverse);
        edge = 1.0f + size;
        if (clip[0] <= -edge || clip[0] >= edge) {
            continue;
        }
        clip[1] *= inverse;
        if (clip[1] <= -edge || clip[1] >= edge) {
            continue;
        }
        /* The width is three quarters of the height for the shape of the screen. */
        halfWidth = (s32)(span * 0.75f * size);
        if (halfWidth <= 0) {
            continue;
        }
        halfHeight = (s32)(span * size);
        if (halfHeight <= 0) {
            continue;
        }
        x = (s32)(clip[0] * scaleX) + centerX;
        y = (s32)(clip[1] * (f32)(height * 2)) + centerY;

        gDPPipeSync(gDisplayListPtr++);
        gDPSetPrimColor(gDisplayListPtr++,
                        0xFF,
                        0xFF,
                        red,
                        green,
                        blue,
                        (particle->alpha * particle->fade) >> 8);
        gDPSetPrimDepth(gDisplayListPtr++, (s32)(clip[2] * 16352.0f) + 0x3FE0, 0);

        /* A rectangle off the left or the top of the screen starts its texture further in. */
        left = x - halfWidth;
        top = y - halfHeight;
        stepS = PARTICLE_TEXTURE_STEPS / 2 / halfWidth;
        stepT = -(PARTICLE_TEXTURE_STEPS / 2) / halfHeight;
        s = left < 0 ? OffscreenTexels(left, stepS) : 0;
        t = top < 0 ? OffscreenTexels(top, stepT) : 0;
        gSPTextureRectangle(gDisplayListPtr++,
                            ClampToScreen(left),
                            ClampToScreen(top),
                            ClampToScreen(x + halfWidth),
                            ClampToScreen(y + halfHeight),
                            G_TX_RENDERTILE,
                            -s,
                            PARTICLE_TEXTURE_LAST_T - t,
                            stepS,
                            stepT);
    }
}

void RenderTireSmoke(void) {
    LoadParticleTexture();

    /* The view, then the axes of a puff to those of the screen, then the perspective. */
    MtxInvert4x4(g_afMvpMatrix, (f32(*)[4])g_pActiveCamera);
    g_afScratchMatrix[0][0] = 0.0f;
    g_afScratchMatrix[0][1] = 0.0f;
    g_afScratchMatrix[0][2] = -1.0f;
    g_afScratchMatrix[0][3] = 0.0f;
    g_afScratchMatrix[1][0] = -1.0f;
    g_afScratchMatrix[1][1] = 0.0f;
    g_afScratchMatrix[1][2] = 0.0f;
    g_afScratchMatrix[1][3] = 0.0f;
    g_afScratchMatrix[2][0] = 0.0f;
    g_afScratchMatrix[2][1] = -1.0f;
    g_afScratchMatrix[2][2] = 0.0f;
    g_afScratchMatrix[2][3] = 0.0f;
    g_afScratchMatrix[3][0] = 0.0f;
    g_afScratchMatrix[3][1] = 0.0f;
    g_afScratchMatrix[3][2] = 0.0f;
    g_afScratchMatrix[3][3] = 1.0f;
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);
    /* The copy is not read again. */
    memcpy(g_afScratchMatrix, g_afMvpMatrix, sizeof(g_afScratchMatrix));
    Mtx4x4Multiply(g_afBillboardMatrix, g_afMvpMatrix, g_afPerspectiveMatrix);

    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PRIM);
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    if (!g_dwNightMode) {
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_DISABLE);
        /* The alpha compare is set to dither the smoke along with the render mode. */
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | Z_UPD | CVG_DST_FULL | ZMODE_OPA | G_AC_DITHER |
                             GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                         GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
        gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
        switch (g_dwSelectedTrackId) {
        case 1:
        case 6:
            RenderBillboardParticles(g_wTireSmokeListHead, 0x60, 0x54, 0x38);
            RenderBillboardParticles(g_wPavedTireSmokeListHead, 0x60, 0x5C, 0x50);
            break;
        case 4:
        case 9:
            RenderBillboardParticles(g_wTireSmokeListHead, 0x70, 0x58, 0x38);
            RenderBillboardParticles(g_wPavedTireSmokeListHead, 0x70, 0x68, 0x58);
            break;
        default:
            RenderBillboardParticles(g_wTireSmokeListHead, 0xA0, 0x88, 0x60);
            RenderBillboardParticles(g_wPavedTireSmokeListHead, 0x70, 0x68, 0x58);
            break;
        }
    }
    RestoreParticleModes();
}

void RenderExhaustAndTireSpray(void) {
    LoadParticleTexture();
    gDPSetDepthSource(gDisplayListPtr++, G_ZS_PRIM);
    gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
    if (g_dwSnowMode) {
        /* The colour dither's bits, which leave the alpha dither at its pattern. */
        gDPSetAlphaDither(gDisplayListPtr++, G_CD_NOISE);
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_XLU | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
        RenderBillboardParticles(g_wExhaustParticleListHead, 0xE0, 0xE0, 0xFF);
    }
    RestoreParticleModes();
    DrawTireSpray();
}

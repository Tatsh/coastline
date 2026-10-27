#include "effects.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "particlecommands.h"
#include "race.h"
#include "render.h"
#include "system.h"
#include "track.h"
#include "trackselect.h"
#include "weather.h"

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

void RenderExhaustAndTireSpray(void) {
    ConfigureParticleTexture();
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

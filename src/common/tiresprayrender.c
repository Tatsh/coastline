#include "common/effects.h"
#include "common/particlecommands.h"
#include "common/trackselect.h"
#include "common/vehiclecount.h"

#if TGR_BOSS_RALLY
#define SPRAY_DRAW_MATRIX(vehicle) ((vehicle)->GetSprayMatrix())
#define SPRAY_DRAW_VERTICES(vehicle) ((vehicle)->GetSprayVertices())
#define SPRAY_DRAW_VELOCITIES(vehicle) ((vehicle)->GetSprayVelocities())
static inline void
LoadSprayVertices(GbiCommand *command, const ConvertedModelVertex *vertices, u32 count, u32 first) {
    gDisplayListWords(command,
                      ((u32)G_VTX << 24) | ((first * 2 & 0xff) << 16) | ((count & 0x3f) << 10) |
                          ((sizeof(ConvertedModelVertex) * count - 1) & 0x3ff),
                      GBI_WORD1(vertices));
}
#define SPRAY_DRAW_LOAD_VERTICES LoadSprayVertices
#else
#define SPRAY_DRAW_MATRIX(vehicle) (&(vehicle)->sprayMtx)
#define SPRAY_DRAW_VERTICES(vehicle) ((vehicle)->sprayVertices)
#define SPRAY_DRAW_VELOCITIES(vehicle) ((vehicle)->sprayVelocities)
#define SPRAY_DRAW_LOAD_VERTICES gSPVertex
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
void DrawTireSpray(const RacerSlot *slots) {
    const Vehicle *vehicle;
    const s16(*velocities)[SPRAY_SEGMENT_VERTICES][3];
    s32 slotIndex;
#else
void DrawTireSpray(void) {
    RacerSlot *slot;
    Vehicle *vehicle;
    s16(*velocities)[SPRAY_SEGMENT_VERTICES][3];
#endif /* TGR_BOSS_RALLY */
    s32 wheel, first;

    if (g_dwSnowMode && g_dwSelectedTrackId != TRACK_COASTLINE &&
        g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE) {
        return;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
#if TGR_BOSS_RALLY
    PackCombineMode(gDisplayListPtr++,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_SHADE,
                    1000 + G_ACMUX_TEXEL0,
                    0,
                    1000 + G_ACMUX_SHADE,
                    0,
                    0,
                    0,
                    0,
                    1000 + G_CCMUX_COMBINED,
                    0,
                    0,
                    0,
                    1000 + G_ACMUX_COMBINED);
#else
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
#endif /* TGR_BOSS_RALLY */
    gDPSetRenderMode(gDisplayListPtr++,
                     GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     Z_CMP | IM_RD | CVG_DST_FULL | ZMODE_XLU | FORCE_BL |
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gSPSetGeometryMode(gDisplayListPtr++, G_SHADE);
    gSPClearGeometryMode(gDisplayListPtr++, G_CULL_FRONT | G_CULL_BACK | G_LIGHTING);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
#if TGR_BOSS_RALLY
    gDisplayListWords(gDisplayListPtr++,
                      ((u32)GBI_OP_BIND_TEXTURE_ENTRY << 24) | (g_dwTireTrailTextureId & 0xffffff),
                      1);
#else
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
#endif /* TGR_BOSS_RALLY */
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, 2, 2, (31 << 2) + 2, (63 << 2) + 2);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);

#if TGR_BOSS_RALLY
    for (slotIndex = 0; slotIndex < g_dwNumVehicles; slotIndex++) {
        vehicle = slots[slotIndex].GetVehicle();
#else
    for (slot = g_aRacerSlots; slot < &g_aRacerSlots[g_dwNumVehicles]; slot++) {
        vehicle = slot->vehicle;
#endif
        if (vehicle == NULL) {
            continue;
        }
        gSPMatrix(gDisplayListPtr++,
                  SPRAY_DRAW_MATRIX(vehicle),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
        for (wheel = 0; wheel < VEHICLE_WHEELS; wheel++) {
            /* The newest segment is not drawn, nor a quad whose two edges have stopped. */
            SPRAY_DRAW_LOAD_VERTICES(gDisplayListPtr++,
                                     SPRAY_DRAW_VERTICES(vehicle)[wheel][1],
                                     (SPRAY_SEGMENTS - 1) * SPRAY_SEGMENT_VERTICES,
                                     0);
            velocities = &SPRAY_DRAW_VELOCITIES(vehicle)[wheel][1];
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

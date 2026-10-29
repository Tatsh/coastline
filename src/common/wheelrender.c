#include <PR/gu.h>

#include "common/modeldisplaylist.h"
#include "common/util.h"
#include "common/vehicleaccess.h"
#include "common/vehiclerender.h"
#include "common/weather.h"

#define FLAGS_FAR                                                                                  \
    (Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL |                                                \
     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM))
#define FLAGS_GHOST_WHEEL                                                                          \
    (Z_CMP | IM_RD | CVG_DST_FULL | ZMODE_XLU | FORCE_BL |                                         \
     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA))

#if TGR_BOSS_RALLY
typedef f32 (*WheelMatrix)[4];
#define WHEEL_MATRIX_ADDRESS(matrix) (matrix)
#else
typedef Mtx *WheelMatrix;
#define WHEEL_MATRIX_ADDRESS(matrix) ((u32)(matrix) + 0x80000000)
#endif

static inline WheelMatrix CopyWheelMatrix(f32 matrix[4][4]) {
#if TGR_BOSS_RALLY
    WheelMatrix block = AllocDisplayListBlock()->matrix;
    CopyMatrix4x4(matrix, block);
#else
    WheelMatrix block = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(matrix, block);
#endif
    return block;
}

void RenderWheelModels(Vehicle *vehicle) {
    WheelMatrix block;
    s32 i;

    if (g_pCurrentVehicleModelData->wheelDisplayLists[0][VEHICLE_WHEEL_DL_NORMAL] == NULL) {
        return;
    }
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
        if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
            /* A ghost fades out through the alpha of the environment. */
            gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, FLAGS_GHOST_WHEEL);
            gDPSetEnvColor(gDisplayListPtr++, 0, 0, 0, g_dwVehicleFogAlpha);
            gDPSetCombineLERP(gDisplayListPtr++,
                              TEXEL0,
                              0,
                              SHADE,
                              0,
                              0,
                              0,
                              0,
                              TEXEL0,
                              0,
                              0,
                              0,
                              COMBINED,
                              COMBINED,
                              0,
                              ENVIRONMENT,
                              0);
        } else {
            gDPSetCombineLERP(gDisplayListPtr++,
                              TEXEL0,
                              0,
                              SHADE,
                              0,
                              0,
                              0,
                              0,
                              TEXEL0,
                              0,
                              0,
                              0,
                              COMBINED,
                              0,
                              0,
                              0,
                              COMBINED);
            gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, FLAGS_FAR);
        }
        guScaleF(g_afScratchMatrix, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f);
        guMtxCatF(
            g_afScratchMatrix, VEHICLE_GET(vehicle, WheelMatrices)[i], g_mtxVehicleModelScaled);
        block = CopyWheelMatrix(g_mtxVehicleModelScaled);
        gSPMatrix(gDisplayListPtr++, block, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
        guMtxCatF(g_mtxVehicleModelScaled, g_afViewProjMatrix, g_afMvpMatrix);
#if !TGR_BOSS_RALLY
        NormalizeLightMatrix(g_afMvpMatrix);
#endif
        block = CopyWheelMatrix(g_afMvpMatrix);
        gSPForceMatrix(gDisplayListPtr++, WHEEL_MATRIX_ADDRESS(block));
        SetModelTextures();
        if (g_dwSnowMode) {
            DrawModelDisplayList(
                g_pCurrentVehicleModelData->wheelDisplayLists[0][VEHICLE_WHEEL_DL_SNOW]);
        } else {
            DrawModelDisplayList(
                g_pCurrentVehicleModelData->wheelDisplayLists[0][VEHICLE_WHEEL_DL_NORMAL]);
        }
        gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    }
}

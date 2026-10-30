#include "common/vehiclerender.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/camera.h"
#include "common/fade.h"
#include "common/input.h"
#include "common/mathutil.h"
#include "common/modeldisplaylist.h"
#include "common/render.h"
#include "common/track.h"
#include "common/util.h"
#include "common/visgrid.h"
#include "common/weather.h"
#include "racemenu.h"

/* The points of the fan of the stage disc, and its radius in its units. */
#define STAGE_DISC_POINTS 31
#define STAGE_DISC_RADIUS 256.0f

void TransformWheelPositions(s32 index) {
    Vehicle *vehicle = &g_aVehicles[index];
    s32 i;

    g_pCurrentVehicleModelData = vehicle->model;
    if (g_pCurrentVehicleModelData->wheelDisplayLists[0][VEHICLE_WHEEL_DL_NORMAL] == NULL) {
        return;
    }
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        memcpy(&vehicle->wheelFrames[i], &vehicle->frame, 3 * sizeof(f32[4]));
        Vec3fTransformByMatrix(
            vehicle->wheelMatrices[i][3], &g_pCurrentVehicleModelData->wheels[i], vehicle->matrix);
    }
}

void DrawCarStageDisc(Vehicle *vehicle) {
    f32 angle;
    s32 i;

    if (g_dwGameFrameCount == 0) {
        for (i = 0; i < STAGE_DISC_POINTS; i++) {
            angle = (f32)i * 0.2094395161f;
            g_aCarStageDiscVertices[i].v.ob[0] = (s32)(cosf(angle) * STAGE_DISC_RADIUS);
            g_aCarStageDiscVertices[i].v.ob[1] = (s32)(sinf(angle) * STAGE_DISC_RADIUS);
            g_aCarStageDiscVertices[i].v.ob[2] = 0;
            g_aCarStageDiscVertices[i].v.cn[0] = 0xFF;
            g_aCarStageDiscVertices[i].v.cn[1] = 0xFF;
            g_aCarStageDiscVertices[i].v.cn[2] = 0xFF;
            g_aCarStageDiscVertices[i].v.cn[3] = 0xFF;
        }
        g_aCarStageDiscVertices[0].v.ob[0] = 0;
        g_aCarStageDiscVertices[0].v.ob[1] = 0;
        g_aCarStageDiscVertices[0].v.ob[2] = 0;
        guScaleF(g_afMvpMatrix, 0.009765625f, 0.009765625f, 0.009765625f);
        guRotateF(g_afScratchMatrix, -45.0f, 0.0f, 0.0f, 1.0f);
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afScratchMatrix);
        guMtxF2L(g_afScratchMatrix, &g_CarStageMtx);
    }
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    /* The address of the normalisation is sent, not its value. */
    gSPPerspNormalize(gDisplayListPtr++, (u32)&g_wPerspNorm);
    gSPDisplayList(gDisplayListPtr++, g_aCarStageDiscDisplayList);
}

void DrawCarStageChoiceMarker(Vehicle *vehicle) {
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    /* The address of the normalisation is sent, not its value. */
    gSPPerspNormalize(gDisplayListPtr++, (u32)&g_wPerspNorm);
    gSPDisplayList(gDisplayListPtr++, g_aCarStageChoiceMarkerDisplayList);
}

void DrawCarStageShadow(Vehicle *vehicle) {
    Mtx *block;
    s32 car;

    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    /* The address of the normalisation is sent, not its value. */
    gSPPerspNormalize(gDisplayListPtr++, (u32)&g_wPerspNorm);
    block = (Mtx *)AllocDisplayListBlock();
    car = g_adwVehicleModelSlotCars[vehicle->model - g_aVehicleModels];
    guScaleF(
        g_afMvpMatrix,
        (f32)((f64)g_aCarModels[car].shadowLength / 256),
        (f32)(((f64)g_aCarModels[car].shadowHalfWidth + g_aCarModels[car].shadowHalfWidth) / 256),
        1.0f / 256.0f);
    memcpy(g_afScratchMatrix, vehicle->matrix, sizeof(g_afScratchMatrix));
    g_afMvpMatrix[3][2] -= 0.18f;
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afScratchMatrix);
    guMtxF2L(g_afScratchMatrix, block);
    gSPMatrix(gDisplayListPtr++, block, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPDisplayList(gDisplayListPtr++, g_aCarStageShadowDisplayList);
}

#include "vehiclerender.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "fade.h"
#include "input.h"
#include "mathutil.h"
#include "modeldisplaylist.h"
#include "racemenu.h"
#include "render.h"
#include "track.h"
#include "util.h"
#include "visgrid.h"
#include "weather.h"

/* The sides of a fan, and the radius of the needle fan in its units. */
#define NEEDLE_FAN_POINTS 31
#define NEEDLE_FAN_RADIUS 256.0f

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

void RenderSpeedometerNeedle(Vehicle *vehicle) {
    f32 angle;
    s32 i;

    if (g_dwGameFrameCount == 0) {
        for (i = 0; i < NEEDLE_FAN_POINTS; i++) {
            angle = (f32)i * 0.2094395161f;
            g_aSpeedometerNeedleVertices[i].v.ob[0] = (s32)(cosf(angle) * NEEDLE_FAN_RADIUS);
            g_aSpeedometerNeedleVertices[i].v.ob[1] = (s32)(sinf(angle) * NEEDLE_FAN_RADIUS);
            g_aSpeedometerNeedleVertices[i].v.ob[2] = 0;
            g_aSpeedometerNeedleVertices[i].v.cn[0] = 0xFF;
            g_aSpeedometerNeedleVertices[i].v.cn[1] = 0xFF;
            g_aSpeedometerNeedleVertices[i].v.cn[2] = 0xFF;
            g_aSpeedometerNeedleVertices[i].v.cn[3] = 0xFF;
        }
        g_aSpeedometerNeedleVertices[0].v.ob[0] = 0;
        g_aSpeedometerNeedleVertices[0].v.ob[1] = 0;
        g_aSpeedometerNeedleVertices[0].v.ob[2] = 0;
        guScaleF(g_afMvpMatrix, 0.009765625f, 0.009765625f, 0.009765625f);
        guRotateF(g_afScratchMatrix, -45.0f, 0.0f, 0.0f, 1.0f);
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afScratchMatrix);
        guMtxF2L(g_afScratchMatrix, &g_SpeedometerNeedleMtx);
    }
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    /* The address of the normalisation is sent, not its value. */
    gSPPerspNormalize(gDisplayListPtr++, (u32)&g_wPerspNorm);
    gSPDisplayList(gDisplayListPtr++, g_aSpeedometerNeedleDisplayList);
}

void EmitTachometerDisplayList(Vehicle *vehicle) {
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    /* The address of the normalisation is sent, not its value. */
    gSPPerspNormalize(gDisplayListPtr++, (u32)&g_wPerspNorm);
    gSPDisplayList(gDisplayListPtr++, g_aTachometerDisplayList);
}

void RenderTrackShadow(Vehicle *vehicle) {
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
    gSPDisplayList(gDisplayListPtr++, g_aTrackShadowDisplayList);
}

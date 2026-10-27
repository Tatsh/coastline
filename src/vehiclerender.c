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

/* The deepest a side is dented before it dents no more. */
#define MAX_DENT 0x100

/* The display lists of a model that the dents leave alone. */
#define UNDENTED_DISPLAY_LIST VEHICLE_DL_HIGHLIGHTED

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

/** How much a coordinate of a vertex moves, by the low bits of another coordinate as noise. */
static inline s32 DentOffset(s32 dent, s32 noise, s32 below, s32 above, s32 shift) {
    if (noise & 0x80) {
        return (dent * (below - (noise & 0xF))) >> shift;
    }
    return (dent * ((noise & 0xF) - above)) >> shift;
}

void DeformVehicleMeshByImpact(Vehicle *vehicle, Vec3f *direction, s16 amount) {
    Gfx *command;
    Vtx *vertex;
    s32 degrees, side, maxX, minX, maxY, minY, jitter, dentX, dentY, dentZ, count, lod, list;
    s16 x, y, z, xy;

    degrees = (s32)(Atan2f(direction->x, direction->y) * 180.0f / 3.141592741f);
    /* The side of the blow picks the box of the model it dents. */
    maxX = 0x3FFF;
    minX = 0xFF;
    maxY = 0x3FFF;
    minY = -0x3FFF;
    if (degrees < 20 || degrees >= 340) {
        side = 0;
    } else if (degrees < 50) {
        minX = 0x80;
        minY = 0x40;
        side = 1;
    } else if (degrees < 130) {
        minX = -0x3FFF;
        minY = 0x40;
        side = 2;
    } else if (degrees < 160) {
        maxX = -0x80;
        minX = -0x3FFF;
        minY = 0x40;
        side = 3;
    } else if (degrees < 200) {
        maxX = -0xFF;
        minX = -0x3FFF;
        side = 4;
    } else if (degrees < 230) {
        maxX = -0x80;
        minX = -0x3FFF;
        maxY = -0x40;
        side = 5;
    } else if (degrees < 310) {
        minX = -0x3FFF;
        maxY = -0x40;
        side = 6;
    } else {
        minX = 0x80;
        maxY = -0x40;
        side = 7;
    }
    amount = (s16)(amount << 2);
    if (vehicle->dents[side] >= MAX_DENT) {
        return;
    }
    vehicle->dents[side] += amount;
    vehicle->dentJitter = ((vehicle->dentJitter + 5) & 7) - 4;
    jitter = vehicle->dentJitter;
    /* A negative offset is moved up by one. */
    if (jitter < 0) {
        jitter++;
    }

    /* The blow dents across by its sideways part, and up or down by its vertical part. */
    if (1.0f < direction->y) {
        dentY = amount;
    } else if (0.0f < direction->y) {
        dentY = (s16)(amount >> 1);
    } else if (direction->y < -1.0f) {
        dentY = (s16)-amount;
    } else {
        dentY = (s16) - (amount >> 1);
    }
    if (1.0f < direction->z) {
        dentZ = (s16)(amount >> 2);
    } else if (0.0f < direction->z) {
        dentZ = (s16)(amount >> 3);
    } else if (direction->z < -1.0f) {
        dentZ = (s16) - (amount >> 2);
    } else {
        dentZ = (s16) - (amount >> 3);
    }
    if (1.25f < direction->x) {
        dentX = amount;
    } else if (0.0f < direction->x) {
        dentX = (s16)(amount >> 1);
    } else {
        /* A blow from behind dents fully along, and doubles the vertical dent. */
        dentX = (s16)-amount;
        if (direction->x < -1.25f) {
            dentZ = (s16)(dentZ << 1);
        }
    }

    /* Every vertex of the box, in every display list but the highlighted parts, moves by a little
     * noise from its own coordinates. */
    for (lod = 0; lod < VEHICLE_LODS; lod++) {
        for (list = 0; list < VEHICLE_DISPLAY_LISTS; list++) {
            if (list == UNDENTED_DISPLAY_LIST) {
                continue;
            }
            command = vehicle->model->displayLists[lod][list];
            if (command == NULL) {
                continue;
            }
            /* The original tests for vertices before the end of the list. */
            for (;; command++) {
                if ((command->words.w0 >> 24) != G_VTX) {
                    if ((command->words.w0 >> 24) == G_ENDDL) {
                        break;
                    }
                    continue;
                }
                count = (command->words.w0 >> 10) & 0x3F;
                for (vertex = (Vtx *)command->words.w1; count != 0; count--, vertex++) {
                    x = vertex->v.ob[0] + jitter;
                    if (!(minX < x && x < maxX)) {
                        continue;
                    }
                    y = vertex->v.ob[1] + jitter;
                    if (!(minY < y && y < maxY)) {
                        continue;
                    }
                    z = vertex->v.ob[2] + jitter;
                    if (z < -0x2F || z >= 0xE0) {
                        continue;
                    }
                    vertex->v.ob[0] += DentOffset(dentX, y, 4, 12, 5);
                    vertex->v.ob[1] += DentOffset(dentY, x, 4, 12, 5);
                    xy = x + y;
                    vertex->v.ob[2] += DentOffset(dentZ, xy, 8, 8, 6);
                }
            }
        }
    }
}

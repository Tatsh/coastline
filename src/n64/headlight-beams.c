#include <PR/gbi.h>

#include "common/camera.h"
#include "common/fade.h"
#include "common/modeldisplaylist.h"
#include "common/render.h"
#include "common/system.h"
#include "common/vehicleaccess.h"
#include "common/vehiclerender.h"
#include "common/weather.h"

void RenderVehicleHeadlightBeams(Vehicle *vehicle) {
    if ((g_dwNightMode != 0 || g_dwRainMode != 0) && g_adwVehicleVisible[vehicle->index] != 0) {
        if (vehicle == g_pActiveCameraVehicle &&
            g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
            return;
        }
        if (vehicle->ghostMode == VEHICLE_GHOST) {
            return;
        }
        g_pCurrentVehicleModelData = vehicle->model;
        gDisplayListWords(
            gDisplayListPtr++, 0x1060040, GBI_WORD1(g_apVehicleModelMatrix[vehicle->index]));
        gDisplayListWords(gDisplayListPtr++, 0x1030040, GBI_WORD1(g_pProjectionMtx));
        gDisplayListWords(
            gDisplayListPtr++, 0x39e0010, GBI_WORD1(g_apVehicleLightMatrix[vehicle->index]));
        gDisplayListWords(gDisplayListPtr++,
                          0x3980010,
                          GBI_WORD1((u8 *)g_apVehicleLightMatrix[vehicle->index] + 0x10));
        gDisplayListWords(gDisplayListPtr++,
                          0x39a0010,
                          GBI_WORD1((u8 *)g_apVehicleLightMatrix[vehicle->index] + 0x20));
        gDisplayListWords(gDisplayListPtr++,
                          0x39c0010,
                          GBI_WORD1((u8 *)g_apVehicleLightMatrix[vehicle->index] + 0x30));
        gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[5]);
        DrawTexturedTile(5, g_pCurrentVehicleModelData->textures);
        gDPPipeSync(gDisplayListPtr++);
        gDisplayListWords(gDisplayListPtr++, 0xba001402, 0);
        gDisplayListWords(gDisplayListPtr++, 0xbc00000a, 0);
        gDisplayListWords(gDisplayListPtr++, 0xbc00040a, 0);
        gDisplayListWords(gDisplayListPtr++, 0xbc00200a, 0xffffff00);
        gDisplayListWords(gDisplayListPtr++, 0xbc00240a, 0xffffff00);
        gDisplayListWords(gDisplayListPtr++, 0xfcffffff, 0xffff73b9);
        {
            Gfx *command = gDisplayListPtr++;
            command->words.w0 = 0xb900031d;
            command->words.w1 = 0x4049d8;
        }
        gDisplayListWords(gDisplayListPtr++, 0xba000602, 0x80);
        if (g_pCurrentVehicleModelData->displayLists[0][VEHICLE_DL_HEADLIGHT_BEAMS] != 0) {
            gSPDisplayList(gDisplayListPtr++,
                           g_pCurrentVehicleModelData->displayLists[0][VEHICLE_DL_HEADLIGHT_BEAMS]);
        }
        gDPPipeSync(gDisplayListPtr++);
        gDisplayListWords(gDisplayListPtr++, 0xba000602, g_dwColorDitherMode);
        if (vehicle != g_pActiveCameraVehicle) {
            /* This slot preserves the original IDO register allocation. */
            s32 unused TGR_MATCHING_UNUSED;
            Vec3f direction;
            Vec3f beam;
            f32 glare;
            f32 distance;

            Vec3fAdd(&direction, &vehicle->frame.position, &vehicle->frame.forward);
            Vec3fSubtract(&direction, &g_pActiveCamera->position, &direction);
            distance = Vec3fLength(&direction);
            if (distance != 0.0f) {
                Vec3fDivideInPlace(&direction, distance);
                if (Vec3fDot(&direction, &g_pActiveCamera->forward) < 0.0f) {
                    Vec3fScale(&beam, &vehicle->frame.forward, 1.0f);
                    Vec3fMulAddInPlace(&beam, &vehicle->frame.up, -0.0f);
                    glare = Vec3fDot(&direction, &beam) *
                            -Vec3fDot(&direction, &g_pActiveCamera->forward);
                    if (glare > 0.95f) {
                        distance = distance * distance;
                        g_fHeadlightGlare += 750.0f * (glare - 0.95f) / distance;
                    }
                }
            }
        }
        gDPPipeSync(gDisplayListPtr++);
        gDisplayListWords(gDisplayListPtr++, 0xba001402, 0);
        gDisplayListWords(gDisplayListPtr++, 0xbd000000, 0);
        gDisplayListWords(gDisplayListPtr++, 0xb6000000, 0x40000);
        gDisplayListWords(gDisplayListPtr++, 0xbc000002, 0x80000040);
        gDisplayListWords(gDisplayListPtr++, 0x3860010, GBI_WORD1(&g_DefaultLights.l[0]));
        gDisplayListWords(gDisplayListPtr++, 0x3880010, GBI_WORD1(&g_DefaultLights.a));
        gDisplayListWords(gDisplayListPtr++, 0xba000c02, g_dwTextureFilterMode);
        gDisplayListWords(gDisplayListPtr++, 0xba000e02, 0);
        gDisplayListWords(gDisplayListPtr++, 0xfc121824, 0xff33ffff);
    }
}

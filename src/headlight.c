#include "camera.h"
#include "fade.h"
#include "modeldisplaylist.h"
#include "system.h"
#include "vehicleaccess.h"
#include "vehiclerender.h"
#include "weather.h"

#define FAR_CLIP_RATIO 6
#define HEADLIGHT_BEAM_TEXTURE 5

void ResetHeadlightGlare(void) {
    g_fHeadlightGlare = 0.0f;
#if TGR_PC
    g_fScreenFogPulse = 0.0f;
#endif
}

void RenderVehicleHeadlightBeams(Vehicle *vehicle) {
    Vec3f toCamera, beam;
#if TGR_PC
    volatile f32 distance, along;
    long double rawDistance, glare;
#else
    f32 distance, along, glare;
#endif

    if (!g_dwNightMode && !g_dwRainMode) {
        return;
    }
    if (!g_adwVehicleVisible[VEHICLE_GET(vehicle, Index)]) {
        return;
    }
    if (vehicle == g_pActiveCameraVehicle &&
        g_pActiveCamera == &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_BUMPER]) {
        return;
    }
    if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
        return;
    }
    g_pCurrentVehicleModelData = VEHICLE_GET(vehicle, Model);
    gSPMatrix(gDisplayListPtr++,
              g_apVehicleModelMatrix[VEHICLE_GET(vehicle, Index)],
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPForceMatrix(gDisplayListPtr++, g_apVehicleLightMatrix[VEHICLE_GET(vehicle, Index)]);
    gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[FAR_CLIP_RATIO - 1]);
    DrawTexturedTile(HEADLIGHT_BEAM_TEXTURE, g_pCurrentVehicleModelData->textures);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    SetVehicleLightColors(0, 0xFFFFFF00);
    gDPSetCombineLERP(gDisplayListPtr++, 0, 0, 0, 1, 0, 0, 0, TEXEL0, 0, 0, 0, 1, 0, 0, 0, TEXEL0);
    gDPSetRenderMode(gDisplayListPtr++,
                     AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | ZMODE_XLU | FORCE_BL |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_IN, G_BL_1MA));
    gDPSetColorDither(gDisplayListPtr++, G_CD_NOISE);
    /* Only the beams of the nearest level of detail are drawn. */
    DrawModelDisplayList(g_pCurrentVehicleModelData->displayLists[0][VEHICLE_DL_HEADLIGHT_BEAMS]);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);

    /* Beams shining at the camera glare the view, the more the nearer. */
    if (vehicle != g_pActiveCameraVehicle) {
        Vec3fAdd(
            &toCamera, &VEHICLE_GET(vehicle, Frame).position, &VEHICLE_GET(vehicle, Frame).forward);
        Vec3fSubtract(&toCamera, &g_pActiveCamera->position, &toCamera);
#if TGR_PC
        rawDistance = Vec3fLength(&toCamera);
        distance = rawDistance;
        if (rawDistance != 0.0L && rawDistance == rawDistance) {
#else
        distance = Vec3fLength(&toCamera);
        if (distance != 0.0f) {
#endif
            Vec3fDivideInPlace(&toCamera, distance);
#if TGR_PC
            if (!(Vec3fDot(&toCamera, &g_pActiveCamera->forward) >= 0.0L)) {
#else
            if (Vec3fDot(&toCamera, &g_pActiveCamera->forward) < 0.0f) {
#endif
                Vec3fScale(&beam, &VEHICLE_GET(vehicle, Frame).forward, 1.0f);
#if TGR_PC
                Vec3fMulAddInPlace(&beam, &VEHICLE_GET(vehicle, Frame).up, 0.0f);
#else
                /* The beam tilt multiplies the up axis by negative zero. */
                Vec3fMulAddInPlace(&beam, &VEHICLE_GET(vehicle, Frame).up, -0.0f);
#endif
                along = Vec3fDot(&toCamera, &beam);
#if TGR_PC
                glare = -(Vec3fDot(&toCamera, &g_pActiveCamera->forward) * along);
                if (0.95f < glare) {
                    distance = (long double)distance * distance;
                    g_fHeadlightGlare += 750.0f * (glare - 0.95f) / distance;
                }
#else
                glare = -Vec3fDot(&toCamera, &g_pActiveCamera->forward) * along;
                if (0.95f < glare) {
                    g_fHeadlightGlare += 750.0f * (glare - 0.95f) / (distance * distance);
                }
#endif /* TGR_PC */
            }
#if TGR_PC
            if (Vec3fDot(&toCamera, &g_pActiveCamera->forward) > 0.95f) {
                Vec3fScale(&beam, &VEHICLE_GET(vehicle, Frame).forward, 1.0f);
                Vec3fMulAddInPlace(&beam, &VEHICLE_GET(vehicle, Frame).up, 0.0f);
                along = Vec3fDot(&toCamera, &beam);
                glare = Vec3fDot(&toCamera, &g_pActiveCamera->forward) * along;
                if (0.95f < glare) {
                    g_fScreenFogPulse +=
                        750.0f * (glare - 0.95f) / ((long double)distance * distance);
                }
            }
#endif /* TGR_PC */
        }
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_DefaultLights.l[0], 1);
    gSPLight(gDisplayListPtr++, &g_DefaultLights.a, 2);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetCombineLERP(gDisplayListPtr++,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      TEXEL0,
                      0,
                      SHADE,
                      0);
}

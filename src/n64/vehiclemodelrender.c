#include <PR/gu.h>

#include "common/camera.h"
#include "common/input.h"
#include "common/modeldisplaylist.h"
#include "common/render.h"
#include "common/system.h"
#include "common/track.h"
#include "common/util.h"
#include "common/vehicleaccess.h"
#include "common/vehiclemodel.h"
#include "common/vehiclerender.h"
#include "common/vehicleview.h"
#include "common/weather.h"

#undef TXL2WORDS_4b
#define TXL2WORDS_4b(txls) (((txls) / 16) > 1 ? ((txls) / 16) : 1)
#undef CALC_DXT_4b
#define CALC_DXT_4b(width) (((1 << 11) + TXL2WORDS_4b(width) - 1) / TXL2WORDS_4b(width))
#define VEC3_COPY(d, s)                                                                            \
    {                                                                                              \
        float *_d = (float *)(d);                                                                  \
        float *_s = (float *)(s);                                                                  \
        _d[0] = _s[0];                                                                             \
        _d[1] = _s[1];                                                                             \
        _d[2] = _s[2];                                                                             \
    }
/* The original GBI expansion fixes the order of the emitted display-list words. */
#undef gDma1p
#define gDma1p(pkt, c, s, l, p)                                                                    \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | _SHIFTL((l), 0, 16));          \
        _g->words.w1 = (unsigned int)(s);                                                          \
    }
#undef gDPSetCombine
#define gDPSetCombine(pkt, muxs0, muxs1)                                                           \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(muxs0, 0, 24);                       \
        _g->words.w1 = (unsigned int)(muxs1);                                                      \
    }
#undef gSPLookAtX
#define gSPLookAtX(pkt, l) gDma1p(pkt, G_MOVEMEM, l, 16, G_MV_LOOKATX)
#undef gSPLookAtY
#define gSPLookAtY(pkt, l) gDma1p(pkt, G_MOVEMEM, l, 16, G_MV_LOOKATY)

void RenderVehicleModel(Vehicle *car, int lodBias) {
    int lod;
    int tunnel;
    float dist;
    float reservedNearDistance
        TGR_MATCHING_UNUSED; /* The original stack layout includes this unused slot. */
    void *hilite;
    LookAt *reflect;
    LookAt *look;
    unsigned int colA;
    unsigned int colB;
    int mode;
    u8(*cls)[2];
    float div;
    int texturePalette;
    int lights;
    int k;
    unsigned char c[4];
    int tw;
    int th;
    int cw;
    int ch;
    unsigned short *pal;
    int reservedScratchWords[4] TGR_MATCHING_UNUSED; /* The original stack layout includes these
                                                        unused slots. */
    Vec3f toCam;
    float x;
    float y;
    float z;
    float atZ;
    float atX;
    float eye[3];

    tw = 64;
    th = 64;
    tunnel = 0;
    if (g_adwVehicleInViewRange[car->index] == 0) {
        return;
    }
    dist = Vec3fDistance((Vec3f *)car->matrix[3], (Vec3f *)((float (*)[4])g_pActiveCamera)[3]);
    if (car->ghostMode == 2) {
        g_dwVehicleFogAlpha = car->opacity * 255.0f;
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);
            _g->words.w0 = _SHIFTL(G_SETFOGCOLOR, 24, 8);
            _g->words.w1 = _SHIFTL(g_bFogR, 24, 8) | _SHIFTL(g_bFogG, 16, 8) |
                           _SHIFTL(g_bFogB, 8, 8) | _SHIFTL(g_dwVehicleFogAlpha, 0, 8);
        }
    }
    *(int *)(&car->inTunnel) = 0;
    if (car->groundInstanceCount != 0 &&
        (g_LoadedTrack.instances[car->groundInstances[0]].flags & 0x10) != 0) {
        *(int *)(&car->inTunnel) = 1;
        tunnel = 1;
    }
    g_pCurrentVehicleModelData = (VehicleModel *)car->model;
    if (g_dwNumViewports == 2) {
        if (dist < 40.0f) {
            lod = 0;
        } else if (dist < 80.0f) {
            lod = 1;
        } else {
            lod = 2;
        }
        if (lod < g_dwVehicleLodBias) {
            lod = g_dwVehicleLodBias;
        }
    } else {
        lod = g_dwVehicleLodBias;
    }
    lod += lodBias;
    if (lod > 2) {
        lod = 2;
    }
    guScaleF(g_afScratchMatrix,
             g_fConstInv255_802a9ed4,
             g_fConstInv255_802a9ed4,
             g_fConstInv255_802a9ed4);
    guMtxCatF(g_afScratchMatrix, car->matrix, g_mtxVehicleModelScaled);
    g_apVehicleModelMatrix[car->index] = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_mtxVehicleModelScaled, (void *)g_apVehicleModelMatrix[car->index]);
    guMtxCatF(g_mtxVehicleModelScaled, g_afViewProjMatrix, g_afMvpMatrix);
    NormalizeLightMatrix(g_afMvpMatrix);
    g_apVehicleLightMatrix[car->index] = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afMvpMatrix, (void *)g_apVehicleLightMatrix[car->index]);
    if (car == g_pActiveCameraVehicle &&
        (car->activeCamera == &car->cameras[0] ||
         car->activeCamera == &car->cameras[VEHICLE_CAMERA_MIRROR]) &&
        g_dwVehicleCullingDisabled == 0) {
        return;
    }
    g_anVehicleLod[car->index] = lod;
    if (g_dwNightMode != 0) {
        div = dist / 10.0f;
        if (div < 1.0f) {
            div = 1.0f;
        }
        c[0] = (unsigned int)g_bDirLightR / div;
        c[1] = (unsigned int)g_bDirLightG / div;
        c[2] = (unsigned int)g_bDirLightB / div;
        colA = c[0] << 24 | c[1] << 16 | c[2] << 8;
        c[0] = g_bAmbientLightR;
        c[1] = g_bAmbientLightG;
        c[2] = g_bAmbientLightB;
        colB = c[0] << 24 | c[1] << 16 | c[2] << 8;
    } else if (tunnel != 0) {
        c[0] = 0;
        c[1] = 0;
        c[2] = 0;
        colA = c[0] << 24 | c[1] << 16 | c[2] << 8;
        c[0] = (g_bAmbientLightR << 2) / 5;
        c[1] = (g_bAmbientLightG << 2) / 5;
        c[2] = (g_bAmbientLightB << 2) / 5;
        colB = c[0] << 24 | c[1] << 16 | c[2] << 8;
    } else {
        c[0] = g_bDirLightR;
        c[1] = g_bDirLightG;
        c[2] = g_bDirLightB;
        colA = c[0] << 24 | c[1] << 16 | c[2] << 8;
        c[0] = g_bAmbientLightR;
        c[1] = g_bAmbientLightG;
        c[2] = g_bAmbientLightB;
        colB = c[0] << 24 | c[1] << 16 | c[2] << 8;
    }
    gSPMatrix(gDisplayListPtr++, g_apVehicleModelMatrix[car->index], 6);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, 3);
    if (g_dwNightMode != 0) {
        if (&g_pActiveCameraVehicle->cameras[3] == (CameraFrame *)g_pActiveCamera) {
            Vec3fNegate(&g_vecVehicleLightDir, (Vec3f *)g_pActiveCamera);
        } else {
            Vec3fNegate(&g_vecVehicleLightDir, (Vec3f *)g_pActiveCameraVehicle);
        }
    } else {
        x = g_afLightDirection[0];
        y = g_afLightDirection[1];
        z = g_afLightDirection[2];
        g_vecVehicleLightDir.x = x;
        g_vecVehicleLightDir.y = y;
        g_vecVehicleLightDir.z = z;
    }
    Vec3fNormalize(&g_vecVehicleLightDir);
    g_vecVehicleHiliteDir.x = g_vecVehicleLightDir.x;
    g_vecVehicleHiliteDir.y = g_vecVehicleLightDir.y;
    g_vecVehicleHiliteDir.z = g_vecVehicleLightDir.z;
    Vec3fSubtract(&toCam, (Vec3f *)((float (*)[4])g_pActiveCamera)[3], (Vec3f *)car->matrix[3]);
    div = Vec3fLength(&toCam);
    if (div == 0) {
        Vec3fNegate(&toCam, (Vec3f *)g_pActiveCamera);
    } else {
        Vec3fDivideInPlace(&toCam, div);
    }
    Vec3fMidpoint(&g_vecVehicleHiliteDir, &toCam, &g_vecVehicleHiliteDir);
    div = Vec3fLength(&g_vecVehicleHiliteDir);
    if (div == 0) {
        VEC3_COPY(&g_vecVehicleHiliteDir, ((float (*)[4])g_pActiveCamera)[2]);
    } else {
        Vec3fDivideInPlace(&g_vecVehicleHiliteDir, div);
    }
    AllocDisplayListBlock();
    hilite = AllocHiliteBlock();
    reflect = AllocLightBlock();
    look = AllocLightBlock();
    atZ = 0.0f;
    atX = 0.0f;
    if (((float (*)[4])g_pActiveCamera)[3][0] == car->matrix[3][0] &&
        ((float (*)[4])g_pActiveCamera)[3][1] == car->matrix[3][1]) {
        if (((float (*)[4])g_pActiveCamera)[3][2] == car->matrix[3][2]) {
            atZ = 1.0f;
        } else {
            atX = g_fConst0p1_802a9ed8;
        }
    }
    eye[0] = ((float (*)[4])g_pActiveCamera)[0][0];
    eye[1] = ((float (*)[4])g_pActiveCamera)[0][1];
    eye[2] = ((float (*)[4])g_pActiveCamera)[0][2];
    if (eye[0] == 0.0f && eye[1] == 0.0f) {
        eye[0] = g_fConst1em4_802a9edc;
    }
    guLookAtReflectF(g_afMvpMatrix, reflect, eye[0], eye[1], 0.0f, 0, 0, 0, 0, 0, 1.0f);
    guLookAtHiliteF(g_afMvpMatrix,
                    look,
                    hilite,
                    ((float (*)[4])g_pActiveCamera)[3][0],
                    ((float (*)[4])g_pActiveCamera)[3][1],
                    ((float (*)[4])g_pActiveCamera)[3][2],
                    car->matrix[3][0] + atX,
                    car->matrix[3][1],
                    car->matrix[3][2] + atZ,
                    0.0f,
                    0.0f,
                    1.0f,
                    g_vecVehicleHiliteDir.x,
                    g_vecVehicleHiliteDir.y,
                    g_vecVehicleHiliteDir.z,
                    g_vecVehicleHiliteDir.x,
                    g_vecVehicleHiliteDir.y,
                    g_vecVehicleHiliteDir.z,
                    64,
                    64);
    if (10.0f < dist) {
        gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[5]);
    } else {
        gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists);
    }
    if (g_dwNightMode != 0 || g_dwRainMode != 0) {
        g_aVehicleLights[car->index] = g_DefaultLights;
        g_aVehicleLights[car->index].l[0].l.dir[0] =
            -(signed char)(int)(g_pActiveCameraVehicle->matrix[0][0] * 120.0f);
        g_aVehicleLights[car->index].l[0].l.dir[1] =
            -(signed char)(int)(g_pActiveCameraVehicle->matrix[0][1] * 120.0f);
        g_aVehicleLights[car->index].l[0].l.dir[2] =
            -(signed char)(int)(g_pActiveCameraVehicle->matrix[0][2] * 120.0f);
        gSPNumLights(gDisplayListPtr++, 1);
        gSPLight(gDisplayListPtr++, &g_aVehicleLights[car->index].l[0], 1);
        gSPLight(gDisplayListPtr++, &g_aVehicleLights[car->index].a, 2);
    } else {
        gSPNumLights(gDisplayListPtr++, 1);
        gSPLight(gDisplayListPtr++, &g_DefaultLights.l[0], 1);
        gSPLight(gDisplayListPtr++, &g_DefaultLights.a, 2);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTextureLOD(gDisplayListPtr++, 0x10000);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);

        _g->words.w0 = 0xB7000000;
        _g->words.w1 = 0x20205;
    }
    if (g_dwWeatherActive != 0) {
        gSPSetGeometryMode(gDisplayListPtr++, 0x10000);
        if (car->ghostMode == 2) {
            g_dwVehicleOtherModeLBlend = 0x0C080000;
        } else {
            g_dwVehicleOtherModeLBlend = 0xC8000000;
        }
    } else {
        g_dwVehicleOtherModeLBlend = 0x0C080000;
    }
    gSPSetGeometryMode(gDisplayListPtr++,
                       g_dwTrackMirrored - g_dwViewportMirrored ? 0x1000 : 0x2000);
    gSPClearGeometryMode(gDisplayListPtr++,
                         g_dwTrackMirrored - g_dwViewportMirrored ? 0x2000 : 0x1000);
    if (car->ghostMode == 2) {
        g_dwVehicleOtherModeLFlags = 0x011049D8;
    } else if ((dist < 100.0f) != 0) {
        if (car == g_pActiveCameraVehicle) {
            g_dwVehicleOtherModeLFlags = 0x00112078;
        } else {
            g_dwVehicleOtherModeLFlags = 0x00112038;
        }
    } else {
        g_dwVehicleOtherModeLFlags = 0x00112230;
    }
    gDPSetCycleType(gDisplayListPtr++, 0x100000);
    gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
    gDPSetCombine(gDisplayListPtr++, 0x127FFF, 0xFFFFF238);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureLOD(gDisplayListPtr++, 0);
    gSPClearGeometryMode(gDisplayListPtr++, 0xC0000);
    gMoveWd(gDisplayListPtr++, 0x0A, 0x00, colA);
    gMoveWd(gDisplayListPtr++, 0x0A, 0x04, colA);
    gMoveWd(gDisplayListPtr++, 0x0A, 0x20, colB);
    gMoveWd(gDisplayListPtr++, 0x0A, 0x24, colB);
    if (car->ghostMode == 2) {
        RenderWheelModels(car);
    }
    gDma1p(gDisplayListPtr++, 0x03, g_apVehicleLightMatrix[car->index], 0x10, 0x9E);
    gDma1p(gDisplayListPtr++, 0x03, (u8 *)g_apVehicleLightMatrix[car->index] + 0x10, 0x10, 0x98);
    gDma1p(gDisplayListPtr++, 0x03, (u8 *)g_apVehicleLightMatrix[car->index] + 0x20, 0x10, 0x9A);
    gDma1p(gDisplayListPtr++, 0x03, (u8 *)g_apVehicleLightMatrix[car->index] + 0x30, 0x10, 0x9C);
    gDPSetTextureImage(gDisplayListPtr++, 0, 2, 1, &g_awVehicleShadePalette);
    gDPTileSync(gDisplayListPtr++);
    gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1E0, 7, 0, 0, 0, 0, 0, 0, 0);
    gDPLoadSync(gDisplayListPtr++);
    gDPLoadTLUTCmd(gDisplayListPtr++, 7, 15);
    gDPPipeSync(gDisplayListPtr++);
    gSPLookAtX(gDisplayListPtr++, look);
    gSPLookAtY(gDisplayListPtr++, (u8 *)look + 0x10);
    if (g_bForceVehicleAltVariant == 0 && car->otherLook == 0) {
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
        gSPClearGeometryMode(gDisplayListPtr++, 0xC0000);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++, 0, 2, 0, 0x0, 7, 0, 0, 0, 0, 0, 0, 0);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
        gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
        gDPSetCombine(gDisplayListPtr++, 0x127FFF, 0xFFFFF238);
        gDPSetTextureLUT(gDisplayListPtr++, 0);
        gDPSetCombine(gDisplayListPtr++, 0x1219FF, 0xFFFFFE38);
        gSPClearGeometryMode(gDisplayListPtr++, 0x40000);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x00, colA);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x04, colA);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x20, colB);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x24, colB);
        gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
        if (g_pCurrentVehicleModelData->displayLists[lod][8] != 0) {
            gSPDisplayList(gDisplayListPtr++, g_pCurrentVehicleModelData->displayLists[lod][8]);
        }
        gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
        gSPClearGeometryMode(gDisplayListPtr++, 0xC0000);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++, 0, 2, 0, 0x0, 7, 0, 0, 0, 0, 0, 0, 0);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
    }
    mode = g_dwNightMode;
    lights = mode != 0 || g_dwRainMode != 0;
    if (mode != 0) {
    }
    pal = g_pCurrentVehicleModelData->textures[g_pCurrentVehicleModelData->materialTextures[10]]
              .palette;
    if (pal != 0) {
        for (k = 12; k < 16; k++) {
            if (lights) {
                pal[k] &= ~1;
            } else {
                pal[k] |= 1;
            }
        }
    }
    pal = g_pCurrentVehicleModelData->textures[g_pCurrentVehicleModelData->materialTextures[11]]
              .palette;
    if (pal != 0) {
        if (lights) {
            pal[15] = 0x7000;
            pal[10] = 0x9082;
        } else {
            pal[15] = 0x9001;
            pal[10] = 0xA001;
        }
        if ((car->input->buttons & 0xC0000) != 0) {
            pal[13] = pal[14] = 0xC000;
            pal[8] = pal[9] = 0xF904;
        } else {
            pal[14] = 0x9001;
            pal[13] = pal[15];
            pal[9] = 0xA001;
            pal[8] = pal[10];
        }
        pal[12] = 0x7981;
        pal[7] = 0x9241;
        if (car->throttle < 0.0f) {
            pal[11] = 0xB5AC;
            pal[6] = 0xFFFE;
        } else {
            pal[11] = 0xAD6B;
            pal[6] = 0xC631;
        }
    }
    gDPSetTextureLOD(gDisplayListPtr++, 0x10000);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
    gDPSetTile(gDisplayListPtr++, 0, 2, 0, 0x0, 7, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
    {
        Vec3f rel;

        Vec3fSubtract(&rel, (Vec3f *)car->matrix[3], (Vec3f *)((float (*)[4])g_pActiveCamera)[3]);
        if (0.0 < Vec3fDot((Vec3f *)car->matrix[2], &rel)) {
            DrawTexturedTile(6, g_pCurrentVehicleModelData->textures);
            gDPPipeSync(gDisplayListPtr++);
            {
                Gfx *_g = (Gfx *)(gDisplayListPtr++);

                _g->words.w0 = 0xBA001402;
                _g->words.w1 = 0x100000;
            }
            gDPSetRenderMode(
                gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
            gDPSetCombine(gDisplayListPtr++, 0x127FFF, 0xFFFFF838);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x00, colA);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x04, colA);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x20, colB);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x24, colB);
            gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
            gDPSetTile(gDisplayListPtr++, 0, 2, 0, 0x0, 7, 0, 0, 0, 0, 0, 0, 0);
            gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
            gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
            if (g_pCurrentVehicleModelData->displayLists[lod][6] != 0) {
                gSPDisplayList(gDisplayListPtr++, g_pCurrentVehicleModelData->displayLists[lod][6]);
            }
        }
    }
    {
        float near;
        int scroll;

        if (((PlayerInputState *)g_pActiveCameraVehicle->input)->playbackBuffer != 0 &&
            &g_pActiveCameraVehicle->cameras[3] == (CameraFrame *)g_pActiveCamera) {
            near = dist < 120.0f;
        } else {
            near = dist < 50.0f;
        }
        if (g_dwViewportMirrored == 0 && near && car->ghostMode != 2 && lod < 1) {
            DrawTexturedTile(3, g_pCurrentVehicleModelData->textures);
            gDPPipeSync(gDisplayListPtr++);
            {
                Gfx *_g = (Gfx *)(gDisplayListPtr++);

                _g->words.w0 = 0xBA001402;
                _g->words.w1 = 0x100000;
            }
            gDPSetRenderMode(
                gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
            gDPSetCombine(gDisplayListPtr++, 0x127FFF, 0xFFFFF838);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x00, colA);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x04, colA);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x20, colB);
            gMoveWd(gDisplayListPtr++, 0x0A, 0x24, colB);
            gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, 0, 1);
            gDPSetTile(gDisplayListPtr++, 0, 2, 0, 0x0, 7, 0, 0, 0, 0, 0, 0, 0);
            gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
            gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
            if (g_pCurrentVehicleModelData->displayLists[lod][3] != 0) {
                gSPDisplayList(gDisplayListPtr++, g_pCurrentVehicleModelData->displayLists[lod][3]);
            }
        }
        if (g_dwNumViewports == 1 && g_bPaintShopPreviewActive == 0 && g_dwViewportMirrored == 0 &&
            g_dwSnowMode == 0 && g_dwRainMode == 0 && (tunnel == 0 || g_dwNightMode != 0) && near &&
            (g_dwNightMode == 0 || car != g_pActiveCameraVehicle) &&
            g_bForceVehicleAltVariant == 0 && car->otherLook == 0) {
            gDPPipeSync(gDisplayListPtr++);
            gDPSetCycleType(gDisplayListPtr++, 0);
            gSPSetGeometryMode(gDisplayListPtr++, 0x40000);
            gSPTexture(gDisplayListPtr++, 0x0F80, 0x0F80, 0, 0, 1);
            gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
            gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0xFF, 0xFF, 0xCC, 0xFF);
            gDPSetCombine(gDisplayListPtr++, 0xFFFFFF, 0xFFFDF2F9);
            {
                Gfx *_g = (Gfx *)(gDisplayListPtr++);

                _g->words.w0 = 0xB900031D;
                _g->words.w1 = 0x00504F50;
            }
            gDPTileSync(gDisplayListPtr++);
            gDPSetTextureLUT(gDisplayListPtr++, 0);
            gDPSetTextureImage(gDisplayListPtr++,
                               4,
                               2,
                               1,
                               g_dwNightMode ? &g_aVehicleNightHighlightTexture :
                                               &g_aVehicleHighlightTexture);
            {
                Gfx *_g = (Gfx *)(gDisplayListPtr++);

                _g->words.w0 = 0xF5900000;
                _g->words.w1 = 0x07018060;
            }
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++, 7, 0, 0, (((tw) * (th) + 3) >> 2) - 1, CALC_DXT_4b(tw));
            gDPPipeSync(gDisplayListPtr++);
            {
                Gfx *_g = (Gfx *)(gDisplayListPtr++);

                _g->words.w0 = _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(4, 21, 3) | _SHIFTL(0, 19, 2) |
                               _SHIFTL((((tw) >> 1) + 7) >> 3, 9, 9) | _SHIFTL(0, 0, 9);
                _g->words.w1 = _SHIFTL(0, 24, 3) | _SHIFTL(0, 20, 4) | _SHIFTL(0, 18, 2) |
                               _SHIFTL(6, 14, 4) | _SHIFTL(0, 10, 4) | _SHIFTL(0, 8, 2) |
                               _SHIFTL(6, 4, 4) | _SHIFTL(0, 0, 4);
            }
            {
                /* clang-format off */
                Gfx *_g = (Gfx *)(gDisplayListPtr++); _g->words.w0 =
                    _SHIFTL(G_SETTILESIZE, 24, 8) | _SHIFTL(0, 12, 12) | _SHIFTL(0, 0, 12);
                /* clang-format on */
                _g->words.w1 = _SHIFTL(0, 24, 3) | _SHIFTL(((tw)-1) << 2, 12, 12) |
                               _SHIFTL(((th)-1) << 2, 0, 12);
            }
            gDPSetColorDither(gDisplayListPtr++, 0xC0);
            gDPSetHilite1Tile(gDisplayListPtr++, 0, (Hilite *)hilite, 64, 64);
            if (g_pCurrentVehicleModelData->displayLists[lod][9] != 0) {
                gSPDisplayList(gDisplayListPtr++, g_pCurrentVehicleModelData->displayLists[lod][9]);
            }
            gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
        }
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, 0x100000);
        gSPSetGeometryMode(gDisplayListPtr++,
                           (g_bVehicleEnvMapLinearTexGen != 0 ? 0x80000 : 0) | 0x40000);
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = 0xBB000001;
            _g->words.w1 = 0x08001000;
        }
        gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
        gDPSetCombine(gDisplayListPtr++, 0x167E2C, 0x55FEF379);
        cls = g_abVehicleReflectionTexSelect;
        if (g_dwWeatherActive != 0) {
            if (tunnel != 0) {
                gDPSetEnvColor(
                    gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, (g_bFogIntensity >> 3) + 0xDF);
            } else {
                gDPSetEnvColor(
                    gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, (g_bFogIntensity >> 1) + 0x7F);
            }
        } else {
            gDPSetEnvColor(gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, 0xFF);
        }
        gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTextureLUT(gDisplayListPtr++, 0);
        cw = 32;
        texturePalette = 0;
        ch = 128;
        gDPSetTextureImage(
            gDisplayListPtr++,
            2,
            2,
            1,
            g_apVehicleReflectionTextures[(
                (unsigned char (*)[2])cls)[g_dwVehicleViewModeIndex][*(int *)(&car->inTunnel)]]);
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = 0xF5500000;
            _g->words.w1 = 0x0701FC5F;
        }
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadBlock(gDisplayListPtr++, 7, 0, 0, (((tw) * (th) + 3) >> 2) - 1, CALC_DXT_4b(cw));
        gDPPipeSync(gDisplayListPtr++);
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(2, 21, 3) | _SHIFTL(0, 19, 2) |
                           _SHIFTL((((cw) >> 1) + 7) >> 3, 9, 9) | _SHIFTL(0, 0, 9);
            _g->words.w1 = _SHIFTL(0, 24, 3) | _SHIFTL(texturePalette, 20, 4) | _SHIFTL(0, 18, 2) |
                           _SHIFTL(7, 14, 4) | _SHIFTL(15, 10, 4) | _SHIFTL(0, 8, 2) |
                           _SHIFTL(5, 4, 4) | _SHIFTL(15, 0, 4);
        }
        {
            /* clang-format off */
            Gfx *_g = (Gfx *)(gDisplayListPtr++); _g->words.w0 =
                _SHIFTL(G_SETTILESIZE, 24, 8) | _SHIFTL(0, 12, 12) | _SHIFTL(0, 0, 12);
            /* clang-format on */
            _g->words.w1 =
                _SHIFTL(0, 24, 3) | _SHIFTL(((cw)-1) << 2, 12, 12) | _SHIFTL(((ch)-1) << 2, 0, 12);
        }
        gDPLoadSync(gDisplayListPtr++);
        gDPSetTextureImage(
            gDisplayListPtr++,
            0,
            2,
            1,
            g_apVehicleReflectionTluts[(
                (unsigned char (*)[2])cls)[g_dwVehicleViewModeIndex][*(int *)(&car->inTunnel)]]);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++,
                   0,
                   0,
                   0,
                   (256 + (((texturePalette) & 0xf) * 16)),
                   7,
                   0,
                   0,
                   0,
                   0,
                   0,
                   0,
                   0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadTLUTCmd(gDisplayListPtr++, 7, 15);
        gDPPipeSync(gDisplayListPtr++);
        gDPSetTextureLUT(gDisplayListPtr++, 0x8000);
        scroll = tw - (int)(g_pActiveCameraVehicle->cameraHeading * tw * g_fConstInvPi_802a9ee0);
        scroll = 0x1F - scroll;
        gDPSetTileSize(gDisplayListPtr++, 0, scroll + 2, 2, scroll + 0x7E, 0x1FE);
        gDPPipeSync(gDisplayListPtr++);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x00, 0);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x04, 0);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x20, 0);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x24, 0);
        gSPLookAtX(gDisplayListPtr++, reflect);
        gSPLookAtY(gDisplayListPtr++, (u8 *)reflect + 0x10);
        if (g_pCurrentVehicleModelData->displayLists[lod][4] != 0) {
            gSPDisplayList(gDisplayListPtr++, g_pCurrentVehicleModelData->displayLists[lod][4]);
        }
        if ((g_bForceVehicleAltVariant != 0 || car->otherLook != 0) &&
            g_pCurrentVehicleModelData->displayLists[lod][9] != 0) {
            gSPDisplayList(gDisplayListPtr++, g_pCurrentVehicleModelData->displayLists[lod][9]);
        }
        gSPPopMatrix(gDisplayListPtr++, 0);
        gSPClearGeometryMode(gDisplayListPtr++, 0x40000);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x00, colA);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x04, colA);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x20, colB);
        gMoveWd(gDisplayListPtr++, 0x0A, 0x24, colB);
        gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
        gDPSetTextureLUT(gDisplayListPtr++, 0);
        if (car->ghostMode != 2) {
            RenderWheelModels(car);
        }
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, 0);
        gDPSetCombine(gDisplayListPtr++, 0x121824, 0xFF33FFFF);
        g_dwStatVehicleTriangles += *(int *)g_pCurrentVehicleModelData;
    }
}

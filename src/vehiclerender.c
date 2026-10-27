#include "vehiclerender.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "fade.h"
#include "input.h"
#include "mathutil.h"
#include "racemenu.h"
#include "render.h"
#include "track.h"
#include "util.h"
#include "visgrid.h"
#include "weather.h"

/* The physical address of a pointer to the memory of the CPU. */
#define PHYSICAL(address) ((u32)(address) + 0x80000000)

/* The distances from the camera that pick a level of detail when the screen is split in two. */
#define SPLIT_LOD1_DISTANCE 40.0f
#define SPLIT_LOD2_DISTANCE 80.0f

/* The distance within which the clipping ratio is 1, and the ratio beyond it. */
#define NEAR_CLIP_DISTANCE 10.0f
#define FAR_CLIP_RATIO 6

/* The distance within which a car is drawn with its finer parts, and while a replay watches it. */
#define NEAR_DISTANCE 50.0f
#define NEAR_REPLAY_DISTANCE 120.0f

/* The distance within which a car is drawn antialiased. */
#define ANTIALIAS_DISTANCE 100.0f

/* The distance that dims the light of the sun once at night, in tens of units. */
#define NIGHT_LIGHT_DISTANCE 10.0f

/* The length of the direction of the headlights in the lights of a vehicle. */
#define HEADLIGHT_DIRECTION 120.0f

/* The levels of detail of a car, the coarsest last. */
#define COARSEST_LOD (VEHICLE_LODS - 1)

/* The texture whose tiles the underside and the near details use. */
#define UNDERSIDE_TEXTURE 6
#define NEAR_DETAIL_TEXTURE 3
#define HEADLIGHT_BEAM_TEXTURE 5

/* The palette of the textured parts. */
#define SHADE_PALETTE 14

/* The palette entries of the lights of a car, which are changed to light them. */
#define HEADLIGHT_FIRST_ENTRY 12
#define HEADLIGHT_ENTRIES 4

/* The size of the texture of the specular highlight. */
#define HILITE_SIZE 64

/* The sides of a fan, and the radius of the needle fan in its units. */
#define NEEDLE_FAN_POINTS 31
#define NEEDLE_FAN_RADIUS 256.0f

/* The placed model under a vehicle that is a tunnel. */
#define TRACK_INSTANCE_TUNNEL 0x10

/* The actions that light the rear lights as brakes. */
#define BRAKE_BUTTONS (INPUT_BRAKE | INPUT_EBRAKE)

/* The deepest a side is dented before it dents no more. */
#define MAX_DENT 0x100

/* The display lists of a model that the dents leave alone. */
#define UNDENTED_DISPLAY_LIST VEHICLE_DL_HIGHLIGHTED

/* The parts of the render mode that the vehicles combine. */
#define BLEND_PLAIN GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)
#define BLEND_FOG GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA)
#define FLAGS_FAR                                                                                  \
    (Z_CMP | Z_UPD | CVG_DST_FULL | ALPHA_CVG_SEL |                                                \
     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM))
#define FLAGS_NEAR                                                                                 \
    (AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | ALPHA_CVG_SEL |                                       \
     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM))
#define FLAGS_GHOST                                                                                \
    (AA_EN | Z_CMP | IM_RD | CVG_DST_WRAP | CLR_ON_CVG | ZMODE_XLU | FORCE_BL |                    \
     GBL_c2(G_BL_CLR_IN, G_BL_A_FOG, G_BL_CLR_MEM, G_BL_1MA))
#define FLAGS_GHOST_WHEEL                                                                          \
    (Z_CMP | IM_RD | CVG_DST_FULL | ZMODE_XLU | FORCE_BL |                                         \
     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA))

/** Set the colours of the light of the sun and of the ambient light. */
static inline void SetVehicleLightColors(u32 dir, u32 ambient) {
    gSPLightColor(gDisplayListPtr++, LIGHT_1, dir);
    gSPLightColor(gDisplayListPtr++, LIGHT_2, ambient);
}

/** Set up the tiles of the textures of a model: the load tile and the two render tiles. */
static inline void SetModelTiles(void) {
    gDPSetTile(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);
}

/** Turn the textures on at full scale and set up the tiles. */
static inline void SetModelTextures(void) {
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPTileSync(gDisplayListPtr++);
    SetModelTiles();
}

/** Draw a display list of the current model when it has one. */
static inline void DrawModelDisplayList(Gfx *displayList) {
    if (displayList != NULL) {
        gSPDisplayList(gDisplayListPtr++, displayList);
    }
}

/** Draw lit, textured parts of the model with the render mode of the vehicle. */
static inline void DrawLitParts(u32 dir, u32 ambient, Gfx *displayList) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
    gDPSetCombineLERP(gDisplayListPtr++,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      0,
                      0,
                      0,
                      SHADE,
                      0,
                      0,
                      0,
                      COMBINED,
                      0,
                      0,
                      0,
                      COMBINED);
    SetVehicleLightColors(dir, ambient);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    SetModelTiles();
    DrawModelDisplayList(displayList);
}

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

void RenderWheelModels(Vehicle *vehicle) {
    Mtx *block;
    s32 i;

    if (g_pCurrentVehicleModelData->wheelDisplayLists[0][VEHICLE_WHEEL_DL_NORMAL] == NULL) {
        return;
    }
    for (i = 0; i < VEHICLE_WHEELS; i++) {
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
        if (vehicle->ghostMode == VEHICLE_GHOST) {
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
        guMtxCatF(g_afScratchMatrix, vehicle->wheelMatrices[i], g_mtxVehicleModelScaled);
        block = (Mtx *)AllocDisplayListBlock();
        guMtxF2L(g_mtxVehicleModelScaled, block);
        gSPMatrix(gDisplayListPtr++, block, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
        guMtxCatF(g_mtxVehicleModelScaled, g_afViewProjMatrix, g_afMvpMatrix);
        NormalizeLightMatrix(g_afMvpMatrix);
        block = (Mtx *)AllocDisplayListBlock();
        guMtxF2L(g_afMvpMatrix, block);
        gSPForceMatrix(gDisplayListPtr++, PHYSICAL(block));
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

/** The word of the colour of a light, as gSPLightColor() takes it. */
static inline u32 PackLightColor(u32 red, u32 green, u32 blue) {
    return (red << 24) | (green << 16) | (blue << 8);
}

/** The colour of a light dimmed by a divisor, as a byte. */
static inline u32 DimLightPart(u8 part, f32 divisor) {
    return (u8)(u32)(part / divisor);
}

void RenderVehicleModel(Vehicle *vehicle, s32 lodOffset) {
    Vec3f toCamera, toVehicle, eye;
    Gfx **displayLists;
    Hilite *hilite;
    LookAt *reflectLookAt, *hiliteLookAt;
    f32 distance, divisor, length, nudgeX, nudgeZ;
    u32 dirColor, ambientColor, reflection;
    u8 ambient[3];
    u16 *palette;
    s32 lod, inTunnel, near, offset, lit, i;

    inTunnel = FALSE;
    if (!g_adwVehicleInViewRange[vehicle->index]) {
        return;
    }
    distance = Vec3fDistance(&vehicle->frame.position, &g_pActiveCamera->position);
    if (vehicle->ghostMode == VEHICLE_GHOST) {
        g_dwVehicleFogAlpha = (s32)(vehicle->opacity * 255.0f);
        gDPSetFogColor(gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, g_dwVehicleFogAlpha);
    }
    vehicle->inTunnel = FALSE;
    if (vehicle->groundInstanceCount &&
        (g_LoadedTrack.instances[vehicle->groundInstances[0]].flags & TRACK_INSTANCE_TUNNEL)) {
        vehicle->inTunnel = TRUE;
        inTunnel = TRUE;
    }
    g_pCurrentVehicleModelData = vehicle->model;

    /* With the screen split in two, the far cars are drawn coarser. */
    if (g_dwNumViewports == 2) {
        if (distance < SPLIT_LOD1_DISTANCE) {
            lod = 0;
        } else if (distance < SPLIT_LOD2_DISTANCE) {
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
    lod += lodOffset;
    if (lod > COARSEST_LOD) {
        lod = COARSEST_LOD;
    }

    guScaleF(g_afScratchMatrix, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f);
    guMtxCatF(g_afScratchMatrix, vehicle->matrix, g_mtxVehicleModelScaled);
    g_apVehicleModelMatrix[vehicle->index] = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_mtxVehicleModelScaled, g_apVehicleModelMatrix[vehicle->index]);
    guMtxCatF(g_mtxVehicleModelScaled, g_afViewProjMatrix, g_afMvpMatrix);
    NormalizeLightMatrix(g_afMvpMatrix);
    g_apVehicleLightMatrix[vehicle->index] = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afMvpMatrix, g_apVehicleLightMatrix[vehicle->index]);
    if (vehicle == g_pActiveCameraVehicle &&
        (vehicle->activeCamera == &vehicle->cameras[VEHICLE_CAMERA_COCKPIT] ||
         vehicle->activeCamera == &vehicle->cameras[VEHICLE_CAMERA_MIRROR]) &&
        !g_dwVehicleCullingDisabled) {
        return;
    }
    g_anVehicleLod[vehicle->index] = lod;

    /* The light of the sun dims with the distance at night, and is gone in a tunnel. The ambient
     * colour is also stored in a local that is never read again. */
    if (g_dwNightMode) {
        divisor = distance / NIGHT_LIGHT_DISTANCE;
        if (divisor < 1.0f) {
            divisor = 1.0f;
        }
        dirColor = PackLightColor(DimLightPart(g_bDirLightR, divisor),
                                  DimLightPart(g_bDirLightG, divisor),
                                  DimLightPart(g_bDirLightB, divisor));
        ambient[0] = g_bAmbientLightR;
        ambient[1] = g_bAmbientLightG;
        ambient[2] = g_bAmbientLightB;
        ambientColor = PackLightColor(ambient[0], ambient[1], ambient[2]);
    } else if (inTunnel) {
        dirColor = PackLightColor(0, 0, 0);
        ambient[0] = g_bAmbientLightR * 4 / 5;
        ambient[1] = g_bAmbientLightG * 4 / 5;
        ambient[2] = g_bAmbientLightB * 4 / 5;
        ambientColor = PackLightColor(ambient[0], ambient[1], ambient[2]);
    } else {
        dirColor = PackLightColor(g_bDirLightR, g_bDirLightG, g_bDirLightB);
        ambient[0] = g_bAmbientLightR;
        ambient[1] = g_bAmbientLightG;
        ambient[2] = g_bAmbientLightB;
        ambientColor = PackLightColor(ambient[0], ambient[1], ambient[2]);
    }
    gSPMatrix(gDisplayListPtr++,
              g_apVehicleModelMatrix[vehicle->index],
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

    /* At night the light comes from the camera; the highlight sits halfway to the camera. */
    if (g_dwNightMode) {
        if (g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
            Vec3fNegate(&g_vecVehicleLightDir, &g_pActiveCamera->forward);
        } else {
            Vec3fNegate(&g_vecVehicleLightDir, &g_pActiveCameraVehicle->frame.forward);
        }
    } else {
        g_vecVehicleLightDir.x = g_afLightDirection[0];
        g_vecVehicleLightDir.y = g_afLightDirection[1];
        g_vecVehicleLightDir.z = g_afLightDirection[2];
    }
    Vec3fNormalize(&g_vecVehicleLightDir);
    g_vecVehicleHiliteDir = g_vecVehicleLightDir;
    Vec3fSubtract(&toCamera, &g_pActiveCamera->position, &vehicle->frame.position);
    length = Vec3fLength(&toCamera);
    if (length == 0.0f) {
        Vec3fNegate(&toCamera, &g_pActiveCamera->forward);
    } else {
        Vec3fDivideInPlace(&toCamera, length);
    }
    Vec3fMidpoint(&g_vecVehicleHiliteDir, &toCamera, &g_vecVehicleHiliteDir);
    length = Vec3fLength(&g_vecVehicleHiliteDir);
    if (length == 0.0f) {
        g_vecVehicleHiliteDir = g_pActiveCamera->up;
    } else {
        Vec3fDivideInPlace(&g_vecVehicleHiliteDir, length);
    }

    /* The block is taken and left unused. */
    AllocDisplayListBlock();
    hilite = AllocHiliteBlock();
    reflectLookAt = AllocLightBlock();
    hiliteLookAt = AllocLightBlock();
    /* A camera exactly at the vehicle looks at a point nudged off it. */
    nudgeZ = 0.0f;
    nudgeX = 0.0f;
    if (g_pActiveCamera->position.x == vehicle->frame.position.x &&
        g_pActiveCamera->position.y == vehicle->frame.position.y) {
        if (g_pActiveCamera->position.z == vehicle->frame.position.z) {
            nudgeZ = 1.0f;
        } else {
            nudgeX = 0.1f;
        }
    }
    /* The original copies the whole direction, and its z is not read. */
    eye = g_pActiveCamera->forward;
    if (eye.x == 0.0f && eye.y == 0.0f) {
        eye.x = 1e-4f;
    }
    guLookAtReflectF(
        g_afMvpMatrix, reflectLookAt, eye.x, eye.y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    guLookAtHiliteF(g_afMvpMatrix,
                    hiliteLookAt,
                    hilite,
                    g_pActiveCamera->position.x,
                    g_pActiveCamera->position.y,
                    g_pActiveCamera->position.z,
                    vehicle->frame.position.x + nudgeX,
                    vehicle->frame.position.y,
                    vehicle->frame.position.z + nudgeZ,
                    0.0f,
                    0.0f,
                    1.0f,
                    g_vecVehicleHiliteDir.x,
                    g_vecVehicleHiliteDir.y,
                    g_vecVehicleHiliteDir.z,
                    g_vecVehicleHiliteDir.x,
                    g_vecVehicleHiliteDir.y,
                    g_vecVehicleHiliteDir.z,
                    HILITE_SIZE,
                    HILITE_SIZE);
    if (NEAR_CLIP_DISTANCE < distance) {
        gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[FAR_CLIP_RATIO - 1]);
    } else {
        gSPDisplayList(gDisplayListPtr++, g_aClipRatioDisplayLists[0]);
    }

    /* At night and in the rain, the light points back along the car of the camera. */
    if (g_dwNightMode || g_dwRainMode) {
        g_aVehicleLights[vehicle->index] = g_DefaultLights;
        g_aVehicleLights[vehicle->index].l[0].l.dir[0] =
            -(s32)(g_pActiveCameraVehicle->frame.forward.x * HEADLIGHT_DIRECTION);
        g_aVehicleLights[vehicle->index].l[0].l.dir[1] =
            -(s32)(g_pActiveCameraVehicle->frame.forward.y * HEADLIGHT_DIRECTION);
        g_aVehicleLights[vehicle->index].l[0].l.dir[2] =
            -(s32)(g_pActiveCameraVehicle->frame.forward.z * HEADLIGHT_DIRECTION);
        gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
        gSPLight(gDisplayListPtr++, &g_aVehicleLights[vehicle->index].l[0], 1);
        gSPLight(gDisplayListPtr++, &g_aVehicleLights[vehicle->index].a, 2);
    } else {
        gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
        gSPLight(gDisplayListPtr++, &g_DefaultLights.l[0], 1);
        gSPLight(gDisplayListPtr++, &g_DefaultLights.a, 2);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_LOD);
    gSPSetGeometryMode(gDisplayListPtr++, G_ZBUFFER | G_SHADE | G_LIGHTING | G_SHADING_SMOOTH);
    if (g_dwWeatherActive) {
        gSPSetGeometryMode(gDisplayListPtr++, G_FOG);
        g_dwVehicleOtherModeLBlend = vehicle->ghostMode == VEHICLE_GHOST ? BLEND_PLAIN : BLEND_FOG;
    } else {
        g_dwVehicleOtherModeLBlend = BLEND_PLAIN;
    }
    gSPSetGeometryMode(gDisplayListPtr++,
                       g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_FRONT : G_CULL_BACK);
    gSPClearGeometryMode(gDisplayListPtr++,
                         g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_BACK : G_CULL_FRONT);
    if (vehicle->ghostMode == VEHICLE_GHOST) {
        g_dwVehicleOtherModeLFlags = FLAGS_GHOST;
    } else if (distance < ANTIALIAS_DISTANCE) {
        g_dwVehicleOtherModeLFlags =
            vehicle == g_pActiveCameraVehicle ? FLAGS_NEAR | IM_RD : FLAGS_NEAR;
    } else {
        g_dwVehicleOtherModeLFlags = FLAGS_FAR;
    }
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
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
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    SetVehicleLightColors(dirColor, ambientColor);
    if (vehicle->ghostMode == VEHICLE_GHOST) {
        RenderWheelModels(vehicle);
    }
    /* The matrix of the lighting is forced from its address as the CPU sees it, unlike the wheels.
     */
    gSPForceMatrix(gDisplayListPtr++, g_apVehicleLightMatrix[vehicle->index]);
    gDPLoadTLUT_pal16(gDisplayListPtr++, SHADE_PALETTE, g_awVehicleShadePalette);
    gSPLookAt(gDisplayListPtr++, hiliteLookAt);

    displayLists = g_pCurrentVehicleModelData->displayLists[lod];
    if (!g_bForceVehicleAltVariant && !vehicle->otherLook) {
        SetModelTextures();
        gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
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
                          0,
                          0,
                          0,
                          COMBINED,
                          0,
                          0,
                          0,
                          COMBINED);
        gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN);
        SetVehicleLightColors(dirColor, ambientColor);
        gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
        DrawModelDisplayList(displayLists[VEHICLE_DL_TEXTURED]);
        SetModelTextures();
    }

    /* The palettes of the lights: lit at night and in rain, braking, and reversing. */
    lit = g_dwNightMode || g_dwRainMode;
    palette =
        g_pCurrentVehicleModelData
            ->textures[g_pCurrentVehicleModelData->materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]]
            .palette;
    if (palette != NULL) {
        for (i = HEADLIGHT_FIRST_ENTRY; i < HEADLIGHT_FIRST_ENTRY + HEADLIGHT_ENTRIES; i++) {
            if (lit) {
                palette[i] &= ~1;
            } else {
                palette[i] |= 1;
            }
        }
    }
    palette =
        g_pCurrentVehicleModelData
            ->textures[g_pCurrentVehicleModelData->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]]
            .palette;
    if (palette != NULL) {
        if (lit) {
            palette[15] = 0x7000;
            palette[10] = 0x9082;
        } else {
            palette[15] = 0x9001;
            palette[10] = 0xA001;
        }
        if (vehicle->input->buttons & BRAKE_BUTTONS) {
            palette[14] = 0xC000;
            palette[13] = 0xC000;
            palette[9] = 0xF904;
            palette[8] = 0xF904;
        } else {
            palette[14] = 0x9001;
            palette[9] = 0xA001;
            palette[13] = palette[15];
            palette[8] = palette[10];
        }
        palette[12] = 0x7981;
        palette[7] = 0x9241;
        if (vehicle->throttle < 0.0f) {
            palette[11] = 0xB5AC;
            palette[6] = 0xFFFE;
        } else {
            palette[11] = 0xAD6B;
            palette[6] = 0xC631;
        }
    }
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_LOD);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    SetModelTiles();

    /* The underside shows only from below the car. */
    Vec3fSubtract(&toVehicle, &vehicle->frame.position, &g_pActiveCamera->position);
    if (Vec3fDot(&vehicle->frame.up, &toVehicle) > 0.0) {
        DrawTexturedTile(UNDERSIDE_TEXTURE, g_pCurrentVehicleModelData->textures);
        DrawLitParts(dirColor, ambientColor, displayLists[VEHICLE_DL_UNDERSIDE]);
    }

    if (g_pActiveCameraVehicle->input->playbackBuffer != NULL &&
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
        near = distance < NEAR_REPLAY_DISTANCE;
    } else {
        near = distance < NEAR_DISTANCE;
    }
    if (!g_dwViewportMirrored && near && vehicle->ghostMode != VEHICLE_GHOST && lod < 1) {
        DrawTexturedTile(NEAR_DETAIL_TEXTURE, g_pCurrentVehicleModelData->textures);
        DrawLitParts(dirColor, ambientColor, displayLists[VEHICLE_DL_NEAR_DETAIL]);
    }

    /* The specular highlight, in a single view by day, or in a tunnel only at night. */
    if (g_dwNumViewports == 1 && !g_bPaintShopPreviewActive && !g_dwViewportMirrored &&
        !g_dwSnowMode && !g_dwRainMode && (!inTunnel || g_dwNightMode) && near &&
        !(g_dwNightMode && vehicle == g_pActiveCameraVehicle) && !g_bForceVehicleAltVariant &&
        !vehicle->otherLook) {
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
        gSPSetGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN);
        gSPTexture(gDisplayListPtr++, 0x0F80, 0x0F80, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
        gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0xFF, 0xFF, 0xCC, 0xFF);
        gDPSetCombineLERP(gDisplayListPtr++,
                          0,
                          0,
                          0,
                          PRIMITIVE,
                          0,
                          0,
                          0,
                          TEXEL0,
                          0,
                          0,
                          0,
                          PRIMITIVE,
                          0,
                          0,
                          0,
                          TEXEL0);
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_DEC | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
        gDPTileSync(gDisplayListPtr++);
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
        gDPLoadTextureBlock_4b(gDisplayListPtr++,
                               g_dwNightMode ? g_aVehicleNightHighlightTexture :
                                               g_aVehicleHighlightTexture,
                               G_IM_FMT_I,
                               HILITE_SIZE,
                               HILITE_SIZE,
                               0,
                               G_TX_WRAP,
                               G_TX_WRAP,
                               6,
                               6,
                               G_TX_NOLOD,
                               G_TX_NOLOD);
        gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
        gDPSetHilite1Tile(gDisplayListPtr++, G_TX_RENDERTILE, hilite, HILITE_SIZE, HILITE_SIZE);
        DrawModelDisplayList(displayLists[VEHICLE_DL_HIGHLIGHTED]);
        gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    }

    /* The paint reflects the sky, which turns with the heading of the camera. */
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gSPSetGeometryMode(gDisplayListPtr++,
                       (g_bVehicleEnvMapLinearTexGen ? G_TEXTURE_GEN_LINEAR : 0) | G_TEXTURE_GEN);
    gSPTexture(gDisplayListPtr++, 0x0800, 0x1000, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetCombineLERP(gDisplayListPtr++,
                      TEXEL0,
                      ENVIRONMENT,
                      ENV_ALPHA,
                      ENVIRONMENT,
                      0,
                      0,
                      0,
                      TEXEL0,
                      TEXEL0,
                      ENVIRONMENT,
                      ENV_ALPHA,
                      ENVIRONMENT,
                      0,
                      0,
                      0,
                      TEXEL0);
    if (g_dwWeatherActive) {
        if (inTunnel) {
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
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    reflection = g_abVehicleReflectionTexSelect[g_dwVehicleViewModeIndex][vehicle->inTunnel];
    gDPLoadTextureBlock_4b(gDisplayListPtr++,
                           g_apVehicleReflectionTextures[reflection],
                           G_IM_FMT_CI,
                           32,
                           128,
                           0,
                           G_TX_WRAP,
                           G_TX_WRAP,
                           5,
                           7,
                           15,
                           15);
    gDPLoadSync(gDisplayListPtr++);
    gDPLoadTLUT_pal16(gDisplayListPtr++, 0, g_apVehicleReflectionTluts[reflection]);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_RGBA16);
    offset = 31 - (64 - (s32)(g_pActiveCameraVehicle->cameraHeading * (f32)64 * 0.3183098733f));
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, offset + 2, 2, offset + 126, 510);
    gDPPipeSync(gDisplayListPtr++);
    SetVehicleLightColors(0, 0);
    gSPLookAt(gDisplayListPtr++, reflectLookAt);
    DrawModelDisplayList(displayLists[VEHICLE_DL_BODY]);
    if (g_bForceVehicleAltVariant || vehicle->otherLook) {
        DrawModelDisplayList(displayLists[VEHICLE_DL_HIGHLIGHTED]);
    }
    gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN);
    SetVehicleLightColors(dirColor, ambientColor);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    if (vehicle->ghostMode != VEHICLE_GHOST) {
        RenderWheelModels(vehicle);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
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
    g_dwStatVehicleTriangles += g_pCurrentVehicleModelData->triangleCount;
}

void RenderVehicleHeadlightBeams(Vehicle *vehicle) {
    Vec3f toCamera, beam;
    f32 distance, along, glare;

    if (!g_dwNightMode && !g_dwRainMode) {
        return;
    }
    if (!g_adwVehicleVisible[vehicle->index]) {
        return;
    }
    if (vehicle == g_pActiveCameraVehicle &&
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) {
        return;
    }
    if (vehicle->ghostMode == VEHICLE_GHOST) {
        return;
    }
    g_pCurrentVehicleModelData = vehicle->model;
    gSPMatrix(gDisplayListPtr++,
              g_apVehicleModelMatrix[vehicle->index],
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPForceMatrix(gDisplayListPtr++, g_apVehicleLightMatrix[vehicle->index]);
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
        Vec3fAdd(&toCamera, &vehicle->frame.position, &vehicle->frame.forward);
        Vec3fSubtract(&toCamera, &g_pActiveCamera->position, &toCamera);
        distance = Vec3fLength(&toCamera);
        if (distance != 0.0f) {
            Vec3fDivideInPlace(&toCamera, distance);
            if (Vec3fDot(&toCamera, &g_pActiveCamera->forward) < 0.0f) {
                Vec3fScale(&beam, &vehicle->frame.forward, 1.0f);
                /* The beam is tilted by the up axis times zero, which leaves it as it is. */
                Vec3fMulAddInPlace(&beam, &vehicle->frame.up, -0.0f);
                along = Vec3fDot(&toCamera, &beam);
                glare = -Vec3fDot(&toCamera, &g_pActiveCamera->forward) * along;
                if (0.95f < glare) {
                    g_fHeadlightGlare += 750.0f * (glare - 0.95f) / (distance * distance);
                }
            }
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

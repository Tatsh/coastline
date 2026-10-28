#include <PR/gu.h>

#include "camera.h"
#include "input.h"
#include "modeldisplaylist.h"
#include "render.h"
#include "system.h"
#include "track.h"
#include "util.h"
#include "vehicleaccess.h"
#include "vehiclerender.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/render.h"
#include "platform/win/renderbackend.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
namespace {
    /**
     * Whether every vehicle is drawn in its other look, as Vehicle.otherLook draws one.
     *
     * @ghidraAddress USA: 0x8028ddc8
     * @ghidraAddress EUR: 0x80290568
     * @ghidraAddress JPN: 0x80290008
     * @ghidraAddress BRD3D: 0x10277b1c
     */
    s32 g_bForceVehicleAltVariant;
    /**
     * The least detailed level the vehicles are drawn at, from 0.
     *
     * @ghidraAddress USA: 0x8028aa9c
     * @ghidraAddress EUR: 0x8028cca4
     * @ghidraAddress JPN: 0x8028c774
     * @ghidraAddress BRD3D: 0x106c6630
     */
    s32 g_dwVehicleLodBias;
    /**
     * @ghidraAddress USA: 0x8028c304
     * @ghidraAddress EUR: 0x8028ea64
     * @ghidraAddress JPN: 0x8028e4b4
     * @ghidraAddress BRD3D: 0x10277e58
     */
    u32 g_dwVehicleOtherModeLFlags;
} // namespace
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x10277e14 */
s32 g_dwVehicleFogAlpha;
/** @ghidraAddress BRD3D: 0x10277e5c */
u32 g_dwVehicleOtherModeLBlend;
/** @ghidraAddress BRD3D: 0x10277ea0 */
s32 g_dwVehicleViewModeIndex;
/** @ghidraAddress BRD3D: 0x106c3308 */
VehicleModel *g_pCurrentVehicleModelData;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

static inline void BindVehicleTextureEntry(u32 entry) {
    gDisplayListPtr->words.w0 = 0xdc000000 | (entry & 0xffffff);
    gDisplayListPtr->words.w1 = 1;
    gDisplayListPtr++;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

static inline s32 HasVehicleAltVariant(Vehicle *vehicle) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return VEHICLE_GET(vehicle, CustomCarModel);
#else
    return vehicle->otherLook;
#endif
}

static inline s32 VehicleRenderEqual(f32 a, f32 b) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return !(a < b || a > b);
#else
    return a == b;
#endif
}

static inline s8 VehicleHeadlightDirection(f32 direction) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return (s8)(s64)((long double)direction * -120.0f);
#else
    return -(s32)(direction * 120.0f);
#endif
}

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

/* The levels of detail of a car, the coarsest last. */
#define COARSEST_LOD (VEHICLE_LODS - 1)

/* The texture whose tiles the underside and the near details use. */
#define UNDERSIDE_TEXTURE 6
#define NEAR_DETAIL_TEXTURE 3

/* The palette of the textured parts. */
#define SHADE_PALETTE 14

/* The palette entries changed to light the headlights of a car. */
#define HEADLIGHT_FIRST_ENTRY 12
#define HEADLIGHT_ENTRIES 4

/* The size of the texture of the specular highlight. */
#define HILITE_SIZE 64

/* The placed model under a vehicle that is a tunnel. */
#define TRACK_INSTANCE_TUNNEL 0x10

/* The actions that light the rear lights as brakes. */
#define BRAKE_BUTTONS (INPUT_BRAKE | INPUT_EBRAKE)

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
/** Draw lit, textured parts of the model with the render mode of the vehicle. */
static inline void
DrawLitParts(u32 dir, u32 ambient, DisplayListCommand *displayList, s32 underside) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (underside) {
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
    } else
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    {
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
    }
    SetVehicleLightColors(dir, ambient);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    SetModelTiles();
    DrawModelDisplayList(displayList);
}

/** The word of the colour of a light, as gSPLightColor() takes it. */
static inline u32 PackLightColor(u32 red, u32 green, u32 blue) {
    return (red << 24) | (green << 16) | (blue << 8);
}

/** The colour of a light dimmed by a divisor, as a byte. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
static inline u32 DimLightPart(u8 part, long double divisor) {
#else
static inline u32 DimLightPart(u8 part, f32 divisor) {
#endif
    return (u8)(u32)(part / divisor);
}

void RenderVehicleModel(Vehicle *vehicle, s32 lodOffset) {
    Vec3f toCamera, toVehicle, eye;
    DisplayListCommand **displayLists;
    Hilite *hilite;
    LookAt *reflectLookAt, *hiliteLookAt;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    volatile f32 distance, length;
    long double rawDistance, rawLength, divisor;
    f32 nudgeX, nudgeZ;
#else
    f32 distance, divisor, length, nudgeX, nudgeZ;
#endif
    u32 dirColor, ambientColor, reflection;
    u8 ambient[3];
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    u16 *palette;
    s32 closeRange, lit, i;
#endif
    s32 lod, inTunnel;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u32 offset;
#else
    s32 offset;
#endif

    inTunnel = FALSE;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (VEHICLE_GET(vehicle, Driver) == NULL || VEHICLE_GET(vehicle, Body).wheels[0] == NULL ||
        VEHICLE_GET(vehicle, Body).wheels[2] == NULL ||
        VEHICLE_GET(vehicle, Body).wheels[1] == NULL ||
        VEHICLE_GET(vehicle, Body).wheels[3] == NULL) {
        return;
    }
#endif
    if (!g_adwVehicleInViewRange[VEHICLE_GET(vehicle, Index)]) {
        return;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    rawDistance = Vec3fDistance(&VEHICLE_GET(vehicle, Frame).position, &g_pActiveCamera->position);
    distance = rawDistance;
#else
    distance = Vec3fDistance(&VEHICLE_GET(vehicle, Frame).position, &g_pActiveCamera->position);
#endif
    if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        g_dwVehicleFogAlpha = (s32)(s64)((long double)VEHICLE_GET(vehicle, Opacity) * 255.0f);
#else
        g_dwVehicleFogAlpha = (s32)(vehicle->opacity * 255.0f);
#endif
        gDPSetFogColor(gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, g_dwVehicleFogAlpha);
    }
    VEHICLE_SET(vehicle, InTunnel, FALSE);
    if (VEHICLE_GET(vehicle, GroundInstanceCount) &&
        (g_LoadedTrack.instances[VEHICLE_GET(vehicle, GroundInstances)[0]].flags &
         TRACK_INSTANCE_TUNNEL)) {
        VEHICLE_SET(vehicle, InTunnel, TRUE);
        inTunnel = TRUE;
    }
    g_pCurrentVehicleModelData = VEHICLE_GET(vehicle, Model);

    /* With the screen split in two, the far cars are drawn coarser. */
    if (g_dwNumViewports == 2) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (!(rawDistance >= SPLIT_LOD1_DISTANCE)) {
#else
        if (distance < SPLIT_LOD1_DISTANCE) {
#endif
            lod = 0;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        } else if (!(rawDistance >= SPLIT_LOD2_DISTANCE)) {
#else
        } else if (distance < SPLIT_LOD2_DISTANCE) {
#endif
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
    guMtxCatF(g_afScratchMatrix, VEHICLE_GET(vehicle, Matrix), g_mtxVehicleModelScaled);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_apVehicleModelMatrix[VEHICLE_GET(vehicle, Index)] = AllocDisplayListBlock()->matrix;
    CopyMatrix4x4(g_mtxVehicleModelScaled, g_apVehicleModelMatrix[VEHICLE_GET(vehicle, Index)]);
#else
    g_apVehicleModelMatrix[vehicle->index] = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_mtxVehicleModelScaled, g_apVehicleModelMatrix[vehicle->index]);
#endif
    guMtxCatF(g_mtxVehicleModelScaled, g_afViewProjMatrix, g_afMvpMatrix);
    NormalizeLightMatrix(g_afMvpMatrix);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_apVehicleLightMatrix[VEHICLE_GET(vehicle, Index)] = AllocDisplayListBlock()->matrix;
    CopyMatrix4x4(g_afMvpMatrix, g_apVehicleLightMatrix[VEHICLE_GET(vehicle, Index)]);
#else
    g_apVehicleLightMatrix[vehicle->index] = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afMvpMatrix, g_apVehicleLightMatrix[vehicle->index]);
#endif
    if (vehicle == g_pActiveCameraVehicle &&
        (VEHICLE_GET(vehicle, ActiveCamera) ==
             &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_COCKPIT] ||
         VEHICLE_GET(vehicle, ActiveCamera) ==
             &VEHICLE_GET(vehicle, Cameras)[VEHICLE_CAMERA_MIRROR]) &&
        !g_dwVehicleCullingDisabled) {
        return;
    }
    g_anVehicleLod[VEHICLE_GET(vehicle, Index)] = lod;

    /* The light of the sun dims with the distance at night, and is gone in a tunnel. The ambient
     * colour is also stored in a local that is never read again. */
    if (g_dwNightMode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        divisor = (long double)distance * 0.1f;
        if (!(divisor >= 1.0f)) {
#else
        divisor = distance / NIGHT_LIGHT_DISTANCE;
        if (divisor < 1.0f) {
#endif
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
              g_apVehicleModelMatrix[VEHICLE_GET(vehicle, Index)],
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPMatrix(gDisplayListPtr++, g_pProjectionMtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);

    /* At night the light comes from the camera; the highlight sits halfway to the camera. */
    if (g_dwNightMode) {
        if (g_pActiveCamera ==
            &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE]) {
            Vec3fNegate(&g_vecVehicleLightDir, &g_pActiveCamera->forward);
        } else {
            Vec3fNegate(&g_vecVehicleLightDir, &VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward);
        }
    } else {
        g_vecVehicleLightDir.x = g_afLightDirection[0];
        g_vecVehicleLightDir.y = g_afLightDirection[1];
        g_vecVehicleLightDir.z = g_afLightDirection[2];
    }
    Vec3fNormalize(&g_vecVehicleLightDir);
    g_vecVehicleHiliteDir = g_vecVehicleLightDir;
    Vec3fSubtract(&toCamera, &g_pActiveCamera->position, &VEHICLE_GET(vehicle, Frame).position);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    rawLength = Vec3fLength(&toCamera);
    length = rawLength;
    if (!(rawLength < 0.0L || rawLength > 0.0L)) {
#else
    length = Vec3fLength(&toCamera);
    if (length == 0.0f) {
#endif
        Vec3fNegate(&toCamera, &g_pActiveCamera->forward);
    } else {
        Vec3fDivideInPlace(&toCamera, length);
    }
    Vec3fMidpoint(&g_vecVehicleHiliteDir, &toCamera, &g_vecVehicleHiliteDir);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    rawLength = Vec3fLength(&g_vecVehicleHiliteDir);
    length = rawLength;
    if (!(rawLength < 0.0L || rawLength > 0.0L)) {
#else
    length = Vec3fLength(&g_vecVehicleHiliteDir);
    if (length == 0.0f) {
#endif
        g_vecVehicleHiliteDir = g_pActiveCamera->up;
    } else {
        Vec3fDivideInPlace(&g_vecVehicleHiliteDir, length);
    }

    /* The allocation result is discarded. */
    AllocDisplayListBlock();
    hilite = AllocHiliteBlock();
    reflectLookAt = AllocLightBlock();
    hiliteLookAt = AllocLightBlock();
    /* A camera exactly at the vehicle looks at a point nudged off it. */
    nudgeZ = 0.0f;
    nudgeX = 0.0f;
    if (VehicleRenderEqual(g_pActiveCamera->position.x, VEHICLE_GET(vehicle, Frame).position.x) &&
        VehicleRenderEqual(g_pActiveCamera->position.y, VEHICLE_GET(vehicle, Frame).position.y)) {
        if (VehicleRenderEqual(g_pActiveCamera->position.z,
                               VEHICLE_GET(vehicle, Frame).position.z)) {
            nudgeZ = 1.0f;
        } else {
            nudgeX = 0.1f;
        }
    }
    eye = g_pActiveCamera->forward;
    if (VehicleRenderEqual(eye.x, 0.0f) && VehicleRenderEqual(eye.y, 0.0f)) {
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
                    VEHICLE_GET(vehicle, Frame).position.x + nudgeX,
                    VEHICLE_GET(vehicle, Frame).position.y,
                    VEHICLE_GET(vehicle, Frame).position.z + nudgeZ,
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
        g_aVehicleLights[VEHICLE_GET(vehicle, Index)] = g_DefaultLights;
        g_aVehicleLights[VEHICLE_GET(vehicle, Index)].l[0].l.dir[0] =
            VehicleHeadlightDirection(VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward.x);
        g_aVehicleLights[VEHICLE_GET(vehicle, Index)].l[0].l.dir[1] =
            VehicleHeadlightDirection(VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward.y);
        g_aVehicleLights[VEHICLE_GET(vehicle, Index)].l[0].l.dir[2] =
            VehicleHeadlightDirection(VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward.z);
        gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
        gSPLight(gDisplayListPtr++, &g_aVehicleLights[VEHICLE_GET(vehicle, Index)].l[0], 1);
        gSPLight(gDisplayListPtr++, &g_aVehicleLights[VEHICLE_GET(vehicle, Index)].a, 2);
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
        g_dwVehicleOtherModeLBlend =
            VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST ? BLEND_PLAIN : BLEND_FOG;
    } else {
        g_dwVehicleOtherModeLBlend = BLEND_PLAIN;
    }
    gSPSetGeometryMode(gDisplayListPtr++,
                       g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_FRONT : G_CULL_BACK);
    gSPClearGeometryMode(gDisplayListPtr++,
                         g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_BACK : G_CULL_FRONT);
    if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
        g_dwVehicleOtherModeLFlags = FLAGS_GHOST;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        gDPSetPrimColor(gDisplayListPtr++, 0, 0, 0, 0, 0, g_dwVehicleFogAlpha);
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    } else if (!(rawDistance >= ANTIALIAS_DISTANCE)) {
#else
    } else if (distance < ANTIALIAS_DISTANCE) {
#endif
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
    if (VEHICLE_GET(vehicle, GhostMode) == VEHICLE_GHOST) {
        RenderWheelModels(vehicle);
    }
    /* The matrix of the lighting is forced from its address as the CPU sees it, unlike the wheels.
     */
    gSPForceMatrix(gDisplayListPtr++, g_apVehicleLightMatrix[VEHICLE_GET(vehicle, Index)]);
    gDPLoadTLUT_pal16(gDisplayListPtr++, SHADE_PALETTE, g_awVehicleShadePalette);
    gSPLookAt(gDisplayListPtr++, hiliteLookAt);

    displayLists = g_pCurrentVehicleModelData->displayLists[lod];
    if (!g_bForceVehicleAltVariant && !HasVehicleAltVariant(vehicle)) {
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_pCurrentVehicleModelData
                ->textures[g_pCurrentVehicleModelData
                               ->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]]
                .palette != NULL &&
        !g_dwNullStubEnable) {
        s32 lightVariant = (VEHICLE_GET(vehicle, Input)->buttons & BRAKE_BUTTONS) ? 1 : 0;
        if (!(VEHICLE_GET(vehicle, Throttle) >= 0.0f)) {
            lightVariant += 2;
        }
        g_pfnAliasTextureEntrySurface(
            g_pCurrentVehicleModelData->rearLightTextureEntry,
            g_pCurrentVehicleModelData->lightTextureEntries[lightVariant]);
    }
#else
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_LOD);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    SetModelTiles();

    /* The underside shows only from below the car. */
    Vec3fSubtract(&toVehicle, &VEHICLE_GET(vehicle, Frame).position, &g_pActiveCamera->position);
    if (Vec3fDot(&VEHICLE_GET(vehicle, Frame).up, &toVehicle) > 0.0) {
        DrawTexturedTile(UNDERSIDE_TEXTURE, g_pCurrentVehicleModelData->textures);
        DrawLitParts(dirColor, ambientColor, displayLists[VEHICLE_DL_UNDERSIDE], TRUE);
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    DrawTexturedTile(NEAR_DETAIL_TEXTURE, g_pCurrentVehicleModelData->textures);
    DrawLitParts(dirColor, ambientColor, displayLists[VEHICLE_DL_NEAR_DETAIL], FALSE);
#else
    if (g_pActiveCameraVehicle->input->playbackBuffer != NULL &&
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
        closeRange = distance < NEAR_REPLAY_DISTANCE;
    } else {
        closeRange = distance < NEAR_DISTANCE;
    }
    if (!g_dwViewportMirrored && closeRange && vehicle->ghostMode != VEHICLE_GHOST && lod < 1) {
        DrawTexturedTile(NEAR_DETAIL_TEXTURE, g_pCurrentVehicleModelData->textures);
        DrawLitParts(dirColor, ambientColor, displayLists[VEHICLE_DL_NEAR_DETAIL], FALSE);
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

    /* The specular highlight, in a single view by day, or in a tunnel only at night. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_GlobalRallyConfig.bSpecularLightingSelection && !g_dwSnowMode && !g_dwRainMode &&
        (!inTunnel || g_dwNightMode) && !(g_dwNightMode && vehicle == g_pActiveCameraVehicle) &&
        !g_bForceVehicleAltVariant && !HasVehicleAltVariant(vehicle)) {
#else
    if (g_dwNumViewports == 1 && !g_bPaintShopPreviewActive && !g_dwViewportMirrored &&
        !g_dwSnowMode && !g_dwRainMode && (!inTunnel || g_dwNightMode) && closeRange &&
        !(g_dwNightMode && vehicle == g_pActiveCameraVehicle) && !g_bForceVehicleAltVariant &&
        !HasVehicleAltVariant(vehicle)) {
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        gDPSetRenderMode(gDisplayListPtr++, 4, 0);
#else
        gDPSetRenderMode(gDisplayListPtr++,
                         Z_CMP | IM_RD | CVG_DST_SAVE | ZMODE_DEC | FORCE_BL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
#endif
        gDPTileSync(gDisplayListPtr++);
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        BindVehicleTextureEntry(g_dwNightMode ? g_dwVehicleEnvMapTextureId :
                                                g_dwFallbackEnvMapTextureId);
#else
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        gDPSetColorDither(gDisplayListPtr++, G_CD_DISABLE);
        gDPSetHilite1Tile(gDisplayListPtr++, G_TX_RENDERTILE, hilite, HILITE_SIZE, HILITE_SIZE);
        DrawModelDisplayList(displayLists[VEHICLE_DL_HIGHLIGHTED]);
        gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    }

    /* The sky reflection turns with the heading of the camera. */
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    gDPSetRenderMode(gDisplayListPtr++, 0, 0);
#endif
    gDPSetRenderMode(gDisplayListPtr++, g_dwVehicleOtherModeLBlend, g_dwVehicleOtherModeLFlags);
    gDPTileSync(gDisplayListPtr++);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    reflection =
        g_abVehicleReflectionTexSelect[g_dwVehicleViewModeIndex][VEHICLE_GET(vehicle, InTunnel)];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    BindVehicleTextureEntry(g_apVehicleReflectionTextures[reflection].entry);
#else
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_RGBA16);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    offset =
        0xffffffdfu - (u32)(s64)((long double)VEHICLE_GET(g_pActiveCameraVehicle, CameraHeading) *
                                 -20.3718318939209f);
#else
    offset = 31 - (64 - (s32)(g_pActiveCameraVehicle->cameraHeading * (f32)64 * 0.3183098733f));
#endif
    gDPSetTileSize(gDisplayListPtr++, G_TX_RENDERTILE, offset + 2, 2, offset + 126, 510);
    gDPPipeSync(gDisplayListPtr++);
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    SetVehicleLightColors(0, 0);
#endif
    gSPLookAt(gDisplayListPtr++, reflectLookAt);
    DrawModelDisplayList(displayLists[VEHICLE_DL_BODY]);
    if (g_bForceVehicleAltVariant || HasVehicleAltVariant(vehicle)) {
        DrawModelDisplayList(displayLists[VEHICLE_DL_HIGHLIGHTED]);
    }
    gSPPopMatrix(gDisplayListPtr++, G_MTX_MODELVIEW);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN);
    SetVehicleLightColors(dirColor, ambientColor);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    if (VEHICLE_GET(vehicle, GhostMode) != VEHICLE_GHOST) {
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    gDPSetRenderMode(gDisplayListPtr++, 3, 0);
#endif
    g_dwStatVehicleTriangles += g_pCurrentVehicleModelData->triangleCount;
}

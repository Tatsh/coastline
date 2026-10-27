#include "trackrender.h"

#include <PR/gbi.h>
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "audiomix.h"
#include "camera.h"
#include "debugtext.h"
#include "game.h"
#include "input.h"
#include "mathutil.h"
#include "race.h"
#include "render.h"
#include "system.h"
#include "timing.h"
#include "track.h"
#include "trackdisplaylist.h"
#include "trackselect.h"
#include "util.h"
#include "visgrid.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/trackskidmarks.h"
#include "vehiclecount.h"
#define PHYSICAL(address) ((u32)(address))
#define TRACK_TIMING(bar, red, green, blue, alpha) NullStub()
#define TRACK_WEATHER_EXCLUDED 8
#else
/* The physical address of a matrix in the cached segment, which wraps past the top of memory. */
#define PHYSICAL(address) ((u32)(address) + 0x80000000)
#define TRACK_TIMING(bar, red, green, blue, alpha) AddTimingMarker(bar, red, green, blue, alpha)
#define TRACK_WEATHER_EXCLUDED 7
#endif

/* The minimum level of detail of the primitive colour, and the colour itself. */
#define PRIM_MIN_LEVEL 0x17

/* The ranges a fixed point matrix holds. */
#define FIXED_MATRIX_MAX 32767.0f

/* g_nTrackTransparentStartIdx before it is known. */
#define TRANSPARENT_START_UNKNOWN 0x1000

/** The geometry modes of the track, with its culling, shading and fog. */
static inline u32 TrackGeometryMode(void) {
    u32 fog = g_dwWeatherActive ? G_FOG : 0;
    u32 shading = g_bSmoothShading ? G_SHADING_SMOOTH : 0;
    u32 cull = g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_FRONT : G_CULL_BACK;

    return (cull | G_ZBUFFER | G_SHADE | G_LIGHTING | G_TEXTURE_GEN_LINEAR) | shading | fog;
}

/** Load the matrices and the lights of the track. */
static inline void LoadTrackMatricesAndLights(u32 identity) {
    gSPMatrix(gDisplayListPtr++,
              PHYSICAL(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].l[0], 1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].a, 2);
    gSPLightColor(gDisplayListPtr++, LIGHT_1, g_dwDirLightColor);
    gSPLightColor(gDisplayListPtr++, LIGHT_2, g_dwAmbientLightColor);
}

/** Set the modes of the textures of the track. */
static inline void SetTrackTextureModes(void) {
    gDPSetTextureDetail(gDisplayListPtr++, G_TD_CLAMP);
    gDPSetTextureLOD(gDisplayListPtr++, g_bTrackTextureLod ? G_TL_LOD : G_TL_TILE);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
}

/** Set the clipping ratio for the views on the screen. */
static inline void SetTrackClipRatio(void) {
    if (g_dwNumViewports == 1) {
        gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_1);
    } else {
        gSPClipRatio(gDisplayListPtr++, FRUSTRATIO_6);
    }
}

/** Draw a placed model without shadows, and count it. */
static inline void DrawPlainInstance(TrackInstance *instance) {
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
    gDPTileSync(gDisplayListPtr++);
    gSPDisplayList(gDisplayListPtr++, instance->displayList);
    g_dwStatTrianglesDrawn += instance->triangleCount;
    g_dwStatVerticesDrawn += instance->vertexCount;
    g_dwStatPolygonsDrawn += instance->polygonCount;
}

/**
 * Load the product of the model view and the projection of a placed model whose matrix holds only
 * its position and a scale: the view and projection scaled by it, put at its position, and scaled
 * again into the range of fixed point.
 */
static inline void ForceUnrotatedMatrix(TrackInstance *instance, s32 index, s32 listIndex) {
    f32(*vp)[4] = g_afViewProjMatrix;
    f32(*mvp)[4] = g_afMvpMatrix;
    f32(*m)[4] = instance->matrix;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    f32(*block)[4];
    f32 scale, high, low;
#else
    Mtx *block;
    f32 divisor, scale, high, low;
#endif
    s32 row, column;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    for (column = 0; column < 4; column++) {
        mvp[3][column] =
            (f32)(((long double)vp[0][column] * m[3][0] + (long double)vp[3][column] * m[3][3]) +
                  (long double)vp[2][column] * m[3][2] + (long double)vp[1][column] * m[3][1]);
    }
    scale = m[0][0];
#else
    divisor = m[0][0] * g_fViewMatrixMaxAbs + 0.375f;
    if (divisor == 0.0f) {
        divisor = m[0][0] * (1.1 * g_fViewMatrixMaxAbs) + 0.375;
    }
    scale = 1.999755859375f / divisor;
    for (column = 0; column < 4; column++) {
        mvp[3][column] =
            (((m[3][0] * vp[0][column] + m[3][1] * vp[1][column]) + m[3][2] * vp[2][column]) +
             vp[3][column] * m[3][3]) *
            scale;
    }
    scale *= m[0][0];
#endif
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 4; column++) {
            mvp[row][column] = vp[row][column] * scale;
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    block = AllocDisplayListBlock()->matrix;
#else
    block = (Mtx *)AllocDisplayListBlock();
#endif

    high = 0.0f;
    low = 0.0f;
    for (column = 0; column < 4; column++) {
        if (high <= mvp[3][column]) {
            high = mvp[3][column];
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (!(mvp[3][column] > low)) {
#else
        if (mvp[3][column] <= low) {
#endif
            low = mvp[3][column];
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (FIXED_MATRIX_MAX < (f64)high || !((f64)low >= -FIXED_MATRIX_MAX)) {
#else
    if (FIXED_MATRIX_MAX < (f64)high || (f64)low < -FIXED_MATRIX_MAX) {
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION != TGR_REGION_JP
        /* Only the second model in view reports its overflow. */
        if (listIndex == 1) {
            osSyncPrintf("Bad Final Matrix: in=%d (%s), s=%f\n-------------\n"
                         "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n-------------\n"
                         "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n-------------\n"
                         "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n-------------\n",
                         index,
                         g_LoadedTrack.instanceNames != NULL ? g_LoadedTrack.instanceNames[index] :
                                                               "???",
                         scale,
                         m[0][0],
                         m[0][1],
                         m[0][2],
                         m[0][3],
                         m[1][0],
                         m[1][1],
                         m[1][2],
                         m[1][3],
                         m[2][0],
                         m[2][1],
                         m[2][2],
                         m[2][3],
                         m[3][0],
                         m[3][1],
                         m[3][2],
                         m[3][3],
                         vp[0][0],
                         vp[0][1],
                         vp[0][2],
                         vp[0][3],
                         vp[1][0],
                         vp[1][1],
                         vp[1][2],
                         vp[1][3],
                         vp[2][0],
                         vp[2][1],
                         vp[2][2],
                         vp[2][3],
                         vp[3][0],
                         vp[3][1],
                         vp[3][2],
                         vp[3][3],
                         mvp[0][0],
                         mvp[0][1],
                         mvp[0][2],
                         mvp[0][3],
                         mvp[1][0],
                         mvp[1][1],
                         mvp[1][2],
                         mvp[1][3],
                         mvp[2][0],
                         mvp[2][1],
                         mvp[2][2],
                         mvp[2][3],
                         mvp[3][0],
                         mvp[3][1],
                         mvp[3][2],
                         mvp[3][3]);
        }
#endif
        scale = -low < high ? FIXED_MATRIX_MAX / high : -FIXED_MATRIX_MAX / low;
        for (row = 0; row < 4; row++) {
            for (column = 0; column < 4; column++) {
                mvp[row][column] *= scale;
            }
        }
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    CopyMatrix4x4(g_afMvpMatrix, block);
#else
    guMtxF2L(g_afMvpMatrix, block);
#endif
    gSPForceMatrix(gDisplayListPtr++, block);
}

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void RenderTrackGeometry(const ViewportRect *viewports,
                         s32 deferred,
                         const RacerSlot *slots,
                         Vehicle *vehicles) {
#else
void RenderTrackGeometry(s32 deferred) {
#endif
    TrackInstance *instance;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    f32(*block)[4];
#else
    Mtx *block;
#endif
    u32 identity;
    s32 clear, deferredLast, opaqueCount, matrixLoaded, i, index;

    clear = !g_dwNightMode && !g_dwRainMode;
    matrixLoaded = FALSE;
    if (!deferred) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        for (i = 0; i < g_dwNumActiveVehicles; ++i) {
            ConvertedModelVertex *quad = vehicles[i].GetShadowQuad();
            quad[0].texture.x = 1024.0f;
            quad[0].texture.y = 1024.0f;
            quad[1].texture.x = 0.0f;
            quad[1].texture.y = 1024.0f;
            quad[2].texture.x = 1024.0f;
            quad[2].texture.y = 0.0f;
            quad[3].texture.x = 0.0f;
            quad[3].texture.y = 0.0f;
            vehicles[i].GetShadowCornerValid()[3] = 0;
            vehicles[i].GetShadowCornerValid()[2] = 0;
            vehicles[i].GetShadowCornerValid()[1] = 0;
            vehicles[i].GetShadowCornerValid()[0] = 0;
        }
        BuildVisibleInstanceList(viewports, slots, vehicles);
#else
        BuildVisibleInstanceList();
#endif
        g_nDeferredInstanceLastIdx = -1;
        deferredLast = -1;
        g_nOpaqueVisibleInstanceCount = opaqueCount = g_nVisibleInstanceCount;
        g_fViewMatrixMaxAbs = GetMaxAbsValue12(g_afViewProjMatrix);
        g_dwInstanceHideFlags = 0;
        if (clear) {
            g_dwInstanceHideFlags = TRACK_INSTANCE_DARK_OR_WET;
        }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (g_dwGameMode != GAME_MODE_ARCADE && g_dwGameMode != GAME_MODE_NETWORK &&
            (g_dwGameMode != GAME_MODE_AFTER_SEASON || vehicles[0].GetProfile()->season != 0)) {
#else
        if (g_dwGameMode != GAME_MODE_ARCADE &&
            (g_dwGameMode != GAME_MODE_AFTER_SEASON || g_pActiveProfile->season != 0)) {
#endif
            g_dwInstanceHideFlags |= TRACK_INSTANCE_ARCADE;
        }
        if (g_dwWeatherActive && (g_dwSnowMode || g_dwRainMode || g_dwNightMode) &&
            g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != TRACK_WEATHER_EXCLUDED) {
            g_dwInstanceHideFlags |= TRACK_INSTANCE_CLEAR_SKY;
        }
        g_nTrackTransparentStartIdxFar = TRANSPARENT_START_UNKNOWN;
        g_nTrackTransparentStartIdxNear = TRANSPARENT_START_UNKNOWN;
        g_nTrackTransparentStartIdx = TRANSPARENT_START_UNKNOWN;
    }
    TRACK_TIMING(0, 0xFF, 0x80, 0x80, 0xFF);
    /* The blender mixes in the fog in fog, and passes the colour through otherwise. */
    if (g_dwWeatherActive) {
        g_dwTrackOtherModeLBlend = GBL_c1(G_BL_CLR_FOG, G_BL_A_SHADE, G_BL_CLR_IN, G_BL_1MA);
    } else {
        g_dwTrackOtherModeLBlend = GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1);
    }
    g_dwTrackOtherModeLFlags = AA_EN | Z_CMP | Z_UPD | CVG_DST_CLAMP | ZMODE_OPA | ALPHA_CVG_SEL |
                               GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM);

    identity = PHYSICAL(&g_mtxIdentity);
    gSPMatrix(gDisplayListPtr++,
              PHYSICAL(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_PUSH);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    gSPLookAt(gDisplayListPtr++, g_pTrackLookAt);
    gSPNumLights(gDisplayListPtr++, NUMLIGHTS_1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].l[0], 1);
    gSPLight(gDisplayListPtr++, &g_aTrackLights[g_nTrackLightBufferIndex].a, 2);
    gSPLightColor(gDisplayListPtr++, LIGHT_1, g_dwDirLightColor);
    gSPLightColor(gDisplayListPtr++, LIGHT_2, g_dwAmbientLightColor);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
    gDPSetTextureConvert(gDisplayListPtr++, G_TC_FILT);
    gDPSetCombineKey(gDisplayListPtr++, G_CK_NONE);
    gDPSetAlphaCompare(gDisplayListPtr++, G_AC_THRESHOLD);
    gDPSetColorDither(gDisplayListPtr++, g_dwColorDitherMode);
    gDPSetAlphaDither(gDisplayListPtr++, g_dwAlphaDitherMode);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwTrackOtherModeLBlend, g_dwTrackOtherModeLFlags);
    SetTrackCombiner(gDisplayListPtr++);
    SetTrackTextureModes();
    gSPSegment(gDisplayListPtr++, 0, 0);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_CLIPPING | G_TEXTURE_GEN | G_FOG | G_CULL_BACK | G_CULL_FRONT |
                             G_SHADING_SMOOTH);
    gSPSetGeometryMode(gDisplayListPtr++, TrackGeometryMode());
    RestoreCurrentViewport();
    LoadTrackMatricesAndLights(identity);
    /* The fog is set up twice over. */
    SetupFog();
    SetupFog();
    gDPSetBlendColor(gDisplayListPtr++, 0, 0, 0, 0);
    gSPClearGeometryMode(gDisplayListPtr++,
                         G_TEXTURE_GEN | G_FOG | G_CULL_BACK | G_CULL_FRONT | G_SHADING_SMOOTH);
    gSPSetGeometryMode(gDisplayListPtr++, TrackGeometryMode());
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_2CYCLE);
    gDPSetRenderMode(gDisplayListPtr++, g_dwTrackOtherModeLBlend, g_dwTrackOtherModeLFlags);
    SetTrackCombiner(gDisplayListPtr++);
    gDPSetPrimColor(gDisplayListPtr++, PRIM_MIN_LEVEL, 0, 0xFF, 0, 0, 0xFF);
    SetTrackTextureModes();
    SetTrackClipRatio();
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON);
    gSPClearGeometryMode(gDisplayListPtr++, G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPTileSync(gDisplayListPtr++);
    gDPSetTile(
        gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x1F0, 6, 0, 0, 0, 0, 0, 0, 0);
    gDPSetTile(gDisplayListPtr++, G_IM_FMT_RGBA, G_IM_SIZ_4b, 0, 0x100, 5, 0, 0, 0, 0, 0, 0, 0);

    if (deferred) {
        deferredLast = g_nDeferredInstanceLastIdx;
        i = opaqueCount = g_nOpaqueVisibleInstanceCount;
        g_nVisibleInstanceCount = opaqueCount + deferredLast + 1;
    } else {
        StepDebugHiddenInstance();
        i = 0;
    }
    for (; i < g_nVisibleInstanceCount; i++) {
        /* The near and far cells set the clipping again, and from the far ones on it clips. */
        if (i == g_nVisibleCellIdxNear || i == g_nVisibleCellIdxFar) {
            SetTrackClipRatio();
            if (i == g_nVisibleCellIdxFar) {
                gSPSetGeometryMode(gDisplayListPtr++, G_CLIPPING);
            }
        }
        if (i < opaqueCount) {
            index = g_awVisibleInstanceList[i];
            instance = &g_LoadedTrack.instances[index];
            if (instance->groups & g_wActiveInstanceHideMask) {
                continue;
            }
            if (g_bDebugHideInstance && g_nDebugHiddenInstanceIndex == index) {
                continue;
            }
            if (instance->flags & g_dwInstanceHideFlags) {
                continue;
            }
            /* The translucent models wait for the second pass, at the start of the list. */
            if (instance->flags & TRACK_INSTANCE_TRANSLUCENT) {
                if (g_nTrackTransparentStartIdx == TRANSPARENT_START_UNKNOWN &&
                    g_nVisibleCellIdxMid < i) {
                    g_nTrackTransparentStartIdx = deferredLast + 1;
                }
                g_awVisibleInstanceList[++deferredLast] = index;
                continue;
            }
        } else {
            /* The second pass draws the translucent models the farthest first. */
            index = g_awVisibleInstanceList[deferredLast - i + opaqueCount];
            instance = &g_LoadedTrack.instances[index];
        }

        if (instance->flags & TRACK_INSTANCE_UNROTATED) {
            if (!matrixLoaded) {
                gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
                matrixLoaded = TRUE;
            }
            ForceUnrotatedMatrix(instance, index, i);
        } else {
            matrixLoaded = FALSE;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            block = AllocDisplayListBlock()->matrix;
            CopyMatrix4x4(instance->matrix, block);
#else
            block = (Mtx *)AllocDisplayListBlock();
            guMtxF2L(instance->matrix, block);
#endif
            gSPMatrix(
                gDisplayListPtr++, PHYSICAL(block), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        }

        /* The flags are read again after each command written. */
        if (instance->flags & TRACK_INSTANCE_MODE_FLAGS) {
            if (instance->flags & TRACK_INSTANCE_RAMPED) {
                if (g_dwNightMode && (instance->flags & TRACK_INSTANCE_NIGHT_LIT)) {
                    gSPLightColor(
                        gDisplayListPtr++, LIGHT_1, (g_dwDirLightColor >> 1) & 0x7F7F7F00);
                } else {
                    gSPLightColor(gDisplayListPtr++, LIGHT_1, 0);
                }
                gSPLightColor(gDisplayListPtr++,
                              LIGHT_2,
                              g_adwLightRampColors[instance->flags & TRACK_INSTANCE_LIGHT_RAMP]);
            }
            if (instance->flags & TRACK_INSTANCE_TWO_SIDED) {
                gSPClearGeometryMode(gDisplayListPtr++, G_CULL_BACK | G_CULL_FRONT);
            }
            if ((instance->flags & TRACK_INSTANCE_CLEAR_SKY) && g_dwWeatherActive &&
                !g_dwNightMode && !g_dwSnowMode && !g_dwRainMode) {
                gSPClearGeometryMode(gDisplayListPtr++, G_FOG);
            }
            if ((instance->flags & TRACK_INSTANCE_FLAT) && g_bSmoothShading) {
                gSPClearGeometryMode(gDisplayListPtr++, G_SHADING_SMOOTH);
            }
        }

        if (g_nVisibleCellIdxMid < i && i < g_nTrackTransparentStartIdx) {
            DrawPlainInstance(instance);
        } else {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            RenderTrackInstance(
                viewports, index, g_abInstanceVisibilityMask[index], deferred, vehicles);
#else
            RenderTrackInstance(index, g_abInstanceVisibilityMask[index], deferred);
#endif
        }

        if (instance->flags & TRACK_INSTANCE_MODE_FLAGS) {
            if ((instance->flags & TRACK_INSTANCE_FLAT) && g_bSmoothShading) {
                gSPSetGeometryMode(gDisplayListPtr++, G_SHADING_SMOOTH);
            }
            if (instance->flags & TRACK_INSTANCE_RAMPED) {
                gSPLightColor(gDisplayListPtr++, LIGHT_1, g_dwDirLightColor);
                gSPLightColor(gDisplayListPtr++, LIGHT_2, g_dwAmbientLightColor);
            }
            if (instance->flags & TRACK_INSTANCE_TWO_SIDED) {
                gSPSetGeometryMode(gDisplayListPtr++,
                                   g_dwTrackMirrored != g_dwViewportMirrored ? G_CULL_FRONT :
                                                                               G_CULL_BACK);
            }
            if (g_dwWeatherActive && !g_dwNightMode && !g_dwSnowMode && !g_dwRainMode &&
                (instance->flags & TRACK_INSTANCE_CLEAR_SKY)) {
                gSPSetGeometryMode(gDisplayListPtr++, G_FOG);
            }
        }
    }

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (deferred) {
        DrawTrackShadows(vehicles);
    }
#endif
    /* After the first pass, where the second starts the translucent models past the middle. */
    if (!deferred) {
        if (g_nTrackTransparentStartIdx == TRANSPARENT_START_UNKNOWN) {
            g_nTrackTransparentStartIdx = deferredLast;
        }
        g_nTrackTransparentStartIdx = deferredLast - g_nTrackTransparentStartIdx + opaqueCount;
    }
    gSPMatrix(gDisplayListPtr++, identity, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(gDisplayListPtr++, G_FOG);
    gDPPipeSync(gDisplayListPtr++);
    TRACK_TIMING(0, 0, 0xB4, 0, 0xFF);
    g_nDeferredInstanceLastIdx = deferredLast;
}

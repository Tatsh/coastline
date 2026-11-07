#include "common/trackrender.h"

#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/camera.h"
#include "common/debugtext.h"
#include "common/game.h"
#include "common/mathutil.h"
#include "common/race.h"
#include "common/render.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackdisplaylist.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/visgrid.h"
#include "common/weather.h"
#define APPEND_TRACK_COMMAND(a, b)                                                                 \
    {                                                                                              \
        Gfx *command = gDisplayListPtr++;                                                          \
        command->words.w0 = (a);                                                                   \
        command->words.w1 = (u32)(b);                                                              \
    }
#if defined(__GNUC__)
#define PHYSICAL_ADDRESS(pointer) ((u32)(pointer) - 0x80000000)
#else
#define PHYSICAL_ADDRESS(pointer) ((int)((char *)(pointer) - 0x80000000))
#endif
/* The original reaches the MVP matrix through three adjacent matrices. */
#if defined(ENABLE_PATCHES) || !defined(__sgi)
#define TRACK_MVP_MATRIX g_afMvpMatrix
#else
#define TRACK_MVP_MATRIX mm[3]
#endif
void RenderTrackGeometry(int deferred) {
    static float viewMatrixMaxAbs;
    static int instanceHideFlags;
    int clearWeather;
    int opaqueCount;
    int deferredLast;
    int i;
    unsigned short instanceIndex;
    int billboardMatrixLoaded;
#if TGR_REGION == TGR_REGION_JP
    Mtx *matrixBlock;
    int flags;
    TrackInstance *instance;
#else
    int flags;
    TrackInstance *instance;
    Mtx *matrixBlock;
#endif
    /* IDO retains this local in the original stack layout. */
    int j TGR_MATCHING_UNUSED;
    Gfx *command;

    billboardMatrixLoaded = 0;
    clearWeather = g_dwNightMode == 0 && g_dwRainMode == 0;
    if (deferred == 0) {
        BuildVisibleInstanceList();
        opaqueCount = g_nOpaqueVisibleInstanceCount = g_nVisibleInstanceCount;
        deferredLast = g_nDeferredInstanceLastIdx = -1;
        viewMatrixMaxAbs = GetMaxAbsValue12(g_afViewProjMatrix);
        instanceHideFlags = 0;
        if (clearWeather) {
            instanceHideFlags = 0x800;
        }
        if (g_dwGameMode != 1 && (g_dwGameMode != 5 || g_pActiveProfile->season != 0)) {
            instanceHideFlags |= 0x4000;
        }
        if (g_dwWeatherActive != 0 &&
            (g_dwSnowMode != 0 || g_dwRainMode != 0 || g_dwNightMode != 0) &&
            g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
            instanceHideFlags |= 0x20;
        }
        g_nTrackTransparentStartIdxFar = 0x1000;
        g_nTrackTransparentStartIdxNear = 0x1000;
        g_nTrackTransparentStartIdx = 0x1000;
    }
    AddTimingMarker(0, 0xFF, 0x80, 0x80, 0xFF);
    if (g_dwWeatherActive != 0) {
        g_dwTrackOtherModeLBlend = 0xC8000000;
        g_dwTrackOtherModeLFlags = 0x112038;
    } else {
        g_dwTrackOtherModeLBlend = 0x0C080000;
        g_dwTrackOtherModeLFlags = 0x112038;
    }
    APPEND_TRACK_COMMAND(0x01030040, PHYSICAL_ADDRESS(g_pProjectionMtx));
    {
        Gfx *g_ = gDisplayListPtr++;

        g_->words.w0 = 0x01060040;
        g_->words.w1 = PHYSICAL_ADDRESS(&g_mtxIdentity);
    }
    APPEND_TRACK_COMMAND(0xBC00000E, g_wPerspNorm);
    APPEND_TRACK_COMMAND(0x03840010, g_pTrackLookAt);
    APPEND_TRACK_COMMAND(0x03820010, (u8 *)g_pTrackLookAt + 0x10);
    APPEND_TRACK_COMMAND(0xBC000002, 0x80000040);
    APPEND_TRACK_COMMAND(0x03860010, (int)&g_aTrackLights[g_nTrackLightBufferIndex].l);
    APPEND_TRACK_COMMAND(0x03880010, (int)&g_aTrackLights[g_nTrackLightBufferIndex]);
    APPEND_TRACK_COMMAND(0xBC00000A, g_dwDirLightColor);
    APPEND_TRACK_COMMAND(0xBC00040A, g_dwDirLightColor);
    APPEND_TRACK_COMMAND(0xBC00200A, g_dwAmbientLightColor);
    APPEND_TRACK_COMMAND(0xBC00240A, g_dwAmbientLightColor);
    APPEND_TRACK_COMMAND(0xE7000000, 0);
    APPEND_TRACK_COMMAND(0xBA001301, 0x80000);
    APPEND_TRACK_COMMAND(0xBA000903, 0xC00);
    APPEND_TRACK_COMMAND(0xBA000801, 0);
    APPEND_TRACK_COMMAND(0xB9000002, 1);
    APPEND_TRACK_COMMAND(0xBA000602, g_dwColorDitherMode);
    APPEND_TRACK_COMMAND(0xBA000402, g_dwAlphaDitherMode);
    APPEND_TRACK_COMMAND(0xBA001402, 0);
    APPEND_TRACK_COMMAND(0xF9000000, 0);
    APPEND_TRACK_COMMAND(0xBA001402, 0x100000);
    APPEND_TRACK_COMMAND(0xB900031D, g_dwTrackOtherModeLBlend | g_dwTrackOtherModeLFlags);
    APPEND_TRACK_COMMAND(0xFC26A004, 0x1FFC93F8);
    APPEND_TRACK_COMMAND(0xBA001102, 0);
    APPEND_TRACK_COMMAND(0xBA001001, g_bTrackTextureLod != 0 ? 0x10000 : 0);
    APPEND_TRACK_COMMAND(0xBA000E02, 0);
    APPEND_TRACK_COMMAND(0xBA000C02, g_dwTextureFilterMode);
    APPEND_TRACK_COMMAND(0xBC000006, 0);
    {
        Gfx *g_ = gDisplayListPtr++;

        g_->words.w0 = 0xB6000000;
        g_->words.w1 = 0x853200;
    }
    APPEND_TRACK_COMMAND(
        0xB7000000,
        (g_dwWeatherActive != 0 ? 0x10000 : 0) |
            ((g_bSmoothShading != 0 ? 0x200 : 0) |
             ((g_dwTrackMirrored ^ g_dwViewportMirrored ? 0x1000 : 0x2000) | 0xA0005)));
    RestoreCurrentViewport();
    APPEND_TRACK_COMMAND(0x01030040, PHYSICAL_ADDRESS(g_pProjectionMtx));
    APPEND_TRACK_COMMAND(0xBC00000E, g_wPerspNorm);
    APPEND_TRACK_COMMAND(0x01060040, PHYSICAL_ADDRESS(&g_mtxIdentity));
    APPEND_TRACK_COMMAND(0xBC000002, 0x80000040);
    APPEND_TRACK_COMMAND(0x03860010, (int)&g_aTrackLights[g_nTrackLightBufferIndex].l);
    APPEND_TRACK_COMMAND(0x03880010, (int)&g_aTrackLights[g_nTrackLightBufferIndex]);
    APPEND_TRACK_COMMAND(0xBC00000A, g_dwDirLightColor);
    APPEND_TRACK_COMMAND(0xBC00040A, g_dwDirLightColor);
    APPEND_TRACK_COMMAND(0xBC00200A, g_dwAmbientLightColor);
    APPEND_TRACK_COMMAND(0xBC00240A, g_dwAmbientLightColor);
    SetupFog();
    SetupFog();
    APPEND_TRACK_COMMAND(0xF9000000, 0);
    {
        Gfx *g_ = gDisplayListPtr++;

        g_->words.w0 = 0xB6000000;
        g_->words.w1 = 0x53200;
    }
    APPEND_TRACK_COMMAND(
        0xB7000000,
        (g_dwWeatherActive != 0 ? 0x10000 : 0) |
            ((g_bSmoothShading != 0 ? 0x200 : 0) |
             ((g_dwTrackMirrored ^ g_dwViewportMirrored ? 0x1000 : 0x2000) | 0xA0005)));
    APPEND_TRACK_COMMAND(0xE7000000, 0);
    APPEND_TRACK_COMMAND(0xBA001402, 0x100000);
    APPEND_TRACK_COMMAND(0xB900031D, g_dwTrackOtherModeLBlend | g_dwTrackOtherModeLFlags);
    APPEND_TRACK_COMMAND(0xFC26A004, 0x1FFC93F8);
    {
        Gfx *g_ = gDisplayListPtr++;

        g_->words.w0 = 0xFA001700;
        g_->words.w1 = 0xFF0000FF;
    }
    APPEND_TRACK_COMMAND(0xBA001102, 0);
    APPEND_TRACK_COMMAND(0xBA001001, g_bTrackTextureLod != 0 ? 0x10000 : 0);
    APPEND_TRACK_COMMAND(0xBA000E02, 0);
    APPEND_TRACK_COMMAND(0xBA000C02, g_dwTextureFilterMode);
    if (g_dwNumViewports == 1) {
        APPEND_TRACK_COMMAND(0xBC000404, 1);
        APPEND_TRACK_COMMAND(0xBC000C04, 1);
        APPEND_TRACK_COMMAND(0xBC001404, 0xFFFF);
        APPEND_TRACK_COMMAND(0xBC001C04, 0xFFFF);
    } else {
        APPEND_TRACK_COMMAND(0xBC000404, 6);
        APPEND_TRACK_COMMAND(0xBC000C04, 6);
        APPEND_TRACK_COMMAND(0xBC001404, 0xFFFA);
        APPEND_TRACK_COMMAND(0xBC001C04, 0xFFFA);
    }
    APPEND_TRACK_COMMAND(0xBB001001, 0xFFFFFFFF);
    APPEND_TRACK_COMMAND(0xB6000000, 0xC0000);
    APPEND_TRACK_COMMAND(0xE8000000, 0);
    APPEND_TRACK_COMMAND(0xF5100000, 0x7000000);
    APPEND_TRACK_COMMAND(0xF50001F0, 0x6000000);
    APPEND_TRACK_COMMAND(0xF5000100, 0x5000000);
    if (deferred != 0) {
        deferredLast = g_nDeferredInstanceLastIdx;
        i = opaqueCount = g_nOpaqueVisibleInstanceCount;
        g_nVisibleInstanceCount = i + deferredLast + 1;
    } else {
        StepDebugHiddenInstance();
        i = 0;
    }
    for (; i < g_nVisibleInstanceCount; i++) {
        if (i == g_nVisibleCellIdxNear || i == g_nVisibleCellIdxFar) {
            if (g_dwNumViewports == 1) {
                APPEND_TRACK_COMMAND(0xBC000404, 1);
                APPEND_TRACK_COMMAND(0xBC000C04, 1);
                APPEND_TRACK_COMMAND(0xBC001404, 0xFFFF);
                APPEND_TRACK_COMMAND(0xBC001C04, 0xFFFF);
            } else {
                APPEND_TRACK_COMMAND(0xBC000404, 6);
                APPEND_TRACK_COMMAND(0xBC000C04, 6);
                APPEND_TRACK_COMMAND(0xBC001404, 0xFFFA);
                APPEND_TRACK_COMMAND(0xBC001C04, 0xFFFA);
            }
            if (i == g_nVisibleCellIdxFar) {
                APPEND_TRACK_COMMAND(0xB7000000, 0x800000);
            }
        }
        if (i < opaqueCount) {

            instanceIndex = g_awVisibleInstanceList[i];
            instance = &g_LoadedTrack.instances[instanceIndex];
            if ((instance->groups & g_wActiveInstanceHideMask) ||
                (g_bDebugHideInstance != 0 && g_nDebugHiddenInstanceIndex == instanceIndex)) {
                continue;
            }
            flags = instance->flags;
            if (flags & instanceHideFlags) {
                continue;
            }
            if (flags & 8) {
                if (g_nTrackTransparentStartIdx == 0x1000 && g_nVisibleCellIdxMid < i) {
                    g_nTrackTransparentStartIdx = deferredLast + 1;
                }
                g_awVisibleInstanceList[++deferredLast] = instanceIndex;
                continue;
            }
        } else {
            instanceIndex = g_awVisibleInstanceList[deferredLast - i + opaqueCount];
            instance = &g_LoadedTrack.instances[instanceIndex];
            flags = instance->flags;
        }
        if (flags & 0x2000) {
            float s;
            float (*mm)[4][4];
            float hi;
            float lo;

            mm = (float (*)[4][4])g_afViewProjMatrix;
            hi = 0.0f;
            lo = 0.f;
            if (!billboardMatrixLoaded) {
                APPEND_TRACK_COMMAND(0x01020040, PHYSICAL_ADDRESS(&g_mtxIdentity));
                billboardMatrixLoaded = 1;
            }
            s = instance->matrix[0][0] * viewMatrixMaxAbs + 0.375f;
            if (s == 0) {
                s = instance->matrix[0][0] * (1.1 * viewMatrixMaxAbs) + 0.375;
            }
            s = 1.99975586f / s;
            TRACK_MVP_MATRIX[3][0] =
                ((instance->matrix[3][0] * mm[0][0][0] + instance->matrix[3][1] * mm[0][1][0] +
                  instance->matrix[3][2] * mm[0][2][0]) +
                 instance->matrix[3][3] * mm[0][3][0]) *
                s;
            TRACK_MVP_MATRIX[3][1] =
                ((instance->matrix[3][0] * mm[0][0][1] + instance->matrix[3][1] * mm[0][1][1] +
                  instance->matrix[3][2] * mm[0][2][1]) +
                 instance->matrix[3][3] * mm[0][3][1]) *
                s;
            TRACK_MVP_MATRIX[3][2] =
                ((instance->matrix[3][0] * mm[0][0][2] + instance->matrix[3][1] * mm[0][1][2] +
                  instance->matrix[3][2] * mm[0][2][2]) +
                 instance->matrix[3][3] * mm[0][3][2]) *
                s;
            TRACK_MVP_MATRIX[3][3] =
                ((instance->matrix[3][0] * mm[0][0][3] + instance->matrix[3][1] * mm[0][1][3] +
                  instance->matrix[3][2] * mm[0][2][3]) +
                 instance->matrix[3][3] * mm[0][3][3]) *
                s;
            s *= instance->matrix[0][0];
            TRACK_MVP_MATRIX[0][0] = mm[0][0][0] * s;
            TRACK_MVP_MATRIX[0][1] = mm[0][0][1] * s;
            TRACK_MVP_MATRIX[0][2] = mm[0][0][2] * s;
            TRACK_MVP_MATRIX[0][3] = mm[0][0][3] * s;
            TRACK_MVP_MATRIX[1][0] = mm[0][1][0] * s;
            TRACK_MVP_MATRIX[1][1] = mm[0][1][1] * s;
            TRACK_MVP_MATRIX[1][2] = mm[0][1][2] * s;
            TRACK_MVP_MATRIX[1][3] = mm[0][1][3] * s;
            TRACK_MVP_MATRIX[2][0] = mm[0][2][0] * s;
            TRACK_MVP_MATRIX[2][1] = mm[0][2][1] * s;
            TRACK_MVP_MATRIX[2][2] = mm[0][2][2] * s;
            TRACK_MVP_MATRIX[2][3] = mm[0][2][3] * s;
            matrixBlock = (Mtx *)AllocDisplayListBlock();

            if (TRACK_MVP_MATRIX[3][0] >= .0f) {
                hi = TRACK_MVP_MATRIX[3][0];
            }
            if (TRACK_MVP_MATRIX[3][0] <= .0f) {
                lo = TRACK_MVP_MATRIX[3][0];
            }
            if (hi <= TRACK_MVP_MATRIX[3][1]) {
                hi = TRACK_MVP_MATRIX[3][1];
            }
            if (TRACK_MVP_MATRIX[3][1] <= lo) {
                lo = TRACK_MVP_MATRIX[3][1];
            }
            if (hi <= TRACK_MVP_MATRIX[3][2]) {
                hi = TRACK_MVP_MATRIX[3][2];
            }
            if (TRACK_MVP_MATRIX[3][2] <= lo) {
                lo = TRACK_MVP_MATRIX[3][2];
            }
            if (hi <= TRACK_MVP_MATRIX[3][3]) {
                hi = TRACK_MVP_MATRIX[3][3];
            }
            if (TRACK_MVP_MATRIX[3][3] <= lo) {
                lo = TRACK_MVP_MATRIX[3][3];
            }
            if (32767.0 < hi || lo < -32767.0) {
#if TGR_REGION != TGR_REGION_JP
                if (i == 1) {
                    osSyncPrintf(
                        "Bad Final Matrix: in=%d (%s), s=%f\n-------------\n%f %f %f %f\n%f %f %f "
                        "%f\n%f %f %f %f\n%f %f %f %f\n-------------\n%f %f %f %f\n%f %f %f %f\n%f "
                        "%f %f %f\n%f %f %f %f\n-------------\n%f %f %f %f\n%f %f %f %f\n%f %f %f "
                        "%f\n%f %f %f %f\n-------------\n",
                        instanceIndex,
                        g_LoadedTrack.instanceNames != 0 ?
                            g_LoadedTrack.instanceNames[instanceIndex] :
                            "???",
                        s,
                        instance->matrix[0][0],
                        instance->matrix[0][1],
                        instance->matrix[0][2],
                        instance->matrix[0][3],
                        instance->matrix[1][0],
                        instance->matrix[1][1],
                        instance->matrix[1][2],
                        instance->matrix[1][3],
                        instance->matrix[2][0],
                        instance->matrix[2][1],
                        instance->matrix[2][2],
                        instance->matrix[2][3],
                        instance->matrix[3][0],
                        instance->matrix[3][1],
                        instance->matrix[3][2],
                        instance->matrix[3][3],
                        mm[0][0][0],
                        mm[0][0][1],
                        mm[0][0][2],
                        mm[0][0][3],
                        mm[0][1][0],
                        mm[0][1][1],
                        mm[0][1][2],
                        mm[0][1][3],
                        mm[0][2][0],
                        mm[0][2][1],
                        mm[0][2][2],
                        mm[0][2][3],
                        mm[0][3][0],
                        mm[0][3][1],
                        mm[0][3][2],
                        mm[0][3][3],
                        g_afMvpMatrix[0][0],
                        g_afMvpMatrix[0][1],
                        g_afMvpMatrix[0][2],
                        g_afMvpMatrix[0][3],
                        g_afMvpMatrix[1][0],
                        g_afMvpMatrix[1][1],
                        g_afMvpMatrix[1][2],
                        g_afMvpMatrix[1][3],
                        g_afMvpMatrix[2][0],
                        g_afMvpMatrix[2][1],
                        g_afMvpMatrix[2][2],
                        g_afMvpMatrix[2][3],
                        g_afMvpMatrix[3][0],
                        g_afMvpMatrix[3][1],
                        g_afMvpMatrix[3][2],
                        g_afMvpMatrix[3][3]);
                }
#endif
                if (-lo < hi) {
                    s = 32767.0f / hi;
                } else {
                    s = -32767.0f / lo;
                }
                TRACK_MVP_MATRIX[0][0] *= s;
                TRACK_MVP_MATRIX[0][1] *= s;
                TRACK_MVP_MATRIX[0][2] *= s;
                TRACK_MVP_MATRIX[0][3] *= s;
                TRACK_MVP_MATRIX[1][0] *= s;
                TRACK_MVP_MATRIX[1][1] *= s;
                TRACK_MVP_MATRIX[1][2] *= s;
                TRACK_MVP_MATRIX[1][3] *= s;
                TRACK_MVP_MATRIX[2][0] *= s;
                TRACK_MVP_MATRIX[2][1] *= s;
                TRACK_MVP_MATRIX[2][2] *= s;
                TRACK_MVP_MATRIX[2][3] *= s;
                TRACK_MVP_MATRIX[3][0] *= s;
                TRACK_MVP_MATRIX[3][1] *= s;
                TRACK_MVP_MATRIX[3][2] *= s;
                TRACK_MVP_MATRIX[3][3] *= s;
            }
            guMtxF2L(g_afMvpMatrix, matrixBlock);
            APPEND_TRACK_COMMAND(0x039E0010, (int)matrixBlock);
            APPEND_TRACK_COMMAND(0x03980010, (int)matrixBlock + 0x10);
            APPEND_TRACK_COMMAND(0x039A0010, (int)matrixBlock + 0x20);
            APPEND_TRACK_COMMAND(0x039C0010, (int)matrixBlock + 0x30);
        } else {
            billboardMatrixLoaded = 0;
            matrixBlock = (Mtx *)AllocDisplayListBlock();
            guMtxF2L(instance->matrix, matrixBlock);
            command = gDisplayListPtr++;
            command->words.w0 = 0x01020040;
            command->words.w1 = (unsigned int)matrixBlock - 0x80000000;
        }

        flags = instance->flags;
        if (flags & 0x4A4) {
            if (flags & 0x400) {
                if (g_dwNightMode != 0 && (flags & 0x100)) {
                    APPEND_TRACK_COMMAND(0xBC00000A, (g_dwDirLightColor >> 1) & 0x7F7F7F00);
                    APPEND_TRACK_COMMAND(0xBC00040A, (g_dwDirLightColor >> 1) & 0x7F7F7F00);
                    APPEND_TRACK_COMMAND(0xBC00200A, g_adwLightRampColors[instance->flags & 3]);
                    APPEND_TRACK_COMMAND(0xBC00240A, g_adwLightRampColors[instance->flags & 3]);
                    flags = instance->flags;
                } else {
                    {
                        Gfx *g_ = gDisplayListPtr++;

                        g_->words.w0 = 0xBC00000A;
                        g_->words.w1 = 0;
                    }
                    {
                        Gfx *g_ = gDisplayListPtr++;

                        g_->words.w0 = 0xBC00040A;
                        g_->words.w1 = 0;
                    }
                    APPEND_TRACK_COMMAND(0xBC00200A, g_adwLightRampColors[instance->flags & 3]);
                    APPEND_TRACK_COMMAND(0xBC00240A, g_adwLightRampColors[instance->flags & 3]);
                    flags = instance->flags;
                }
            }
            if (flags & 4) {
                APPEND_TRACK_COMMAND(0xB6000000, 0x3000);
                flags = instance->flags;
            }
            if ((flags & 0x20) && g_dwWeatherActive != 0 && g_dwNightMode == 0 &&
                g_dwSnowMode == 0 && g_dwRainMode == 0) {
                APPEND_TRACK_COMMAND(0xB6000000, 0x10000);
                flags = instance->flags;
            }
            if ((flags & 0x80) && g_bSmoothShading != 0) {
                APPEND_TRACK_COMMAND(0xB6000000, 0x200);
            }
        }
        if (g_nVisibleCellIdxMid < i && i < g_nTrackTransparentStartIdx) {
            APPEND_TRACK_COMMAND(0xBB001001, 0xFFFFFFFF);
            APPEND_TRACK_COMMAND(0xE8000000, 0);
            APPEND_TRACK_COMMAND(0x06000000, instance->displayList);
            g_dwStatTrianglesDrawn += instance->triangleCount;
            g_dwStatVerticesDrawn += instance->vertexCount;
            g_dwStatPolygonsDrawn += instance->polygonCount;
        } else {
            RenderTrackInstance(instanceIndex, g_abInstanceVisibilityMask[instanceIndex], deferred);
        }
        flags = instance->flags;
        if (flags & 0x4A4) {
            if ((flags & 0x80) && g_bSmoothShading != 0) {
                APPEND_TRACK_COMMAND(0xB7000000, 0x200);
                flags = instance->flags;
            }
            if (flags & 0x400) {
                APPEND_TRACK_COMMAND(0xBC00000A, g_dwDirLightColor);
                APPEND_TRACK_COMMAND(0xBC00040A, g_dwDirLightColor);
                APPEND_TRACK_COMMAND(0xBC00200A, g_dwAmbientLightColor);
                APPEND_TRACK_COMMAND(0xBC00240A, g_dwAmbientLightColor);
                flags = instance->flags;
            }
            if (flags & 4) {
                APPEND_TRACK_COMMAND(0xB7000000,
                                     g_dwTrackMirrored ^ g_dwViewportMirrored ? 0x1000 : 0x2000);
            }
            if (g_dwWeatherActive != 0 && g_dwNightMode == 0 && g_dwSnowMode == 0 &&
                g_dwRainMode == 0 && (instance->flags & 0x20)) {
                APPEND_TRACK_COMMAND(0xB7000000, 0x10000);
            }
        }
    }
    if (deferred == 0) {
        if (g_nTrackTransparentStartIdx == 0x1000) {
            g_nTrackTransparentStartIdx = deferredLast;
        }
        g_nTrackTransparentStartIdx = deferredLast - g_nTrackTransparentStartIdx + opaqueCount;
    }
    APPEND_TRACK_COMMAND(0x01020040, PHYSICAL_ADDRESS(&g_mtxIdentity));
    APPEND_TRACK_COMMAND(0xB6000000, 0x10000);
    APPEND_TRACK_COMMAND(0xE7000000, 0);
    AddTimingMarker(0, 0, 0xB4, 0, 0xFF);
    g_nDeferredInstanceLastIdx = deferredLast;
}

#include <string.h>

#include <PR/gu.h>

#include "common/audiomix.h"
#include "common/camera.h"
#include "common/debugmessage.h"
#include "common/displaylistptr.h"
#include "common/mathutil.h"
#include "common/render.h"
#include "common/screendraw.h"
#include "common/shadowbuffers.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackrender.h"
#include "common/trackshadow.h"
#include "common/vehicle.h"
#include "common/vehiclemodel.h"
#include "common/weather.h"
#ifdef __sgi
#pragma weak g_vShadowForward
Vec3f g_vShadowForward;
#endif
#define G(a, b)                                                                                    \
    {                                                                                              \
        Gfx *g_ = gDisplayListPtr++;                                                               \
        g_->words.w0 = (a);                                                                        \
        g_->words.w1 = (u32)(b);                                                                   \
    }

void RenderTrackInstance(int idx, int cars, int pass) {
    unsigned int *cmd;
    unsigned int *save;
    int c;
    unsigned int c2;
    int n;
    int i;
    Vtx *vtx;
    unsigned char *fl;
    unsigned char f;
    unsigned int w TGR_MATCHING_UNUSED;
    Gfx **gp;
    VehicleModel *model;
    float k;
    unsigned int *dl;
    TrackInstance *obj;
    unsigned int *start;

    dl = (u32 *)g_pShadowDisplayListWritePtr;
    gp = &gDisplayListPtr;
    vtx = g_pShadowVertexWritePtr;
    obj = &g_LoadedTrack.instances[idx];
    start = (unsigned int *)obj->displayList;
    if (cars != 0 && g_pActiveCamera != &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_MIRROR] &&
        (obj->flags & 0x200) == 0) {
        AddTimingMarker(0, 0x50, 0xFA, 0x50, 0xFF);
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xBB001001;
            g_->words.w1 = 0xFFFFFFFF;
        }
        G(0xE8000000, 0);
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xFA001700;
            g_->words.w1 = 0xFF0000FF;
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0x06000000;
            g_->words.w1 = (u32)start;
        }
        G(0xE7000000, 0);
        G(0xBA001402, 0);
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xFD900000;
            g_->words.w1 = (u32)&g_aParticleTexture;
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xF5900000;
            g_->words.w1 = 0x07018060;
        }
        G(0xE6000000, 0);
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xF3000000;
            g_->words.w1 = 0x077FF100;
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xF5881000;
            g_->words.w1 = 0x0009BE6F;
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xF2000000;
            g_->words.w1 = 0x000FC0FC;
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xBB000001;
            g_->words.w1 = 0xFFFFFFFF;
        }
        G(0xBA001001, 0);
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xFA001700;
            g_->words.w1 = 0xFF0000FF;
        }
        if (pass != 0 && g_dwRainMode == 0 && g_dwNightMode == 0) {
            {
                Gfx *g_ = gDisplayListPtr++;

                g_->words.w0 = 0xB900031D;
                g_->words.w1 = 0x00504B50;
            }
        } else {
            {
                Gfx *g_ = gDisplayListPtr++;

                g_->words.w0 = 0xB900031D;
                g_->words.w1 = 0x00504F50;
            }
        }
        G(0xB9000002, 1);
        G(0xF9000000, 8);
        G(0xFC567EAC, 0xFFFFF3F9);
        G(0xB6000000, 0x00070004);
        G(0xBA000602, 0xC0);
        G(0x06000000, dl);
        G(0xE7000000, 0);
        if (g_bShadowCullDisabled == 0) {
            SetScissorRect(g_aViewportRects[g_dwCurrentSplitScreenIndex].x,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].y,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width,
                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height);
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xBA000602;
            g_->words.w1 = g_dwColorDitherMode;
        }
        G(0xF9000000, 0);
        if (g_dwWeatherActive != 0) {
            G(0xB7000000, 0x00030004);
        } else {
            G(0xB7000000, 0x00020004);
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xBA001001;
            g_->words.w1 = g_bTrackTextureLod != 0 ? 0x10000 : 0;
        }
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xFA001700;
            g_->words.w1 = 0xFF0000FF;
        }
        G(0xBA001402, 0x00100000);
        G(0xB900031D, g_dwTrackOtherModeLBlend | g_dwTrackOtherModeLFlags);
        G(0xFC26A004, 0x1FFC93F8);
        g_dwStatTrianglesDrawn += obj->triangleCount * 3;
        g_dwStatVerticesDrawn += obj->vertexCount * 3;
        g_dwStatPolygonsDrawn += obj->polygonCount * 3;
        {
            unsigned char clip[32];
            unsigned int *mark;
            float s0;
            float s1;
            float a;
            Vtx *vtxBase;
            unsigned int *p;
            float b;
            float d;
            float e;
            unsigned int src;
            unsigned char *cl;
            float len2 TGR_MATCHING_UNUSED;
            float vx;
            float vy;
            float vz;
            float sz TGR_MATCHING_UNUSED;
            float m00, m10, m20, m30, m01, m11, m21, m31;

            cl = clip;
            mark = 0;
            vtxBase = 0;
            for (i = 0; cars != 0; i++, cars = (int)cars >> 1) {
                if ((cars & 1) == 0 ||
                    ((g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_COCKPIT] ||
                      g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_BUMPER]) &&
                     i == g_pActiveCameraVehicle->index)) {
                    continue;
                }
                p = dl;
                p[0] = 0xE7000000;
                p[1] = 0;
                p += 2;
                {
                    Gfx *g_ = (Gfx *)p;

                    p += 2;
                    g_->words.w0 = 0xFB000000;
                    g_->words.w1 = (_SHIFTL(g_bFogR, 24, 8) | _SHIFTL(g_bFogG, 16, 8) |
                                    _SHIFTL(g_bFogB, 8, 8) |
                                    _SHIFTL(FloatToInt(g_aVehicles[i].fogFactor * 255.0f), 0, 8));
                }
                cmd = start;
                dl = p;
                g_vShadowForward.x = g_aVehicles[i].matrix[0][0];
                g_vShadowForward.y = g_aVehicles[i].matrix[0][1];
                g_vShadowForward.z = g_aVehicles[i].matrix[0][2];
                if (g_vShadowForward.x == 0.0f && g_vShadowForward.y == 0.0f) {
                    g_vShadowForward.x = g_aVehicles[i].matrix[2][0];
                    g_vShadowForward.y = g_aVehicles[i].matrix[2][1];
                    g_vShadowForward.z = g_aVehicles[i].matrix[2][2];
                }
                g_vShadowForward.z = 0;
                s0 = Vec2fLength((Vec2f *)&g_vShadowForward);
                if (s0 < 0.5f) {
                    s0 = 0.5f;
                }
                s1 = Vec2fLength((Vec2f *)g_aVehicles[i].matrix[1]);
                if (s1 < 0.5f) {
                    s1 = 0.5f;
                }
                model = g_aVehicles[i].model;
                s0 = g_fShadowLengthScale / ((f32 *)model->wheels)[0] / s0;
                s1 = g_fShadowWidthScale / ((f32 *)model->wheels)[1] / s1;
                Vec3fNormalize(&g_vShadowForward);
                a = g_vShadowForward.x * s0;
                b = -g_vShadowForward.y * s0;
                d = g_vShadowForward.x * s1;
                e = -g_vShadowForward.y * s1;
                memcpy(g_afMvpMatrix, obj->matrix, 0x40);
                g_afMvpMatrix[3][0] -= g_aVehicles[i].matrix[3][0];
                g_afMvpMatrix[3][1] -= g_aVehicles[i].matrix[3][1];
                ((f32 *)g_afScratchMatrix)[0] = a;
                ((f32 *)g_afScratchMatrix)[4] = -b;
                ((f32 *)g_afScratchMatrix)[8] = 0;
                ((f32 *)g_afScratchMatrix)[12] = 512.0f;
                ((f32 *)g_afScratchMatrix)[1] = e;
                ((f32 *)g_afScratchMatrix)[5] = d;
                ((f32 *)g_afScratchMatrix)[9] = 0;
                ((f32 *)g_afScratchMatrix)[13] = 512.0f;
                ((f32 *)g_afScratchMatrix)[2] = 0;
                ((f32 *)g_afScratchMatrix)[6] = 0;
                ((f32 *)g_afScratchMatrix)[10] = 1.0f;
                ((f32 *)g_afScratchMatrix)[14] = 0;
                ((f32 *)g_afScratchMatrix)[3] = 0;
                ((f32 *)g_afScratchMatrix)[7] = 0;
                ((f32 *)g_afScratchMatrix)[11] = 0;
                ((f32 *)g_afScratchMatrix)[15] = 1.0f;
                Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);
                k = g_afMvpMatrix[0][3] + g_afMvpMatrix[1][3] + g_afMvpMatrix[2][3] +
                    g_afMvpMatrix[3][3];
                if (k != 0.0f) {
                    k = 1.0f / k;
                } else {
                    k = 1.0f;
                }
                g_afMvpMatrix[0][0] *= k;
                g_afMvpMatrix[1][0] *= k;
                g_afMvpMatrix[2][0] *= k;
                g_afMvpMatrix[3][0] *= k;
                g_afMvpMatrix[0][1] *= k;
                g_afMvpMatrix[1][1] *= k;
                g_afMvpMatrix[2][1] *= k;
                g_afMvpMatrix[3][1] *= k;
                if (g_bShadowCullDisabled == 0) {
                    save = (unsigned int *)*gp;
                    *gp = (Gfx *)p;
                    SetScissorRect(((short *)g_aVehicles[i].shadowCorners)[0],
                                   ((short *)g_aVehicles[i].shadowCorners)[3],
                                   ((short *)g_aVehicles[i].shadowCorners)[2] -
                                       ((short *)g_aVehicles[i].shadowCorners)[0],
                                   ((short *)g_aVehicles[i].shadowCorners)[1] -
                                       ((short *)g_aVehicles[i].shadowCorners)[3]);
                    dl = (unsigned int *)gDisplayListPtr;
                    gDisplayListPtr = (Gfx *)save;
                }
                for (;;) {
                    if ((dl - (u32 *)g_pShadowDisplayListStart) >= 0x3C9) {
                        break;
                    }
                    switch (cmd[0] >> 24) {
                    case 0xB1:
                        if (g_bShadowCullDisabled != 0) {
                            dl[0] = cmd[0];
                            dl[1] = cmd[1];
                            dl += 2;
                            cmd += 2;
                            break;
                        }
                        c2 = cmd[0] >> 1;
                        f = cl[c2 & 0x1F];
                        c2 >>= 8;
                        f &= cl[c2 & 0x1F];
                        c2 >>= 8;
                        f &= cl[c2 & 0x1F];
                        if (f) {
                            c2 = cmd[1];
                            c2 >>= 1;
                            f = cl[c2 & 0x1F];
                            c2 >>= 8;
                            f &= cl[c2 & 0x1F];
                            c2 >>= 8;
                            f &= cl[c2 & 0x1F];
                            if (f) {
                                cmd += 2;
                                break;
                            }
                            dl[0] = 0xBF000000;
                            dl[1] = cmd[1] & 0xFFFFFF;
                            dl += 2;
                            cmd += 2;
                            break;
                        }
                        c2 = cmd[1];
                        c2 >>= 1;
                        f = cl[c2 & 0x1F];
                        c2 >>= 8;
                        f &= cl[c2 & 0x1F];
                        c2 >>= 8;
                        f &= cl[c2 & 0x1F];
                        if (f) {
                            dl[0] = 0xBF000000;
                            dl[1] = cmd[0] & 0xFFFFFF;
                            dl += 2;
                            cmd += 2;
                            break;
                        }
                        dl[0] = cmd[0];
                        dl[1] = cmd[1];
                        dl += 2;
                        cmd += 2;
                        break;
                    case 0xB8:
                        goto done;
                    case 0xBF:
                        if (g_bShadowCullDisabled != 0) {
                            dl[0] = cmd[0];
                            dl[1] = cmd[1];
                            dl += 2;
                            cmd += 2;
                            break;
                        }
                        c2 = cmd[1];
                        c2 >>= 1;
                        f = cl[c2 & 0x1F];
                        c2 >>= 8;
                        f &= cl[c2 & 0x1F];
                        c2 >>= 8;
                        f &= cl[c2 & 0x1F];
                        if (f) {
                            cmd += 2;
                            break;
                        }
                        dl[0] = cmd[0];
                        dl[1] = cmd[1];
                        dl += 2;
                        cmd += 2;
                        break;
                    case 0x04:
                        c = cmd[0] >> 10;
                        m00 = g_afMvpMatrix[0][0];
                        m10 = g_afMvpMatrix[1][0];
                        m20 = g_afMvpMatrix[2][0];
                        m30 = g_afMvpMatrix[3][0];
                        m01 = g_afMvpMatrix[0][1];
                        m11 = g_afMvpMatrix[1][1];
                        m21 = g_afMvpMatrix[2][1];
                        m31 = g_afMvpMatrix[3][1];
                        if (dl == mark) {
                            vtx = vtxBase;
                            dl -= 2;
                        }
                        c &= 0x3F;
                        n = c;
                        *dl++ = cmd[0];
                        cmd++;
                        if (vtx + n - g_pShadowVertexStart > 1000) {
                            dl--;
                            goto next;
                        }
                        fl = clip;
                        vtxBase = vtx;
                        do {
                            if (c > 32) {
                                SetDebugMessage("BAD VTX DL");
#ifdef ENABLE_PATCHES
                                goto next;
#endif
                            }
                        } while (0);
                        src = *cmd++;
                        *dl++ = c = (unsigned int)vtx;
                        mark = dl;
                        while (n--) {
                            CopyVec4(vtx, (void *)src);
                            src += 0x10;
                            vx = vtx->v.ob[0];
                            vy = vtx->v.ob[1];
                            vz = vtx->v.ob[2];
                            vtx->v.tc[0] = vx * m00 + vy * m10 + vz * m20 + m30;
                            vtx->v.tc[1] = vx * m01 + vy * m11 + vz * m21 + m31;
                            c = vtx->v.tc[0];
                            if (c < 0) {
                                f = 1;
                            } else if (c >= 0x400) {
                                f = 2;
                            } else {
                                f = 0;
                            }
                            if (vtx->v.tc[1] < 0) {
                                f |= 4;
                            } else if (vtx->v.tc[1] >= 0x1000) {
                                f |= 8;
                            }
                            *fl++ = f;
                            vtx++;
                        }
                        break;
                    default:
                        cmd += 2;
                        break;
                    }
                }
            done:
                if (dl == mark) {
                    vtx = vtxBase;
                    dl -= 2;
                }
            next:;
            }
            dl[0] = 0xB8000000;
            dl[1] = 0;
            dl += 2;
        }
        AddTimingMarker(0, 0xFF, 0x80, 0x80, 0xFF);
    } else {
        {
            Gfx *g_ = gDisplayListPtr++;

            g_->words.w0 = 0xBB001001;
            g_->words.w1 = 0xFFFFFFFF;
        }
        G(0xE8000000, 0);
        G(0x06000000, start);
        g_dwStatTrianglesDrawn += obj->triangleCount;
        g_dwStatVerticesDrawn += obj->vertexCount;
        g_dwStatPolygonsDrawn += obj->polygonCount;
    }
    g_pShadowVertexWritePtr = vtx;
    g_pShadowDisplayListWritePtr = (Gfx *)dl;
}

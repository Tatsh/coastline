#include <PR/gu.h>

#include "common/audiomix.h"
#include "common/camera.h"
#include "common/render.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/weather.h"

void RenderPrecipitationEffect(void) {
    int unusedFrameWord;
    s16(*pt)[3];
    CameraFrame *camera;
    float sx;
    float sy;
    float mirror;
    int i;
    int k;
    int ix;
    int iy;
    float x;
    float y;
    float fx;
    float fy;
    float w;
    float tx;
    float ty;
    int hw;
    int hh;
    int px;
    int py;
    u8 unusedShade;
    float (*m)[4];
    int unusedIndex;
    int unusedRow;
    int unusedColumn;

    if (g_dwSnowMode == 0 && g_dwRainMode == 0) {
        return;
    }
    for (i = 0; i < g_dwCameraRayHitCount; i++) {
        if (g_LoadedTrack.instances[g_awCameraRayHitInstances[i]].flags & TRACK_INSTANCE_SHELTER) {
            return;
        }
    }
    pt = g_awRainParticlePos[g_dwCurrentSplitScreenIndex];
    if (g_dwRainMode != 0) {
        StubWeatherUpdate();
    }
    gDPPipeSync(gDisplayListPtr++);
    gDisplayListWords(gDisplayListPtr++, 0xba001402, 0);
    {
        Gfx *_g = gDisplayListPtr++;

        _g->words.w0 = 0xbb000001;
        _g->words.w1 = 0xffffffff;
    }
    gDisplayListWords(gDisplayListPtr++, 0xba000c02, g_dwTextureFilterMode);
    gDisplayListWords(gDisplayListPtr++, 0xfcffffff, 0xfffdf2f9);
    gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x504240);
    {
        Gfx *_g = gDisplayListPtr++;

        _g->words.w0 = 0xfd900000;
        if (g_dwRainMode != 0) {
            _g->words.w1 = (unsigned int)g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex];
        } else {
            _g->words.w1 = (unsigned int)&g_aParticleTexture;
        }
    }
    {
        Gfx *_g = gDisplayListPtr++;

        _g->words.w0 = 0xf5900000;
        _g->words.w1 = 0x07018060;
    }
    gDPLoadSync(gDisplayListPtr++);
    {
        Gfx *_g = gDisplayListPtr++;

        _g->words.w0 = 0xf3000000;
        _g->words.w1 = 0x077ff100;
    }
    gDPPipeSync(gDisplayListPtr++);
    {
        Gfx *_g = gDisplayListPtr++;

        _g->words.w0 = 0xf5881000;
        _g->words.w1 = 0x00018060;
    }
    {
        Gfx *_g = gDisplayListPtr++;

        _g->words.w0 = 0xf2000000;
        _g->words.w1 = 0x000fc0fc;
    }
    gDisplayListWords(gDisplayListPtr++, 0xba000e02, 0);
    gDisplayListWords(gDisplayListPtr++, 0xba001301, 0);
    if (g_dwSnowMode != 0) {
        gDisplayListWords(gDisplayListPtr++, 0xfa00ffff, 0xe0e0ffff);
    } else {
        gDisplayListWords(gDisplayListPtr++, 0xfa00ffff, 0x788088ff);
    }
    camera = g_pActiveCamera;
    MtxInvert4x4(g_afMvpMatrix, ((f32(*)[4])camera));
    g_afMvpMatrix[3][2] = 0.0f;
    g_afScratchMatrix[0][2] = PARTICLE_SCALE;
    g_afScratchMatrix[1][0] = PARTICLE_SCALE;
    g_afScratchMatrix[2][1] = PARTICLE_SCALE;
    g_afScratchMatrix[0][0] = 0.0f;
    g_afScratchMatrix[0][1] = 0.0f;
    g_afScratchMatrix[0][3] = 0.0f;
    g_afScratchMatrix[1][1] = 0.0f;
    g_afScratchMatrix[1][2] = 0.0f;
    g_afScratchMatrix[1][3] = 0.0f;
    g_afScratchMatrix[2][0] = 0.0f;
    g_afScratchMatrix[2][2] = 0.0f;
    g_afScratchMatrix[2][3] = 0.0f;
    g_afScratchMatrix[3][0] = 0.0f;
    g_afScratchMatrix[3][1] = 0.0f;
    g_afScratchMatrix[3][2] = 0.0f;
    g_afMvpMatrix[3][0] = g_afMvpMatrix[3][1] = g_afMvpMatrix[3][2];
    g_afScratchMatrix[3][3] = 1.0f;
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afScratchMatrix);
    if (g_dwRainMode != 0) {
        Vec3f d;

        MemFill64Aligned(g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex],
                         sizeof(g_abPrecipitationTextures[0]),
                         0);
        Mtx4x4TransformNoTranslation(
            &d, &g_avRainVelocity[g_dwCurrentSplitScreenIndex], g_afMvpMatrix);
        Vec3fNormalize(&d);
        if (g_dwViewportMirrored ^ g_dwTrackMirrored) {
            d.x = -d.x;
        }
        x = 32.0f - d.x * 28.0f;
        y = d.y * 28.0f + 32.0f;
        for (k = 0; k < 16; k++) {
            x += d.x * 3.5f;
            y += -d.y * 3.5f;
            ix = (int)x;
            iy = (int)y;
            if (iy > 1 && iy < 62) {
                if (ix > 1 && ix < 62) {
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 2) * RAIN_STREAK_TEXTURE_SIDE + (ix - 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 2) * RAIN_STREAK_TEXTURE_SIDE + (ix)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 2) * RAIN_STREAK_TEXTURE_SIDE + (ix + 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 1) * RAIN_STREAK_TEXTURE_SIDE + (ix - 2)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 1) * RAIN_STREAK_TEXTURE_SIDE + (ix - 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 1) * RAIN_STREAK_TEXTURE_SIDE + (ix)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 1) * RAIN_STREAK_TEXTURE_SIDE + (ix + 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy - 1) * RAIN_STREAK_TEXTURE_SIDE + (ix + 2)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy)*RAIN_STREAK_TEXTURE_SIDE + (ix - 2)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy)*RAIN_STREAK_TEXTURE_SIDE + (ix - 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy)*RAIN_STREAK_TEXTURE_SIDE + (ix)] = (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy)*RAIN_STREAK_TEXTURE_SIDE + (ix + 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy)*RAIN_STREAK_TEXTURE_SIDE + (ix + 2)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 1) * RAIN_STREAK_TEXTURE_SIDE + (ix - 2)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 1) * RAIN_STREAK_TEXTURE_SIDE + (ix - 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 1) * RAIN_STREAK_TEXTURE_SIDE + (ix)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 1) * RAIN_STREAK_TEXTURE_SIDE + (ix + 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 1) * RAIN_STREAK_TEXTURE_SIDE + (ix + 2)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 2) * RAIN_STREAK_TEXTURE_SIDE + (ix - 1)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 2) * RAIN_STREAK_TEXTURE_SIDE + (ix)] =
                                                 (k << 4) | k;
                    g_abPrecipitationTextures[g_dwCurrentSplitScreenIndex]
                                             [(iy + 2) * RAIN_STREAK_TEXTURE_SIDE + (ix + 1)] =
                                                 (k << 4) | k;
                }
            }
        }
    }
    AddTimingMarker(0, 0x80, 0xff, 0xff, 0xff);
    Mtx4x4Multiply(g_afMvpMatrix, g_afMvpMatrix, g_afPerspectiveMatrix);
    if (g_dwSnowMode != 0) {
        sx = 24.0f;
        sy = 36.0f;
    } else {
        sy = sx = 128.0f;
    }
    if (g_dwViewportMirrored ^ g_dwTrackMirrored) {
        mirror = -640.0f;
    } else {
        mirror = 640.0f;
    }
    m = g_afMvpMatrix;
    for (k = 0; k < g_dwRainParticleCount; k++) {
        int unusedX;
        int unusedY;
        Gfx *g1;
        Gfx *g2;
        Gfx *g3;

        fx = pt[k][0];
        fy = pt[k][1];
        w = pt[k][2];
        tx = (fx * m[0][0] + fy * m[1][0] + w * m[2][0]) + m[3][0];
        ty = (fx * m[0][1] + fy * m[1][1] + w * m[2][1]) + m[3][1];
        w = (fx * m[0][3] + fy * m[1][3] + w * m[2][3]) + m[3][3];
        if (w < PARTICLE_NEAR) {
            continue;
        }
        w = 0.5f / w;
        ty *= w;
        if (ty < -1.0f || 1.0f < ty) {
            continue;
        }
        tx *= w;
        if (tx < -1.0f || 1.0f < tx) {
            continue;
        }
        px = tx * mirror;
        g1 = NULL;
        g2 = NULL;
        g3 = NULL;
        hw = sx * w;
        if (hw < 2) {
            continue;
        }
        hh = sy * w;
        if (hh < 2) {
            continue;
        }
        py = ty * 480.0f;
        g1 = gDisplayListPtr++;
        g1->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(px + hw + 0x280, 12, 12) |
                        _SHIFTL(py + hh + 0x1e0, 0, 12));
        g1->words.w1 =
            (_SHIFTL(0, 24, 3) | _SHIFTL(px + 0x280, 12, 12) | _SHIFTL(py + 0x1e0, 0, 12));
        g2 = gDisplayListPtr++;
        g2->words.w0 = _SHIFTL(G_RDPHALF_1, 24, 8);
        g2->words.w1 = (unsigned int)(_SHIFTL(0, 16, 16) | _SHIFTL(0x7e0, 0, 16));
        g3 = gDisplayListPtr++;
        g3->words.w0 = _SHIFTL(G_RDPHALF_2, 24, 8);
        g3->words.w1 =
            (unsigned int)(_SHIFTL(0x3f000 / hw, 16, 16) | _SHIFTL(-0x3f000 / hh, 0, 16));
    }
    gDPPipeSync(gDisplayListPtr++);
    gDisplayListWords(gDisplayListPtr++, 0xba001301, 0x80000);
}

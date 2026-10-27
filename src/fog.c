#include "render.h"
#include "weather.h"

#define LIGHT_COLOR(red, green, blue) ((u32)(red) << 24 | (u32)(green) << 16 | (u32)(blue) << 8)

#if TGR_PLATFORM == TGR_PLATFORM_WIN
f32 g_afViewProjMatrix[4][4];
s32 g_dwFogFactorOffset;
s32 g_dwFogFactorScale;
u32 g_dwDirLightColor;
u32 g_dwAmbientLightColor;
f32 g_afLightDirection[3];
u32 g_adwLightRampColors[4];
u8 g_bDirLightR;
u8 g_bDirLightG;
u8 g_bDirLightB;
u8 g_bAmbientLightR;
u8 g_bAmbientLightG;
u8 g_bAmbientLightB;
u8 g_abLightRampR[4];
u8 g_abLightRampG[4];
u8 g_abLightRampB[4];
u8 g_bFogR;
u8 g_bFogG;
u8 g_bFogB;
u8 g_bFogIntensity;
#endif

f32 ComputeFogFactorAtPoint(Vec3f *point) {
    f32 depth;
    f32 clip[4];
    f32 fog;

    if (!g_dwWeatherActive) {
        return 0.0f;
    }
    Vec3fTransformByMatrix(clip, point, g_afViewProjMatrix);
    depth = clip[2];
    depth /= clip[3];
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    fog = ((long double)depth * (f32)g_dwFogFactorScale + (f32)g_dwFogFactorOffset) *
          (f32)(1.0 / 255.0);
    if (!(fog >= 0.0f)) {
        return 0.0f;
    }
#else
    /* The cast is redundant, and the original compiles with the depth as the first factor. */
    fog = ((f32)depth * g_dwFogFactorScale + g_dwFogFactorOffset) * (1.0 / 255.0);
    if (fog < 0.0f) {
        return 0.0f;
    }
#endif
    if (1.0f < fog) {
        return 1.0f;
    }
    return fog;
}

void SetupWeatherLights(void) {
    s32 i;

    if (!g_dwNightMode) {
        g_afLightDirection[0] = 15.0f;
        g_afLightDirection[1] = 10.0f;
        g_afLightDirection[2] = 20.0f;
        Vec3fNormalize((Vec3f *)g_afLightDirection);
    }
    if (g_dwSnowMode || g_dwRainMode) {
        g_bDirLightR = (g_bFogR + 765) >> 2;
        g_bDirLightG = (g_bFogG + 765) >> 2;
        g_bDirLightB = (g_bFogB + 612) >> 2;
        g_bAmbientLightR = g_bFogR * 5 / 8;
        g_bAmbientLightG = g_bFogG * 5 / 8;
        g_bAmbientLightB = g_bFogB * 5 / 8;
    } else if (g_dwNightMode) {
        g_bDirLightR = 0xDD;
        g_bDirLightG = 0xEE;
        g_bDirLightB = 0xFF;
        g_bAmbientLightR = 0x3C;
        g_bAmbientLightG = 0x39;
        g_bAmbientLightB = 0x36;
    } else if (g_dwWeatherActive) {
        /* The fog blends the lights of a clear day toward its colour by its intensity. */
        g_bDirLightR =
            (((g_bFogR + 255) >> 1) * g_bFogIntensity + (255 - g_bFogIntensity) * 255) / 255;
        g_bDirLightG =
            (((g_bFogG + 255) >> 1) * g_bFogIntensity + (255 - g_bFogIntensity) * 255) / 255;
        g_bDirLightB =
            (((g_bFogB + 204) >> 1) * g_bFogIntensity + (255 - g_bFogIntensity) * 204) / 255;
        g_bAmbientLightR =
            (g_bFogR * 4 / 5 * g_bFogIntensity + (255 - g_bFogIntensity) * 102) / 255;
        g_bAmbientLightG =
            (g_bFogG * 4 / 5 * g_bFogIntensity + (255 - g_bFogIntensity) * 102) / 255;
        g_bAmbientLightB =
            (g_bFogB * 4 / 5 * g_bFogIntensity + (255 - g_bFogIntensity) * 119) / 255;
    } else {
        g_bDirLightR = 255;
        g_bDirLightG = 255;
        g_bDirLightB = 204;
        g_bAmbientLightR = 102;
        g_bAmbientLightG = 102;
        g_bAmbientLightB = 119;
    }
    if (g_dwNightMode) {
        g_abLightRampR[0] = g_abLightRampG[0] = g_abLightRampB[0] = 0x22;
        g_abLightRampR[1] = g_abLightRampG[1] = g_abLightRampB[1] = 0x44;
        g_abLightRampR[2] = g_abLightRampG[2] = g_abLightRampB[2] = 0x66;
        g_abLightRampR[3] = g_abLightRampG[3] = g_abLightRampB[3] = 0xFF;
    } else if (g_dwSnowMode) {
        g_abLightRampR[0] = g_abLightRampG[0] = 0xD0;
        g_abLightRampB[0] = 0xF0;
        g_abLightRampR[1] = g_abLightRampG[1] = 0xE0;
        g_abLightRampB[1] = 0xFF;
        g_abLightRampR[2] = g_abLightRampG[2] = 0xF0;
        g_abLightRampB[2] = 0xFF;
        g_abLightRampR[3] = g_abLightRampG[3] = g_abLightRampB[3] = 0xFF;
    } else {
        g_abLightRampR[0] = g_bDirLightR >> 2;
        g_abLightRampR[1] = g_bDirLightR >> 1;
        g_abLightRampR[2] = (g_bDirLightR >> 1) + (g_bDirLightR >> 2);
        g_abLightRampR[3] = g_bDirLightR;
        g_abLightRampG[0] = g_bDirLightG >> 2;
        g_abLightRampG[1] = g_bDirLightG >> 1;
        g_abLightRampG[2] = (g_bDirLightG >> 1) + (g_bDirLightG >> 2);
        g_abLightRampG[3] = g_bDirLightG;
        g_abLightRampB[0] = g_bDirLightB >> 2;
        g_abLightRampB[1] = g_bDirLightB >> 1;
        g_abLightRampB[2] = (g_bDirLightB >> 1) + (g_bDirLightB >> 2);
        g_abLightRampB[3] = g_bDirLightB;
    }
    g_dwAmbientLightColor = LIGHT_COLOR(g_bAmbientLightR, g_bAmbientLightG, g_bAmbientLightB);
    g_dwDirLightColor = LIGHT_COLOR(g_bDirLightR, g_bDirLightG, g_bDirLightB);
    for (i = 0; i < 4; i++) {
        g_adwLightRampColors[i] =
            LIGHT_COLOR(g_abLightRampR[i], g_abLightRampG[i], g_abLightRampB[i]);
    }
}

#include "camera.h"
#include "displaylistptr.h"
#include "game.h"
#include "render.h"
#include "track.h"
#include "trackselect.h"
#include "vehicleaccess.h"
#include "weather.h"

#if TGR_PC
#include "win32/gbicommand.h"
#endif

/* The y past which the fog of the desert turns grey. */
#define DESERT_HAZE_Y 1024.0f

/* A colour of a light, as the microcode reads it. */
#define LIGHT_COLOR(red, green, blue) ((u32)(red) << 24 | (u32)(green) << 16 | (u32)(blue) << 8)

#if TGR_PC
f32 g_afViewProjMatrix[4][4];
s32 g_dwFogFactorOffset;
s32 g_dwFogFactorScale;
s32 g_dwFogMinDistance;
s32 g_dwFogMaxDistance;
CameraFrame *g_pActiveCamera;
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

static s32 FogTruncateToWord(long double value) {
    /* The runtime converts to signed 64-bit; fog uses its low word, even on failure. */
    if (!(value >= -9223372036854775808.0L && value < 9223372036854775808.0L)) {
        return 0;
    }
    return (s32)(u32)(s64)value;
}
#endif /* TGR_PC */

void SetupFog(void) {
    Vehicle *vehicle;
    f32 flash, haze, y;
#if TGR_PC
    long double level;
#else
    f32 fade;
    f32 level;
#endif
    s32 intensity;

    if (g_dwNightMode) {
        g_bFogR = g_bFogG = g_bFogB = 0;
        g_bFogIntensity = 0x40;
        if (g_dwNumLocalPlayers == 2) {
            g_dwFogMinDistance = 992;
            g_dwFogMaxDistance = 1020;
        } else {
            g_dwFogMinDistance = 968;
            g_dwFogMaxDistance = 1020;
        }
    } else if (g_dwSnowMode) {
        g_bFogR = 0xB8;
        g_bFogG = 0xB8;
        g_bFogB = 0xD8;
        g_bFogIntensity = 0x40;
        if (g_dwNumLocalPlayers == 2) {
            g_dwFogMinDistance = 950;
            g_dwFogMaxDistance = 1000;
        } else {
            g_dwFogMinDistance = 800;
            g_dwFogMaxDistance = 1050;
        }
    } else if (g_dwRainMode) {
        g_bFogR = 0x60;
        g_bFogG = 0x68;
        g_bFogB = 0x70;
        g_bFogIntensity = 0x40;
        if (g_dwLightningFlashCounter > 0 && (g_dwLightningFlashCounter & 1)) {
            /* A flash of lightning lights the fog, the more the nearer the bolt. */
            flash = 100.0f / (Vec2fDistance((Vec2f *)&g_pActiveCamera->position,
                                            (Vec2f *)g_avLightningBolt) +
                              100.0f);
#if TGR_PC
            g_bFogR = FogTruncateToWord((1.0L - flash) * g_bFogR + 240.0L * flash);
            g_bFogG = FogTruncateToWord((1.0L - flash) * g_bFogG + 248.0L * flash);
            g_bFogB = FogTruncateToWord((1.0L - flash) * g_bFogB + 255.0L * flash);
#else
            fade = 1.0f - flash;
            g_bFogR = g_bFogR * fade + 240.0f * flash;
            g_bFogG = g_bFogG * fade + 248.0f * flash;
            g_bFogB = g_bFogB * fade + 255.0f * flash;
#endif /* TGR_PC */
        }
        if (g_dwNumLocalPlayers == 2) {
            g_dwFogMinDistance = 930;
            g_dwFogMaxDistance = 1000;
        } else {
            g_dwFogMinDistance = 850;
            g_dwFogMaxDistance = 1025;
        }
    } else if (g_dwWeatherActive) {
        /* The intensity follows the height of the vehicle of the view within the track. */
        vehicle = g_pActiveCameraVehicle;
#if TGR_PC
        level = ((long double)VEHICLE_GET(vehicle, Frame).position.z - g_LoadedTrack.minZ) /
                ((long double)g_LoadedTrack.maxZ - g_LoadedTrack.minZ) * 255.0L;
        intensity = FogTruncateToWord(level);
#else
        level = (VEHICLE_GET(vehicle, Frame).position.z - g_LoadedTrack.minZ) /
                (g_LoadedTrack.maxZ - g_LoadedTrack.minZ) * 255.0f;
        intensity = (s32)level;
#endif
        if (intensity < 0) {
            intensity = 0;
        } else if (intensity >= 0x100) {
            intensity = 0xFF;
        }
        if (g_dwSelectedTrackId == TRACK_DESERT) {
            /* Past a y of 1024, the fog of the desert turns grey by an amount with no limit. */
            y = g_pActiveCamera->position.y;
            if (DESERT_HAZE_Y < y) {
#if TGR_PC
                haze = ((long double)g_pActiveCamera->position.x * 0.00043945311f) *
                       (((long double)y - DESERT_HAZE_Y) * 0.00087890622f);
#else
                haze = (y - DESERT_HAZE_Y) * 0.00087890622f *
                       (g_pActiveCamera->position.x * 0.00043945311f);
#endif
            } else {
                haze = 0.0f;
            }
#if TGR_PC
            g_bFogR = FogTruncateToWord((1.0L - haze) * g_LoadedTrack.fogColor[0] + 160.0L * haze);
            g_bFogG = FogTruncateToWord((1.0L - haze) * g_LoadedTrack.fogColor[1] + 160.0L * haze);
            g_bFogB = FogTruncateToWord((1.0L - haze) * g_LoadedTrack.fogColor[2] + 160.0L * haze);
#else
            fade = 1.0f - haze;
            haze = 160.0f * haze;
            g_bFogR = g_LoadedTrack.fogColor[0] * fade + haze;
            g_bFogG = g_LoadedTrack.fogColor[1] * fade + haze;
            g_bFogB = g_LoadedTrack.fogColor[2] * fade + haze;
#endif /* TGR_PC */
        } else {
            g_bFogR = g_LoadedTrack.fogColor[0];
            g_bFogG = g_LoadedTrack.fogColor[1];
            g_bFogB = g_LoadedTrack.fogColor[2];
        }
        g_bFogIntensity = intensity;
        if (g_dwNumLocalPlayers == 2) {
            g_dwFogMinDistance = 995;
            g_dwFogMaxDistance = 1000;
        } else {
            g_dwFogMinDistance = 980;
            g_dwFogMaxDistance = 1000;
        }
    } else {
        g_bFogR = 0;
        g_bFogG = 0;
        g_bFogB = 0;
        g_bFogIntensity = 0xFF;
        g_dwFogMinDistance = 0;
        g_dwFogMaxDistance = 1000;
    }
    g_dwFogFactorScale = 128000 / (g_dwFogMaxDistance - g_dwFogMinDistance);
    g_dwFogFactorOffset =
        (500 - g_dwFogMinDistance) * 256 / (g_dwFogMaxDistance - g_dwFogMinDistance);
    gSPFogPosition(gDisplayListPtr++, g_dwFogMinDistance, g_dwFogMaxDistance);
    gDPSetFogColor(gDisplayListPtr++, g_bFogR, g_bFogG, g_bFogB, 0xFF);
}

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
#if TGR_PC
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
#endif /* TGR_PC */
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

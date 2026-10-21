#ifndef COASTLINE_WEATHER_H
#define COASTLINE_WEATHER_H

/**
 * @file
 * The weather of a race and the flags the renderer tests.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"

/** The weathers, in the order of the weather menu. */
enum {
    WEATHER_SUNNY,
    WEATHER_FOG,
    WEATHER_RAIN,
    WEATHER_SNOW,
    WEATHER_NIGHT,
};

/** The weather the player chose. @ghidraAddress USA: 0x8028c800 */
extern u32 g_dwCurrentWeather;

/** Whether the weather is anything but sunny. @ghidraAddress USA: 0x8028aa78 */
extern u32 g_dwWeatherActive;

/** @ghidraAddress USA: 0x8028aa80 */
extern u32 g_dwNightMode;

/** @ghidraAddress USA: 0x8028aa84 */
extern u32 g_dwSnowMode;

/** @ghidraAddress USA: 0x8028aa8c */
extern u32 g_dwRainMode;

/** The colour of the fog. @ghidraAddress USA: 0x8028ab20 */
extern u8 g_bFogR;

/** @ghidraAddress USA: 0x8028ab24 */
extern u8 g_bFogG;

/** @ghidraAddress USA: 0x8028ab28 */
extern u8 g_bFogB;

/** How far the fog colour covers the sky. @ghidraAddress USA: 0x8028ab2c */
extern u8 g_bFogIntensity;

/**
 * The frames of lightning left, during which the sky flashes on the odd ones. It stays 0 until
 * the next strike is allowed, and is -1 while a strike may come.
 *
 * @ghidraAddress USA: 0x8028c818
 */
extern s32 g_dwLightningFlashCounter;

/** The views that keep their own rain. */
#define WEATHER_VIEWS 2

/** The rain particles of all views, shared among the views on the screen. */
#define RAIN_PARTICLES 512

/** The particle slots each view keeps, more than the particles it draws. */
#define RAIN_PARTICLE_SLOTS 522

/** The side of the texture of the rain streaks, in texels. */
#define RAIN_STREAK_TEXTURE_SIDE 64

/** The points of the lightning bolt. */
#define LIGHTNING_BOLT_POINTS 459

/**
 * The particles each view draws.
 *
 * @ghidraAddress USA: 0x8028c804
 */
extern s32 g_dwRainParticleCount;

/**
 * The direction the wind blows towards, in radians.
 *
 * @ghidraAddress USA: 0x8028c808
 */
extern f32 g_fWindAngle;

/**
 * The strength of the wind, from 0.5 to 1.
 *
 * @ghidraAddress USA: 0x8028c80c
 */
extern f32 g_fWindStrength;

/**
 * The speed of the camera of the last view through the rain, limited.
 *
 * @ghidraAddress USA: 0x8028c810
 */
extern f32 g_fRainCameraSpeed;

/**
 * The share of its movement the camera of the last view passes on to the rain.
 *
 * @ghidraAddress USA: 0x8028c814
 */
extern f32 g_fRainCameraSpeedScale;

/**
 * Four times the depth of the lightning bolt being divided.
 *
 * @ghidraAddress USA: 0x8028c81c
 */
extern s32 g_nLightningBoltDepth;

/**
 * The time since the last strike, in 343 units a second.
 *
 * @ghidraAddress USA: 0x8028c820
 */
extern f32 g_fLightningFlashTimer;

/**
 * Whether the rain particles have their first positions.
 *
 * @ghidraAddress USA: 0x8028c824
 */
extern s32 g_bRainParticlesInitialized;

/**
 * The texture of the snowflakes and other particles, 64 texels square.
 *
 * @ghidraAddress USA: 0x802a3790
 */
extern u8 g_aParticleTexture[];

/**
 * The positions of the rain particles of each view, in a cube around the camera that wraps.
 *
 * @ghidraAddress USA: 0x80361c40
 */
extern s16 g_awRainParticlePos[WEATHER_VIEWS][RAIN_PARTICLE_SLOTS][3];

/**
 * The point ahead of the camera of each view last frame.
 *
 * @ghidraAddress USA: 0x803634b8
 */
extern Vec3f g_avRainPrevCameraPos[WEATHER_VIEWS];

/**
 * How far the wind moves things in a frame.
 *
 * @ghidraAddress USA: 0x803634d0
 */
extern Vec3f g_vWindVelocity;

/**
 * How far the rain of each view falls in a frame, against its camera.
 *
 * @ghidraAddress USA: 0x803634e0
 */
extern Vec3f g_avRainVelocity[WEATHER_VIEWS];

/**
 * The points of the lightning bolt. The first is the point the lightning strikes.
 *
 * @ghidraAddress USA: 0x803634f8
 */
extern Vec3f g_avLightningBolt[LIGHTNING_BOLT_POINTS];

/**
 * The texture of the rain streaks of each view, drawn along the fall of the rain.
 *
 * @ghidraAddress USA: 0x80364a80
 */
extern u8 g_abPrecipitationTextures[WEATHER_VIEWS]
                                   [RAIN_STREAK_TEXTURE_SIDE * RAIN_STREAK_TEXTURE_SIDE];

/**
 * Scatter the rain particles of every view.
 *
 * @ghidraAddress USA: 0x80239cf0
 */
void InitRandomRainParticles(void);

/**
 * Turn the wind and change its strength at random, and set its velocity for the frame.
 *
 * @ghidraAddress USA: 0x80239d84
 */
void UpdateWindDirection(void);

/**
 * Put a point of the lightning bolt at random near the middle of the points a span before and
 * after it, then divide the two halves the same way.
 *
 * @ghidraAddress USA: 0x80239f28
 */
void SubdivideLightningBolt(s32 index, s32 span);

/**
 * Count down the flash of a strike, and strike at random when the last strike is long enough ago.
 *
 * @ghidraAddress USA: 0x8023a0bc
 */
void UpdateLightningFlash(void);

/**
 * Called each frame of rain before the rain is drawn. It does nothing.
 *
 * @ghidraAddress USA: 0x8023a1c0
 */
void StubWeatherUpdate(void);

/**
 * Move the wind and the lightning, and move the rain or snow of each view against its camera.
 *
 * @ghidraAddress USA: 0x8023a1c8
 */
void UpdatePrecipitationParticles(void);

/**
 * Draw the rain or snow of the current view, unless a placed model shelters the camera.
 *
 * @ghidraAddress USA: 0x8023a784
 */
void RenderPrecipitationEffect(void);

#endif

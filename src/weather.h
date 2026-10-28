#ifndef COASTLINE_WEATHER_H
#define COASTLINE_WEATHER_H

/**
 * @file
 * The weather of a race and the flags the renderer tests.
 */

#include "types.h"
#include "vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The weathers, in the order of the weather menu. */
enum {
    WEATHER_SUNNY, /*!< Clear weather by day. */
    WEATHER_FOG,   /*!< Fog. */
    WEATHER_RAIN,  /*!< Rain. */
    WEATHER_SNOW,  /*!< Snow. */
    WEATHER_NIGHT, /*!< Night. */
};

/**
 * The weather the player chose.
 *
 * @ghidraAddress USA: 0x8028c800
 * @ghidraAddress EUR: 0x8028ef60
 * @ghidraAddress JPN: 0x8028e9b0
 * @ghidraAddress BRD3D: 0x104bbe08
 * @ghidraAddress BRGlide: 0x104b15e8
 */
extern u32 g_dwCurrentWeather;

/**
 * Whether the weather is anything but sunny.
 *
 * @ghidraAddress USA: 0x8028aa78
 * @ghidraAddress EUR: 0x8028cc80
 * @ghidraAddress JPN: 0x8028c750
 * @ghidraAddress BRD3D: 0x106c6618
 * @ghidraAddress BRGlide: 0x106ed6a8
 */
extern u32 g_dwWeatherActive;

/**
 * @ghidraAddress USA: 0x8028aa80
 * @ghidraAddress EUR: 0x8028cc88
 * @ghidraAddress JPN: 0x8028c758
 * @ghidraAddress BRD3D: 0x106c661c
 * @ghidraAddress BRGlide: 0x106ed6ac
 */
extern u32 g_dwNightMode;

/**
 * @ghidraAddress USA: 0x8028aa84
 * @ghidraAddress EUR: 0x8028cc8c
 * @ghidraAddress JPN: 0x8028c75c
 * @ghidraAddress BRD3D: 0x106c6620
 * @ghidraAddress BRGlide: 0x106ed6b0
 */
extern u32 g_dwSnowMode;

/**
 * @ghidraAddress USA: 0x8028aa8c
 * @ghidraAddress EUR: 0x8028cc94
 * @ghidraAddress JPN: 0x8028c764
 * @ghidraAddress BRD3D: 0x106c6624
 * @ghidraAddress BRGlide: 0x106ed6b4
 */
extern u32 g_dwRainMode;

/**
 * The colour of the fog.
 *
 * @ghidraAddress USA: 0x8028ab20
 * @ghidraAddress EUR: 0x8028cd28
 * @ghidraAddress JPN: 0x8028c7f8
 * @ghidraAddress BRD3D: 0x106c0260
 * @ghidraAddress BRGlide: 0x106e72f0
 */
extern u8 g_bFogR;

/**
 * @ghidraAddress USA: 0x8028ab24
 * @ghidraAddress EUR: 0x8028cd2c
 * @ghidraAddress JPN: 0x8028c7fc
 * @ghidraAddress BRD3D: 0x106c1614
 * @ghidraAddress BRGlide: 0x106e86a4
 */
extern u8 g_bFogG;

/**
 * @ghidraAddress USA: 0x8028ab28
 * @ghidraAddress EUR: 0x8028cd30
 * @ghidraAddress JPN: 0x8028c800
 * @ghidraAddress BRD3D: 0x106c0200
 * @ghidraAddress BRGlide: 0x106e7290
 */
extern u8 g_bFogB;

/**
 * How far the fog colour covers the sky.
 *
 * @ghidraAddress USA: 0x8028ab2c
 * @ghidraAddress EUR: 0x8028cd34
 * @ghidraAddress JPN: 0x8028c804
 * @ghidraAddress BRD3D: 0x10690be8
 * @ghidraAddress BRGlide: 0x106b7c78
 */
extern u8 g_bFogIntensity;

/**
 * The frames of lightning left, during which the sky flashes on the odd ones. It stays 0 until
 * the next strike is allowed, and is -1 while a strike may come.
 *
 * @ghidraAddress USA: 0x8028c818
 * @ghidraAddress EUR: 0x8028ef78
 * @ghidraAddress JPN: 0x8028e9c8
 * @ghidraAddress BRD3D: 0x100a79cc
 * @ghidraAddress BRGlide: 0x100a718c
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RAIN_STREAK_TEXTURE_BYTES_PER_TEXEL 4
#else
#define RAIN_STREAK_TEXTURE_BYTES_PER_TEXEL 1
#endif

/** The points of the lightning bolt. */
#define LIGHTNING_BOLT_POINTS 459

/**
 * The particles each view draws.
 *
 * @ghidraAddress USA: 0x8028c804
 * @ghidraAddress EUR: 0x8028ef64
 * @ghidraAddress JPN: 0x8028e9b4
 * @ghidraAddress BRD3D: 0x104b2550
 * @ghidraAddress BRGlide: 0x104add38
 */
extern s32 g_dwRainParticleCount;

/**
 * The direction the wind blows towards, in radians.
 *
 * @ghidraAddress USA: 0x8028c808
 * @ghidraAddress EUR: 0x8028ef68
 * @ghidraAddress JPN: 0x8028e9b8
 * @ghidraAddress BRD3D: 0x104bbe0c
 * @ghidraAddress BRGlide: 0x104b15ec
 */
extern f32 g_fWindAngle;

/**
 * The strength of the wind, from 0.5 to 1.
 *
 * @ghidraAddress USA: 0x8028c80c
 * @ghidraAddress EUR: 0x8028ef6c
 * @ghidraAddress JPN: 0x8028e9bc
 * @ghidraAddress BRD3D: 0x100a79c8
 * @ghidraAddress BRGlide: 0x100a7188
 */
extern f32 g_fWindStrength;

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The speed of the camera of the last view through the rain, limited.
 *
 * @ghidraAddress USA: 0x8028c810
 * @ghidraAddress EUR: 0x8028ef70
 * @ghidraAddress JPN: 0x8028e9c0
 * @ghidraAddress BRD3D: 0x104bbde8
 * @ghidraAddress BRGlide: 0x104b15c8
 */
extern f32 g_fRainCameraSpeed;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The share of its movement the camera of the last view passes on to the rain.
 *
 * @ghidraAddress USA: 0x8028c814
 * @ghidraAddress EUR: 0x8028ef74
 * @ghidraAddress JPN: 0x8028e9c4
 * @ghidraAddress BRD3D: 0x104b1f00
 * @ghidraAddress BRGlide: 0x104ad6e8
 */
extern f32 g_fRainCameraSpeedScale;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

/**
 * Four times the depth of the lightning bolt being divided.
 *
 * @ghidraAddress USA: 0x8028c81c
 * @ghidraAddress EUR: 0x8028ef7c
 * @ghidraAddress JPN: 0x8028e9cc
 */
extern s32 g_nLightningBoltDepth;

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The time since the last strike, in 343 units a second.
 *
 * @ghidraAddress USA: 0x8028c820
 * @ghidraAddress EUR: 0x8028ef80
 * @ghidraAddress JPN: 0x8028e9d0
 * @ghidraAddress BRD3D: 0x104b255c
 * @ghidraAddress BRGlide: 0x104add3c
 */
extern f32 g_fLightningFlashTimer;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * Whether the rain particles have their first positions.
 *
 * @ghidraAddress USA: 0x8028c824
 * @ghidraAddress EUR: 0x8028ef84
 * @ghidraAddress JPN: 0x8028e9d4
 * @ghidraAddress BRD3D: 0x104bbe14
 * @ghidraAddress BRGlide: 0x104b15f4
 */
extern s32 g_bRainParticlesInitialized;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

/**
 * The texture of the snowflakes and other particles, 64 texels square.
 *
 * @ghidraAddress USA: 0x802a3790
 * @ghidraAddress EUR: 0x802a5f50
 * @ghidraAddress JPN: 0x802a59f0
 * @ghidraAddress BRD3D: 0x100baac8
 * @ghidraAddress BRGlide: 0x100ba2d0
 */
extern u8 g_aParticleTexture[];

/** The last texel of the particle texture, in the 10.5 fixed point of a coordinate. */
#define PARTICLE_TEXTURE_LAST_T (63 << 5)
/** The last texel of the particle texture, in the 20.12 fixed point of a step. */
#define PARTICLE_TEXTURE_STEPS (63 << 12)

/* The load of the particle texture. */
#define PARTICLE_LOAD_TEXELS 2047 /*!< The texels to load as 16-bit texels, less one. */
#define PARTICLE_LOAD_DXT 0x100   /*!< The step of the load to the next line of texels. */

/**
 * The positions of the rain particles of each view, in a cube around the camera that wraps.
 *
 * @ghidraAddress USA: 0x80361c40
 * @ghidraAddress EUR: 0x80364140
 * @ghidraAddress JPN: 0x80362db0
 * @ghidraAddress BRD3D: 0x104b2570
 * @ghidraAddress BRGlide: 0x104add50
 */
extern s16 g_awRainParticlePos[WEATHER_VIEWS][RAIN_PARTICLE_SLOTS][3];

/**
 * The point ahead of the camera of each view last frame.
 *
 * @ghidraAddress USA: 0x803634b8
 * @ghidraAddress EUR: 0x803659b8
 * @ghidraAddress JPN: 0x80364628
 * @ghidraAddress BRD3D: 0x104b0688
 * @ghidraAddress BRGlide: 0x104abe70
 */
extern Vec3f g_avRainPrevCameraPos[WEATHER_VIEWS];

/**
 * How far the wind moves things in a frame.
 *
 * @ghidraAddress USA: 0x803634d0
 * @ghidraAddress EUR: 0x803659d0
 * @ghidraAddress JPN: 0x80364640
 * @ghidraAddress BRD3D: 0x104b2560
 * @ghidraAddress BRGlide: 0x104add40
 */
extern Vec3f g_vWindVelocity;

/**
 * How far the rain of each view falls in a frame, against its camera.
 *
 * @ghidraAddress USA: 0x803634e0
 * @ghidraAddress EUR: 0x803659e0
 * @ghidraAddress JPN: 0x80364650
 * @ghidraAddress BRD3D: 0x104bbdf0
 * @ghidraAddress BRGlide: 0x104b15d0
 */
extern Vec3f g_avRainVelocity[WEATHER_VIEWS];

/**
 * The points of the lightning bolt. The first is the point the lightning strikes.
 *
 * @ghidraAddress USA: 0x803634f8
 * @ghidraAddress EUR: 0x803659f8
 * @ghidraAddress JPN: 0x80364668
 * @ghidraAddress BRD3D: 0x104b0378
 * @ghidraAddress BRGlide: 0x104abb60
 */
extern Vec3f g_avLightningBolt[LIGHTNING_BOLT_POINTS];

/**
 * The texture of the rain streaks of each view, drawn along the fall of the rain.
 *
 * @ghidraAddress USA: 0x80364a80
 * @ghidraAddress EUR: 0x80366f80
 * @ghidraAddress JPN: 0x80365bf0
 * @ghidraAddress BRD3D: 0x104b3de8
 * @ghidraAddress BRGlide: 0x104af5c8
 */
extern u8 g_abPrecipitationTextures[WEATHER_VIEWS][RAIN_STREAK_TEXTURE_SIDE *
                                                   RAIN_STREAK_TEXTURE_SIDE *
                                                   RAIN_STREAK_TEXTURE_BYTES_PER_TEXEL];

/**
 * Scatter the rain particles of every view.
 *
 * @ghidraAddress USA: 0x80239cf0
 * @ghidraAddress EUR: 0x8023b200
 * @ghidraAddress JPN: 0x8023ad00
 * @ghidraAddress BRD3D: 0x10019490
 * @ghidraAddress BRGlide: 0x10016a50
 */
void InitRandomRainParticles(void);

/**
 * Turn the wind and change its strength at random, and set its velocity for the frame.
 *
 * @ghidraAddress USA: 0x80239d84
 * @ghidraAddress EUR: 0x8023b294
 * @ghidraAddress JPN: 0x8023ad94
 * @ghidraAddress BRD3D: 0x100194e0
 * @ghidraAddress BRGlide: 0x10016aa0
 */
void UpdateWindDirection(void);

/**
 * Put a point of the lightning bolt at random near the middle of the points a span before and
 * after it, then divide the two halves the same way.
 *
 * @param index The index of the point in the bolt.
 * @param span The distance in points to the neighbours, halved at each level.
 * @ghidraAddress USA: 0x80239f28
 * @ghidraAddress EUR: 0x8023b438
 * @ghidraAddress JPN: 0x8023af38
 */
void SubdivideLightningBolt(s32 index, s32 span);

/**
 * Count down the flash of a strike, and strike at random when the last strike is long enough ago.
 *
 * @ghidraAddress USA: 0x8023a0bc
 * @ghidraAddress EUR: 0x8023b5cc
 * @ghidraAddress JPN: 0x8023b0cc
 * @ghidraAddress BRD3D: 0x10019620
 * @ghidraAddress BRGlide: 0x10016be0
 */
void UpdateLightningFlash(void);

/**
 * Called each frame of rain before the rain is drawn. It does nothing.
 *
 * @ghidraAddress USA: 0x8023a1c0
 * @ghidraAddress EUR: 0x8023b6d0
 * @ghidraAddress JPN: 0x8023b1d0
 */
void StubWeatherUpdate(void);

/**
 * Move the wind and the lightning, and move the rain or snow of each view against its camera.
 *
 * @ghidraAddress USA: 0x8023a1c8
 * @ghidraAddress EUR: 0x8023b6d8
 * @ghidraAddress JPN: 0x8023b1d8
 * @ghidraAddress BRD3D: 0x100196d0
 * @ghidraAddress BRGlide: 0x10016c90
 */
void UpdatePrecipitationParticles(void);

/**
 * Draw the rain or snow of the current view, unless a placed model shelters the camera.
 *
 * @ghidraAddress USA: 0x8023a784
 * @ghidraAddress EUR: 0x8023bc94
 * @ghidraAddress JPN: 0x8023b794
 * @ghidraAddress BRD3D: 0x10019b50
 */
void RenderPrecipitationEffect(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_WEATHER_H */

#pragma once

/**
 * @file
 * The settings of the PC port that BossRally.ini and the command line set.
 */

#include "common/game.h"
#include "common/trackselect.h"
#include "win32/assetnames.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x100b8c90
 * @ghidraAddress BRGlide: 0x100b8498
 */
extern int g_dwTextureQualityLevel;

/**
 * Path of BossRally.ini, built from g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x1022af48
 * @ghidraAddress BRGlide: 0x10226a78
 */
extern char g_szBossRallyIniPath[];

/**
 * The mode of network play, from `NetworkPlay=`.
 *
 * @ghidraAddress BRD3D: 0x1022af18
 * @ghidraAddress BRGlide: 0x10226a48
 */
extern int g_dwNetworkPlayMode;

/**
 * The car chosen, from `chosenCar=`.
 *
 * @ghidraAddress BRD3D: 0x1022b34c
 * @ghidraAddress BRGlide: 0x10226e7c
 */
extern int g_dwChosenCar;

/**
 * The weather chosen, from `chosenWeather=`.
 *
 * @ghidraAddress BRD3D: 0x1022b350
 * @ghidraAddress BRGlide: 0x10226e80
 */
extern int g_dwChosenWeather;

/**
 * The handling of the car, from `HandlingType=`.
 *
 * @ghidraAddress BRD3D: 0x10094350
 * @ghidraAddress BRGlide: 0x1007b320
 */
extern int g_dwHandlingType;

/**
 * The transmission of the car, from `TransmissionType=`.
 *
 * @ghidraAddress BRD3D: 0x10094354
 * @ghidraAddress BRGlide: 0x1007b324
 */
extern int g_dwTransmissionType;

/**
 * The suspension of the car, from `SuspensionType=`.
 *
 * @ghidraAddress BRD3D: 0x10094358
 * @ghidraAddress BRGlide: 0x1007b328
 */
extern int g_dwSuspensionType;

/**
 * The tires of the car, from `TireType=`.
 *
 * @ghidraAddress BRD3D: 0x1009435c
 * @ghidraAddress BRGlide: 0x1007b32c
 */
extern int g_dwTireType;

/**
 * The directory of the tracks, from `TrackDir=`.
 *
 * @ghidraAddress BRD3D: 0x100b7cb8
 * @ghidraAddress BRGlide: 0x100b74c0
 */
extern char g_szTracks[];

/**
 * The directory of the cars, from `CarDir=`.
 *
 * @ghidraAddress BRD3D: 0x100b80f8
 * @ghidraAddress BRGlide: 0x100b7900
 */
extern char g_szCarsDirPath[];

/**
 * The directory of the sound effects, from `SFXDir=`.
 *
 * @ghidraAddress BRD3D: 0x100b8538
 * @ghidraAddress BRGlide: 0x100b7d40
 */
extern char g_szSfxDirPath[];

/**
 * Whether frames are interpolated, from `Interpolate=`.
 *
 * @ghidraAddress BRD3D: 0x100a66ec
 * @ghidraAddress BRGlide: 0x100a5eac
 */
extern int g_bInterpolateFrames;

/**
 * Whether speed reduces analogue steering lock, from `SpeedSensitive=`.
 *
 * @ghidraAddress BRD3D: 0x100b3664
 * @ghidraAddress BRGlide: 0x100b2e6c
 */
extern int g_bSpeedSensitiveSteering;

/**
 * Whether the shadow of a car is not drawn, the inverse of `D3DDrawCarShadow=`.
 *
 * @ghidraAddress BRD3D: 0x1039b6c8
 * @ghidraAddress BRGlide: 0x10396eb0
 */
extern int g_bDisableCarShadow;

/**
 * Whether the benchmark runs, from `RunBenchmark=`.
 *
 * @ghidraAddress BRD3D: 0x118abdc4
 * @ghidraAddress BRGlide: 0x118eeedc
 */
extern int g_dwRunBenchmark;

/**
 * How music plays, from `PlayMusic=`.
 *
 * @ghidraAddress BRD3D: 0x100940a4
 * @ghidraAddress BRGlide: 0x1007b074
 */
extern int g_dwPlayMusicMode;

/**
 * Whether sound effects play, from `PlaySFX=`.
 *
 * @ghidraAddress BRD3D: 0x100b5de8
 * @ghidraAddress BRGlide: 0x100b55f0
 */
extern int g_bSoundEnabledIni;

/**
 * Whether Direct3D tests the alpha of a pixel, from `D3DAlphaCompare=`.
 *
 * @ghidraAddress BRD3D: 0x100aa720
 */
extern int g_bD3DAlphaCompare;

/**
 * Whether textures are made square, from `D3DAlwaysSquareTextures=`.
 *
 * @ghidraAddress BRD3D: 0x10575420
 */
extern int g_dwD3DAlwaysSquareTextures;

/**
 * Whether the depth buffer is cleared each frame, from `D3DClearZBuffer=`.
 *
 * @ghidraAddress BRD3D: 0x104c5168
 */
extern int g_dwD3DClearZBuffer;

/**
 * Whether a flip waits until the surface can flip, from `D3DWaitCanFlip=`.
 *
 * @ghidraAddress BRD3D: 0x100a62f8
 */
extern int g_bD3DWaitCanFlip;

/**
 * Whether a flip waits until it is done, from `D3DWaitFlipDone=`.
 *
 * @ghidraAddress BRD3D: 0x100a62fc
 */
extern int g_bD3DWaitFlipDone;

/**
 * The blend of the destination of a translucent pixel, `D3DBLEND_INVSRCALPHA` when
 * `D3DInvSrcAlpha=` is set and `D3DBLEND_ONE` otherwise.
 *
 * @ghidraAddress BRD3D: 0x100a79d8
 */
extern int g_dwD3DDestBlendMode;

/**
 * The most players of a network game, from `cPlayers=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x102212c8
 * @ghidraAddress BRGlide: 0x1021cdf8
 */
extern int g_bNetMaxPlayers;

/**
 * The cars a network game allows, from `bcar=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x10221320
 * @ghidraAddress BRGlide: 0x1021ce50
 */
extern int g_wNetAllowedCarsMask;

/**
 * The tires of a network game, from `btire=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x1022af10
 * @ghidraAddress BRGlide: 0x10226a40
 */
extern int g_bNetTireSetting;

/**
 * The suspension of a network game, from `bsuspension=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x1022af0c
 * @ghidraAddress BRGlide: 0x10226a3c
 */
extern int g_bNetSuspensionSetting;

/**
 * The width of the display mode set.
 *
 * @ghidraAddress BRD3D: 0x100a81c0
 * @ghidraAddress BRGlide: 0x100a7514
 */
extern int g_dwConfigFramebufferWidth;

/**
 * The height of the display mode set.
 *
 * @ghidraAddress BRD3D: 0x100a81c4
 * @ghidraAddress BRGlide: 0x100a7518
 */
extern int g_dwConfigFramebufferHeight;

#ifdef __cplusplus
}
#endif

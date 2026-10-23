#pragma once

/**
 * @file
 * The settings of the PC port that BossRally.ini and the command line set.
 */

#include <windows.h>

/**
 * Path of BossRally.ini, built from g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x1022af48
 */
extern char g_szBossRallyIniPath[];

/**
 * The mode of network play, from `NetworkPlay=`.
 *
 * @ghidraAddress BRD3D: 0x1022af18
 */
extern DWORD g_dwNetworkPlayMode;

/**
 * The track chosen, from `chosenTrack=`.
 *
 * @ghidraAddress BRD3D: 0x100b380c
 */
extern DWORD g_dwSelectedTrackId;

/**
 * The car chosen, from `chosenCar=`.
 *
 * @ghidraAddress BRD3D: 0x1022b34c
 */
extern DWORD g_dwChosenCar;

/**
 * The weather chosen, from `chosenWeather=`.
 *
 * @ghidraAddress BRD3D: 0x1022b350
 */
extern DWORD g_dwChosenWeather;

/**
 * The mode of the game, from `gameMode=`.
 *
 * @ghidraAddress BRD3D: 0x100aa010
 */
extern DWORD g_dwGameMode;

/**
 * The handling of the car, from `HandlingType=`.
 *
 * @ghidraAddress BRD3D: 0x10094350
 */
extern DWORD g_dwHandlingType;

/**
 * The transmission of the car, from `TransmissionType=`.
 *
 * @ghidraAddress BRD3D: 0x10094354
 */
extern DWORD g_dwTransmissionType;

/**
 * The suspension of the car, from `SuspensionType=`.
 *
 * @ghidraAddress BRD3D: 0x10094358
 */
extern DWORD g_dwSuspensionType;

/**
 * The tires of the car, from `TireType=`.
 *
 * @ghidraAddress BRD3D: 0x1009435c
 */
extern DWORD g_dwTireType;

/**
 * The directory of the tracks, from `TrackDir=`.
 *
 * @ghidraAddress BRD3D: 0x100b7cb8
 */
extern char g_szTracks[];

/**
 * The directory of the cars, from `CarDir=`.
 *
 * @ghidraAddress BRD3D: 0x100b80f8
 */
extern char g_szCarsDirPath[];

/**
 * The directory of the sound effects, from `SFXDir=`.
 *
 * @ghidraAddress BRD3D: 0x100b8538
 */
extern char g_szSfxDirPath[];

/**
 * Whether frames are interpolated, from `Interpolate=`.
 *
 * @ghidraAddress BRD3D: 0x100a66ec
 */
extern BOOL g_bInterpolateFrames;

/**
 * Whether the gearbox is automatic, from `SpeedSensitive=`.
 *
 * @ghidraAddress BRD3D: 0x100b3664
 */
extern BOOL g_bAutomaticGearboxIni;

/**
 * Whether the shadow of a car is not drawn, the inverse of `D3DDrawCarShadow=`.
 *
 * @ghidraAddress BRD3D: 0x1039b6c8
 */
extern BOOL g_bDisableCarShadow;

/**
 * Whether the benchmark runs, from `RunBenchmark=`.
 *
 * @ghidraAddress BRD3D: 0x118abdc4
 */
extern DWORD g_dwRunBenchmark;

/**
 * How music plays, from `PlayMusic=`.
 *
 * @ghidraAddress BRD3D: 0x100940a4
 */
extern DWORD g_dwPlayMusicMode;

/**
 * Whether sound effects play, from `PlaySFX=`.
 *
 * @ghidraAddress BRD3D: 0x100b5de8
 */
extern BOOL g_bSoundEnabledIni;

/**
 * Whether Direct3D tests the alpha of a pixel, from `D3DAlphaCompare=`.
 *
 * @ghidraAddress BRD3D: 0x100aa720
 */
extern BOOL g_bD3DAlphaCompare;

/**
 * Whether textures are made square, from `D3DAlwaysSquareTextures=`.
 *
 * @ghidraAddress BRD3D: 0x10575420
 */
extern DWORD g_dwD3DAlwaysSquareTextures;

/**
 * Whether the depth buffer is cleared each frame, from `D3DClearZBuffer=`.
 *
 * @ghidraAddress BRD3D: 0x104c5168
 */
extern DWORD g_dwD3DClearZBuffer;

/**
 * Whether a flip waits until the surface can flip, from `D3DWaitCanFlip=`.
 *
 * @ghidraAddress BRD3D: 0x100a62f8
 */
extern BOOL g_bD3DWaitCanFlip;

/**
 * Whether a flip waits until it is done, from `D3DWaitFlipDone=`.
 *
 * @ghidraAddress BRD3D: 0x100a62fc
 */
extern BOOL g_bD3DWaitFlipDone;

/**
 * The blend of the destination of a translucent pixel, `D3DBLEND_INVSRCALPHA` when
 * `D3DInvSrcAlpha=` is set and `D3DBLEND_ONE` otherwise.
 *
 * @ghidraAddress BRD3D: 0x100a79d8
 */
extern DWORD g_dwD3DDestBlendMode;

/**
 * The most players of a network game, from `cPlayers=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x102212c8
 */
extern DWORD g_bNetMaxPlayers;

/**
 * The cars a network game allows, from `bcar=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x10221320
 */
extern DWORD g_wNetAllowedCarsMask;

/**
 * The tires of a network game, from `btire=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x1022af10
 */
extern DWORD g_bNetTireSetting;

/**
 * The suspension of a network game, from `bsuspension=` on the command line.
 *
 * @ghidraAddress BRD3D: 0x1022af0c
 */
extern DWORD g_bNetSuspensionSetting;

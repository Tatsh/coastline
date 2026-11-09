#ifndef COASTLINE_VEHICLE_VIEW_H
#define COASTLINE_VEHICLE_VIEW_H

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The 32 by 128 textures of the reflection of the sky, of 4-bit colour indices, and their palettes.
 *
 * @ghidraAddress USA: 0x8028c308
 * @ghidraAddress Asia: 0x8028BE58
 * @ghidraAddress EUR: 0x8028ea68
 * @ghidraAddress JPN: 0x8028e4b8
 * @ghidraAddress BRD3D: 0x100a6498
 */
#if TGR_BOSS_RALLY
typedef union VehicleReflectionTexture {
    u8 *image;
    unsigned entry;
} VehicleReflectionTexture;
#if TGR_PLATFORM != TGR_PLATFORM_SDL
extern VehicleReflectionTexture g_apVehicleReflectionTextures[4];
#endif
#else
extern u8 *g_apVehicleReflectionTextures[4];
#endif

/**
 * @ghidraAddress USA: 0x8028c318
 * @ghidraAddress Asia: 0x8028BE68
 * @ghidraAddress EUR: 0x8028ea78
 * @ghidraAddress JPN: 0x8028e4c8
 */
extern u16 *g_apVehicleReflectionTluts[4];

/**
 * The reflection of each view mode, in the open and in a tunnel.
 *
 * @ghidraAddress USA: 0x8028c32c
 * @ghidraAddress Asia: 0x8028BE7C
 * @ghidraAddress EUR: 0x8028ea8c
 * @ghidraAddress JPN: 0x8028e4dc
 * @ghidraAddress BRD3D: 0x100a64b8
 * @ghidraAddress BRGlide: 0x100a5c78
 */
#if TGR_BOSS_RALLY
#if TGR_PLATFORM != TGR_PLATFORM_SDL
extern s8 g_abVehicleReflectionTexSelect[][2];
#endif
#else
extern u8 g_abVehicleReflectionTexSelect[4][2];
#endif

/**
 * Set to 0 by the track menu.
 *
 * @ghidraAddress USA: 0x8028c328
 * @ghidraAddress Asia: 0x8028BE78
 * @ghidraAddress EUR: 0x8028ea88
 * @ghidraAddress JPN: 0x8028e4d8
 * @ghidraAddress BRD3D: 0x10277ea0
 * @ghidraAddress BRGlide: 0x10273688
 */
extern s32 g_dwVehicleViewModeIndex;

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** Sky images and reflection descriptors for one SDL texture collection. */
typedef struct {
    u8 daySky[2048];                      /*!< The daytime sky reflection image. */
    u8 nightSky[2048];                    /*!< The night-time sky reflection image. */
    VehicleReflectionTexture textures[4]; /*!< The reflection texture descriptors. */
    s8 selection[3][2];                   /*!< The selected reflections by view and sky. */
} SdlVehicleReflections;

/** @return Reflection storage for the active SDL texture collection. */
SdlVehicleReflections *GetSdlVehicleReflections(void);

#define g_abDaySkyTexture (GetSdlVehicleReflections()->daySky)
#define g_abNightSkyTexture (GetSdlVehicleReflections()->nightSky)
#define g_apVehicleReflectionTextures (GetSdlVehicleReflections()->textures)
#define g_abVehicleReflectionTexSelect (GetSdlVehicleReflections()->selection)
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLE_VIEW_H */

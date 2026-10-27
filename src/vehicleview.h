#ifndef COASTLINE_VEHICLE_VIEW_H
#define COASTLINE_VEHICLE_VIEW_H

#include <PR/ultratypes.h>

/**
 * The 32 by 128 textures of the reflection of the sky, of 4-bit colour indices, and their palettes.
 *
 * @ghidraAddress USA: 0x8028c308
 * @ghidraAddress EUR: 0x8028ea68
 * @ghidraAddress JPN: 0x8028e4b8
 * @ghidraAddress BRD3D: 0x100a6498
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern unsigned g_apVehicleReflectionTextures[4];
#else
extern u8 *g_apVehicleReflectionTextures[4];
#endif

/**
 * @ghidraAddress USA: 0x8028c318
 * @ghidraAddress EUR: 0x8028ea78
 * @ghidraAddress JPN: 0x8028e4c8
 */
extern u16 *g_apVehicleReflectionTluts[4];

/**
 * The reflection of each view mode, in the open and in a tunnel.
 *
 * @ghidraAddress USA: 0x8028c32c
 * @ghidraAddress EUR: 0x8028ea8c
 * @ghidraAddress JPN: 0x8028e4dc
 * @ghidraAddress BRD3D: 0x100a64b8
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern s8 g_abVehicleReflectionTexSelect[][2];
#else
extern u8 g_abVehicleReflectionTexSelect[4][2];
#endif

/**
 * Set to 0 by the track menu.
 *
 * @ghidraAddress USA: 0x8028c328
 * @ghidraAddress EUR: 0x8028ea88
 * @ghidraAddress JPN: 0x8028e4d8
 * @ghidraAddress BRD3D: 0x10277ea0
 */
extern s32 g_dwVehicleViewModeIndex;

#endif

#pragma once

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Save volume settings and clear network input state before game setup. */
void PrepareWindowsGameSession(void);
/** Initialise the Windows race callback and menu state. */
void PrepareWindowsGameMenus(void);
/** Load a menu configuration source with optional-file handling. */
s32 LoadWindowsMenuData(s32 dataType);
/** Apply configured audio volumes. */
void ApplyWindowsMenuVolumeSettings(void);
/** Apply the selected race and equipment settings after menu configuration loading. */
void FinishWindowsGameSessionSetup(void);

/**
 * @ghidraAddress BRD3D: 0x106c56d0
 * @ghidraAddress BRGlide: 0x106ec760
 */
extern s32 g_dwSavedSfxVolume;
/**
 * @ghidraAddress BRD3D: 0x106c29a4
 * @ghidraAddress BRGlide: 0x106e9a34
 */
extern s32 g_dwSavedMusicVolume;
/**
 * @ghidraAddress BRD3D: 0x106c664c
 * @ghidraAddress BRGlide: 0x106ed6dc
 */
extern s32 g_dwUnk_InitGameAndShowMenus_106c664c;
/**
 * @ghidraAddress BRD3D: 0x10b4df28
 * @ghidraAddress BRGlide: 0x10b71288
 */
extern s32 g_dwUnk_InitGameAndShowMenus_10b4df28;

#ifdef __cplusplus
}
#endif

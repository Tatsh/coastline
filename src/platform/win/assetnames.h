#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress BRD3D: 0x100b89c8 */
extern const char *g_apszBgmTrackNames[12];

/** @ghidraAddress BRD3D: 0x100b80b8 */
extern const char *g_apszTrackFilenames[15];

/**
 * The null sound prefix precedes the sixteen car prefixes in shared storage.
 *
 * @ghidraAddress BRD3D: 0x100b84f4
 * @ghidraAddress BRGlide: 0x100b7cfc
 */
extern const char *const g_apszSfxNamePrefixes[17];

/** @ghidraAddress BRD3D: 0x100b84f8 */
#define g_apszCarModelPrefixes (g_apszSfxNamePrefixes + 1)

/**
 * The empty string of the user interface.
 *
 * @ghidraAddress BRD3D: 0x1039b720
 * @ghidraAddress BRGlide: 0x10396f08
 */
extern const char g_szEmptyUiString[];

#define SOUND_SET_FRONT_END_COUNT 9
#define SOUND_SET_RACE_COUNT 25

/**
 * @ghidraAddress BRD3D: 0x100b8938
 * @ghidraAddress BRGlide: 0x100b8140
 */
extern const char *const g_SoundSetTableA[SOUND_SET_RACE_COUNT];

/**
 * @ghidraAddress BRD3D: 0x100b89a0
 * @ghidraAddress BRGlide: 0x100b81a8
 */
extern const char *const g_SoundSetTableB[SOUND_SET_FRONT_END_COUNT];

#ifdef __cplusplus
}
#endif

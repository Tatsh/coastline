#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** The twelve display labels for the PC background-music tracks.
 * @ghidraAddress BRD3D: 0x100b89c8
 */
extern const char *g_apszBgmTrackNames[12];

/**
 * The PC track filenames, indexed by the race track selector.
 *
 * @ghidraAddress BRD3D: 0x100b80b8
 * @ghidraAddress BRGlide: 0x100b78c0
 */
extern const char *g_apszTrackFilenames[15];

/**
 * The null sound prefix precedes the sixteen car prefixes in shared storage.
 *
 * @ghidraAddress BRD3D: 0x100b84f4
 * @ghidraAddress BRGlide: 0x100b7cfc
 */
extern const char *const g_apszSfxNamePrefixes[17];

/**
 * The sixteen car asset prefixes, excluding the null-sound entry in shared prefix storage.
 *
 * @ghidraAddress BRD3D: 0x100b84f8
 * @ghidraAddress BRGlide: 0x100b7d00
 */
#define g_apszCarModelPrefixes (g_apszSfxNamePrefixes + 1)

/**
 * The empty string of the user interface.
 *
 * @ghidraAddress BRD3D: 0x1039b720
 * @ghidraAddress BRGlide: 0x10396f08
 */
extern const char g_szEmptyUiString[];

/** The front-end sound-table capacity, including its two null boundary entries. */
#define SOUND_SET_FRONT_END_COUNT 9
/** The race sound-table capacity, including its two null boundary entries. */
#define SOUND_SET_RACE_COUNT 25

/**
 * The PC race WAV filenames, indexed by the race sample selector, with null boundary entries.
 *
 * @ghidraAddress BRD3D: 0x100b8938
 * @ghidraAddress BRGlide: 0x100b8140
 */
extern const char *const g_SoundSetTableA[SOUND_SET_RACE_COUNT];

/**
 * The PC front-end WAV filenames, indexed by the menu sample selector, with null boundary entries.
 *
 * @ghidraAddress BRD3D: 0x100b89a0
 * @ghidraAddress BRGlide: 0x100b81a8
 */
extern const char *const g_SoundSetTableB[SOUND_SET_FRONT_END_COUNT];

#ifdef __cplusplus
}
#endif

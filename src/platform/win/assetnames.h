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

#ifdef __cplusplus
}
#endif

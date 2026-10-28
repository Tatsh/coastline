#pragma once

/**
 * @file
 * The loading of saved profiles, ghosts, and binary configuration blocks.
 */

/** The configuration sources accepted by LoadConfigData(). */
enum ConfigLoadType {
    CONFIG_LOAD_CURRENT_SEASON = 0, /*!< Apply the current season profile. */
    CONFIG_LOAD_GHOST = 1,          /*!< Read the selected ghost file. */
    CONFIG_LOAD_EQUIPMENT = 2,      /*!< Read both configuration blocks and apply equipment. */
    CONFIG_LOAD_RAW = 3,            /*!< Read both configuration blocks without applying them. */
    CONFIG_LOAD_SEASON_FILE = 4,    /*!< Read the selected season file. */
};

/** The shared argument slot for an optional-load flag or a custom path. */
union ConfigLoadArgument {
    unsigned value;   /*!< The flag, or the byte count for a custom read. */
    const char *path; /*!< The path for an unrecognised configuration type. */

    ConfigLoadArgument(bool bAllowFailure) : value(bAllowFailure ? 1 : 0) {
    }
    ConfigLoadArgument(const char *pszPath) : path(pszPath) {
    }
};

/**
 * The first persisted configuration value, unused after loading.
 *
 * @ghidraAddress BRD3D: 0x100adf58
 * @ghidraAddress BRGlide: 0x100ad760
 */
extern int g_dwConfigValue0;

/**
 * The second persisted configuration value, unused after loading.
 *
 * @ghidraAddress BRD3D: 0x100adf5c
 * @ghidraAddress BRGlide: 0x100ad764
 */
extern int g_dwConfigValue1;

/**
 * Whether the speed display uses miles per hour.
 *
 * @ghidraAddress BRD3D: 0x100adf60
 * @ghidraAddress BRGlide: 0x100ad768
 */
extern int g_bSpeedUnitsMph;

/**
 * The path used for binary configuration reads.
 *
 * @ghidraAddress BRD3D: 0x100b5dd0
 * @ghidraAddress BRGlide: 0x100b55d8
 */
extern const char g_szCRallyConfigDat[];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load the selected profile, ghost, or binary configuration data.
 *
 * @param nConfigType A #ConfigLoadType value, or a custom-read type.
 * @param argument The optional-load flag for known types, or a custom path whose pointer bits
 * also provide the read size.
 * @return Whether loading succeeded. An open failure returns the low-byte flag; short reads fail.
 * @ghidraAddress BRD3D: 0x10071130
 * @ghidraAddress BRGlide: 0x1006a080
 */
bool LoadConfigData(int nConfigType, ConfigLoadArgument argument);
#ifdef __cplusplus
}
#endif

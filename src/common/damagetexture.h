#ifndef COASTLINE_DAMAGETEXTURE_H
#define COASTLINE_DAMAGETEXTURE_H

/**
 * @file
 * The damage textures of the vehicles of the PC release.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** The highest level of damage of IncrementDamageTextureLevel(). */
#define DAMAGE_TEXTURE_MAX_LEVEL 3

/** The number of damage textures of IncrementDamageTextureLevel(). */
#define DAMAGE_TEXTURE_COUNT 8

/**
 * Raise the level of damage of a damage texture by one, up to #DAMAGE_TEXTURE_MAX_LEVEL, and load
 * the texture again.
 *
 * @param nTexture The damage texture, below #DAMAGE_TEXTURE_COUNT.
 * @param pnEntries The entry of each damage texture.
 * @param pnLevels The level of damage of each damage texture.
 * @ghidraAddress BRD3D: 0x10074ed0
 * @ghidraAddress BRGlide: 0x1006e130
 */
void IncrementDamageTextureLevel(int nTexture, const int *pnEntries, int *pnLevels);

/**
 * Raise the level of damage of a damage texture or of one of its two neighbours, wrapping at the
 * ends, chosen at random from those below #DAMAGE_TEXTURE_MAX_LEVEL.
 *
 * @param nTexture The damage texture, below #DAMAGE_TEXTURE_COUNT.
 * @param pnEntries The entry of each damage texture.
 * @param pnLevels The level of damage of each damage texture.
 * @ghidraAddress BRD3D: 0x10074e40
 * @ghidraAddress BRGlide: 0x1006e0a0
 */
void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels);

/**
 * The texture entries of the damage textures of the vehicles.
 *
 * @ghidraAddress BRD3D: 0x11829850
 * @ghidraAddress BRGlide: 0x1186c968
 */
extern int g_anDamageTextureEntries[DAMAGE_TEXTURE_COUNT];

/**
 * Clear g_anDamageTextureEntries.
 *
 * @ghidraAddress BRD3D: 0x10074e00
 * @ghidraAddress BRGlide: 0x1006e060
 */
void ClearDamageTextureEntries(void);

/**
 * Copy g_anDamageTextureEntries.
 *
 * @param pnEntries Receives the #DAMAGE_TEXTURE_COUNT texture entries.
 * @ghidraAddress BRD3D: 0x10074e20
 * @ghidraAddress BRGlide: 0x1006e080
 */
void CopyDamageTextureEntries(int *pnEntries);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_DAMAGETEXTURE_H */

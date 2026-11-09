#pragma once

#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern "C" {

/**
 * The imported daytime sky palette used for car reflections.
 *
 * @ghidraAddress BRD3D: 0x118aa0d8
 * @ghidraAddress BRGlide: 0x118ed1f0
 */
extern unsigned char g_abDaySkyPalette[32];

/**
 * The imported nighttime sky palette used for car reflections.
 *
 * @ghidraAddress BRD3D: 0x118ab0f8
 * @ghidraAddress BRGlide: 0x118ee210
 */
extern unsigned char g_abNightSkyPalette[32];

/**
 * Create the daytime sky reflection texture in the first reflection slot.
 *
 * @ghidraAddress BRD3D: 0x10073ac0
 * @ghidraAddress BRGlide: 0x1006c910
 */
void CreateDaySkyTexture(void);

/**
 * Create the nighttime sky reflection texture in the second and third reflection slots.
 *
 * @ghidraAddress BRD3D: 0x10073b00
 * @ghidraAddress BRGlide: 0x1006c950
 */
void CreateNightSkyTexture(void);
}
#elif TGR_PLATFORM == TGR_PLATFORM_SDL
namespace coastline {

/**
 * Set the texture segment of the loaded track before relocating its textures.
 *
 * @param textures The borrowed segment base, valid until the track is unloaded or replaced.
 */
void SetTrackTextureSource(unsigned char *textures);

} // namespace coastline
#endif

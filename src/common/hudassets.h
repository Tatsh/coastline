#ifndef COASTLINE_HUDASSETS_H
#define COASTLINE_HUDASSETS_H

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "common/vehicleview.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** The first large-font glyph pixel buffer. @ghidraAddress BRD3D: 0x1009b4c8 */
extern unsigned char g_abHudFont0GlyphPixels[28160];
/** The second large-font glyph pixel buffer. @ghidraAddress BRD3D: 0x100946c8 */
extern unsigned char g_abHudFont1GlyphPixels[28160];
/** The first small-font glyph pixel buffer. @ghidraAddress BRD3D: 0x100a4170 */
extern unsigned char g_abHudFont2GlyphPixels[7840];
/** The second small-font glyph pixel buffer. @ghidraAddress BRD3D: 0x100a22d0 */
extern unsigned char g_abHudFont3GlyphPixels[7840];
/**
 * Pixel data of the HUD overlay texture.
 *
 * @ghidraAddress BRD3D: 0x100baac8
 * @ghidraAddress BRGlide: 0x100ba2d0
 */
extern unsigned char g_abHudOverlayTextureData[4096];
/**
 * Pixel data of the first 64-pixel HUD texture.
 *
 * @ghidraAddress BRD3D: 0x100b6ca8
 * @ghidraAddress BRGlide: 0x100b64b0
 */
extern unsigned char g_abHudTexture64AData[4096];
/**
 * Pixel data of the 32-by-64-pixel HUD texture.
 *
 * @ghidraAddress BRD3D: 0x100b9cb0
 * @ghidraAddress BRGlide: 0x100b94b8
 */
extern unsigned char g_abHudTexture32x64Data[1024];
/**
 * Pixel data of the second 64-pixel HUD texture.
 *
 * @ghidraAddress BRD3D: 0x100b94a8
 * @ghidraAddress BRGlide: 0x100b8cb0
 */
extern unsigned char g_abHudTexture64BData[2048];
/**
 * Pixel data of the fallback environment texture.
 *
 * @ghidraAddress BRD3D: 0x100b8ca0
 * @ghidraAddress BRGlide: 0x100b84a8
 */
extern unsigned char g_abFallbackEnvTextureData[2048];
/** The tunnel window reflection texture. @ghidraAddress BRD3D: 0x100ba0b8 */
extern unsigned char g_abTunnelReflectionTexture[2048];
#if TGR_PLATFORM != TGR_PLATFORM_SDL
/**
 * Pixel data of the day sky texture.
 *
 * @ghidraAddress BRD3D: 0x118aa8f8
 * @ghidraAddress BRGlide: 0x118eda10
 */
extern unsigned char g_abDaySkyTexture[2048];
/**
 * Pixel data of the night sky texture.
 *
 * @ghidraAddress BRD3D: 0x118aa0f8
 * @ghidraAddress BRGlide: 0x118ed210
 */
extern unsigned char g_abNightSkyTexture[2048];
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_HUDASSETS_H */

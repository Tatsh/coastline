#ifndef COASTLINE_PLATFORM_WIN_GLIDE_TEXTURE_H
#define COASTLINE_PLATFORM_WIN_GLIDE_TEXTURE_H

/**
 * @file
 * The sizes, the formats, and the colours of the textures of the Glide renderer.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Select the Glide mip level for the larger dimension, clamping dimensions above 256.
 * @return Whether the larger dimension is at most 256.
 * @ghidraAddress BRGlide: 0x100242e0
 */
int CalculateMipMapLevel(int *level, int width, int height);

/**
 * Select the Glide aspect level from the truncated ratio of eight times the larger dimension.
 * @return Whether the ratio exactly matches a supported aspect.
 * @ghidraAddress BRGlide: 0x100275c0
 */
int CalculateAspectRatioLevel(int *level, int width, int height);

/**
 * Get one byte for texture formats 2, 3, and 4, and two bytes for other formats.
 * @ghidraAddress BRGlide: 0x10024df0
 */
int GetGlideTexFormatBytesPerPixel(int format);

/**
 * Swap the bytes of an N64 RGBA5551 colour and move its alpha bit to the high bit.
 * @ghidraAddress BRGlide: 0x100271f0
 */
unsigned ConvertN64Rgba5551ToArgb1555(unsigned color);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_TEXTURE_H */

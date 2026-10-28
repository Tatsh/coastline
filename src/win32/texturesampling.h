#ifndef COASTLINE_PLATFORM_WIN_TEXTURE_SAMPLING_H
#define COASTLINE_PLATFORM_WIN_TEXTURE_SAMPLING_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bilinearly sample four decoded four-byte pixels.
 *
 * @param pDest Receives the sampled pixel.
 * @param pTopLeft The top-left pixel.
 * @param pTopRight The top-right pixel.
 * @param pBottomLeft The bottom-left pixel.
 * @param pBottomRight The bottom-right pixel.
 * @param x The horizontal interpolation fraction.
 * @param y The vertical interpolation fraction.
 * @ghidraAddress BRD3D: 0x100254f0
 */
void SampleTexturePixelBilinear(unsigned char *pDest,
                                const unsigned char *pTopLeft,
                                const unsigned char *pTopRight,
                                const unsigned char *pBottomLeft,
                                const unsigned char *pBottomRight,
                                float x,
                                float y);

/**
 * Bilinearly sample four texels of one byte, rounding and clamping the result.
 *
 * @param destination Receives the sampled texel.
 * @param topLeft The top-left texel.
 * @param topRight The top-right texel.
 * @param bottomLeft The bottom-left texel.
 * @param bottomRight The bottom-right texel.
 * @param x The horizontal interpolation fraction.
 * @param y The vertical interpolation fraction.
 * @ghidraAddress BRGlide: 0x10024680
 */
void BilinearInterpolate8bpp(unsigned char *destination,
                             const unsigned char *topLeft,
                             const unsigned char *topRight,
                             const unsigned char *bottomLeft,
                             const unsigned char *bottomRight,
                             float x,
                             float y);

/**
 * Bilinearly sample four packed ARGB1555 pixels, rounding and clamping each channel.
 * @ghidraAddress BRGlide: 0x10024750
 */
void BilinearInterpolateArgb1555(unsigned char *destination,
                                 const unsigned char *topLeft,
                                 const unsigned char *topRight,
                                 const unsigned char *bottomLeft,
                                 const unsigned char *bottomRight,
                                 float x,
                                 float y);

/**
 * Bilinearly sample four packed ARGB4444 pixels, rounding and clamping each channel.
 * @ghidraAddress BRGlide: 0x10024aa0
 */
void BilinearInterpolateArgb4444(unsigned char *destination,
                                 const unsigned char *topLeft,
                                 const unsigned char *topRight,
                                 const unsigned char *bottomLeft,
                                 const unsigned char *bottomRight,
                                 float x,
                                 float y);

#ifdef __cplusplus
}
#endif

#endif

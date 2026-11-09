#ifndef COASTLINE_PLATFORM_WIN_GLIDE_RGB565IMAGE_H
#define COASTLINE_PLATFORM_WIN_GLIDE_RGB565IMAGE_H

/**
 * @file
 * The images of 16-bit RGB 565 pixels the Glide renderer loads from bitmaps and copies to the frame
 * buffer, with a colour key.
 */

#include <stdint.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A bitmap of RGB 565 pixels with a transparent colour key. */
typedef struct Rgb565Image {
    uint16_t *pixels;  /*!< The pixel buffer. */
    int width;         /*!< The image width in pixels. */
    int height;        /*!< The image height in pixels. */
    uint16_t colorKey; /*!< The pixel value treated as transparent. */
} Rgb565Image;

/** Allocate an RGB 565 image and an uninitialised pixel buffer.
 * @return The image, or NULL when allocation fails.
 * @ghidraAddress BRGlide: 0x10001130 */
Rgb565Image *CreateRgb565Image(int width, int height);

/** Free @p image and its pixel buffer, accepting NULL.
 * @ghidraAddress BRGlide: 0x10001190 */
void FreeRgb565Image(Rgb565Image *image);

/** Convert 24-bit BGR rows to RGB 565 while reversing their vertical order.
 * @param destination Output pixels, with no row padding.
 * @param source Input bitmap pixels.
 * @param width Pixels per row.
 * @param height Number of rows.
 * @param sourceStride Bytes between source rows.
 * @ghidraAddress BRGlide: 0x100011c0 */
void ConvertBgr24ToRgb565FlipVertical(
    uint16_t *destination, const uint8_t *source, int width, int height, int sourceStride);

/** Convert a 24-bit Windows bitmap to an RGB 565 image.
 * @return The image, or NULL for another bit depth or failed allocation.
 * @ghidraAddress BRGlide: 0x10001240 */
Rgb565Image *CreateRgb565ImageFromBitmap(const BITMAP *bitmap);

/** Load a bitmap resource or file at the requested dimensions and convert it to RGB 565.
 * @return The image with a green colour key, or NULL when loading or conversion fails.
 * @ghidraAddress BRGlide: 0x10001290 */
Rgb565Image *LoadBitmapToRgb565Image(const char *name, int width, int height);

/** The flag of BlitRgb565ImageRegion() that skips the pixels of the colour key of the source. */
#define RGB565_BLIT_COLOR_KEYED 0x1

/**
 * Copy a rectangle of an image into another at a position, clipped to the right and bottom edges
 * of the destination.
 *
 * @param destination The image copied into.
 * @param x The left of the copy in the destination.
 * @param y The top of the copy in the destination.
 * @param source The image copied from.
 * @param rect The rectangle of the source.
 * @param flags #RGB565_BLIT_COLOR_KEYED or 0.
 * @ghidraAddress BRGlide: 0x10001320
 */
void BlitRgb565ImageRegion(Rgb565Image *destination,
                           unsigned x,
                           unsigned y,
                           const Rgb565Image *source,
                           const RECT *rect,
                           DWORD flags);

/** Copy RGB 565 rows without clipping or interpreting colour keys.
 * @param destination Output pixels.
 * @param destinationStride Bytes between destination rows.
 * @param width Pixels per row.
 * @param height Number of rows.
 * @param source Input pixels.
 * @param sourceStride Bytes between source rows.
 * @ghidraAddress BRGlide: 0x100013f0 */
void CopyRgb565Rect(void *destination,
                    int destinationStride,
                    unsigned width,
                    int height,
                    const void *source,
                    int sourceStride);

/** Copy RGB 565 rows, omitting source pixels equal to @p colorKey.
 * @param destination Output pixels.
 * @param destinationStride Bytes between destination rows.
 * @param width Pixels per row.
 * @param height Number of rows.
 * @param source Input pixels.
 * @param sourceStride Bytes between source rows.
 * @param colorKey Source pixel value to omit.
 * @ghidraAddress BRGlide: 0x10001440 */
void CopyRgb565RectColorKeyed(uint16_t *destination,
                              int destinationStride,
                              int width,
                              int height,
                              const uint16_t *source,
                              int sourceStride,
                              uint16_t colorKey);

/** Convert a Windows RGB colour to the image's RGB 565 transparency key.
 * @ghidraAddress BRGlide: 0x100014a0 */
void SetRgb565ImageColorKey(Rgb565Image *image, COLORREF color);

/**
 * Write an image to the back buffer of Glide at its top left.
 *
 * @param image The image, its pixels in RGB 565.
 * @ghidraAddress BRGlide: 0x100014e0
 */
void WriteRgb565ImageToGlideLfb(const Rgb565Image *image);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_RGB565IMAGE_H */

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

typedef struct Rgb565Image {
    uint16_t *pixels;
    int width;
    int height;
    uint16_t colorKey;
} Rgb565Image;

/** @ghidraAddress BRGlide: 0x10001130 */
Rgb565Image *CreateRgb565Image(int width, int height);

/** @ghidraAddress BRGlide: 0x10001190 */
void FreeRgb565Image(Rgb565Image *image);

/** @ghidraAddress BRGlide: 0x100011c0 */
void ConvertBgr24ToRgb565FlipVertical(
    uint16_t *destination, const uint8_t *source, int width, int height, int sourceStride);

/** @ghidraAddress BRGlide: 0x10001240 */
Rgb565Image *CreateRgb565ImageFromBitmap(const BITMAP *bitmap);

/** @ghidraAddress BRGlide: 0x10001290 */
Rgb565Image *LoadBitmapToRgb565Image(const char *name, int width, int height);

/** @ghidraAddress BRGlide: 0x100013f0 */
void CopyRgb565Rect(void *destination,
                    int destinationStride,
                    unsigned width,
                    int height,
                    const void *source,
                    int sourceStride);

/** @ghidraAddress BRGlide: 0x10001440 */
void CopyRgb565RectColorKeyed(uint16_t *destination,
                              int destinationStride,
                              int width,
                              int height,
                              const uint16_t *source,
                              int sourceStride,
                              uint16_t colorKey);

/** @ghidraAddress BRGlide: 0x100014a0 */
void SetRgb565ImageColorKey(Rgb565Image *image, COLORREF color);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_RGB565IMAGE_H */

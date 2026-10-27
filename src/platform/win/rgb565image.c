#include "platform/win/rgb565image.h"

static_assert(sizeof(Rgb565Image) == 16, "The native image descriptor has sixteen bytes.");

#include <stdlib.h>
#include <string.h>

Rgb565Image *CreateRgb565Image(int width, int height) {
    Rgb565Image *image = malloc(sizeof(*image));
    if (image == NULL) {
        return NULL;
    }
    image->width = width;
    image->height = height;
    image->colorKey = 0;
    image->pixels = malloc((unsigned)height * (unsigned)width * 2u);
    if (image->pixels == NULL) {
        free(image);
        return NULL;
    }
    return image;
}

void FreeRgb565Image(Rgb565Image *image) {
    if (image != NULL) {
        if (image->pixels != NULL) {
            free(image->pixels);
        }
        free(image);
    }
}

void ConvertBgr24ToRgb565FlipVertical(
    uint16_t *destination, const uint8_t *source, int width, int height, int sourceStride) {
    const uint8_t *row = source + (height - 1) * sourceStride;
    for (; height != 0; --height) {
        const uint8_t *pixel = row;
        for (int remaining = width; remaining != 0; --remaining) {
            *destination++ =
                ((pixel[2] & 0xf8u) << 8) | ((pixel[1] & 0xfcu) << 3) | (pixel[0] >> 3);
            pixel += 3;
        }
        row -= sourceStride;
    }
}

Rgb565Image *CreateRgb565ImageFromBitmap(const BITMAP *bitmap) {
    if (bitmap->bmBitsPixel != 24) {
        return NULL;
    }
    Rgb565Image *image = CreateRgb565Image(bitmap->bmWidth, bitmap->bmHeight);
    if (image == NULL) {
        return NULL;
    }
    ConvertBgr24ToRgb565FlipVertical(
        image->pixels, bitmap->bmBits, bitmap->bmWidth, bitmap->bmHeight, bitmap->bmWidthBytes);
    return image;
}

Rgb565Image *LoadBitmapToRgb565Image(const char *name, int width, int height) {
    HBITMAP handle =
        LoadImageA(GetModuleHandleA(NULL), name, IMAGE_BITMAP, width, height, LR_CREATEDIBSECTION);
    if (handle == NULL) {
        handle = LoadImageA(
            NULL, name, IMAGE_BITMAP, width, height, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
        if (handle == NULL) {
            return NULL;
        }
    }
    BITMAP bitmap;
    GetObjectA(handle, sizeof(bitmap), &bitmap);
    Rgb565Image *image = CreateRgb565ImageFromBitmap(&bitmap);
    DeleteObject(handle);
    if (image != NULL) {
        image->colorKey = 0x07e0;
    }
    return image;
}

void CopyRgb565Rect(void *destination,
                    int destinationStride,
                    unsigned width,
                    int height,
                    const void *source,
                    int sourceStride) {
    uint8_t *destinationRow = destination;
    const uint8_t *sourceRow = source;
    unsigned bytes = width * 2u;
    for (; height != 0; --height) {
        unsigned offset = 0;
        for (; offset < (bytes & ~3u); offset += 4) {
            uint32_t word;
            memcpy(&word, sourceRow + offset, sizeof(word));
            memcpy(destinationRow + offset, &word, sizeof(word));
        }
        for (; offset < bytes; ++offset) {
            destinationRow[offset] = sourceRow[offset];
        }
        destinationRow += destinationStride;
        sourceRow += sourceStride;
    }
}

void CopyRgb565RectColorKeyed(uint16_t *destination,
                              int destinationStride,
                              int width,
                              int height,
                              const uint16_t *source,
                              int sourceStride,
                              uint16_t colorKey) {
    for (; height != 0; --height) {
        for (int x = 0; x != width; ++x) {
            if (source[x] != colorKey) {
                destination[x] = source[x];
            }
        }
        destination = (uint16_t *)((uint8_t *)destination + destinationStride);
        source = (const uint16_t *)((const uint8_t *)source + sourceStride);
    }
}

void SetRgb565ImageColorKey(Rgb565Image *image, COLORREF color) {
    image->colorKey =
        ((color & 0xf8u) << 8) | (((color >> 8) & 0xfcu) << 3) | ((color >> 19) & 0x1fu);
}

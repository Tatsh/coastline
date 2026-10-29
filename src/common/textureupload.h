#ifndef COASTLINE_TEXTUREUPLOAD_H
#define COASTLINE_TEXTUREUPLOAD_H

#include "common/types.h"

/** A texture upload receives numeric command data or a pixel buffer. */
typedef union TextureUploadData {
    unsigned value;
    const u8 *pixels;
#ifdef __cplusplus
    TextureUploadData() = default;
    TextureUploadData(unsigned value) : value(value) {
    }
    TextureUploadData(const u8 *pixels) : pixels(pixels) {
    }
#endif
} TextureUploadData;

/** A texture waiting in g_aTextureUploadQueue. */
typedef struct TextureUploadRequest {
    int nEntry;                /*!< The entry of the texture the game loaded. */
    TextureUploadData payload; /*!< The second argument of g_pfnUploadTextureEntryToSurface. */
} TextureUploadRequest;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x118aa0bc
 */
extern u8 *(*g_pfnGetTextureEntryPixelsWithPaint)(int nEntry, unsigned *pSize);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TEXTUREUPLOAD_H */

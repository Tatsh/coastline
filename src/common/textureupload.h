#ifndef COASTLINE_TEXTUREUPLOAD_H
#define COASTLINE_TEXTUREUPLOAD_H

#include "common/types.h"

/** A texture upload receives numeric command data or a pixel buffer. */
typedef union TextureUploadData {
    unsigned value;   /*!< The numeric argument of an upload command. */
    const u8 *pixels; /*!< The pixel data of an upload command. */
#ifdef __cplusplus
    /** Construct an empty upload argument. */
    TextureUploadData() = default;
    /** Construct a numeric upload argument. */
    TextureUploadData(unsigned value) : value(value) {
    }
    /** Construct a pixel-buffer upload argument. */
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
 * The routine that retrieves painted pixels of a texture entry and returns its size.
 *
 * @ghidraAddress BRD3D: 0x118aa0bc
 */
extern u8 *(*g_pfnGetTextureEntryPixelsWithPaint)(int nEntry, unsigned *pSize);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_TEXTUREUPLOAD_H */

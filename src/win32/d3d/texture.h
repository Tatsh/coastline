#pragma once

/**
 * @file
 * The pixel formats of the textures of Direct3D, the conversion of images into them, and the image
 * files of RGBA 5551.
 */

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/d3dcompat.h"
#else
#include <windows.h>
// The headers below need the types of windows.h.
#include <ddraw.h>
#endif

#include "win32/textureworkspace.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM != TGR_PLATFORM_SDL
/** The number of entries of g_aTexturePixelFormats. */
#define TEXTURE_PIXEL_FORMAT_COUNT 5

/** A pixel format of textures the game requests by its identifier. */
struct TexturePixelFormat {
    int nId;                     /*!< The identifier the game requests the format by. */
    unsigned short wMaxBitCount; /*!< The most bits a pixel may have during the enumeration. */
    char abUnknown6[2];          /* +0x06 */
    DDPIXELFORMAT ddpf; /*!< The format of the device, filled when the formats are enumerated. */
};

/**
 * Find a pixel format of textures by its identifier.
 *
 * @param nId The identifier.
 * @return The format of the device, or null for an identifier that is not known.
 * @ghidraAddress BRD3D: 0x10009830
 */
DDPIXELFORMAT *LookupPixelFormatById(int nId);
#endif /* TGR_PLATFORM != TGR_PLATFORM_SDL */

/**
 * Count the bits up to and including the highest set bit of a mask.
 *
 * @param nMask The mask.
 * @return The index of the highest set bit plus one, or 0 for an empty mask.
 * @ghidraAddress BRD3D: 0x10009880
 */
int CountBitWidth(unsigned nMask);

/**
 * Count the clear bits below the lowest set bit of a mask. An empty mask does not return.
 *
 * @param nMask The mask.
 * @return The index of the lowest set bit.
 * @ghidraAddress BRD3D: 0x10009860
 */
int CountTrailingZeroBits32(unsigned nMask);

/**
 * Count the set bits of a mask.
 *
 * @param nMask The mask.
 * @return The number of set bits.
 * @ghidraAddress BRD3D: 0x10009640
 */
int PopCount32(unsigned nMask);

/**
 * Store a texture format of the device in an entry of g_aTexturePixelFormats when it suits the
 * entry better than the format the entry has. A format with more bits a pixel than wMaxBitCount of
 * the entry is skipped, and an empty entry takes the first format of its rule. The rule depends on
 * the identifier of the entry. 2 wants the
 * most alpha bits up to 8, 3 wants RGB with three channels of one width up to 8, 4 and 0xc want RGB
 * with the most alpha bits, and 0xb wants RGB with the fewest alpha bits above none.
 *
 * @param pDesc The surface description of the format.
 * @param pContext The entry of g_aTexturePixelFormats.
 * @return Always D3DENUMRET_OK, to continue the enumeration.
 * @ghidraAddress BRD3D: 0x10009360
 */
HRESULT CALLBACK EnumTextureFormatsCallback_MatchPreset(DDSURFACEDESC *pDesc, void *pContext);

/**
 * Unpack pixels of RGBA 5551 to RGBA 8888. The low bits of each channel repeat bits of the
 * channel, and an alpha bit becomes 0 or 0xff.
 *
 * @param pDest Receives four bytes for each pixel.
 * @param pSrc The pixels to unpack.
 * @param nCount The number of pixels.
 * @ghidraAddress BRD3D: 0x10009a50
 */
void UnpackRgba5551ToRgba8(unsigned char *pDest, const unsigned short *pSrc, int nCount);

/**
 * Convert a host-loaded N64 RGBA 5551 word to four RGBA bytes.
 *
 * @param nColor The word loaded from the big-endian image bytes.
 * @return RGBA channels from the least to the most significant byte.
 * @ghidraAddress BRD3D: 0x10027b10
 */
unsigned ConvertN64Rgba5551ToRgba8(unsigned nColor);

/**
 * Convert pixels of RGBA 8888 to a pixel format of the device.
 *
 * @param pDest Receives the pixels.
 * @param pSrc The pixels to convert.
 * @param nSize The size of the source in bytes.
 * @param nPixelFormatId The pixel format.
 * @return The number of bytes written. A format of three bytes for each pixel writes none.
 * @ghidraAddress BRD3D: 0x100098a0
 */
int ConvertRGBAToPixelFormat(void *pDest, const void *pSrc, int nSize, int nPixelFormatId);

/**
 * Convert RGBA pixels, replacing alpha below 128 with the lowest green bit of the device format.
 *
 * @param pDest Receives the pixels.
 * @param pSrc The source pixels.
 * @param nSize The source byte count.
 * @param nPixelFormatId The device format identifier.
 * @param pColorKey Receives the transparent pixel value, even when the source byte count is zero.
 * @return The output byte count.
 * @ghidraAddress BRD3D: 0x10009660
 */
int ConvertRGBAToPixelFormatWithColorKey(
    void *pDest, const void *pSrc, int nSize, int nPixelFormatId, unsigned *pColorKey);

/**
 * Load an image file of RGBA 5551 pixels and convert it to pixel format 0xb. The file has the
 * width, the height, a word that is not read, and the pixels. A checksum that does not match ends
 * the process.
 *
 * @param pszPath The path of the image.
 * @param nAdlerExpected The Adler-32 checksum of the pixels, or 0 to skip the check.
 * @param pnWidth Receives the width of the image.
 * @param pnHeight Receives the height of the image.
 * @return The converted pixels in the shared decoding workspace.
 * @ghidraAddress BRD3D: 0x10009ad0
 */
void *
LoadRgba5551ImageFile(const char *pszPath, unsigned nAdlerExpected, int *pnWidth, int *pnHeight);

#ifdef __cplusplus
}
#endif

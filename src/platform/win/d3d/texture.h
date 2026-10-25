#pragma once

/** The pixels of an image file RGBA 5551 can hold, 256 by 256. */
#define RGBA5551_IMAGE_PIXEL_COUNT 0x10000

/**
 * Query DirectDraw for the video memory left for textures.
 *
 * @return The free bytes of texture memory.
 * @ghidraAddress BRD3D: 0x1003e220
 */
unsigned GetAvailableTextureMemory(void);

/**
 * The pixels of the last image file read, in RGBA 5551.
 *
 * @ghidraAddress BRD3D: 0x11849870
 */
extern unsigned short g_awRgba5551FileBuffer[RGBA5551_IMAGE_PIXEL_COUNT];

/**
 * The pixels of the last image decoded, four bytes each. In the original, g_awRgba5551FileBuffer
 * lies in the upper half of the buffer, and a decode of a full image overwrites the pixels of the
 * file behind the ones it reads.
 *
 * @ghidraAddress BRD3D: 0x11829870
 */
extern unsigned char g_abTextureDecodeBuffer[RGBA5551_IMAGE_PIXEL_COUNT * 4];

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
 * Convert pixels of RGBA 8888 to a pixel format of the device.
 *
 * @param pDest Receives the pixels.
 * @param pSrc The pixels to convert.
 * @param nSize The size of the source in bytes.
 * @param nPixelFormatId The pixel format.
 * @return A count of the conversion. Its meaning is not recovered yet.
 * @ghidraAddress BRD3D: 0x100098a0
 */
int ConvertRGBAToPixelFormat(void *pDest, const void *pSrc, int nSize, int nPixelFormatId);

/**
 * Load an image file of RGBA 5551 pixels and convert it to pixel format 0xb. The file holds the
 * width, the height, a word that is not read, and the pixels. A checksum that does not match ends
 * the process.
 *
 * @param pszPath The path of the image.
 * @param nAdlerExpected The Adler-32 checksum of the pixels, or 0 to skip the check.
 * @param pnWidth Receives the width of the image.
 * @param pnHeight Receives the height of the image.
 * @return The converted pixels, in g_abTextureDecodeBuffer.
 * @ghidraAddress BRD3D: 0x10009ad0
 */
void *
LoadRgba5551ImageFile(const char *pszPath, unsigned nAdlerExpected, int *pnWidth, int *pnHeight);

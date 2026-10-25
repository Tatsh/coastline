#include "platform/win/d3d/texture.h"

#include <cstdlib>
#include <cstring>

#include <zlib.h>

#include "platform/win/d3d/renderer.h"
#include "platform/win/fileio.h"

// The pixel format LoadRgba5551ImageFile() converts an image to.
#define RGBA5551_IMAGE_PIXEL_FORMAT 0xb

TexturePixelFormat g_aTexturePixelFormats[TEXTURE_PIXEL_FORMAT_COUNT] = {
    {2, {}, {}}, {3, {}, {}}, {4, {}, {}}, {0xb, {}, {}}, {0xc, {}, {}}};
unsigned short g_awRgba5551FileBuffer[RGBA5551_IMAGE_PIXEL_COUNT];

DDPIXELFORMAT *LookupPixelFormatById(int nId) {
    for (TexturePixelFormat &format : g_aTexturePixelFormats) {
        if (format.nId == nId) {
            return &format.ddpf;
        }
    }
    return nullptr;
}

int CountBitWidth(unsigned nMask) {
    int nWidth = 0;

    for (; nMask != 0; nMask >>= 1) {
        nWidth++;
    }
    return nWidth;
}

// A channel of 8 bits is moved by a shift that is negative for a right shift.
static inline unsigned ShiftChannel(unsigned char nChannel, int nShift) {
    return nShift >= 0 ? static_cast<unsigned>(nChannel) << nShift :
                         static_cast<unsigned>(nChannel) >> -nShift;
}

int ConvertRGBAToPixelFormat(void *pDest, const void *pSrc, int nSize, int nPixelFormatId) {
    const DDPIXELFORMAT *pFormat = LookupPixelFormatById(nPixelFormatId);
    unsigned char *pOut = static_cast<unsigned char *>(pDest);
    const unsigned char *pIn = static_cast<const unsigned char *>(pSrc);
    unsigned nBytesPerPixel = pFormat->dwRGBBitCount >> 3;
    int nRedShift = CountBitWidth(pFormat->dwRBitMask) - 8;
    int nGreenShift = CountBitWidth(pFormat->dwGBitMask) - 8;
    int nBlueShift = CountBitWidth(pFormat->dwBBitMask) - 8;
    int nAlphaShift = CountBitWidth(pFormat->dwRGBAlphaBitMask) - 8;
    unsigned nPixel;

    if (nSize > 0) {
        for (unsigned nCount = (static_cast<unsigned>(nSize) + 3) >> 2; nCount != 0; nCount--) {
            nPixel = (ShiftChannel(pIn[0], nRedShift) & pFormat->dwRBitMask) |
                     (ShiftChannel(pIn[1], nGreenShift) & pFormat->dwGBitMask) |
                     (ShiftChannel(pIn[2], nBlueShift) & pFormat->dwBBitMask) |
                     (ShiftChannel(pIn[3], nAlphaShift) & pFormat->dwRGBAlphaBitMask);
            pIn += 4;
            if (nBytesPerPixel == 1) {
                *pOut = static_cast<unsigned char>(nPixel);
                pOut += 1;
            } else if (nBytesPerPixel == 2) {
                memcpy(pOut, &nPixel, 2);
                pOut += 2;
            } else if (nBytesPerPixel == 4) {
                memcpy(pOut, &nPixel, 4);
                pOut += 4;
            }
        }
    }
    return static_cast<int>(pOut - static_cast<unsigned char *>(pDest));
}
unsigned char g_abTextureDecodeBuffer[RGBA5551_IMAGE_PIXEL_COUNT * 4];

void UnpackRgba5551ToRgba8(unsigned char *pDest, const unsigned short *pSrc, int nCount) {
    for (int i = 0; i < nCount; i++) {
        unsigned nPixel = *pSrc++;

        *pDest++ = static_cast<unsigned char>(((nPixel >> 7) & 0xf8) | ((nPixel >> 10) & 7));
        *pDest++ = static_cast<unsigned char>(((nPixel >> 2) & 0xf8) | ((nPixel >> 5) & 7));
        *pDest++ = static_cast<unsigned char>(((nPixel << 3) & 0xf8) | (nPixel & 7));
        *pDest++ = (nPixel & 0x8000) ? 0xff : 0;
    }
}

void *
LoadRgba5551ImageFile(const char *pszPath, unsigned nAdlerExpected, int *pnWidth, int *pnHeight) {
    GameFile *pFile;
    int nWidth;
    int nHeight;
    size_t nSize;

    pFile = CHK_FReadOpen(pszPath);
    CHK_FRead(&nWidth, sizeof(nWidth), 1, pFile);
    CHK_FRead(&nHeight, sizeof(nHeight), 1, pFile);
    nSize = static_cast<size_t>(nHeight * nWidth) * sizeof(g_awRgba5551FileBuffer[0]);
    // The word read here is overwritten by the clear below.
    CHK_FRead(g_awRgba5551FileBuffer, 4, 1, pFile);
    memset(g_awRgba5551FileBuffer, 0, sizeof(g_awRgba5551FileBuffer));
    CHK_FRead(g_awRgba5551FileBuffer, nSize, 1, pFile);
    CHK_FClose(pFile);
    if (nAdlerExpected != 0 && adler32(adler32(0, nullptr, 0),
                                       reinterpret_cast<const Bytef *>(g_awRgba5551FileBuffer),
                                       static_cast<uInt>(nSize)) != nAdlerExpected) {
        exit(1);
    }
    UnpackRgba5551ToRgba8(
        g_abTextureDecodeBuffer, g_awRgba5551FileBuffer, RGBA5551_IMAGE_PIXEL_COUNT);
    ConvertRGBAToPixelFormat(g_abTextureDecodeBuffer,
                             g_abTextureDecodeBuffer,
                             sizeof(g_abTextureDecodeBuffer),
                             RGBA5551_IMAGE_PIXEL_FORMAT);
    *pnWidth = nWidth;
    *pnHeight = nHeight;
    return g_abTextureDecodeBuffer;
}

unsigned GetAvailableTextureMemory(void) {
    DDSCAPS caps;
    DWORD dwTotal;
    DWORD dwFree;

    caps.dwCaps = DDSCAPS_TEXTURE;
    g_pRenderHost->pRenderer->pDirectDraw2->GetAvailableVidMem(&caps, &dwTotal, &dwFree);
    return dwFree;
}

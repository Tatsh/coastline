#include "platform/win/paintshop.h"

#include <stdio.h>
#include <stdlib.h>

CarPaintTextureSet g_CarPaintTextureSets[CAR_PAINT_GROUP_COUNT][CAR_PAINT_SET_COUNT];
void *g_apPaintDamageBitmaps[PAINT_DAMAGE_BITMAP_COUNT];
int g_dwConvertedBitmapWidth;
int g_dwConvertedBitmapHeight;

// The bits of a pixel of the bitmaps the paint shop reads.
#define PAINT_BITMAP_BITS 24
// The bytes of a pixel of RGBA 8888.
#define RGBA32_PIXEL_SIZE 4

void CopyBgr24ToRgba32Flipped(
    unsigned char *pDest, const unsigned char *pSrc, int nWidth, int nHeight, int nStride) {
    const unsigned char *pRow = pSrc + (nHeight - 1) * nStride;

    for (int y = nHeight; y != 0; y--) {
        const unsigned char *pPixel = pRow;

        for (int x = nWidth; x != 0; x--) {
            unsigned char bBlue = pPixel[0];
            unsigned char bGreen = pPixel[1];

            *pDest++ = pPixel[2];
            *pDest++ = bGreen;
            *pDest++ = bBlue;
            *pDest++ = 0xff;
            pPixel += 3;
        }
        pRow -= nStride;
    }
}

void *ConvertBitmap24ToRgba32(const BITMAP *pBitmap) {
    unsigned char *pPixels;

    if (pBitmap->bmBitsPixel != PAINT_BITMAP_BITS) {
        return nullptr;
    }
    pPixels = malloc(pBitmap->bmHeight * pBitmap->bmWidth * RGBA32_PIXEL_SIZE);
    if (pPixels != nullptr) {
        CopyBgr24ToRgba32Flipped(
            pPixels, pBitmap->bmBits, pBitmap->bmWidth, pBitmap->bmHeight, pBitmap->bmWidthBytes);
        g_dwConvertedBitmapWidth = pBitmap->bmWidth;
        g_dwConvertedBitmapHeight = pBitmap->bmHeight;
    }
    return pPixels;
}

void *LoadBitmapFromFile(const char *pszPath) {
    HBITMAP hBitmap;
    BITMAP bm;
    void *pPixels;

    hBitmap =
        LoadImageA(nullptr, pszPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hBitmap == nullptr) {
        return nullptr;
    }
    GetObjectA(hBitmap, sizeof(bm), &bm);
    if (bm.bmBitsPixel != PAINT_BITMAP_BITS) {
        // The bitmap is not deleted here, as in the original.
        return nullptr;
    }
    pPixels = ConvertBitmap24ToRgba32(&bm);
    DeleteObject(hBitmap);
    return pPixels;
}

// The size of the path of a bitmap of the damage of the paint.
#define PAINT_DAMAGE_PATH_SIZE 0x400

void LoadPaintDamageBitmaps(void) {
    char szPath[PAINT_DAMAGE_PATH_SIZE];

    for (int i = 0; i < PAINT_DAMAGE_BITMAP_COUNT; i++) {
        sprintf(szPath, "Paint\\damage%d.bmp", i + 1);
        g_apPaintDamageBitmaps[i] = LoadBitmapFromFile(szPath);
    }
}

// A bitmap from malloc() is freed and its pointer cleared.
static inline void FreeBitmap(void **ppBitmap) {
    if (*ppBitmap != nullptr) {
        free(*ppBitmap);
        *ppBitmap = nullptr;
    }
}

void FreeCarPaintTextures(void) {
    for (int nGroup = 0; nGroup < CAR_PAINT_GROUP_COUNT; nGroup++) {
        for (int nSet = 0; nSet < CAR_PAINT_SET_COUNT; nSet++) {
            for (int nBitmap = 0; nBitmap < CAR_PAINT_BITMAP_COUNT; nBitmap++) {
                FreeBitmap(&g_CarPaintTextureSets[nGroup][nSet].apBitmaps[nBitmap]);
            }
        }
    }
}

void FreeAllPaintBitmaps(void) {
    FreeCarPaintTextures();
    for (int i = 0; i < PAINT_DAMAGE_BITMAP_COUNT; i++) {
        FreeBitmap(&g_apPaintDamageBitmaps[i]);
    }
}

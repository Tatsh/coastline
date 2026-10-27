#include "platform/win/paintshop.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CarPaintTextureSet g_CarPaintTextureSets[CAR_PAINT_GROUP_COUNT][CAR_PAINT_SET_COUNT];
void *g_apPaintDamageBitmaps[PAINT_DAMAGE_BITMAP_COUNT];
int g_dwColorTintRed;
int g_dwColorTintGreen;
int g_dwColorTintBlue;
int g_dwConvertedBitmapWidth;
int g_dwConvertedBitmapHeight;
int g_nSelectedPaintOption10;

void *GetPaintImage(int nGroup, int nSet, int nDamage) {
    CarPaintTextureSet *set = &g_CarPaintTextureSets[nGroup][nSet];
    g_nSelectedPaintOption10 = set->nOption10;
    return set->apBitmaps[nDamage];
}

void GetPaintRectangle(int nGroup, int nSet, int *pLeft, int *pRight, int *pTop, int *pBottom) {
    CarPaintTextureSet *set = &g_CarPaintTextureSets[nGroup][nSet];
    *pLeft = set->nLeft;
    *pRight = set->nRight;
    *pTop = set->nTop;
    *pBottom = set->nBottom;
}

int GetSelectedPaintOption(void) {
    return g_nSelectedPaintOption10;
}

int HasPaintTintPixels(const void *pImage, int nLeft, int nRight, int nTop, int nBottom) {
    int width = (int)((unsigned)nRight - (unsigned)nLeft);
    int height = (int)((unsigned)nBottom - (unsigned)nTop);
    if (pImage != NULL && width > 0 && height > 0) {
        const unsigned char *pixel = pImage;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (pixel[0] == 0 && pixel[1] == pixel[2]) {
                    return 1;
                }
                pixel += 4;
            }
        }
    }
    return 0;
}

int BlitPaintImageToPixels(const void *pImage,
                           int nLeft,
                           int nRight,
                           int nTop,
                           int nBottom,
                           unsigned char *pDest,
                           int nWidth,
                           int nHeight) {
    int width = (int)((unsigned)nRight - (unsigned)nLeft);
    int height = (int)((unsigned)nBottom - (unsigned)nTop);
    if (pImage != NULL && width > 0 && height > 0) {
        const unsigned char *source = pImage;
        for (int y = 0; y < height; y++) {
            unsigned char *dest = pDest + ((nHeight - nTop - y - 1) * nWidth + nLeft) * 4;
            for (int x = 0; x < width; x++) {
                unsigned pixel;
                memcpy(&pixel, source, sizeof(pixel));
                memcpy(dest, &pixel, sizeof(pixel));
                if (source[0] == 0 && source[1] == source[2]) {
                    unsigned intensity = source[1];
                    dest[0] = (unsigned char)((int)(intensity * (unsigned)g_dwColorTintRed) / 255);
                    dest[1] =
                        (unsigned char)((int)(intensity * (unsigned)g_dwColorTintGreen) / 255);
                    dest[2] = (unsigned char)((int)(intensity * (unsigned)g_dwColorTintBlue) / 255);
                }
                source += 4;
                dest += 4;
            }
        }
    }
    return 1;
}

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

void SetColorTintMultipliers(int nRed, int nGreen, int nBlue) {
    g_dwColorTintRed = nRed;
    g_dwColorTintGreen = nGreen;
    g_dwColorTintBlue = nBlue;
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

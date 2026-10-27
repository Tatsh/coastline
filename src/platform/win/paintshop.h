#ifndef COASTLINE_PLATFORM_WIN_PAINTSHOP_H
#define COASTLINE_PLATFORM_WIN_PAINTSHOP_H

/**
 * @file
 * The bitmaps of the paint shop of the PC port.
 */

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The number of groups of g_CarPaintTextureSets. */
#define CAR_PAINT_GROUP_COUNT 16
/** The number of sets of a group of g_CarPaintTextureSets. */
#define CAR_PAINT_SET_COUNT 30
/** The number of bitmaps of a set of g_CarPaintTextureSets. */
#define CAR_PAINT_BITMAP_COUNT 4
/** The number of entries of g_apPaintDamageBitmaps. */
#define PAINT_DAMAGE_BITMAP_COUNT 3

/** The bitmaps, selection option and rectangle of a car paint set. */
typedef struct CarPaintTextureSet {
    void *apBitmaps[CAR_PAINT_BITMAP_COUNT]; /*!< The bitmaps, from malloc(). */
    int nOption10;
    int nLeft;
    int nRight;
    int nTop;
    int nBottom;
    const char *pszBitmapName;
} CarPaintTextureSet;

/**
 * The option at offset 16 of the paint set selected last.
 *
 * @ghidraAddress BRD3D: 0x100b2ad0
 */
extern int g_nSelectedPaintOption10;

/**
 * The group used to prepare car paint textures.
 *
 * @ghidraAddress BRD3D: 0x10aa3444
 */
extern int g_nPaintTextureGroup;

/**
 * The paint set index, advanced before preparing each texture.
 *
 * @ghidraAddress BRD3D: 0x10aa3460
 */
extern int g_nPaintTextureSetIndex;

/**
 * Select a paint set and return its bitmap for a damage level.
 *
 * @param nGroup The paint group.
 * @param nSet The paint set.
 * @param nDamage The damage level.
 * @return The selected bitmap.
 * @ghidraAddress BRD3D: 0x10060f70
 */
void *GetPaintImage(int nGroup, int nSet, int nDamage);

/**
 * Get the bounds of the rectangle of a paint set.
 *
 * @param nGroup The paint group.
 * @param nSet The paint set.
 * @param pLeft Receives the left bound.
 * @param pRight Receives the right bound.
 * @param pTop Receives the top bound.
 * @param pBottom Receives the bottom bound.
 * @ghidraAddress BRD3D: 0x10060fb0
 */
void GetPaintRectangle(int nGroup, int nSet, int *pLeft, int *pRight, int *pTop, int *pBottom);

/**
 * Get the option of the paint set selected last.
 *
 * @return The captured selection option.
 * @ghidraAddress BRD3D: 0x10061000
 */
int GetSelectedPaintOption(void);

/**
 * Test whether packed paint pixels contain a pixel to tint.
 *
 * @param pImage The packed rectangle pixels.
 * @param nLeft The left bound.
 * @param nRight The right bound.
 * @param nTop The top bound.
 * @param nBottom The bottom bound.
 * @return Whether a pixel has red zero and equal green and blue.
 * @ghidraAddress BRD3D: 0x100615b0
 */
int HasPaintTintPixels(const void *pImage, int nLeft, int nRight, int nTop, int nBottom);

/**
 * Copy paint pixels into the destination rectangle, flipping rows and applying the paint tint.
 *
 * @param pImage The paint image.
 * @param nLeft The left bound.
 * @param nRight The right bound.
 * @param nTop The top bound.
 * @param nBottom The bottom bound.
 * @param pDest Receives the pixels.
 * @param nWidth The destination width.
 * @param nHeight The destination height.
 * @return Always 1, including a null image or an empty rectangle.
 * @ghidraAddress BRD3D: 0x10061480
 */
int BlitPaintImageToPixels(const void *pImage,
                           int nLeft,
                           int nRight,
                           int nTop,
                           int nBottom,
                           unsigned char *pDest,
                           int nWidth,
                           int nHeight);

/**
 * The sets of paint of the cars. The meaning of the two dimensions is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x100adfd0
 * @ghidraAddress BRGlide: 0x100ad7d8
 */
extern CarPaintTextureSet g_CarPaintTextureSets[CAR_PAINT_GROUP_COUNT][CAR_PAINT_SET_COUNT];

/**
 * The bitmaps of the damage of the paint, from malloc().
 *
 * @ghidraAddress BRD3D: 0x10aa3450
 * @ghidraAddress BRGlide: 0x10ac67b0
 */
extern void *g_apPaintDamageBitmaps[PAINT_DAMAGE_BITMAP_COUNT];

/**
 * Scale RGB by the first channel of the selected 64 by 64 damage mask, preserving alpha.
 *
 * @param damage The damage level, from 1 to 3.
 * @param pixels The RGBA pixels to update.
 * @param width The width in pixels.
 * @param height The height in pixels.
 * @ghidraAddress BRD3D: 0x10061290
 * @ghidraAddress BRGlide: 0x1005a300
 */
void ApplyPaintDamageOverlay(int damage, unsigned char *pixels, int width, int height);

/**
 * @ghidraAddress BRD3D: 0x10061210
 */
void ModulateRgbaByMask(unsigned char *pixels, int width, int height, const unsigned char *mask);

/**
 * @ghidraAddress BRD3D: 0x100adfc8
 */
extern int g_bCarPaintTexturesEnabled;

/**
 * @ghidraAddress BRD3D: 0x10061010
 */
void LoadCarPaintTextures(int group, int damageVariants);

/**
 * The multiplier of the red of the tint of the paint.
 *
 * @ghidraAddress BRD3D: 0x10aa3440
 * @ghidraAddress BRGlide: 0x10ac67a0
 */
extern int g_dwColorTintRed;

/**
 * The multiplier of the green of the tint of the paint.
 *
 * @ghidraAddress BRD3D: 0x10aa3448
 * @ghidraAddress BRGlide: 0x10ac67a8
 */
extern int g_dwColorTintGreen;

/**
 * The multiplier of the blue of the tint of the paint.
 *
 * @ghidraAddress BRD3D: 0x10aa345c
 * @ghidraAddress BRGlide: 0x10ac67bc
 */
extern int g_dwColorTintBlue;

/**
 * Set the multipliers of the tint of the paint.
 *
 * @param nRed The multiplier of the red.
 * @param nGreen The multiplier of the green.
 * @param nBlue The multiplier of the blue.
 * @ghidraAddress BRD3D: 0x10061460
 * @ghidraAddress BRGlide: 0x1005a4e0
 */
void SetColorTintMultipliers(int nRed, int nGreen, int nBlue);

/**
 * The width of the bitmap ConvertBitmap24ToRgba32() converted last.
 *
 * @ghidraAddress BRD3D: 0x10aa3464
 * @ghidraAddress BRGlide: 0x10ac67c4
 */
extern int g_dwConvertedBitmapWidth;

/**
 * The height of the bitmap ConvertBitmap24ToRgba32() converted last.
 *
 * @ghidraAddress BRD3D: 0x10aa3468
 * @ghidraAddress BRGlide: 0x10ac67c8
 */
extern int g_dwConvertedBitmapHeight;

/**
 * Copy the rows of a bitmap of 24 bits from the bottom up into pixels of RGBA 8888, opaque.
 *
 * @param pDest The pixels, width by height.
 * @param pSrc The rows of the bitmap, the bottom row first in memory.
 * @param nWidth The width.
 * @param nHeight The height.
 * @param nStride The bytes of a row of the bitmap.
 * @ghidraAddress BRD3D: 0x10060f00
 * @ghidraAddress BRGlide: 0x10059f70
 */
void CopyBgr24ToRgba32Flipped(
    unsigned char *pDest, const unsigned char *pSrc, int nWidth, int nHeight, int nStride);

/**
 * Convert a bitmap of 24 bits to pixels of RGBA 8888 from malloc(), and record its size in
 * g_dwConvertedBitmapWidth and g_dwConvertedBitmapHeight.
 *
 * @param pBitmap The bitmap.
 * @return The pixels, or null for a bitmap of another depth or when they are not allocated.
 * @ghidraAddress BRD3D: 0x10060ea0
 * @ghidraAddress BRGlide: 0x10059f10
 */
void *ConvertBitmap24ToRgba32(const BITMAP *pBitmap);

/**
 * Load a bitmap file of 24 bits as pixels of RGBA 8888.
 *
 * @param pszPath The path of the file.
 * @return The pixels from malloc(), or null.
 * @ghidraAddress BRD3D: 0x100611a0
 * @ghidraAddress BRGlide: 0x1005a210
 */
void *LoadBitmapFromFile(const char *pszPath);

/**
 * Load the bitmaps of the damage of the paint from `Paint\damage%d.bmp`.
 *
 * @ghidraAddress BRD3D: 0x10061410
 * @ghidraAddress BRGlide: 0x1005a490
 */
void LoadPaintDamageBitmaps(void);

/**
 * Free the bitmaps of every set of paint of the cars.
 *
 * @ghidraAddress BRD3D: 0x100613b0
 * @ghidraAddress BRGlide: 0x1005a420
 */
void FreeCarPaintTextures(void);

/**
 * Free the bitmaps of the paint of the cars and of the damage of the paint.
 *
 * @ghidraAddress BRD3D: 0x10061620
 * @ghidraAddress BRGlide: 0x1005a6a0
 */
void FreeAllPaintBitmaps(void);

#ifdef __cplusplus
}
#endif

#endif // COASTLINE_PLATFORM_WIN_PAINTSHOP_H

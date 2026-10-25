#ifndef COASTLINE_PLATFORM_WIN_PAINTSHOP_H
#define COASTLINE_PLATFORM_WIN_PAINTSHOP_H

/**
 * @file
 * The bitmaps of the paint shop of the PC port.
 */

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

/** The bitmaps of a set of paint of a car. The layout past the bitmaps is not recovered yet. */
typedef struct CarPaintTextureSet {
    void *apBitmaps[CAR_PAINT_BITMAP_COUNT]; /*!< The bitmaps, from malloc(). */
    char abUnknown10[0x18];                  /* +0x10 */
} CarPaintTextureSet;

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

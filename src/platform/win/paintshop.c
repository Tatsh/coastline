#include "platform/win/paintshop.h"

#include <stdlib.h>

CarPaintTextureSet g_CarPaintTextureSets[CAR_PAINT_GROUP_COUNT][CAR_PAINT_SET_COUNT];
void *g_apPaintDamageBitmaps[PAINT_DAMAGE_BITMAP_COUNT];

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

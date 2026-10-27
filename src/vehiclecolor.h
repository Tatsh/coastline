#ifndef COASTLINE_VEHICLECOLOR_H
#define COASTLINE_VEHICLECOLOR_H

#include "vehiclemodel.h"

static inline u16 PackMaterialPaletteColor(u16 color) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return (color >> 8) | (color << 8);
#else
    return color;
#endif
}

static inline s32 GetMaterialTextureFormat(const ModelTexture *texture) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return (texture->textureFlags >> 24) & 0xf;
#else
    return texture->format;
#endif
}

static inline void SetMaterialPaletteColors(VehicleModel *model, s32 red, s32 green, s32 blue) {
    ModelTexture *texture;
    u16 *palette;
    s32 i;

    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        texture = &model->textures[model->materialTextures[i]];
        palette = texture->palette;
        if (palette != NULL && GetMaterialTextureFormat(texture) == TEXTURE_FORMAT_CI4) {
            /* Each store preserves bit 0 from the material's indexed entry in native byte order. */
            palette[0] = PackMaterialPaletteColor((palette[i] & 1) | (red << 11) | (green << 6) |
                                                  (blue << 1));
            /* The second entry is a darker shade. */
            palette[1] = PackMaterialPaletteColor((palette[i] & 1) | ((red & 0x1E) << 10) |
                                                  ((green & 0x1E) << 5) | (blue & 0x1E));
        }
    }
}

#endif

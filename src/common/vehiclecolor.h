#ifndef COASTLINE_VEHICLECOLOR_H
#define COASTLINE_VEHICLECOLOR_H

/**
 * @file
 * The paint and light palettes of the materials of a vehicle model.
 */

#include "common/vehiclemodel.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
/** Refresh the painted material textures of a vehicle model.
 * @param model The vehicle model.
 */
void RefreshVehicleMaterialTextures(VehicleModel *model);

/** Set the palette entries used to light a vehicle.
 * @param palette The palette to update.
 * @param variant The lighting variant.
 * @param dark Whether the dark palette is selected.
 */
static inline void SetVehicleLightPalette(u16 *palette, s32 variant, s32 dark) {
    if (variant == 0) {
        palette[15] = dark ? 0x0070 : 0x0190;
        palette[10] = dark ? 0x8290 : 0x01a0;
        palette[12] = 0x8179;
        palette[7] = 0x4192;
    }
    palette[11] = variant < 2 ? 0x6bad : 0x38e7;
    palette[6] = variant < 2 ? 0x31c6 : 0xfeff;
    palette[14] = variant & 1 ? 0x00c0 : 0x0190;
    palette[13] = variant & 1 ? 0x00c0 : palette[15];
    palette[9] = variant & 1 ? 0x04f9 : 0x01a0;
    palette[8] = variant & 1 ? 0x04f9 : palette[10];
}
#endif /* TGR_BOSS_RALLY */

/**
 * Convert a palette colour to the byte order of the renderer. Boss Rally swaps the two bytes.
 *
 * @param color The RGBA5551 colour, in native order.
 * @return @p color in the byte order used by the selected renderer.
 */
static inline u16 PackMaterialPaletteColor(u16 color) {
#if TGR_BOSS_RALLY
    return (color >> 8) | (color << 8);
#else
    return color;
#endif
}

/**
 * Get the format of a texture. Boss Rally reads bits 24 to 27 of its texture flags.
 *
 * @param texture The texture.
 * @return The indexed or direct format of @p texture.
 */
static inline s32 GetMaterialTextureFormat(const ModelTexture *texture) {
#if TGR_BOSS_RALLY
    return (texture->textureFlags >> 24) & 0xf;
#else
    return texture->format;
#endif
}

/**
 * Set the material palettes of @p model to the requested red, green, and blue colour. The first
 * entry of each CI4 material palette receives the colour, and the second a darker shade.
 *
 * @param model The vehicle model.
 * @param red The red component, from 0 to 31.
 * @param green The green component, from 0 to 31.
 * @param blue The blue component, from 0 to 31.
 */
static inline void SetMaterialPaletteColors(VehicleModel *model, s32 red, s32 green, s32 blue) {
    ModelTexture *texture;
    u16 *palette;
    s32 i;

    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        texture = &model->textures[model->materialTextures[i]];
        palette = texture->palette;
        if (palette != NULL && GetMaterialTextureFormat(texture) == TEXTURE_FORMAT_CI4) {
            /* Each store preserves bit 0 from the material's indexed entry in native byte order. */
            palette[0] = PackMaterialPaletteColor(
                (u16)((palette[i] & 1) | (red << 11) | (green << 6) | (blue << 1)));
            /* The second entry is a darker shade. */
            palette[1] = PackMaterialPaletteColor((u16)((palette[i] & 1) | ((red & 0x1E) << 10) |
                                                        ((green & 0x1E) << 5) | (blue & 0x1E)));
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLECOLOR_H */

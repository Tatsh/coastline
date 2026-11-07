#if TGR_BOSS_RALLY
#include "common/vehiclecolor.h"
#include "common/vehiclecount.h"
#else
#include "common/vehiclemodel.h"
#endif

#if !TGR_BOSS_RALLY
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MATERIAL_TEXTURE_FORMAT(texture) ((texture)->format)
#else
#define MATERIAL_TEXTURE_FORMAT(texture) ((texture)->formatByte & 0xF)
#endif
#endif

void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue) {
#if TGR_BOSS_RALLY
    if (g_dwNumActiveVehicles == 0) {
        return;
    }
    SetMaterialPaletteColors(model, red, green, blue);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    RefreshVehicleMaterialTextures(model);
#endif
#else
    s32 i;
    ModelTexture *parts;
    ModelTexture *first;
    ModelTexture *second;
    u16 *palette;

    for (i = 0; i < VEHICLE_MATERIALS; i += 2) {
        parts = model->textures;
        first = &parts[model->materialTextures[i]];
        palette = first->palette;
        if (palette != NULL && MATERIAL_TEXTURE_FORMAT(first) == TEXTURE_FORMAT_CI4) {
            palette[0] = (palette[i] & 1) | (red << 11) | (green << 6) | (blue << 1);
            palette[1] =
                (palette[i] & 1) | ((red & 0x1e) << 10) | ((green & 0x1e) << 5) | (blue & 0x1e);
        }
        second = &model->textures[model->materialTextures[i + 1]];
        palette = second->palette;
        if (palette != NULL && MATERIAL_TEXTURE_FORMAT(second) == TEXTURE_FORMAT_CI4) {
            palette[0] = (palette[i + 1] & 1) | (red << 11) | (green << 6) | (blue << 1);
            palette[1] =
                (palette[i + 1] & 1) | ((red & 0x1e) << 10) | ((green & 0x1e) << 5) | (blue & 0x1e);
        }
    }
#endif
}

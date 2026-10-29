#include "common/vehiclepaint.h"

#include "common/vehicle.h"

void ExtractVehicleBaseColor(Vehicle *vehicle, VehicleModel *model) {
    ModelTexture *texture = &model->textures[model->materialTextures[VEHICLE_MATERIAL_PAINT]];
    u16 *palette = texture->palette;
    s32 color;
    s32 red;
    s32 green;
    s32 blue;

    if (palette != NULL && MODEL_TEXTURE_FORMAT(texture) == TEXTURE_FORMAT_CI4) {
        /* Each 5-bit component widens to 8 bits, with its top bits repeated below. */
        color = palette[0];
        red = ((color >> 13) & 7) | ((color >> 8) & 0xF8);
        green = ((color >> 8) & 7) | ((color >> 3) & 0xF8);
        blue = ((color >> 3) & 7) | ((color << 2) & 0xF8);
#if TGR_PC
        vehicle->SetVehiclePaintColorRgb(red, green, blue);
#else
        vehicle->color[2] = blue;
        vehicle->color[1] = green;
        vehicle->color[0] = red;
#endif
    }
}

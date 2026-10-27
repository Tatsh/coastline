#include "platform/win/boot/app.h"
#include "platform/win/render.h"
#include "racerslot.h"
#include "vehicle.h"

void ApplyEnemyCarColorPanels(Vehicle *vehicle, RacerSlot *slot) {
    VehicleModel *model = vehicle->model;
    s32 count = model->textureEntryCount;
    for (s32 index = 0; index < count; index++) {
        EnqueueTextureUploadEntry(model->textureEntries[index], slot->colorPanels[index]);
    }
    ModelTexture *lights = &model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]];
    if (lights->palette != nullptr && !g_dwNullStubEnable) {
        for (s32 index = 0; index < 4; index++) {
            if (model->lightTextureEntries[index] != 0) {
                EnqueueTextureUploadEntry(model->lightTextureEntries[index],
                                          slot->colorPanels[count + index]);
            }
        }
    }
}

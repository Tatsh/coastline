#include <cstring>

#include "game.h"
#include "platform/win/boot/app.h"
#include "platform/win/fileio.h"
#include "platform/win/paintshop.h"
#include "platform/win/render.h"
#include "platform/win/textureupload.h"
#include "racerslot.h"
#include "vehicle.h"
#include "vehiclecolor.h"
#include "weather.h"

void RefreshVehicleMaterialTextures(VehicleModel *model) {
    for (s32 index = 0; index < model->textureEntryCount; index++) {
        g_pfnReloadTextureEntryData(model->textureEntries[index]);
    }
    u16 *palette = model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette;
    if (palette == nullptr || g_dwNullStubEnable) {
        return;
    }
    for (s32 index = 0; index < 4; index++) {
        if (model->lightTextureEntries[index] != 0) {
            SetVehicleLightPalette(palette, index, index == 0 && (g_dwNightMode || g_dwRainMode));
            g_pfnReloadTextureEntryData(model->lightTextureEntries[index]);
        }
    }
}

static inline void CopyRacerColorPanel(RacerSlot *slot, s32 index, s32 entry, const char *caller) {
    unsigned size;
    u8 *source = g_pfnGetTextureEntryPixelsWithPaint(entry, &size);
    slot->colorPanels[index] = static_cast<u8 *>(CHK_AllocateMemory(size, caller));
    if (size != 0) {
        memcpy(slot->colorPanels[index], source, size);
    }
}

void MakeEnemyCarColorPanels(RacerSlot *slot) {
    Vehicle *vehicle = &g_aVehicles[g_dwNumLocalPlayers + slot->straggler];
    SetColorTintMultipliers(slot->color[0], slot->color[1], slot->color[2]);
    SetMaterialPaletteColors(
        vehicle->model, slot->color[0] >> 3, slot->color[1] >> 3, slot->color[2] >> 3);
    s32 count = vehicle->model->textureEntryCount;
    slot->colorPanelCount = count + 4;
    slot->colorPanels = static_cast<u8 **>(CHK_AllocateMemory(
        slot->colorPanelCount * sizeof(u8 *), "MakeEnemyCarColorPanels: penm->aptex"));
    for (s32 index = 0; index < count; index++) {
        CopyRacerColorPanel(slot,
                            index,
                            vehicle->model->textureEntries[index],
                            "MakeEnemyCarColorPanels: penm->aptex[i]");
    }
    VehicleModel *model = vehicle->model;
    u16 *palette = model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette;
    if (palette == nullptr || g_dwNullStubEnable) {
        return;
    }
    /* Unused light-panel entries remain uninitialised, as in the original. */
    if (vehicle->model->lightTextureEntries[0] != 0) {
        SetVehicleLightPalette(palette, 0, g_dwNightMode || g_dwRainMode);
        CopyRacerColorPanel(slot,
                            count,
                            vehicle->model->lightTextureEntries[0],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+0]");
    }
    if (vehicle->model->lightTextureEntries[1] != 0) {
        SetVehicleLightPalette(palette, 1, 0);
        CopyRacerColorPanel(slot,
                            count + 1,
                            vehicle->model->lightTextureEntries[1],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+1]");
    }
    if (vehicle->model->lightTextureEntries[2] != 0) {
        SetVehicleLightPalette(palette, 2, 0);
        CopyRacerColorPanel(slot,
                            count + 2,
                            vehicle->model->lightTextureEntries[2],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+2]");
    }
    if (vehicle->model->lightTextureEntries[3] != 0) {
        SetVehicleLightPalette(palette, 3, 0);
        CopyRacerColorPanel(slot,
                            count + 3,
                            vehicle->model->lightTextureEntries[3],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+3]");
    }
}

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

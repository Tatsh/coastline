#include <cstring>

#include "game.h"
#include "racerslot.h"
#include "racerslotaccess.h"
#include "vehicle.h"
#include "vehicleaccess.h"
#include "vehiclecolor.h"
#include "weather.h"
#include "win32/boot/app.h"
#include "win32/fileio.h"
#include "win32/paintshop.h"
#include "win32/render.h"
#include "win32/textureupload.h"

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
    RACER_SLOT_GET(slot, ColorPanels)[index] = static_cast<u8 *>(CHK_AllocateMemory(size, caller));
    if (size != 0) {
        memcpy(RACER_SLOT_GET(slot, ColorPanels)[index], source, size);
    }
}

void MakeEnemyCarColorPanels(RacerSlot *slot) {
    Vehicle *vehicle = &g_aVehicles[g_dwNumLocalPlayers + RACER_SLOT_GET(slot, Straggler)];
    SetColorTintMultipliers(RACER_SLOT_GET(slot, Color)[0],
                            RACER_SLOT_GET(slot, Color)[1],
                            RACER_SLOT_GET(slot, Color)[2]);
    SetMaterialPaletteColors(VEHICLE_GET(vehicle, Model),
                             RACER_SLOT_GET(slot, Color)[0] >> 3,
                             RACER_SLOT_GET(slot, Color)[1] >> 3,
                             RACER_SLOT_GET(slot, Color)[2] >> 3);
    s32 count = VEHICLE_GET(vehicle, Model)->textureEntryCount;
    RACER_SLOT_SET(slot, ColorPanelCount, count + 4);
    RACER_SLOT_SET(
        slot,
        ColorPanels,
        static_cast<u8 **>(CHK_AllocateMemory(RACER_SLOT_GET(slot, ColorPanelCount) * sizeof(u8 *),
                                              "MakeEnemyCarColorPanels: penm->aptex")));
    for (s32 index = 0; index < count; index++) {
        CopyRacerColorPanel(slot,
                            index,
                            VEHICLE_GET(vehicle, Model)->textureEntries[index],
                            "MakeEnemyCarColorPanels: penm->aptex[i]");
    }
    VehicleModel *model = VEHICLE_GET(vehicle, Model);
    u16 *palette = model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette;
#ifdef ENABLE_PATCHES
    /* Restart cleanup frees every slot, including absent light textures. */
    std::memset(RACER_SLOT_GET(slot, ColorPanels) + count, 0, 4 * sizeof(u8 *));
#endif
    if (palette == nullptr || g_dwNullStubEnable) {
        return;
    }
    if (VEHICLE_GET(vehicle, Model)->lightTextureEntries[0] != 0) {
        SetVehicleLightPalette(palette, 0, g_dwNightMode || g_dwRainMode);
        CopyRacerColorPanel(slot,
                            count,
                            VEHICLE_GET(vehicle, Model)->lightTextureEntries[0],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+0]");
    }
    if (VEHICLE_GET(vehicle, Model)->lightTextureEntries[1] != 0) {
        SetVehicleLightPalette(palette, 1, 0);
        CopyRacerColorPanel(slot,
                            count + 1,
                            VEHICLE_GET(vehicle, Model)->lightTextureEntries[1],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+1]");
    }
    if (VEHICLE_GET(vehicle, Model)->lightTextureEntries[2] != 0) {
        SetVehicleLightPalette(palette, 2, 0);
        CopyRacerColorPanel(slot,
                            count + 2,
                            VEHICLE_GET(vehicle, Model)->lightTextureEntries[2],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+2]");
    }
    if (VEHICLE_GET(vehicle, Model)->lightTextureEntries[3] != 0) {
        SetVehicleLightPalette(palette, 3, 0);
        CopyRacerColorPanel(slot,
                            count + 3,
                            VEHICLE_GET(vehicle, Model)->lightTextureEntries[3],
                            "MakeEnemyCarColorPanels: penm->aptex[cptex+3]");
    }
}

#ifdef ENABLE_PATCHES
// The panels were made from the model of the straggler, which can differ from this vehicle's, and
// light panels are absent when that model has no light palette.
static void EnqueueRacerColorPanel(RacerSlot *slot, s32 panel, int entry) {
    u8 **panels = RACER_SLOT_GET(slot, ColorPanels);
    if (panels != nullptr && panel < RACER_SLOT_GET(slot, ColorPanelCount) && panels[panel]) {
        EnqueueTextureUploadEntry(entry, panels[panel]);
    }
}
#else
// The original queues each panel without testing it.
#define EnqueueRacerColorPanel(slot, panel, entry)                                                 \
    EnqueueTextureUploadEntry((entry), RACER_SLOT_GET((slot), ColorPanels)[(panel)])
#endif /* ENABLE_PATCHES */

void ApplyEnemyCarColorPanels(Vehicle *vehicle, RacerSlot *slot) {
    VehicleModel *model = VEHICLE_GET(vehicle, Model);
    s32 count = model->textureEntryCount;
    for (s32 index = 0; index < count; index++) {
        EnqueueRacerColorPanel(slot, index, model->textureEntries[index]);
    }
    ModelTexture *lights = &model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]];
    if (lights->palette != nullptr && !g_dwNullStubEnable) {
        for (s32 index = 0; index < 4; index++) {
            if (model->lightTextureEntries[index] != 0) {
                EnqueueRacerColorPanel(slot, count + index, model->lightTextureEntries[index]);
            }
        }
    }
}

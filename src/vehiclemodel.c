#include <PR/os.h>
#include <PR/os_libc.h>

#include "model.h"
#include "render.h"
#include "system.h"
#include "vehicle.h"

/* The room for the message of a car too big. */
#define LOAD_MESSAGE_SIZE 0x50

void ExtractVehicleBaseColor(Vehicle *vehicle, VehicleModel *model) {
    ModelTexture *texture = &model->textures[model->materialTextures[VEHICLE_MATERIAL_PAINT]];
    u16 *palette = texture->palette;
    s32 color;
    s32 red;
    s32 green;
    s32 blue;

    if (palette != NULL && texture->format == TEXTURE_FORMAT_CI4) {
        /* Each 5-bit component widened to 8 bits, its top bits repeated below. */
        color = palette[0];
        red = ((color >> 13) & 7) | ((color >> 8) & 0xF8);
        green = ((color >> 8) & 7) | ((color >> 3) & 0xF8);
        blue = ((color >> 3) & 7) | ((color << 2) & 0xF8);
        vehicle->color[2] = blue;
        vehicle->color[1] = green;
        vehicle->color[0] = red;
    }
}

/* Remap a pointer of a model loaded for the first slot to the slot of the model. */
#define REMAP_MODEL_POINTER(pointer, model)                                                        \
    RemapPointer((u32 *)(pointer), MODEL_SLOT_LO, MODEL_SLOT_HI, (u32)(model))

/* The addresses of the first slot of g_aVehicleModels, which the files of the cars hold. */
#define MODEL_SLOT_LO ((u32)g_aVehicleModels)
#define MODEL_SLOT_HI ((u32) & g_aVehicleModels[1])

void FixupVehicleModelPointers(VehicleModel *model) {
    s32 lod;
    s32 k;

    for (lod = 0; lod < VEHICLE_LODS; lod++) {
        for (k = 0; k < VEHICLE_DISPLAY_LISTS; k++) {
            REMAP_MODEL_POINTER(&model->displayLists[lod][k], model);
            FixupDisplayListPointers(
                model->displayLists[lod][k], MODEL_SLOT_LO, MODEL_SLOT_HI, (u32)model);
        }
        for (k = 0; k < VEHICLE_WHEEL_DISPLAY_LISTS; k++) {
            if (model->wheelDisplayLists[lod][k] != NULL) {
                REMAP_MODEL_POINTER(&model->wheelDisplayLists[lod][k], model);
                FixupDisplayListPointers(
                    model->wheelDisplayLists[lod][k], MODEL_SLOT_LO, MODEL_SLOT_HI, (u32)model);
            }
        }
    }
    REMAP_MODEL_POINTER(&model->textures, model);
    REMAP_MODEL_POINTER(&model->unreconstructed90, model);
    REMAP_MODEL_POINTER(&model->unreconstructed94, model);
    /* The original counts the textures and the materials with the variable of the levels. */
    for (lod = 0; lod < model->textureCount; lod++) {
        REMAP_MODEL_POINTER(&model->textures[lod].image, model);
        REMAP_MODEL_POINTER(&model->textures[lod].palette, model);
    }
    REMAP_MODEL_POINTER(&model->decalMasks, model);
    for (lod = 0; lod < VEHICLE_MATERIALS; lod++) {
        REMAP_MODEL_POINTER(&model->decalMasks[lod], model);
    }
}

void LoadVehicleModelFromRom(VehicleModel *model, s32 car) {
    char message[LOAD_MESSAGE_SIZE];

    g_aCarModels[car].modelSize = DmaReadAssetSize(g_aCarModels[car].modelRom);
    if (g_aCarModels[car].modelSize > sizeof(VehicleModel)) {
        sprintf(message,
                "Car %d too big (%d vs. %d)",
                car,
                g_aCarModels[car].modelSize,
                sizeof(VehicleModel));
        SetDebugMessage(message);
    } else {
        osSyncPrintf(
            "Loading car %d (%d / %d)\n", car, g_aCarModels[car].modelSize, sizeof(VehicleModel));
    }
    /* A car too big is decompressed all the same. */
    DecompressFromRom(model, g_aCarModels[car].modelRom, NULL);
}

void LoadCar(s32 slot, s32 car, void *source) {
    if (source == NULL) {
        LoadVehicleModelFromRom(&g_aVehicleModels[slot], car);
    } else {
        /* The source must be a model not yet remapped, whose addresses are of the first slot. */
        memcpy(&g_aVehicleModels[slot], source, g_aCarModels[car].modelSize);
    }
    FixupVehicleModelPointers(&g_aVehicleModels[slot]);
    g_adwVehicleModelSlotCars[slot] = car;
}

s32 IsVehicleModelValid(s32 car) {
    return g_aCarModels[car].modelRomEnd != 0;
}

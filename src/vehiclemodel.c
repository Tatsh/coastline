#include <PR/os.h>
#include <PR/os_libc.h>

#include "model.h"
#include "render.h"
#include "system.h"
#include "vehicle.h"

/* The room for the message of a car too big. */
#define LOAD_MESSAGE_SIZE 0x50

void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue) {
    ModelTexture *texture;
    u16 *palette;
    s32 i;

    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        texture = &model->textures[model->materialTextures[i]];
        palette = texture->palette;
        if (palette != NULL && texture->format == TEXTURE_FORMAT_CI4) {
            /* The alpha bit is kept from the entry of the material's own index, not from 0. */
            palette[0] = (palette[i] & 1) | (red << 11) | (green << 6) | (blue << 1);
            /* The second entry is a darker shade. */
            palette[1] =
                (palette[i] & 1) | ((red & 0x1E) << 10) | ((green & 0x1E) << 5) | (blue & 0x1E);
        }
    }
}

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
static void RemapModelPointer(void *pointer, VehicleModel *model) {
    u32 lo = (u32)g_aVehicleModels;

    /* The pointer is the word it is stored in, which holds an address in the first slot. */
    RemapPointer(pointer, lo, lo + sizeof(VehicleModel), (u32)model);
}

void FixupVehicleModelPointers(VehicleModel *model) {
    u32 lo = (u32)g_aVehicleModels;
    u32 hi = lo + sizeof(VehicleModel);
    s32 lod;
    s32 k;
    s32 i;

    /* The files of the cars hold the addresses of the first slot of g_aVehicleModels. */
    for (lod = 0; lod < VEHICLE_LODS; lod++) {
        for (k = 0; k < VEHICLE_DISPLAY_LISTS; k++) {
            RemapModelPointer(&model->displayLists[lod][k], model);
            FixupDisplayListPointers(model->displayLists[lod][k], lo, hi, (u32)model);
        }
        for (k = 0; k < VEHICLE_WHEEL_DISPLAY_LISTS; k++) {
            if (model->wheelDisplayLists[lod][k] != NULL) {
                RemapModelPointer(&model->wheelDisplayLists[lod][k], model);
                FixupDisplayListPointers(model->wheelDisplayLists[lod][k], lo, hi, (u32)model);
            }
        }
    }
    RemapModelPointer(&model->textures, model);
    RemapModelPointer(&model->unreconstructed90, model);
    RemapModelPointer(&model->unreconstructed94, model);
    for (i = 0; i < model->textureCount; i++) {
        RemapModelPointer(&model->textures[i].image, model);
        RemapModelPointer(&model->textures[i].palette, model);
    }
    RemapModelPointer(&model->decalMasks, model);
    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        RemapModelPointer(&model->decalMasks[i], model);
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

void LoadOrCopyVehicleModel(s32 slot, s32 car, void *source) {
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

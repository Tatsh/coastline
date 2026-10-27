#include "types.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "vehiclemodel.h"
#else
#include <PR/os.h>
#include <PR/os_libc.h>

#include "model.h"
#include "render.h"
#include "system.h"
#include "vehicle.h"
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN

VehicleModel *g_pTextureCaptureVehicleModel;
s32 g_dwTextureCaptureOverflowCount;

s32 IsVehicleModelValid(void) {
    return 1;
}

void FixupVehicleModelPointers(VehicleModel *model) {
}

void AppendVehicleModelTextureEntry(s32 entry) {
    VehicleModel *model = g_pTextureCaptureVehicleModel;
    s32 count = model->textureEntryCount;
    if (count < 30) {
        model->textureEntries[count] = entry;
        g_pTextureCaptureVehicleModel->textureEntryCount =
            (s32)((u32)g_pTextureCaptureVehicleModel->textureEntryCount + 1);
    } else {
        g_dwTextureCaptureOverflowCount = (s32)((u32)g_dwTextureCaptureOverflowCount + 1);
        model->textureEntryCount = (s32)((u32)model->textureEntryCount + 1);
    }
}

s32 IsVehicleMaterialPalette(const void *palette) {
    VehicleModel *model = g_pTextureCaptureVehicleModel;
    s32 i;
    for (i = 0; i < VEHICLE_MATERIALS; i++) {
        ModelTexture *texture = &model->textures[model->materialTextures[i]];
        if (texture->palette != NULL &&
            (texture->textureFlags & 0xf000000) == (TEXTURE_FORMAT_CI4 << 24) &&
            texture->palette == palette) {
            return 1;
        }
    }
    return 0;
}

#else

/* The room for the message of a car too big. */
#define LOAD_MESSAGE_SIZE 0x50

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

s32 IsVehicleModelValid(s32 car) {
    return g_aCarModels[car].modelRomEnd != 0;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

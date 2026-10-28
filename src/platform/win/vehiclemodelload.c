#include <stdio.h>
#include <string.h>

#include "platform/win/boot/fatalerror.h"
#include "platform/win/fileio.h"
#include "platform/win/paintshop.h"
#include "platform/win/settings.h"
#include "vehiclemodel.h"

void LoadVehicleModelFromRom(VehicleModel *model, s32 car) {
    int savedQuality = 0;
    char error[256];
    char path[1024];

    g_nPaintTextureGroup = car;
    if (model != g_aVehicleModels) {
        savedQuality = g_dwTextureQualityLevel;
        if (savedQuality == 0) {
            g_dwTextureQualityLevel = 1;
        }
    }
    LoadCarPaintTextures(car, model == g_aVehicleModels);
    g_nPaintTextureSetIndex = 0;
    strcpy(path, g_szCarsDirPath);
    strcat(path, g_apszCarModelPrefixes[car]);
    strcat(path, ".rca");
    ReadEntireFile(model, path, -1);
    if (memcmp(model, "RCar", 4) != 0) {
        /* The original error formatter reads its uninitialised destination as the filename. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#pragma GCC diagnostic ignored "-Wformat-overflow"
#endif
        sprintf(error, "not a car file: %s", error);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
        ShowFatalErrorScreen(error);
    }
    ByteSwapAndRelocateCarModel(model);
    if (model != g_aVehicleModels) {
        g_dwTextureQualityLevel = savedQuality;
    }
}

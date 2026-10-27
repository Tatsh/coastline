#include <cstdio>
#include <cstring>

#include "platform/win/boot/app.h"
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
    std::strcpy(path, g_szCarsDirPath);
    std::strcat(path, g_apszCarModelPrefixes[car]);
    std::strcat(path, ".rca");
    ReadEntireFile(model, path, -1);
    if (std::memcmp(model, "RCar", 4) != 0) {
        /* The original error formatter reads its uninitialised destination as the filename. */
        std::sprintf(error, "not a car file: %s", error);
        ShowFatalErrorScreen(error);
    }
    ByteSwapAndRelocateCarModel(model);
    if (model != g_aVehicleModels) {
        g_dwTextureQualityLevel = savedQuality;
    }
}

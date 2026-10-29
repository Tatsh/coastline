#include <stdio.h>
#include <string.h>

#include "common/fileio.h"
#include "common/settings.h"
#include "common/vehiclemodel.h"
#include "win32/fatalerror.h"
#include "win32/paintshop.h"

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
#ifdef ENABLE_PATCHES
        /* The precision fits the path in the buffer after the text and its terminator. */
        snprintf(error,
                 sizeof(error),
                 "not a car file: %.*s",
                 (int)(sizeof(error) - sizeof("not a car file: ")),
                 path);
#else
        /* The original error formatter reads its uninitialised destination as the filename. */
        sprintf(error, "not a car file: %s", error);
#endif /* ENABLE_PATCHES */
        ShowFatalErrorScreen(error);
    }
    ByteSwapAndRelocateCarModel(model);
    if (model != g_aVehicleModels) {
        g_dwTextureQualityLevel = savedQuality;
    }
}

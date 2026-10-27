#include "vehiclecolor.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "vehiclecount.h"
#endif

void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_dwNumActiveVehicles == 0) {
        return;
    }
#endif
    SetMaterialPaletteColors(model, red, green, blue);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    RefreshVehicleMaterialTextures(model);
#endif
}

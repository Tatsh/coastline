#include "common/vehiclecolor.h"
#if TGR_PC
#include "common/vehiclecount.h"
#endif

void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue) {
#if TGR_PC
    if (g_dwNumActiveVehicles == 0) {
        return;
    }
#endif
    SetMaterialPaletteColors(model, red, green, blue);
#if TGR_PC
    RefreshVehicleMaterialTextures(model);
#endif
}

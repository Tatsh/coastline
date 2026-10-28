#include "vehiclemodel.h"

#if TGR_PC
#include "win32/boot/app.h"
#else
#include <string.h>

#include "vehicle.h"
#endif

void LoadCar(s32 slot, s32 car, void *source) {
    if (source == NULL) {
        LoadVehicleModelFromRom(&g_aVehicleModels[slot], car);
    } else {
#if TGR_PC
        ShowFatalErrorScreen("LoadCar()");
#else
        /* The source must be a model not yet remapped, whose addresses are of the first slot. */
        memcpy(&g_aVehicleModels[slot], source, g_aCarModels[car].modelSize);
#endif
    }
    FixupVehicleModelPointers(&g_aVehicleModels[slot]);
    g_adwVehicleModelSlotCars[slot] = car;
}

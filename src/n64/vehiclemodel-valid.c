#include "common/vehicle.h"

s32 IsVehicleModelValid(s32 car) {
    return g_aCarModels[car].modelRomEnd != 0;
}

#include "common/hudassets.h"
#include "common/vehicleview.h"

VehicleReflectionTexture g_apVehicleReflectionTextures[4] = {
    {.image = g_abDaySkyTexture},
    {.image = g_abNightSkyTexture},
    {.image = g_abNightSkyTexture},
    {.image = g_abTunnelReflectionTexture},
};

s8 g_abVehicleReflectionTexSelect[3][2] = {{3, 3}, {0, 1}, {2, 2}};

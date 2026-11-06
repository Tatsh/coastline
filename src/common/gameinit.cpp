#include "common/gameinit.h"

#include "common/screen.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

s32 g_dwSavedSfxVolume;
s32 g_dwSavedMusicVolume;
s32 g_dwUnk_InitGameAndShowMenus_106c664c;
s32 g_dwUnk_InitGameAndShowMenus_10b4df28;
s32 g_dwNumActiveVehicles = 2;
u32 g_dwCurrentWeather;

s32 ReturnZero(void) {
    return 0;
}

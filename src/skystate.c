#include "input.h"
#include "sky.h"
#include "track.h"
#include "weather.h"

s32 IsSkyFogFilled(void) {
    return g_dwNightMode != 0 || g_dwSnowMode != 0 || g_dwRainMode != 0 ||
           g_LoadedTrack.skyDisplayList == NULL || g_dwNumLocalPlayers == 2;
}

#include <cmath>

#include <common/hud.h>

#include "common/camera.h"
#include "common/laptime.h"
#include "common/settings.h"
#include "common/text.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "render.h"
#include "vehicle.h"
#include "win32/hud.h"
#include "win32/hudcanvas.h"

s32 g_bHudShowLapTimes = 1;

#ifdef ENABLE_PATCHES
#define g_dwConfigFramebufferWidth GetHudCanvasWidth()
#endif

namespace {

// @ghidraAddress BRD3D: 0x104aff20
// @ghidraAddress BRGlide: 0x104ab708
char g_szGapToLeaderText[1 + 7 + 1 + 3 + 1 + 3 + 1];

} // namespace

void DrawLapTimeListHud(const ViewportRect *viewports) {
    if (g_bHudShowLapTimes != 0) {
        s32 x = (s32)((u32)g_dwConfigFramebufferWidth - 16);
        s32 below = g_dwNumViewports == 1 ? 30 : 0;
        s32 y = (s32)((u32)viewports[g_dwCurrentSplitScreenIndex].y + 20);

        SetTextStyleNormal();
        SetTextAlignRight();
        SetTextSize(15);
        y = (s32)((u32)y + below + 37);
        for (s32 index = 0; index < VEHICLE_GET(g_pActiveCameraVehicle, Lap); ++index) {
            DrawFormattedLapTimeText(
                "%ww", index + 1, VEHICLE_GET(g_pActiveCameraVehicle, LapTimes)[index], x, y);
            y = (s32)((u32)y + 15);
        }
    }
}

long double ComputeTimeGapToLeader(const Vehicle *vehicles, int index) {
    int bestPosition = 255;
    long double gap = 0.0L;

    for (int other = 0; other < g_dwNumActiveVehicles; ++other) {
        if (VEHICLE_GET(&(vehicles[other]), RacePosition) < bestPosition) {
            bestPosition = VEHICLE_GET(&(vehicles[other]), RacePosition);
            gap = (long double)VEHICLE_GET(&(vehicles[other]), Distance) -
                  VEHICLE_GET(&(vehicles[index]), Distance);
        }
    }
    if (gap == 0.0L || std::isnan(gap)) {
        return gap;
    }
    long double speed = (long double)VEHICLE_GET(&(vehicles[index]), SpeedKph) * 0.44642857f;
    if (speed == 0.0L || std::isnan(speed)) {
        return 1000.0L;
    }
    if (speed < 25.0L) {
        speed = 25.0L;
    }
    return gap / speed;
}

char *FormatGapToLeaderString(const Vehicle *vehicles, int index) {
    long double gap = ComputeTimeGapToLeader(vehicles, index);
    f32 seconds = gap;

    if (gap > 0.0L) {
        g_szGapToLeaderText[0] = '+';
        FormatLapTime(g_szGapToLeaderText + 1, seconds);
    } else {
        g_szGapToLeaderText[0] = '\0';
    }
    return g_szGapToLeaderText;
}

#include <cmath>

#include <hud.h>

#include "camera.h"
#include "laptime.h"
#include "platform/win/hud.h"
#include "platform/win/settings.h"
#include "render.h"
#include "text.h"
#include "vehicle.h"
#include "vehiclecount.h"

s32 g_bHudShowLapTimes = 1;

void DrawLapTimeListHud(const ViewportRect *viewports) {
    if (g_bHudShowLapTimes != 0) {
        s32 x = (s32)((u32)g_dwConfigFramebufferWidth - 16);
        s32 below = g_dwNumViewports == 1 ? 30 : 0;
        s32 y = (s32)((u32)viewports[g_dwCurrentSplitScreenIndex].y + 20);

        SetTextStyleNormal();
        SetTextAlignRight();
        SetTextSize(15);
        y = (s32)((u32)y + below + 37);
        for (s32 index = 0; index < g_pActiveCameraVehicle->lap; ++index) {
            DrawFormattedLapTimeText(
                "%ww", index + 1, g_pActiveCameraVehicle->lapTimes[index], x, y);
            y = (s32)((u32)y + 15);
        }
    }
}

long double ComputeTimeGapToLeader(const Vehicle *vehicles, int index) {
    int bestPosition = 255;
    long double gap = 0.0L;

    for (int other = 0; other < g_dwNumActiveVehicles; ++other) {
        if (vehicles[other].racePosition < bestPosition) {
            bestPosition = vehicles[other].racePosition;
            gap = (long double)vehicles[other].distance - vehicles[index].distance;
        }
    }
    if (gap == 0.0L || std::isnan(gap)) {
        return gap;
    }
    long double speed = (long double)vehicles[index].speedKph * 0.44642857f;
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

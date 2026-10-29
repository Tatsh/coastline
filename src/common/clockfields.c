#include <stddef.h>

#include "common/frametime.h"
#include "common/replay.h"
#include "common/system.h"
#include "common/vecmath.h"

SaveField g_aPausedClockFields[] = {
    {&g_dwRandomSeed, sizeof(g_dwRandomSeed)},
    {&g_qwTimestampBase, sizeof(g_qwTimestampBase)},
    {&g_dwFrameTimestamp, sizeof(g_dwFrameTimestamp)},
    {&g_dwPrevElapsedTimeMs, sizeof(g_dwPrevElapsedTimeMs)},
    {&g_dwElapsedTimeMs, sizeof(g_dwElapsedTimeMs)},
    {&g_fFrameDeltaTime, sizeof(g_fFrameDeltaTime)},
    {&g_dwFrameDeltaMs, sizeof(g_dwFrameDeltaMs)},
    {NULL, 0},
};

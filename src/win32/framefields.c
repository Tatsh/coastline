#include <stddef.h>

#include "driving.h"
#include "frameclear.h"
#include "racers.h"

/* Native code clears these bytes each frame without reading their values. */
/* @ghidraAddress BRD3D: 0x10af9b48 */
static u8 g_abPerFrameClearScratch[12];

PerFrameVariable g_aPerFrameVariables[] = {
    {g_abPerFrameClearScratch, sizeof(g_abPerFrameClearScratch)},
    {(u8 *)g_avBestLeftLane, sizeof(g_avBestLeftLane)},
    {(u8 *)g_avBestCenterLine, sizeof(g_avBestCenterLine)},
    {(u8 *)g_avBestRightLane, sizeof(g_avBestRightLane)},
    {(u8 *)&g_dwBestTraceLength, sizeof(g_dwBestTraceLength)},
    {(u8 *)g_avTraceLeftLane, sizeof(g_avTraceLeftLane)},
    {(u8 *)g_avTraceLeftHalf, sizeof(g_avTraceLeftHalf)},
    {(u8 *)g_avTraceCenterLine, sizeof(g_avTraceCenterLine)},
    {(u8 *)g_avTraceRightHalf, sizeof(g_avTraceRightHalf)},
    {(u8 *)g_avTraceRightLane, sizeof(g_avTraceRightLane)},
    {(u8 *)&g_dwBestTraceDepth, sizeof(g_dwBestTraceDepth)},
    {(u8 *)&g_bAiNudgeRight, sizeof(g_bAiNudgeRight)},
    {(u8 *)&g_bAiNudgeLeft, sizeof(g_bAiNudgeLeft)},
    {(u8 *)&g_dwBestTracePoint, sizeof(g_dwBestTracePoint)},
    {(u8 *)&g_pBestTraceSection, sizeof(g_pBestTraceSection)},
    {(u8 *)&g_dwTraceUnusedB, sizeof(g_dwTraceUnusedB)},
    {(u8 *)&g_dwTraceUnusedA, sizeof(g_dwTraceUnusedA)},
    {(u8 *)&g_bTraceBlocked, sizeof(g_bTraceBlocked)},
    {(u8 *)&g_vTraceFarPoint, sizeof(g_vTraceFarPoint)},
    {(u8 *)&g_vTraceHalfwayPoint, sizeof(g_vTraceHalfwayPoint)},
    {(u8 *)&g_vTrackRoutePoint, sizeof(g_vTrackRoutePoint)},
    {(u8 *)&g_pTrackRouteSection, sizeof(g_pTrackRouteSection)},
    {(u8 *)&g_dwTrackRoutePointIndex, sizeof(g_dwTrackRoutePointIndex)},
    {(u8 *)&g_dwTrackRouteGates, sizeof(g_dwTrackRouteGates)},
    {(u8 *)&g_dwTrackRouteLaps, sizeof(g_dwTrackRouteLaps)},
    {NULL, 0},
};

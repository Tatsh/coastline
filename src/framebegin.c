#include "render.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
s32 g_dwStatUnused3;
s32 g_dwStatUnused2;
s32 g_dwStatUnused1;
s32 g_dwStatVehicleTriangles;
s32 g_dwStatPolygonsDrawn;
s32 g_dwStatUnused0;
s32 g_dwStatVerticesDrawn;
s32 g_dwStatTrianglesDrawn;
#endif

void ClearRenderStats(void) {
    g_dwStatUnused1 = g_dwStatUnused2 = g_dwStatUnused3 = 0;
    g_dwStatPolygonsDrawn = g_dwStatVehicleTriangles = 0;
    g_dwStatTrianglesDrawn = g_dwStatVerticesDrawn = g_dwStatUnused0 = 0;
}

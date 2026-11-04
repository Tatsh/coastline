#include "common/render.h"

void ClearRenderStats(void) {
    g_dwStatUnused1 = g_dwStatUnused2 = g_dwStatUnused3 = 0;
    g_dwStatPolygonsDrawn = g_dwStatVehicleTriangles = 0;
    g_dwStatTrianglesDrawn = g_dwStatVerticesDrawn = g_dwStatUnused0 = 0;
}

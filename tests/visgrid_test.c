/* Host check of BuildVisibleGrid() in src/visgrid.c on a square frustum footprint. */
#include <assert.h>
#include <stdio.h>

#include "common/visgrid.h"

Vec3f g_avFrustumPoints[FRUSTUM_POINTS] = {
    {100, 100, 0},
    {100, 300, 0},
    {300, 300, 0},
    {300, 100, 0},
    {100, 100, 0},
    {200, 200, 0},
};
s32 g_dwVisibleGridMinCol, g_dwVisibleGridMinRow, g_dwVisibleGridMaxCol, g_dwVisibleGridMaxRow;
s32 g_adwVisibleGridRowMinCol[VISIBLE_GRID_SIZE], g_adwVisibleGridRowMaxCol[VISIBLE_GRID_SIZE];
s32 g_adwVisibleGridColMinRow[VISIBLE_GRID_SIZE], g_adwVisibleGridColMaxRow[VISIBLE_GRID_SIZE];

int main(void) {
    BuildVisibleGrid();
    /* The square runs over cells 3 to 9 on both axes. */
    assert(g_dwVisibleGridMinCol == 3 && g_dwVisibleGridMaxCol == 9);
    assert(g_dwVisibleGridMinRow == 3 && g_dwVisibleGridMaxRow == 9);
    assert(IsPointVisible(200, 200));
    assert(IsPointVisible(110, 290));
    assert(!IsPointVisible(50, 200));
    assert(!IsPointVisible(200, 400));
    assert(g_adwVisibleGridColMinRow[5] == 3 && g_adwVisibleGridColMaxRow[5] == 9);
#if defined(ENABLE_PATCHES) && TGR_PLATFORM == TGR_PLATFORM_SDL
    {
        s32 point, row, col;
        f32 invalid[] = {NAN, INFINITY, -INFINITY, 1.0e30f, -1.0e30f};
        unsigned int value;
        for (point = 0; point < FRUSTUM_POINTS; point++) {
            Vec3f original = g_avFrustumPoints[point];
            for (value = 0; value < sizeof(invalid) / sizeof(invalid[0]); value++) {
                g_avFrustumPoints[point].x = invalid[value];
                BuildVisibleGrid();
                for (row = 0; row < VISIBLE_GRID_SIZE; row++) {
                    for (col = 0; col < VISIBLE_GRID_SIZE; col++) {
                        assert(IsCellVisible(col, row));
                    }
                }
                assert(!IsCellVisible(0, -1));
                assert(!IsCellVisible(0, VISIBLE_GRID_SIZE));
            }
            g_avFrustumPoints[point] = original;
        }
        BuildVisibleGrid();
        assert(g_dwVisibleGridMinCol == 3 && g_dwVisibleGridMaxCol == 9);
    }
#endif
    puts("visgrid ok");
    return 0;
}

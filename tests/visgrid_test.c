/* Host check of BuildVisibleGrid() in src/visgrid.c on a square frustum footprint. */
#include <assert.h>
#include <stdio.h>

#include "visgrid.h"

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
    puts("visgrid ok");
    return 0;
}

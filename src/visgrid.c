#include "visgrid.h"

#include "mathutil.h"

/* The size of a cell in track units, and its reciprocal. */
#define CELL_SIZE 32.0f
#define CELL_SCALE (1.0f / 32.0f)

/* The edges of the frustum, as pairs of its points: from the camera and from the far point to each
 * corner, then around the corners. */
static const u8 sFrustumEdges[][2] = {
    {0, 1},
    {0, 2},
    {0, 3},
    {0, 4},
    {5, 1},
    {5, 2},
    {5, 3},
    {5, 4},
    {1, 2},
    {2, 3},
    {3, 4},
    {4, 1},
};

void MarkVisibleCell(s32 col, s32 row) {
    if (col < 0) {
        col = 0;
    }
    if (col >= VISIBLE_GRID_SIZE) {
        col = VISIBLE_GRID_SIZE - 1;
    }
    if (row < 0) {
        row = 0;
    }
    if (row >= VISIBLE_GRID_SIZE) {
        row = VISIBLE_GRID_SIZE - 1;
    }
    if (col < g_adwVisibleGridRowMinCol[row]) {
        g_adwVisibleGridRowMinCol[row] = col;
    }
    if (g_adwVisibleGridRowMaxCol[row] < col) {
        g_adwVisibleGridRowMaxCol[row] = col;
    }
}

void MarkVisibleEdge(f32 x0, f32 y0, f32 x1, f32 y1) {
    s32 first, last, row, col;
    f32 swap, rowY, x;

    MarkVisibleCell(FloatToInt(x0 * CELL_SCALE), FloatToInt(y0 * CELL_SCALE));
    MarkVisibleCell(FloatToInt(x1 * CELL_SCALE), FloatToInt(y1 * CELL_SCALE));

    first = FloatToInt(x0 * CELL_SCALE);
    last = FloatToInt(x1 * CELL_SCALE);
    if (last < first) {
        col = first;
        first = last;
        last = col;
    }
    if (first < g_dwVisibleGridMinCol) {
        g_dwVisibleGridMinCol = first;
    }
    if (g_dwVisibleGridMaxCol < last) {
        g_dwVisibleGridMaxCol = last;
    }

    /* Walk the rows from the lower end of the line. A flat line marks only its ends. */
    if (y1 < y0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    if (y1 == y0) {
        return;
    }
    first = FloatToInt(y0 * CELL_SCALE);
    last = FloatToInt(y1 * CELL_SCALE);
    if (first < g_dwVisibleGridMinRow) {
        g_dwVisibleGridMinRow = first;
    }
    if (g_dwVisibleGridMaxRow < last) {
        g_dwVisibleGridMaxRow = last;
    }

    /* Where the line crosses the boundary below a row, mark the cells on both sides of it, and
     * the neighbouring column when the crossing lies on a column boundary. */
    for (row = first; row <= last; row++) {
        rowY = row * CELL_SIZE;
        if (y0 <= rowY && rowY <= y1) {
            x = (x1 - x0) * (rowY - y0) / (y1 - y0) + x0;
            col = FloatToInt(x * CELL_SCALE);
            MarkVisibleCell(col, row - 1);
            MarkVisibleCell(col, row);
            if (x <= col * CELL_SIZE) {
                MarkVisibleCell(col - 1, row - 1);
                MarkVisibleCell(col - 1, row);
            }
            if ((col + 1) * CELL_SIZE <= x) {
                MarkVisibleCell(col + 1, row - 1);
                MarkVisibleCell(col + 1, row);
            }
        }
    }
}

s32 IsCellVisible(s32 col, s32 row) {
    return row >= g_dwVisibleGridMinRow && row <= g_dwVisibleGridMaxRow &&
           col >= g_adwVisibleGridRowMinCol[row] && col <= g_adwVisibleGridRowMaxCol[row];
}

s32 IsPointVisible(f32 x, f32 y) {
    s32 col = FloatToInt(x * CELL_SCALE);

    return IsCellVisible(col, FloatToInt(y * CELL_SCALE));
}

void BuildVisibleGrid(void) {
    s32 i, col, first, last;
    const Vec3f *from, *to;

    g_dwVisibleGridMaxCol = 0;
    g_dwVisibleGridMaxRow = 0;
    g_dwVisibleGridMinCol = VISIBLE_GRID_SIZE - 1;
    g_dwVisibleGridMinRow = VISIBLE_GRID_SIZE - 1;
    for (i = 0; i < VISIBLE_GRID_SIZE; i++) {
        g_adwVisibleGridRowMinCol[i] = VISIBLE_GRID_SIZE;
        g_adwVisibleGridRowMaxCol[i] = 0;
        g_adwVisibleGridColMinRow[i] = VISIBLE_GRID_SIZE;
        g_adwVisibleGridColMaxRow[i] = 0;
    }

    for (i = 0; i < (s32)(sizeof(sFrustumEdges) / sizeof(sFrustumEdges[0])); i++) {
        from = &g_avFrustumPoints[sFrustumEdges[i][0]];
        to = &g_avFrustumPoints[sFrustumEdges[i][1]];
        MarkVisibleEdge(from->x, from->y, to->x, to->y);
    }

    if (g_dwVisibleGridMinCol < 0) {
        g_dwVisibleGridMinCol = 0;
    }
    if (g_dwVisibleGridMinRow < 0) {
        g_dwVisibleGridMinRow = 0;
    }
    if (g_dwVisibleGridMaxCol >= VISIBLE_GRID_SIZE) {
        g_dwVisibleGridMaxCol = VISIBLE_GRID_SIZE - 1;
    }
    if (g_dwVisibleGridMaxRow >= VISIBLE_GRID_SIZE) {
        g_dwVisibleGridMaxRow = VISIBLE_GRID_SIZE - 1;
    }

    /* The covered rows of each column, from the first and last rows that take it in. */
    for (col = g_dwVisibleGridMinCol; col <= g_dwVisibleGridMaxCol; col++) {
        first = g_dwVisibleGridMinRow;
        while (col < g_adwVisibleGridRowMinCol[first] || g_adwVisibleGridRowMaxCol[first] < col) {
            first++;
        }
        last = g_dwVisibleGridMaxRow;
        while (col < g_adwVisibleGridRowMinCol[last] || g_adwVisibleGridRowMaxCol[last] < col) {
            last--;
        }
        g_adwVisibleGridColMinRow[col] = first;
        g_adwVisibleGridColMaxRow[col] = last;
    }
}

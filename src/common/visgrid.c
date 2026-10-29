#include "common/visgrid.h"

#include "common/mathutil.h"

#if TGR_PC
/**
 * @ghidraAddress USA: 0x8028bd98
 * @ghidraAddress EUR: 0x8028e4f8
 * @ghidraAddress JPN: 0x8028df28
 * @ghidraAddress BRD3D: 0x10a9bbc0
 * @ghidraAddress BRGlide: 0x10ac2c50
 */
static s32 g_dwVisibleGridMaxCol;
/**
 * The bounds of the covered cells.
 *
 * @ghidraAddress USA: 0x8028bd90
 * @ghidraAddress EUR: 0x8028e4f0
 * @ghidraAddress JPN: 0x8028df20
 * @ghidraAddress BRD3D: 0x10a9bbc8
 * @ghidraAddress BRGlide: 0x10ac2c58
 */
static s32 g_dwVisibleGridMinCol;
#endif /* TGR_PC */

#if TGR_PC
/** @ghidraAddress BRD3D: 0x10a9bbc4 */
s32 g_dwVisibleGridMaxRow;
/** @ghidraAddress BRD3D: 0x10a9bbcc */
s32 g_dwVisibleGridMinRow;
#endif /* TGR_PC */

/* The size of a cell in track units, and its reciprocal. */
#define CELL_SIZE 32.0f
#define CELL_SCALE (1.0f / 32.0f)

/* Mark the edge of the frustum between two of its points. */
#define MARK_FRUSTUM_EDGE(from, to)                                                                \
    MarkVisibleEdge(g_avFrustumPoints[from].x,                                                     \
                    g_avFrustumPoints[from].y,                                                     \
                    g_avFrustumPoints[to].x,                                                       \
                    g_avFrustumPoints[to].y)

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
        /* The original swaps the columns with three chained exclusive ors. C leaves the chain
         * unsequenced, and IDO evaluates it right to left. */
#ifdef __GNUC__
        last ^= first;
        first ^= last;
        last ^= first;
#else
        last ^= first ^= last ^= first;
#endif
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
    return IsCellVisible(FloatToInt(x * CELL_SCALE), FloatToInt(y * CELL_SCALE));
}

void BuildVisibleGrid(void) {
    s32 i, col, first, last;

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

    /* The edges of the frustum: from the camera and from the far point to each corner, then
     * around the corners. */
    MARK_FRUSTUM_EDGE(0, 1);
    MARK_FRUSTUM_EDGE(0, 2);
    MARK_FRUSTUM_EDGE(0, 3);
    MARK_FRUSTUM_EDGE(0, 4);
    MARK_FRUSTUM_EDGE(5, 1);
    MARK_FRUSTUM_EDGE(5, 2);
    MARK_FRUSTUM_EDGE(5, 3);
    MARK_FRUSTUM_EDGE(5, 4);
    MARK_FRUSTUM_EDGE(1, 2);
    MARK_FRUSTUM_EDGE(2, 3);
    MARK_FRUSTUM_EDGE(3, 4);
    MARK_FRUSTUM_EDGE(4, 1);

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
        for (first = g_dwVisibleGridMinRow;
             col < g_adwVisibleGridRowMinCol[first] || g_adwVisibleGridRowMaxCol[first] < col;
             first++) {
        }
        for (last = g_dwVisibleGridMaxRow;
             col < g_adwVisibleGridRowMinCol[last] || g_adwVisibleGridRowMaxCol[last] < col;
             last--) {
        }
        g_adwVisibleGridColMinRow[col] = first;
        g_adwVisibleGridColMaxRow[col] = last;
    }
}

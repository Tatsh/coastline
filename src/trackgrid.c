#include "track.h"

/* The grid of the track: 64 cells of 32 units each way. */
#define GRID_SIZE 2048.0f
#define GRID_CELL_SCALE 0.03125f
#define GRID_CELLS 64
#define GRID_ROW_SHIFT 6

/* The cell at a column and row of the grid. */
#define GRID_CELL(column, row) ((u8)(column) + ((u8)(row) << GRID_ROW_SHIFT))

/* Whether a point is off the grid, and the column or row of a coordinate on it. */
#if TGR_PC
#define IS_OFF_GRID(x, y) (!((x) >= 0.0f && (x) < GRID_SIZE && (y) >= 0.0f && (y) < GRID_SIZE))
#define GRID_INDEX(value) ((u32)(s64)((value) * GRID_CELL_SCALE))
#else
#define IS_OFF_GRID(x, y) ((x) < 0.0f || GRID_SIZE <= (x) || (y) < 0.0f || GRID_SIZE <= (y))
#define GRID_INDEX(value) ((u32)((value) * GRID_CELL_SCALE))
#endif

/* Whether a column and row are off the grid. */
#define IS_OFF_GRID_CELL(column, row)                                                              \
    ((column) < 0 || (column) >= GRID_CELLS || (row) < 0 || (row) >= GRID_CELLS)

/* The triangles or models of a cell of a grid, packed with the count in the upper half and the
 * start in the lower. */
#define PACK_GRID_CELL(starts, cell, start)                                                        \
    (((u32)((starts)[(u16)((cell) + 1)] - (start)) << 16) | (start))

u32 LookupTrackGridCell(f32 x, f32 y) {
    u16 cell;
    u16 start;

    if (IS_OFF_GRID(x, y)) {
        return 0;
    }
    cell = GRID_CELL(GRID_INDEX(x), GRID_INDEX(y));
    start = g_LoadedTrack.gridCellStarts[cell];
    return PACK_GRID_CELL(g_LoadedTrack.gridCellStarts, cell, start);
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
u32 LookupInstanceCellAtPoint(f32 x, f32 y) {
    u16 cell;
    u16 start;

    if (IS_OFF_GRID(x, y)) {
        return 0;
    }
    cell = GRID_CELL(GRID_INDEX(x), GRID_INDEX(y));
    start = g_LoadedTrack.gridInstanceStarts[cell];
    return PACK_GRID_CELL(g_LoadedTrack.gridInstanceStarts, cell, start);
}

u32 FindTrackGridCellByIndex(s32 column, s32 row) {
    u16 cell;
    u32 start;

    if (IS_OFF_GRID_CELL(column, row)) {
        return 0;
    }
    cell = GRID_CELL(column, row);
    start = g_LoadedTrack.gridCellStarts[cell];
    return PACK_GRID_CELL(g_LoadedTrack.gridCellStarts, cell, start);
}

#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

u32 LookupInstanceGridCell(s32 column, s32 row) {
    u16 cell;
    u32 start;

    if (IS_OFF_GRID_CELL(column, row)) {
        return 0;
    }
    cell = GRID_CELL(column, row);
    start = g_LoadedTrack.gridInstanceStarts[cell];
    return PACK_GRID_CELL(g_LoadedTrack.gridInstanceStarts, cell, start);
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
u16 GetTrackGridCellCount(TrackGridCursor *cursor) {
    return cursor->count;
}

u16 GetInstanceGridCellCount(TrackGridCursor *cursor) {
    return cursor->count;
}

#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */

/* The cursor of a cell advanced past the entry at start, packed as LookupTrackGridCell packs it. */
#define NEXT_GRID_CURSOR(cursor, start) (((start) + 1) | ((u32)((cursor)->count - 1) << 16))

/* Advance a cursor of a cell past the entry at start. The original keeps only the lower half of the
 * packed cursor, in a signed local, and packs it again for the count. */
#define ADVANCE_GRID_CURSOR(cursor, start, packed)                                                 \
    packed = NEXT_GRID_CURSOR(cursor, start);                                                      \
    (cursor)->count = NEXT_GRID_CURSOR(cursor, start) >> 16;                                       \
    (cursor)->start = (packed)

u16 NextGridCellTriangle(TrackGridCursor *cursor) {
    s16 packed;
    u16 start;

    if (cursor->count != 0) {
        start = cursor->start;
        ADVANCE_GRID_CURSOR(cursor, start, packed);
        return g_LoadedTrack.gridTriangles[start];
    }
    return 0;
}

u16 NextGridCellInstance(TrackGridCursor *cursor) {
    s16 packed;
    u16 start;

    if (cursor->count != 0) {
        start = cursor->start;
        ADVANCE_GRID_CURSOR(cursor, start, packed);
        return g_LoadedTrack.gridInstances[start];
    }
    return 0;
}

#ifndef COASTLINE_VISGRID_H
#define COASTLINE_VISGRID_H

/**
 * @file
 * The grid of the cells of the track the view frustum covers, which decides what is drawn.
 */

#include <PR/ultratypes.h>

#include "vecmath.h"

/** The cells along each side of the grid. */
#define VISIBLE_GRID_SIZE 64

/** The points of the view frustum: the camera, the four corners and the far point. */
#define FRUSTUM_POINTS 6
#define FRUSTUM_CAMERA 0
#define FRUSTUM_RIGHT_TOP 1
#define FRUSTUM_LEFT_TOP 2
#define FRUSTUM_LEFT_BOTTOM 3
#define FRUSTUM_RIGHT_BOTTOM 4
#define FRUSTUM_FAR 5

/** @ghidraAddress USA: 0x8031b1f0 */
extern Vec3f g_avFrustumPoints[FRUSTUM_POINTS];

/** The bounds of the covered cells. @ghidraAddress USA: 0x8028bd90 */
extern s32 g_dwVisibleGridMinCol;

/** @ghidraAddress USA: 0x8028bd94 */
extern s32 g_dwVisibleGridMinRow;

/** @ghidraAddress USA: 0x8028bd98 */
extern s32 g_dwVisibleGridMaxCol;

/** @ghidraAddress USA: 0x8028bd9c */
extern s32 g_dwVisibleGridMaxRow;

/** The first covered column of each row. @ghidraAddress USA: 0x8034e5b0 */
extern s32 g_adwVisibleGridRowMinCol[VISIBLE_GRID_SIZE];

/** The last covered column of each row. @ghidraAddress USA: 0x8034e6b0 */
extern s32 g_adwVisibleGridRowMaxCol[VISIBLE_GRID_SIZE];

/** The first covered row of each column. @ghidraAddress USA: 0x8034e7b0 */
extern s32 g_adwVisibleGridColMinRow[VISIBLE_GRID_SIZE];

/** The last covered row of each column. @ghidraAddress USA: 0x8034e8b0 */
extern s32 g_adwVisibleGridColMaxRow[VISIBLE_GRID_SIZE];

/**
 * Widen the covered columns of a row to take in a cell, clamped to the grid.
 *
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @ghidraAddress USA: 0x8022d090
 */
void MarkVisibleCell(s32 col, s32 row);

/**
 * Mark the cells a line between two points of the track passes through.
 *
 * @param x0 The X coordinate of the first point.
 * @param y0 The Y coordinate of the first point.
 * @param x1 The X coordinate of the second point.
 * @param y1 The Y coordinate of the second point.
 * @ghidraAddress USA: 0x8022d104
 */
void MarkVisibleEdge(f32 x0, f32 y0, f32 x1, f32 y1);

/**
 * Whether a cell is covered.
 *
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @return Non-zero when the cell is covered.
 * @ghidraAddress USA: 0x8022d3d4
 */
s32 IsCellVisible(s32 col, s32 row);

/**
 * Whether the cell under a point of the track is covered.
 *
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return Non-zero when the cell is covered.
 * @ghidraAddress USA: 0x8022d43c
 */
s32 IsPointVisible(f32 x, f32 y);

/**
 * Build the grid from the edges of the view frustum.
 *
 * @ghidraAddress USA: 0x8022d49c
 */
void BuildVisibleGrid(void);

#endif

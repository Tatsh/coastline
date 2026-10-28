#ifndef COASTLINE_VISGRID_H
#define COASTLINE_VISGRID_H

/**
 * @file
 * The grid of the cells of the track the view frustum covers, which decides what is drawn.
 */

#include "types.h"
#include "vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The cells along each side of the grid. */
#define VISIBLE_GRID_SIZE 64

/* The points of the view frustum. */
#define FRUSTUM_POINTS 6       /*!< The number of points of the frustum. */
#define FRUSTUM_CAMERA 0       /*!< The point of the camera. */
#define FRUSTUM_RIGHT_TOP 1    /*!< The far corner at the top right. */
#define FRUSTUM_LEFT_TOP 2     /*!< The far corner at the top left. */
#define FRUSTUM_LEFT_BOTTOM 3  /*!< The far corner at the bottom left. */
#define FRUSTUM_RIGHT_BOTTOM 4 /*!< The far corner at the bottom right. */
#define FRUSTUM_FAR 5          /*!< The centre of the far plane. */

/**
 * @ghidraAddress USA: 0x8031b1f0
 * @ghidraAddress EUR: 0x8031d6f0
 * @ghidraAddress JPN: 0x8031c320
 * @ghidraAddress BRD3D: 0x106c3310
 * @ghidraAddress BRGlide: 0x106ea3a0
 */
extern Vec3f g_avFrustumPoints[FRUSTUM_POINTS];

#if !TGR_PC
/**
 * The bounds of the covered cells.
 *
 * @ghidraAddress USA: 0x8028bd90
 * @ghidraAddress EUR: 0x8028e4f0
 * @ghidraAddress JPN: 0x8028df20
 * @ghidraAddress BRD3D: 0x10a9bbc8
 * @ghidraAddress BRGlide: 0x10ac2c58
 */
extern s32 g_dwVisibleGridMinCol;
#endif /* !TGR_PC */

/**
 * @ghidraAddress USA: 0x8028bd94
 * @ghidraAddress EUR: 0x8028e4f4
 * @ghidraAddress JPN: 0x8028df24
 * @ghidraAddress BRD3D: 0x10a9bbcc
 * @ghidraAddress BRGlide: 0x10ac2c5c
 */
extern s32 g_dwVisibleGridMinRow;

#if !TGR_PC
/**
 * @ghidraAddress USA: 0x8028bd98
 * @ghidraAddress EUR: 0x8028e4f8
 * @ghidraAddress JPN: 0x8028df28
 * @ghidraAddress BRD3D: 0x10a9bbc0
 * @ghidraAddress BRGlide: 0x10ac2c50
 */
extern s32 g_dwVisibleGridMaxCol;
#endif /* !TGR_PC */

/**
 * @ghidraAddress USA: 0x8028bd9c
 * @ghidraAddress EUR: 0x8028e4fc
 * @ghidraAddress JPN: 0x8028df2c
 * @ghidraAddress BRD3D: 0x10a9bbc4
 * @ghidraAddress BRGlide: 0x10ac2c54
 */
extern s32 g_dwVisibleGridMaxRow;

/**
 * The first covered column of each row.
 *
 * @ghidraAddress USA: 0x8034e5b0
 * @ghidraAddress EUR: 0x80350ab0
 * @ghidraAddress JPN: 0x8034eb30
 * @ghidraAddress BRD3D: 0x10a9bbd0
 * @ghidraAddress BRGlide: 0x10ac2c60
 */
extern s32 g_adwVisibleGridRowMinCol[VISIBLE_GRID_SIZE];

/**
 * The last covered column of each row.
 *
 * @ghidraAddress USA: 0x8034e6b0
 * @ghidraAddress EUR: 0x80350bb0
 * @ghidraAddress JPN: 0x8034ec30
 * @ghidraAddress BRD3D: 0x10a9bcd0
 * @ghidraAddress BRGlide: 0x10ac2d60
 */
extern s32 g_adwVisibleGridRowMaxCol[VISIBLE_GRID_SIZE];

/**
 * The first covered row of each column.
 *
 * @ghidraAddress USA: 0x8034e7b0
 * @ghidraAddress EUR: 0x80350cb0
 * @ghidraAddress JPN: 0x8034ed30
 * @ghidraAddress BRD3D: 0x10a9bdd0
 * @ghidraAddress BRGlide: 0x10ac2e60
 */
extern s32 g_adwVisibleGridColMinRow[VISIBLE_GRID_SIZE];

/**
 * The last covered row of each column.
 *
 * @ghidraAddress USA: 0x8034e8b0
 * @ghidraAddress EUR: 0x80350db0
 * @ghidraAddress JPN: 0x8034ee30
 * @ghidraAddress BRD3D: 0x10a9bed0
 * @ghidraAddress BRGlide: 0x10ac2f60
 */
extern s32 g_adwVisibleGridColMaxRow[VISIBLE_GRID_SIZE];

/**
 * Widen the covered columns of a row to take in a cell, clamped to the grid.
 *
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @ghidraAddress USA: 0x8022d090
 * @ghidraAddress EUR: 0x8022e5a0
 * @ghidraAddress JPN: 0x8022dd50
 * @ghidraAddress BRD3D: 0x1003a660
 * @ghidraAddress BRGlide: 0x10033ce0
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
 * @ghidraAddress EUR: 0x8022e614
 * @ghidraAddress JPN: 0x8022ddc4
 * @ghidraAddress BRD3D: 0x1003a6b0
 * @ghidraAddress BRGlide: 0x10033d30
 */
void MarkVisibleEdge(f32 x0, f32 y0, f32 x1, f32 y1);

/**
 * Whether a cell is covered.
 *
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @return Non-zero when the cell is covered.
 * @ghidraAddress USA: 0x8022d3d4
 * @ghidraAddress EUR: 0x8022e8e4
 * @ghidraAddress JPN: 0x8022e094
 * @ghidraAddress BRD3D: 0x1003a910
 * @ghidraAddress BRGlide: 0x10033f90
 */
s32 IsCellVisible(s32 col, s32 row);

/**
 * Whether the cell under a point of the track is covered.
 *
 * @param x The X coordinate of the point.
 * @param y The Y coordinate of the point.
 * @return Non-zero when the cell is covered.
 * @ghidraAddress USA: 0x8022d43c
 * @ghidraAddress EUR: 0x8022e94c
 * @ghidraAddress JPN: 0x8022e0fc
 * @ghidraAddress BRD3D: 0x1003a950
 * @ghidraAddress BRGlide: 0x10033fd0
 */
s32 IsPointVisible(f32 x, f32 y);

/**
 * Build the grid from the edges of the view frustum.
 *
 * @ghidraAddress USA: 0x8022d49c
 * @ghidraAddress EUR: 0x8022e9ac
 * @ghidraAddress JPN: 0x8022e15c
 * @ghidraAddress BRD3D: 0x1003a990
 * @ghidraAddress BRGlide: 0x10034010
 */
void BuildVisibleGrid(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VISGRID_H */

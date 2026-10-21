#ifndef COASTLINE_TRACKRENDER_H
#define COASTLINE_TRACKRENDER_H

/**
 * @file
 * The drawing of the placed models of the track near the camera, and of the shadows of the
 * vehicles on them.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "vecmath.h"
#include "vehicle.h"

/* The room for the shadows, in each of two frames. */
#define SHADOW_COMMANDS 500  /*!< The display list commands the shadows of a frame can use. */
#define SHADOW_VERTICES 1000 /*!< The vertices the shadows of a frame can use. */

/**
 * The display lists and vertices of the shadows, one of each for each frame buffer.
 *
 * @ghidraAddress USA: 0x80353580
 */
extern Gfx g_aShadowDisplayLists[2][SHADOW_COMMANDS];

/** @ghidraAddress USA: 0x803554c0 */
extern Vtx g_aShadowVertices[2][SHADOW_VERTICES];

/** The next command of the shadows, and their first this frame. @ghidraAddress USA: 0x8028c75c */
extern Gfx *g_pShadowDisplayListWritePtr;

/** @ghidraAddress USA: 0x8028c760 */
extern Gfx *g_pShadowDisplayListStart;

/** The next vertex of the shadows, and their first this frame. @ghidraAddress USA: 0x8028c764 */
extern Vtx *g_pShadowVertexWritePtr;

/** @ghidraAddress USA: 0x8028c768 */
extern Vtx *g_pShadowVertexStart;

/**
 * Whether the shadows keep the triangles off their texture, and draw beyond the rectangle of the
 * screen of their vehicle.
 *
 * @ghidraAddress USA: 0x8028aa60
 */
extern s32 g_bShadowCullDisabled;

/** A cell of the grid of the track in view, and its squared distance from the camera in cells. */
typedef struct {
    u8 column;    /*!< The column of the cell in the grid. */
    u8 row;       /*!< The row of the cell in the grid. */
    s16 distance; /*!< The squared distance of the cell from the camera, in cells. */
} VisibleCell;

/** The most cells in view, past which the farthest are left out. */
#define VISIBLE_CELLS 192

/**
 * The cells in view, the nearest first, ending with a column of 0xFF.
 *
 * @ghidraAddress USA: 0x8035d1d0
 */
extern VisibleCell g_aVisibleCellTuples[VISIBLE_CELLS + 1];

/** Whether the cells in view were more than VISIBLE_CELLS. @ghidraAddress USA: 0x8028c76c */
extern s32 g_bVisibleCellListOverflow;

/** The placed models in view this frame. @ghidraAddress USA: 0x8028c740 */
extern s32 g_nVisibleInstanceCount;

/**
 * For each placed model: 0xFF until it is listed in view, then 0 with a bit for each vehicle near
 * enough to cast its shadow on it.
 *
 * @ghidraAddress USA: 0x80351d80
 */
extern u8 g_abInstanceVisibilityMask[];

/**
 * Where in the list of the models the cells start that are at least 3 cells from the camera, more
 * than 1 or, in a replay from the trackside camera, 3, and within 3 cells of the far plane, or -1.
 *
 * @ghidraAddress USA: 0x8028c778
 */
extern s32 g_nVisibleCellIdxNear;

/** @ghidraAddress USA: 0x8028c788 */
extern s32 g_nVisibleCellIdxMid;

/** @ghidraAddress USA: 0x8028c780 */
extern s32 g_nVisibleCellIdxFar;

/**
 * Where the translucent models past g_nVisibleCellIdxMid start in the second pass, which draws
 * them without shadows, and 0x1000 until it is known.
 *
 * @ghidraAddress USA: 0x8028c78c
 */
extern s32 g_nTrackTransparentStartIdx;

/**
 * Set to 0x1000 with g_nTrackTransparentStartIdx, and never read.
 *
 * @ghidraAddress USA: 0x8028c77c
 */
extern s32 g_nTrackTransparentStartIdxNear;

/** @ghidraAddress USA: 0x8028c784 */
extern s32 g_nTrackTransparentStartIdxFar;

/** The last translucent model left for the second pass, or -1. @ghidraAddress USA: 0x8028c748 */
extern s32 g_nDeferredInstanceLastIdx;

/** The TRACK_INSTANCE_ flags of the placed models not drawn. @ghidraAddress USA: 0x8035d520 */
extern u32 g_dwInstanceHideFlags;

/** The groups of placed models hidden from the camera. @ghidraAddress USA: 0x8028c770 */
extern u16 g_wActiveInstanceHideMask;

/** Whether g_nDebugHiddenInstanceIndex is left undrawn. @ghidraAddress USA: 0x8028c758 */
extern s32 g_bDebugHideInstance;

/**
 * The blender and the other bits of the render mode of the track.
 *
 * @ghidraAddress USA: 0x8028c74c
 */
extern u32 g_dwTrackOtherModeLBlend;

/** @ghidraAddress USA: 0x8028c750 */
extern u32 g_dwTrackOtherModeLFlags;

/**
 * The vehicles nearest the camera that cast shadows, nearest first.
 *
 * @ghidraAddress USA: 0x8035d4fc
 */
extern s32 g_nNearbyVehicleCount;

/** @ghidraAddress USA: 0x8035d4e8 */
extern s32 g_anNearbyVehicleIdx[MAX_VEHICLES];

/** How far ahead of the camera each one is. @ghidraAddress USA: 0x8035d4d8 */
extern f32 g_afNearbyVehicleDot[MAX_VEHICLES];

/** From the camera to a vehicle. @ghidraAddress USA: 0x8035d500 */
extern Vec3f g_vVehicleVisibilityDelta;

/**
 * The largest absolute value of the first three rows of g_afViewProjMatrix.
 *
 * @ghidraAddress USA: 0x8035d51c
 */
extern f32 g_fViewMatrixMaxAbs;

/** The look-at of the reflections of the track. @ghidraAddress USA: 0x8028c774 */
extern LookAt *g_pTrackLookAt;

/** The lights of the track, one of four in turn each frame. @ghidraAddress USA: 0x8028c640 */
extern Lights1 g_aTrackLights[4];

/** @ghidraAddress USA: 0x8028c6a0 */
extern s32 g_nTrackLightBufferIndex;

/** The direction of the light of the track. @ghidraAddress USA: 0x8035d510 */
extern Vec3f g_vTrackLightDirection;

/**
 * The forward of the vehicle whose shadow is being cast, on the ground.
 *
 * @ghidraAddress USA: 0x8035d1c0
 */
extern Vec3f g_vShadowForward;

/**
 * Take a point and a size around it to the corners of the rectangle of the screen they cover in
 * the view being drawn, which are left as they were when the point is at the camera.
 *
 * @param point The point.
 * @param size The size around it.
 * @param low The left and the lower edge.
 * @param high The right and the upper edge.
 * @ghidraAddress USA: 0x80233e10
 */
void ProjectWorldToScreenRect(Vec3f *point, s32 size, s16 low[2], s16 high[2]);

/**
 * Start the display lists and vertices of the shadows of the frame.
 *
 * @ghidraAddress USA: 0x80233fdc
 */
void ResetShadowBuffers(void);

/**
 * Draw a placed model, and on it the shadow of each vehicle in a mask, by its vertices taken to the
 * texture of the shadow and its triangles off the texture left out.
 *
 * @param index The placed model.
 * @param vehicles A bit for each vehicle whose shadow falls on it.
 * @param deferred Whether this is the second pass, which draws the shadows as translucent.
 * @ghidraAddress USA: 0x80234050
 */
void RenderTrackInstance(s32 index, s32 vehicles, s32 deferred);

/**
 * Order two cells in view by their distance from the camera.
 *
 * @param a The first cell.
 * @param b The second cell.
 * @return -1 when the first is nearer, 1 when the second is, or 0 when they are as near.
 * @ghidraAddress USA: 0x80234fc0
 */
s32 CompareVisibleCellDistance(const void *a, const void *b);

/**
 * List the placed models of the cells in view, the nearest cells first, mark the models the
 * nearest vehicles cast shadows on, set the lights and the reflections of the track, and find the
 * rectangle of the screen of the shadow of each vehicle.
 *
 * @ghidraAddress USA: 0x80234ff8
 */
void BuildVisibleInstanceList(void);

/**
 * Draw the placed models in view: in the first pass the opaque ones, leaving the translucent ones,
 * and in the second pass those, the farthest first.
 *
 * @param deferred Whether this is the second pass.
 * @ghidraAddress USA: 0x80235bac
 */
void RenderTrackGeometry(s32 deferred);

#endif

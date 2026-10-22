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
 * @ghidraAddress JPN: 0x803546f0
 * @ghidraAddress EUR: 0x80355a80
 */
extern Gfx g_aShadowDisplayLists[2][SHADOW_COMMANDS];

/**
 * @ghidraAddress USA: 0x803554c0
 * @ghidraAddress JPN: 0x80356630
 * @ghidraAddress EUR: 0x803579c0
 */
extern Vtx g_aShadowVertices[2][SHADOW_VERTICES];

/**
 * The next command of the shadows, and their first this frame.
 *
 * @ghidraAddress USA: 0x8028c75c
 * @ghidraAddress JPN: 0x8028e90c
 * @ghidraAddress EUR: 0x8028eebc
 */
extern Gfx *g_pShadowDisplayListWritePtr;

/**
 * @ghidraAddress USA: 0x8028c760
 * @ghidraAddress JPN: 0x8028e910
 * @ghidraAddress EUR: 0x8028eec0
 */
extern Gfx *g_pShadowDisplayListStart;

/**
 * The next vertex of the shadows, and their first this frame.
 *
 * @ghidraAddress USA: 0x8028c764
 * @ghidraAddress JPN: 0x8028e914
 * @ghidraAddress EUR: 0x8028eec4
 */
extern Vtx *g_pShadowVertexWritePtr;

/**
 * @ghidraAddress USA: 0x8028c768
 * @ghidraAddress JPN: 0x8028e918
 * @ghidraAddress EUR: 0x8028eec8
 */
extern Vtx *g_pShadowVertexStart;

/**
 * Whether the shadows keep the triangles off their texture, and draw beyond the rectangle of the
 * screen of their vehicle.
 *
 * @ghidraAddress USA: 0x8028aa60
 * @ghidraAddress JPN: 0x8028c738
 * @ghidraAddress EUR: 0x8028cc68
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
 * @ghidraAddress JPN: 0x8035e340
 * @ghidraAddress EUR: 0x8035f6d0
 */
extern VisibleCell g_aVisibleCellTuples[VISIBLE_CELLS + 1];

/**
 * Whether the cells in view were more than VISIBLE_CELLS.
 *
 * @ghidraAddress USA: 0x8028c76c
 * @ghidraAddress JPN: 0x8028e91c
 * @ghidraAddress EUR: 0x8028eecc
 */
extern s32 g_bVisibleCellListOverflow;

/**
 * The placed models in view this frame.
 *
 * @ghidraAddress USA: 0x8028c740
 * @ghidraAddress JPN: 0x8028e8f0
 * @ghidraAddress EUR: 0x8028eea0
 */
extern s32 g_nVisibleInstanceCount;

/**
 * For each placed model: 0xFF until it is listed in view, then 0 with a bit for each vehicle near
 * enough to cast its shadow on it.
 *
 * @ghidraAddress USA: 0x80351d80
 * @ghidraAddress JPN: 0x80352ef0
 * @ghidraAddress EUR: 0x80354280
 */
extern u8 g_abInstanceVisibilityMask[];

/**
 * Where in the list of the models the cells start that are at least 3 cells from the camera, more
 * than 1 or, in a replay from the trackside camera, 3, and within 3 cells of the far plane, or -1.
 *
 * @ghidraAddress USA: 0x8028c778
 * @ghidraAddress JPN: 0x8028e928
 * @ghidraAddress EUR: 0x8028eed8
 */
extern s32 g_nVisibleCellIdxNear;

/**
 * @ghidraAddress USA: 0x8028c788
 * @ghidraAddress JPN: 0x8028e938
 * @ghidraAddress EUR: 0x8028eee8
 */
extern s32 g_nVisibleCellIdxMid;

/**
 * @ghidraAddress USA: 0x8028c780
 * @ghidraAddress JPN: 0x8028e930
 * @ghidraAddress EUR: 0x8028eee0
 */
extern s32 g_nVisibleCellIdxFar;

/**
 * Where the translucent models past g_nVisibleCellIdxMid start in the second pass, which draws
 * them without shadows, and 0x1000 until it is known.
 *
 * @ghidraAddress USA: 0x8028c78c
 * @ghidraAddress JPN: 0x8028e93c
 * @ghidraAddress EUR: 0x8028eeec
 */
extern s32 g_nTrackTransparentStartIdx;

/**
 * Set to 0x1000 with g_nTrackTransparentStartIdx, and never read.
 *
 * @ghidraAddress USA: 0x8028c77c
 * @ghidraAddress JPN: 0x8028e92c
 * @ghidraAddress EUR: 0x8028eedc
 */
extern s32 g_nTrackTransparentStartIdxNear;

/**
 * @ghidraAddress USA: 0x8028c784
 * @ghidraAddress JPN: 0x8028e934
 * @ghidraAddress EUR: 0x8028eee4
 */
extern s32 g_nTrackTransparentStartIdxFar;

/**
 * The last translucent model left for the second pass, or -1.
 *
 * @ghidraAddress USA: 0x8028c748
 * @ghidraAddress JPN: 0x8028e8f8
 * @ghidraAddress EUR: 0x8028eea8
 */
extern s32 g_nDeferredInstanceLastIdx;

/**
 * The TRACK_INSTANCE_ flags of the placed models not drawn.
 *
 * @ghidraAddress USA: 0x8035d520
 * @ghidraAddress EUR: 0x8035fa20
 */
extern u32 g_dwInstanceHideFlags;

/**
 * The groups of placed models hidden from the camera.
 *
 * @ghidraAddress USA: 0x8028c770
 * @ghidraAddress JPN: 0x8028e920
 * @ghidraAddress EUR: 0x8028eed0
 */
extern u16 g_wActiveInstanceHideMask;

/**
 * Whether g_nDebugHiddenInstanceIndex is left undrawn.
 *
 * @ghidraAddress USA: 0x8028c758
 * @ghidraAddress JPN: 0x8028e908
 * @ghidraAddress EUR: 0x8028eeb8
 */
extern s32 g_bDebugHideInstance;

/**
 * The blender and the other bits of the render mode of the track.
 *
 * @ghidraAddress USA: 0x8028c74c
 * @ghidraAddress JPN: 0x8028e8fc
 * @ghidraAddress EUR: 0x8028eeac
 */
extern u32 g_dwTrackOtherModeLBlend;

/**
 * @ghidraAddress USA: 0x8028c750
 * @ghidraAddress JPN: 0x8028e900
 * @ghidraAddress EUR: 0x8028eeb0
 */
extern u32 g_dwTrackOtherModeLFlags;

/**
 * The vehicles nearest the camera that cast shadows, nearest first.
 *
 * @ghidraAddress USA: 0x8035d4fc
 * @ghidraAddress JPN: 0x8035e66c
 * @ghidraAddress EUR: 0x8035f9fc
 */
extern s32 g_nNearbyVehicleCount;

/**
 * @ghidraAddress USA: 0x8035d4e8
 * @ghidraAddress JPN: 0x8035e658
 * @ghidraAddress EUR: 0x8035f9e8
 */
extern s32 g_anNearbyVehicleIdx[MAX_VEHICLES];

/**
 * How far ahead of the camera each one is.
 *
 * @ghidraAddress USA: 0x8035d4d8
 * @ghidraAddress JPN: 0x8035e648
 * @ghidraAddress EUR: 0x8035f9d8
 */
extern f32 g_afNearbyVehicleDot[MAX_VEHICLES];

/**
 * From the camera to a vehicle.
 *
 * @ghidraAddress USA: 0x8035d500
 * @ghidraAddress JPN: 0x8035e670
 * @ghidraAddress EUR: 0x8035fa00
 */
extern Vec3f g_vVehicleVisibilityDelta;

/**
 * The largest absolute value of the first three rows of g_afViewProjMatrix.
 *
 * @ghidraAddress USA: 0x8035d51c
 * @ghidraAddress JPN: 0x8035e68c
 * @ghidraAddress EUR: 0x8035fa1c
 */
extern f32 g_fViewMatrixMaxAbs;

/**
 * The look-at of the reflections of the track.
 *
 * @ghidraAddress USA: 0x8028c774
 * @ghidraAddress JPN: 0x8028e924
 * @ghidraAddress EUR: 0x8028eed4
 */
extern LookAt *g_pTrackLookAt;

/**
 * The lights of the track, one of four in turn each frame.
 *
 * @ghidraAddress USA: 0x8028c640
 * @ghidraAddress JPN: 0x8028e7f0
 * @ghidraAddress EUR: 0x8028eda0
 */
extern Lights1 g_aTrackLights[4];

/**
 * @ghidraAddress USA: 0x8028c6a0
 * @ghidraAddress JPN: 0x8028e850
 * @ghidraAddress EUR: 0x8028ee00
 */
extern s32 g_nTrackLightBufferIndex;

/**
 * The direction of the light of the track.
 *
 * @ghidraAddress USA: 0x8035d510
 * @ghidraAddress JPN: 0x8035e680
 * @ghidraAddress EUR: 0x8035fa10
 */
extern Vec3f g_vTrackLightDirection;

/**
 * The forward of the vehicle whose shadow is being cast, on the ground.
 *
 * @ghidraAddress USA: 0x8035d1c0
 * @ghidraAddress JPN: 0x8035e330
 * @ghidraAddress EUR: 0x8035f6c0
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
 * @ghidraAddress JPN: 0x80235120
 * @ghidraAddress EUR: 0x80235320
 */
void ProjectWorldToScreenRect(Vec3f *point, s32 size, s16 low[2], s16 high[2]);

/**
 * Start the display lists and vertices of the shadows of the frame.
 *
 * @ghidraAddress USA: 0x80233fdc
 * @ghidraAddress JPN: 0x802352ec
 * @ghidraAddress EUR: 0x802354ec
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
 * @ghidraAddress JPN: 0x80235360
 * @ghidraAddress EUR: 0x80235560
 */
void RenderTrackInstance(s32 index, s32 vehicles, s32 deferred);

/**
 * Order two cells in view by their distance from the camera.
 *
 * @param a The first cell.
 * @param b The second cell.
 * @return -1 when the first is nearer, 1 when the second is, or 0 when they are as near.
 * @ghidraAddress USA: 0x80234fc0
 * @ghidraAddress JPN: 0x802362d0
 * @ghidraAddress EUR: 0x802364d0
 */
s32 CompareVisibleCellDistance(const void *a, const void *b);

/**
 * List the placed models of the cells in view, the nearest cells first, mark the models the
 * nearest vehicles cast shadows on, set the lights and the reflections of the track, and find the
 * rectangle of the screen of the shadow of each vehicle.
 *
 * @ghidraAddress USA: 0x80234ff8
 * @ghidraAddress JPN: 0x80236308
 * @ghidraAddress EUR: 0x80236508
 */
void BuildVisibleInstanceList(void);

/**
 * Draw the placed models in view: in the first pass the opaque ones, leaving the translucent ones,
 * and in the second pass those, the farthest first.
 *
 * @param deferred Whether this is the second pass.
 * @ghidraAddress USA: 0x80235bac
 * @ghidraAddress JPN: 0x80236ebc
 * @ghidraAddress EUR: 0x802370bc
 * @stub{JPN}
 */
void RenderTrackGeometry(s32 deferred);

#endif

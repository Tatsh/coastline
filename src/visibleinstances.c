#include <PR/gu.h>

#include "camera.h"
#include "instantreplay.h"
#include "race.h"
#include "racerslotaccess.h"
#include "render.h"
#include "track.h"
#include "trackrender.h"
#include "util.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "visgrid.h"
#include "weather.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <stdlib.h>

#include "platform/win/ghost.h"
#else
#include "audiomix.h"
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
namespace {
    /**
     * Whether the cells in view were more than #VISIBLE_CELLS.
     *
     * @ghidraAddress USA: 0x8028c76c
     * @ghidraAddress EUR: 0x8028eecc
     * @ghidraAddress JPN: 0x8028e91c
     * @ghidraAddress BRD3D: 0x102e5ed0
     */
    [[maybe_unused, gnu::used]] s32 g_bVisibleCellListOverflow;
    /**
     * The vehicles nearest the camera that cast shadows, nearest first.
     *
     * @ghidraAddress USA: 0x8035d4fc
     * @ghidraAddress EUR: 0x8035f9fc
     * @ghidraAddress JPN: 0x8035e66c
     * @ghidraAddress BRD3D: 0x102e5ed8
     */
    s32 g_nNearbyVehicleCount;
} // namespace
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x1036439c */
u32 g_dwTrackOtherModeLBlend;
/** @ghidraAddress BRD3D: 0x103643a0 */
u32 g_dwTrackOtherModeLFlags;
/** @ghidraAddress BRD3D: 0x1039b6c0 */
s32 g_nDebugHiddenInstanceIndex;
/** @ghidraAddress BRD3D: 0x10364388 */
s32 g_nTrackLightBufferIndex;
/** @ghidraAddress BRD3D: 0x102e5f24 */
s32 g_nVisibleCellIdxFar;
/** @ghidraAddress BRD3D: 0x102e5ec0 */
s32 g_nVisibleCellIdxMid;
/** @ghidraAddress BRD3D: 0x10363fec */
s32 g_nVisibleCellIdxNear;
/** @ghidraAddress BRD3D: 0x103643a4 */
s32 g_nVisibleInstanceCount;
/** @ghidraAddress BRD3D: 0x103643b8 */
LookAt *g_pTrackLookAt;
/** @ghidraAddress BRD3D: 0x1039b6cc */
u16 g_wActiveInstanceHideMask;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN

#define VIS_FILL_MASK FillMemoryPatternBytes
#define VIS_SORT qsort
#define VIS_INTERMEDIATE(value) ((long double)(value))
#define VIS_TRUNCATE(value) ((s32)(s64)(value))
#else
#define VIS_FILL_MASK MemFill64Aligned
#define VIS_SORT QuickSort
#define VIS_INTERMEDIATE(value) (value)
#define VIS_TRUNCATE(value) ((s32)(value))
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#define GRID_CELL_SIZE 32.0f
#define GRID_CELLS 64
#define NEAR_CELL_DISTANCE 9

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define VIS_SCREEN_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
inline void Vehicle::UpdateShadowScreenRect(const ViewportRect *viewports) {
    Vec3f point;
    point.x = frame.position.x;
    point.y = frame.position.y;
    point.z = (long double)frame.position.z - groundDistance;
    ProjectWorldToScreenRect(viewports, &point, 8, shadowCorners[0], shadowCorners[1]);
    /* The view is indexed afresh for each bound, and read again after each store. */
    if (shadowCorners[0][0] < viewports[g_dwCurrentSplitScreenIndex].x) {
        shadowCorners[0][0] = viewports[g_dwCurrentSplitScreenIndex].x;
    }
    if (shadowCorners[1][0] < viewports[g_dwCurrentSplitScreenIndex].x) {
        shadowCorners[1][0] = viewports[g_dwCurrentSplitScreenIndex].x;
    }
    if (shadowCorners[0][1] < viewports[g_dwCurrentSplitScreenIndex].y) {
        shadowCorners[0][1] = viewports[g_dwCurrentSplitScreenIndex].y;
    }
    if (shadowCorners[1][1] < viewports[g_dwCurrentSplitScreenIndex].y) {
        shadowCorners[1][1] = viewports[g_dwCurrentSplitScreenIndex].y;
    }
    if (VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].x,
                       viewports[g_dwCurrentSplitScreenIndex].width) < shadowCorners[0][0]) {
        shadowCorners[0][0] = VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].x,
                                             viewports[g_dwCurrentSplitScreenIndex].width);
    }
    if (VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].x,
                       viewports[g_dwCurrentSplitScreenIndex].width) < shadowCorners[1][0]) {
        shadowCorners[1][0] = VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].x,
                                             viewports[g_dwCurrentSplitScreenIndex].width);
    }
    if (VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].y,
                       viewports[g_dwCurrentSplitScreenIndex].height) < shadowCorners[0][1]) {
        shadowCorners[0][1] = VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].y,
                                             viewports[g_dwCurrentSplitScreenIndex].height);
    }
    if (VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].y,
                       viewports[g_dwCurrentSplitScreenIndex].height) < shadowCorners[1][1]) {
        shadowCorners[1][1] = VIS_SCREEN_ADD(viewports[g_dwCurrentSplitScreenIndex].y,
                                             viewports[g_dwCurrentSplitScreenIndex].height);
    }
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void BuildVisibleInstanceList(const ViewportRect *viewports,
                              const RacerSlot *slots,
                              const Vehicle *vehicles) {
#else
void BuildVisibleInstanceList(void) {
#endif
    VisibleCell *cell;
    TrackGridCursor cursor;
    Vehicle *vehicle;
#if TGR_PLATFORM == TGR_PLATFORM_N64
    Vec3f point;
#endif
    f32 *position;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double dot;
    f32 forwardX, forwardY;
#else
    f32 dot, forwardX, forwardY;
#endif
    s32 count, row, column, cameraColumn, cameraRow, farDistance, midDistance, i, j, bit;
    s32 columnDistance, rowDistance;
    u32 found;
    u16 index;

    g_bVisibleCellListOverflow = FALSE;
    count = 0;
    for (row = g_dwVisibleGridMinRow; row <= g_dwVisibleGridMaxRow; row++) {
        for (column = g_adwVisibleGridRowMinCol[row]; column <= g_adwVisibleGridRowMaxCol[row];
             column++) {
            if (count == VISIBLE_CELLS) {
                g_bVisibleCellListOverflow = TRUE;
                goto terminate;
            }
            g_aVisibleCellTuples[count].column = column;
            g_aVisibleCellTuples[count].row = row;
            count++;
        }
    }
terminate:
    g_aVisibleCellTuples[count].column = 0xFF;
    g_aVisibleCellTuples[count].row = 0xFF;

    /* The cell of the camera, kept on the grid at its far edges. */
    cameraColumn =
        VIS_TRUNCATE(VIS_INTERMEDIATE(g_pActiveCamera->position.x) * (1.0f / GRID_CELL_SIZE));
    if (cameraColumn == GRID_CELLS) {
        cameraColumn--;
    }
    cameraRow =
        VIS_TRUNCATE(VIS_INTERMEDIATE(g_pActiveCamera->position.y) * (1.0f / GRID_CELL_SIZE));
    if (cameraRow == GRID_CELLS) {
        cameraRow--;
    }
    for (i = 0; i < count; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        columnDistance = (s32)((u32)g_aVisibleCellTuples[i].column - (u32)cameraColumn);
        rowDistance = (s32)((u32)g_aVisibleCellTuples[i].row - (u32)cameraRow);
        g_aVisibleCellTuples[i].distance =
            (s16)((u32)columnDistance * (u32)columnDistance + (u32)rowDistance * (u32)rowDistance);
#else
        columnDistance = g_aVisibleCellTuples[i].column - cameraColumn;
        rowDistance = g_aVisibleCellTuples[i].row - cameraRow;
        g_aVisibleCellTuples[i].distance =
            columnDistance * columnDistance + rowDistance * rowDistance;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }
    VIS_SORT(g_aVisibleCellTuples, count, sizeof(VisibleCell), CompareVisibleCellDistance);

    VIS_FILL_MASK(g_abInstanceVisibilityMask, g_LoadedTrack.instanceCount, -1);
    g_nVisibleInstanceCount = 0;
    g_nVisibleCellIdxNear = -1;
    g_nVisibleCellIdxFar = -1;
    g_nVisibleCellIdxMid = -1;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    farDistance = VIS_TRUNCATE((long double)gFrustumFar * (-1.0f / GRID_CELL_SIZE));
    farDistance = (s32)(-3u - (u32)farDistance);
    farDistance = (s32)((u32)farDistance * (u32)farDistance);
    if (gInstantReplayActive &&
        g_pActiveCamera ==
            &VEHICLE_GET(g_pActiveCameraVehicle, Cameras)[VEHICLE_CAMERA_TRACKSIDE]) {
#else
    farDistance = (s32)(gFrustumFar / GRID_CELL_SIZE);
    farDistance -= 3;
    farDistance *= farDistance;
    if (g_pActiveCameraVehicle->input->playbackBuffer != NULL &&
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        midDistance = NEAR_CELL_DISTANCE;
    } else {
        midDistance = 1;
    }

    /* The models of the cells, the nearest cells first, each model once. */
    for (cell = g_aVisibleCellTuples; cell->column != 0xFF; cell++) {
        if (cell->distance >= NEAR_CELL_DISTANCE && g_nVisibleCellIdxNear == -1) {
            g_nVisibleCellIdxNear = g_nVisibleInstanceCount;
        }
        if (midDistance < cell->distance && g_nVisibleCellIdxMid == -1) {
            g_nVisibleCellIdxMid = g_nVisibleInstanceCount;
        }
        if (farDistance < cell->distance && g_nVisibleCellIdxFar == -1) {
            g_nVisibleCellIdxFar = g_nVisibleInstanceCount;
        }
        /* The model an animation flies is listed with every cell while it is in view. */
        /* The original takes the address of the position, and only the call reads it. */
        if (g_dwAirplaneInstance != 0 && g_abInstanceVisibilityMask[g_dwAirplaneInstance] != 0 &&
            (position = g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[3],
             IsPointVisible(position[0], position[1]))) {
            g_awVisibleInstanceList[g_nVisibleInstanceCount++] = g_dwAirplaneInstance;
            g_abInstanceVisibilityMask[g_dwAirplaneInstance] = 0;
        }
        found = LookupInstanceGridCell(cell->column, cell->row);
        cursor.start = found;
        cursor.count = found >> 16;
        if (found != 0) {
            while ((index = NextGridCellInstance(&cursor)) != 0) {
                if (g_abInstanceVisibilityMask[index] != 0 && g_dwAirplaneInstance != index) {
                    g_awVisibleInstanceList[g_nVisibleInstanceCount++] = index;
                    g_abInstanceVisibilityMask[index] = 0;
                }
            }
        }
    }

    /* The vehicles in view ahead of the camera, the nearest first, cast shadows. */
    g_nNearbyVehicleCount = 0;
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        if (VEHICLE_GET(&vehicles[i], Driver) == NULL || !g_adwVehicleVisible[i]) {
            continue;
        }
        Vec3fSubtract(&g_vVehicleVisibilityDelta,
                      &VEHICLE_GET(&vehicles[i], Frame).position,
                      &g_pActiveCamera->position);
        dot = Vec3fDot(&g_pActiveCamera->forward, &g_vVehicleVisibilityDelta);
        if (!(dot >= 2.0f)) {
            continue;
        }
        for (j = g_nNearbyVehicleCount - 1; j >= 0 && !(dot >= g_afNearbyVehicleDot[j]); j--) {
#else
        if (!g_adwVehicleVisible[i]) {
            continue;
        }
        Vec3fSubtract(
            &g_vVehicleVisibilityDelta, &g_aVehicles[i].frame.position, &g_pActiveCamera->position);
        dot = Vec3fDot(&g_pActiveCamera->forward, &g_vVehicleVisibilityDelta);
        if (dot < 2.0f) {
            continue;
        }
        for (j = g_nNearbyVehicleCount - 1; j >= 0 && dot < g_afNearbyVehicleDot[j]; j--) {
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
            g_afNearbyVehicleDot[j + 1] = g_afNearbyVehicleDot[j];
            g_anNearbyVehicleIdx[j + 1] = g_anNearbyVehicleIdx[j];
        }
        g_afNearbyVehicleDot[j + 1] = dot;
        g_anNearbyVehicleIdx[j + 1] = i;
        g_nNearbyVehicleCount++;
    }
    if (g_nNearbyVehicleCount >= 5) {
        g_nNearbyVehicleCount = 4;
    }
    for (i = 0; i < g_nNearbyVehicleCount; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        bit = (u8)(1u << ((u32)g_anNearbyVehicleIdx[i] & 31));
        vehicles[g_anNearbyVehicleIdx[i]].MarkNearbyInstanceShadows(g_abInstanceVisibilityMask,
                                                                    bit);
#else
        vehicle = &g_aVehicles[g_anNearbyVehicleIdx[i]];
        bit = 1 << g_anNearbyVehicleIdx[i];
        for (j = 0; j < vehicle->nearbyInstanceCount; j++) {
            g_abInstanceVisibilityMask[vehicle->nearbyInstances[j]] |= bit;
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }

    g_wActiveInstanceHideMask = 0;
    for (i = 0; i < g_dwCameraRayHitCount; i++) {
        g_wActiveInstanceHideMask |=
            g_LoadedTrack.instances[g_awCameraRayHitInstances[i]].hidesGroups;
    }

    /* The reflections look along the camera on the ground, which must not be straight down. */
    forwardX = g_pActiveCamera->forward.x;
    forwardY = g_pActiveCamera->forward.y;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!(forwardX < 0.0f || forwardX > 0.0f) && !(forwardY < 0.0f || forwardY > 0.0f)) {
#else
    if (forwardX == 0.0 && forwardY == 0.0) {
#endif
        forwardX = 0.0001f;
    }
    g_pTrackLookAt = AllocLightBlock();
    guLookAtReflectF(g_afMvpMatrix,
                     g_pTrackLookAt,
                     VIS_INTERMEDIATE(forwardX) * 50.0f,
                     VIS_INTERMEDIATE(forwardY) * 50.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f,
                     1.0f);

    /* At night the light of the track comes from the headlights of the car in view. */
    if (g_dwNightMode) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        g_vTrackLightDirection.x =
            (long double)VEHICLE_GET(g_pActiveCameraVehicle, Frame).up.x -
            (long double)VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward.x * 4.0f;
        g_vTrackLightDirection.y =
            (long double)VEHICLE_GET(g_pActiveCameraVehicle, Frame).up.y -
            (long double)VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward.y * 4.0f;
        g_vTrackLightDirection.z =
            (long double)VEHICLE_GET(g_pActiveCameraVehicle, Frame).up.z -
            (long double)VEHICLE_GET(g_pActiveCameraVehicle, Frame).forward.z * 4.0f;
#else
        g_vTrackLightDirection.x =
            g_pActiveCameraVehicle->frame.forward.x * -4.0f + g_pActiveCameraVehicle->frame.up.x;
        g_vTrackLightDirection.y =
            g_pActiveCameraVehicle->frame.forward.y * -4.0f + g_pActiveCameraVehicle->frame.up.y;
        g_vTrackLightDirection.z =
            g_pActiveCameraVehicle->frame.forward.z * -4.0f + g_pActiveCameraVehicle->frame.up.z;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    } else {
        g_vTrackLightDirection.x = g_afLightDirection[0];
        g_vTrackLightDirection.y = g_afLightDirection[1];
        g_vTrackLightDirection.z = g_afLightDirection[2];
    }
    Vec3fNormalize(&g_vTrackLightDirection);
    Vec3fScaleInPlace(&g_vTrackLightDirection, 120.0f);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_nTrackLightBufferIndex = (s32)((u32)g_nTrackLightBufferIndex + 1) % 4;
#else
    g_nTrackLightBufferIndex = (g_nTrackLightBufferIndex + 1) % 4;
#endif
    g_aTrackLights[g_nTrackLightBufferIndex] = g_DefaultLights;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[0] = VIS_TRUNCATE(g_vTrackLightDirection.x);
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[1] = VIS_TRUNCATE(g_vTrackLightDirection.y);
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[2] = VIS_TRUNCATE(g_vTrackLightDirection.z);

    /* The rectangle of the screen the shadow of each vehicle covers, kept to the view. */
    for (i = 0; i < g_dwNumVehicles; i++) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        vehicle = RACER_SLOT_GET(&slots[i], Vehicle);
        if (vehicle != NULL) {
            vehicle->UpdateShadowScreenRect(viewports);
        }
#else
        vehicle = g_aRacerSlots[i].vehicle;
        if (vehicle == NULL) {
            continue;
        }
        point.x = vehicle->frame.position.x;
        point.y = vehicle->frame.position.y;
        point.z = vehicle->frame.position.z - vehicle->groundDistance;
        ProjectWorldToScreenRect(&point, 8, vehicle->shadowCorners[0], vehicle->shadowCorners[1]);
        /* The view is indexed afresh for each bound, and read again after each store. */
        if (vehicle->shadowCorners[0][0] < g_aViewportRects[g_dwCurrentSplitScreenIndex].x) {
            vehicle->shadowCorners[0][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
        }
        if (vehicle->shadowCorners[1][0] < g_aViewportRects[g_dwCurrentSplitScreenIndex].x) {
            vehicle->shadowCorners[1][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
        }
        if (vehicle->shadowCorners[0][1] < g_aViewportRects[g_dwCurrentSplitScreenIndex].y) {
            vehicle->shadowCorners[0][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
        }
        if (vehicle->shadowCorners[1][1] < g_aViewportRects[g_dwCurrentSplitScreenIndex].y) {
            vehicle->shadowCorners[1][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].width <
            vehicle->shadowCorners[0][0]) {
            vehicle->shadowCorners[0][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].width <
            vehicle->shadowCorners[1][0]) {
            vehicle->shadowCorners[1][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].width;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].height <
            vehicle->shadowCorners[0][1]) {
            vehicle->shadowCorners[0][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height;
        }
        if (g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                g_aViewportRects[g_dwCurrentSplitScreenIndex].height <
            vehicle->shadowCorners[1][1]) {
            vehicle->shadowCorners[1][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].height;
        }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    }
}

#include <PR/gu.h>

#include "common/audiomix.h"
#include "common/camera.h"
#include "common/race.h"
#include "common/render.h"
#include "common/track.h"
#include "common/trackrender.h"
#include "common/util.h"
#include "common/vecmath.h"
#include "common/vehiclecount.h"
#include "common/visgrid.h"
#include "common/weather.h"

void BuildVisibleInstanceList(void) {
    static int g_nNearbyVehicleCount;
    int c;
    int r;
    int n;
    int i;
    int k;
    int d;
    int dx;
    int dy;
    int near2;
    TrackGridCursor q;
    unsigned short o;
    int lim;
    int slot;
    unsigned int cell;
    float dot;
    Vehicle *car;
    float lx;
    float ly;
    float *pos;
    Vec3f v;

    g_bVisibleCellListOverflow = 0;
    n = 0;
    for (r = g_dwVisibleGridMinRow; r <= g_dwVisibleGridMaxRow; r++) {
        for (c = g_adwVisibleGridRowMinCol[r]; c <= g_adwVisibleGridRowMaxCol[r]; c++) {
            if (n == 0xC0) {
                g_bVisibleCellListOverflow = 1;
                goto full;
            }
            g_aVisibleCellTuples[n].column = c;
            g_aVisibleCellTuples[n].row = r;
            n++;
        }
    }
full:
    g_aVisibleCellTuples[n].column = 0xFF;
    g_aVisibleCellTuples[n].row = 0xFF;
    c = g_pActiveCamera->position.x / 32.0f;
    if (c == 64) {
        c--;
    }
    r = g_pActiveCamera->position.y / 32.0f;
    if (r == 64) {
        r--;
    }
    for (i = 0; i < n; i++) {
        dx = g_aVisibleCellTuples[i].column - c;
        dy = g_aVisibleCellTuples[i].row - r;
        g_aVisibleCellTuples[i].distance = dx * dx + dy * dy;
    }
    QuickSort(g_aVisibleCellTuples, n, 4, CompareVisibleCellDistance);
    MemFill64Aligned(g_abInstanceVisibilityMask, g_LoadedTrack.instanceCount, -1);
    g_nVisibleInstanceCount = 0;
    g_nVisibleCellIdxNear = -1;
    g_nVisibleCellIdxFar = -1;
    g_nVisibleCellIdxMid = -1;
    near2 = gFrustumFar / (float)32;
    near2 -= 3;
    near2 *= near2;
    if (g_pActiveCameraVehicle->input->playbackBuffer != 0 &&
        g_pActiveCamera == &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE]) {
        lim = 9;
    } else {
        lim = 1;
    }
    for (n = 0; (c = g_aVisibleCellTuples[n].column) != 0xFF; n++) {
        d = g_aVisibleCellTuples[n].distance;
        if (8 < d && g_nVisibleCellIdxNear == -1) {
            g_nVisibleCellIdxNear = g_nVisibleInstanceCount;
        }
        if (lim < d && g_nVisibleCellIdxMid == -1) {
            g_nVisibleCellIdxMid = g_nVisibleInstanceCount;
        }
        if (near2 < d && g_nVisibleCellIdxFar == -1) {
            g_nVisibleCellIdxFar = g_nVisibleInstanceCount;
        }
        r = g_aVisibleCellTuples[n].row;
        if (g_dwAirplaneInstance != 0 && g_abInstanceVisibilityMask[g_dwAirplaneInstance] != 0) {
            pos = g_LoadedTrack.instances[g_dwAirplaneInstance].matrix[3];
            if (IsPointVisible(pos[0], pos[1])) {
                g_awVisibleInstanceList[g_nVisibleInstanceCount++] = g_dwAirplaneInstance;
                g_abInstanceVisibilityMask[g_dwAirplaneInstance] = 0;
            }
        }
        cell = LookupInstanceGridCell(c, r);
        q.start = cell & 0xffff, q.count = cell >> 16;
        if (cell != 0) {
            while ((o = NextGridCellInstance(&q)) != 0) {
                if (g_abInstanceVisibilityMask[o] != 0 && g_dwAirplaneInstance != o) {
                    g_awVisibleInstanceList[g_nVisibleInstanceCount++] = o;
                    g_abInstanceVisibilityMask[o] = 0;
                }
            }
        }
    }
    g_nNearbyVehicleCount = 0;
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
        if (g_adwVehicleVisible[i] == 0) {
            continue;
        }
        Vec3fSubtract(
            &g_vVehicleVisibilityDelta, &g_aVehicles[i].frame.position, &g_pActiveCamera->position);
        dot = Vec3fDot(&g_pActiveCamera->forward, &g_vVehicleVisibilityDelta);
        if (dot < 2.0f) {
            continue;
        }
        slot = g_nNearbyVehicleCount - 1;
        for (k = slot; k >= 0 && dot < g_afNearbyVehicleDot[k]; k--) {
            g_afNearbyVehicleDot[k + 1] = g_afNearbyVehicleDot[k];
            g_anNearbyVehicleIdx[k + 1] = g_anNearbyVehicleIdx[k];
        }
        g_afNearbyVehicleDot[k + 1] = dot;
        g_anNearbyVehicleIdx[k + 1] = i;
        g_nNearbyVehicleCount++;
    }
    if (g_nNearbyVehicleCount > 4) {
        g_nNearbyVehicleCount = 4;
    }
    for (i = 0; i < g_nNearbyVehicleCount; i++) {
        slot = g_anNearbyVehicleIdx[i];
        for (k = 0; k < g_aVehicles[slot].nearbyInstanceCount; k++) {
            g_abInstanceVisibilityMask[g_aVehicles[slot].nearbyInstances[k]] |= 1 << slot;
        }
    }
    g_wActiveInstanceHideMask = 0;
    for (k = 0; k < g_dwCameraRayHitCount; k++) {
        g_wActiveInstanceHideMask |=
            g_LoadedTrack.instances[g_awCameraRayHitInstances[k]].hidesGroups;
    }
    lx = g_pActiveCamera->forward.x;
    ly = g_pActiveCamera->forward.y;
    if (lx == 0.0 && ly == 0.0) {
        lx = 0.0001f;
    }
    g_pTrackLookAt = AllocLightBlock();
    guLookAtReflectF(
        g_afMvpMatrix, g_pTrackLookAt, lx * 50.0f, ly * 50.0f, 0.0f, 0, 0, 0, 0, 0, 1.0f);
    if (g_dwNightMode != 0) {
        g_vTrackLightDirection.x =
            g_pActiveCameraVehicle->matrix[2][0] + g_pActiveCameraVehicle->matrix[0][0] * -4.0f;
        g_vTrackLightDirection.y =
            g_pActiveCameraVehicle->matrix[2][1] + g_pActiveCameraVehicle->matrix[0][1] * -4.0f;
        g_vTrackLightDirection.z =
            g_pActiveCameraVehicle->matrix[2][2] + g_pActiveCameraVehicle->matrix[0][2] * -4.0f;
    } else {
        g_vTrackLightDirection.x = g_afLightDirection[0];
        g_vTrackLightDirection.y = g_afLightDirection[1];
        g_vTrackLightDirection.z = g_afLightDirection[2];
    }
    Vec3fNormalize(&g_vTrackLightDirection);
    Vec3fScaleInPlace(&g_vTrackLightDirection, 120.0f);
    g_nTrackLightBufferIndex = (g_nTrackLightBufferIndex + 1) % 4;
    g_aTrackLights[g_nTrackLightBufferIndex] = g_DefaultLights;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[0] = g_vTrackLightDirection.x;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[1] = g_vTrackLightDirection.y;
    g_aTrackLights[g_nTrackLightBufferIndex].l[0].l.dir[2] = g_vTrackLightDirection.z;
    for (i = 0; i < g_dwNumVehicles; i++) {
        car = g_aRacerSlots[i].vehicle;
        if (car != 0) {
            v.x = car->frame.position.x;
            v.y = car->frame.position.y;
            v.z = car->frame.position.z - car->groundDistance;
            ProjectWorldToScreenRect(&v, 8, car->shadowCorners[0], car->shadowCorners[1]);
            if (car->shadowCorners[0][0] < g_aViewportRects[g_dwCurrentSplitScreenIndex].x) {
                car->shadowCorners[0][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
            }
            if (car->shadowCorners[1][0] < g_aViewportRects[g_dwCurrentSplitScreenIndex].x) {
                car->shadowCorners[1][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
            }
            if (car->shadowCorners[0][1] < g_aViewportRects[g_dwCurrentSplitScreenIndex].y) {
                car->shadowCorners[0][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
            }
            if (car->shadowCorners[1][1] < g_aViewportRects[g_dwCurrentSplitScreenIndex].y) {
                car->shadowCorners[1][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
            }
            if (g_aViewportRects[g_dwCurrentSplitScreenIndex].width +
                    g_aViewportRects[g_dwCurrentSplitScreenIndex].x <
                car->shadowCorners[0][0]) {
                car->shadowCorners[0][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].width +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
            }
            if (g_aViewportRects[g_dwCurrentSplitScreenIndex].width +
                    g_aViewportRects[g_dwCurrentSplitScreenIndex].x <
                car->shadowCorners[1][0]) {
                car->shadowCorners[1][0] = g_aViewportRects[g_dwCurrentSplitScreenIndex].width +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].x;
            }
            if (g_aViewportRects[g_dwCurrentSplitScreenIndex].height +
                    g_aViewportRects[g_dwCurrentSplitScreenIndex].y <
                car->shadowCorners[0][1]) {
                car->shadowCorners[0][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].height +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
            }
            if (g_aViewportRects[g_dwCurrentSplitScreenIndex].height +
                    g_aViewportRects[g_dwCurrentSplitScreenIndex].y <
                car->shadowCorners[1][1]) {
                car->shadowCorners[1][1] = g_aViewportRects[g_dwCurrentSplitScreenIndex].height +
                                           g_aViewportRects[g_dwCurrentSplitScreenIndex].y;
            }
        }
    }
}

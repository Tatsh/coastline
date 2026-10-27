#include "camera.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "game.h"
#include "system.h"
#include "tricache.h"
#include "util.h"
#include "visgrid.h"

/* The share of the way from the camera to each corner of the far rectangle the frustum keeps. */
#define FRUSTUM_CORNER_SHARE 0.75f

/* The perspective of a camera: its near plane, the shape of the screen, and degrees a radian. */
#define CAMERA_NEAR 2.4f
#define ASPECT_4_3 1.3333334f
#define DEGREES_PER_RADIAN 57.295776f

/* The point SetupProjectionMatrix() looks at, its distance, and the perspective, in degrees. */
#define SCREEN_CENTER_X 512.0f
#define SCREEN_CENTER_Y 384.0f
#define SCREEN_DISTANCE 1000.0f
#define SCREEN_FOV 45.0f
#define SCREEN_NEAR 10.0f
#define SCREEN_FAR 2000.0f

/* The point SetupMenuProjection() looks at, its distance, and its default clip planes. */
#define MENU_CENTER_X 160.0f
#define MENU_CENTER_Y 120.0f
#define MENU_DISTANCE 290.0f
#define MENU_NEAR 1.0
#define MENU_FAR 500.0

/* The RSP takes physical addresses, which are the KSEG0 addresses less its base. */
#define TO_RSP_ADDRESS(p) ((u32)((u8 *)(p) - K0BASE))

void ClearCameraChanged(Vehicle *vehicle) {
    vehicle->cameraChanged = 0;
}

void SetupFrustumCorners(CameraFrame *camera, f32 fov, f32 farDistance, f32 width, f32 height) {
    f32 halfWidth = sinf(fov) * farDistance;
    f32 halfHeight = halfWidth * height / width;

    if (g_dwNumViewports == 2) {
        halfHeight *= 0.5f;
    }
    g_avFrustumPoints[FRUSTUM_CAMERA].x = camera->position.x;
    g_avFrustumPoints[FRUSTUM_CAMERA].y = camera->position.y;
    g_avFrustumPoints[FRUSTUM_CAMERA].z = camera->position.z;
    Vec3fMulAdd(
        &g_afFrustumCenter, &g_avFrustumPoints[FRUSTUM_CAMERA], &camera->forward, farDistance);
    Vec3fScale(&g_vFrustumHalfRight, &camera->side, halfWidth);
    Vec3fScale(&g_vFrustumHalfUp, &camera->up, halfHeight);
    g_avFrustumPoints[FRUSTUM_FAR].x = g_afFrustumCenter.x;
    g_avFrustumPoints[FRUSTUM_FAR].y = g_afFrustumCenter.y;
    g_avFrustumPoints[FRUSTUM_FAR].z = g_afFrustumCenter.z;

    Vec3fAdd(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fAddInPlace(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP], &g_vFrustumHalfUp);
    Vec3fAdd(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fSubtractInPlace(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM], &g_vFrustumHalfUp);
    Vec3fSubtract(&g_avFrustumPoints[FRUSTUM_LEFT_TOP], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fAddInPlace(&g_avFrustumPoints[FRUSTUM_LEFT_TOP], &g_vFrustumHalfUp);
    Vec3fSubtract(
        &g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM], &g_afFrustumCenter, &g_vFrustumHalfRight);
    Vec3fSubtractInPlace(&g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM], &g_vFrustumHalfUp);

    /* The corners are pulled a quarter of the way back towards the camera; the far point is not. */
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_LEFT_TOP],
              &g_avFrustumPoints[FRUSTUM_LEFT_TOP],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_LEFT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_RIGHT_TOP],
              &g_avFrustumPoints[FRUSTUM_RIGHT_TOP],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    Vec3fLerp(&g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_RIGHT_BOTTOM],
              &g_avFrustumPoints[FRUSTUM_CAMERA],
              FRUSTUM_CORNER_SHARE);
    gFrustumFar = farDistance;
    g_fFrustumFovAngle = fov;
}

void SetupCameraMatrix(CameraFrame *camera, f32 fov, f32 farDistance, f32 width, f32 height) {
    guLookAtF(g_ViewLookAtMtx,
              camera->position.x,
              camera->position.y,
              camera->position.z,
              camera->forward.x + camera->position.x,
              camera->forward.y + camera->position.y,
              camera->forward.z + camera->position.z,
              camera->up.x,
              camera->up.y,
              camera->up.z);
    g_fCameraFarDistance = farDistance;
    g_fCameraNearScale = CAMERA_NEAR;
    guPerspectiveF(g_afPerspectiveMatrix,
                   &g_wPerspNorm,
                   fov * ASPECT_4_3 * (height / width) * DEGREES_PER_RADIAN,
                   width / height,
                   g_fCameraNearScale,
                   g_fCameraFarDistance,
                   1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afPerspectiveMatrix, g_afViewProjMatrix);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
}

/* Look at a point of a flat screen from in front of it, with y up. */
#define LOOK_AT_SCREEN(x, y, distance)                                                             \
    guLookAtF(g_ViewLookAtMtx, x, y, distance, x, y, 0.0f, 0.0f, 1.0f, 0.0f)

void SetupProjectionMatrix(f32 width, f32 height) {
    LOOK_AT_SCREEN(SCREEN_CENTER_X, SCREEN_CENTER_Y, SCREEN_DISTANCE);
    guPerspectiveF(
        g_afScratchMatrix, &g_wPerspNorm, SCREEN_FOV, ASPECT_4_3, SCREEN_NEAR, SCREEN_FAR, 1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afScratchMatrix, g_afViewProjMatrix);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
}

void SetupMenuProjection(f32 width, f32 height) {
    LOOK_AT_SCREEN(MENU_CENTER_X, MENU_CENTER_Y, MENU_DISTANCE);
    if (g_dwKeepCustomClipPlanes) {
        g_dwKeepCustomClipPlanes = FALSE;
    } else {
        g_fCameraNearScale = MENU_NEAR;
        g_fCameraFarDistance = MENU_FAR;
    }
    guPerspectiveF(g_afPerspectiveMatrix,
                   &g_wPerspNorm,
                   SCREEN_FOV,
                   ASPECT_4_3,
                   g_fCameraNearScale,
                   g_fCameraFarDistance,
                   1.0f);
    guMtxCatF(g_ViewLookAtMtx, g_afPerspectiveMatrix, g_afViewProjMatrix);
    gSPPerspNormalize(gDisplayListPtr++, g_wPerspNorm);
    g_pProjectionMtx = (Mtx *)AllocDisplayListBlock();
    guMtxF2L(g_afViewProjMatrix, g_pProjectionMtx);
    gSPMatrix(gDisplayListPtr++,
              TO_RSP_ADDRESS(g_pProjectionMtx),
              G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
}

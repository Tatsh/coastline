#ifndef COASTLINE_CAMERA_H
#define COASTLINE_CAMERA_H

/**
 * @file
 * The cameras that follow each vehicle.
 */

#include <PR/ultratypes.h>

#include "cameraprojection.h"
#include "instantreplay.h"
#include "render.h"
#include "vecmath.h"
#include "vehicle.h"

/**
 * Whether the last call of ResolveVehicleTrackCollision() moved the camera.
 *
 * @ghidraAddress USA: 0x8028b710
 * @ghidraAddress EUR: 0x8028d9c0
 * @ghidraAddress JPN: 0x8028d3f0
 * @ghidraAddress BRD3D: 0x100c129c
 */
extern s32 g_bVehicleTrackCollisionCorrected;

/**
 * The vehicle the view being drawn follows.
 *
 * @ghidraAddress USA: 0x8028aaf0
 * @ghidraAddress EUR: 0x8028ccf8
 * @ghidraAddress JPN: 0x8028c7c8
 * @ghidraAddress BRD3D: 0x106c2cf8
 */
extern Vehicle *g_pActiveCameraVehicle;

/**
 * Counts down the frames after the chase camera hits the track in a replay.
 *
 * @ghidraAddress USA: 0x8028b7fc
 * @ghidraAddress EUR: 0x8028daac
 * @ghidraAddress JPN: 0x8028d4dc
 * @ghidraAddress BRD3D: 0x10af9bb4
 */
extern s32 g_dwReplayCameraHoldTimer;

/**
 * @ghidraAddress USA: 0x8028b7f8
 * @ghidraAddress EUR: 0x8028daa8
 * @ghidraAddress JPN: 0x8028d4d8
 * @ghidraAddress BRD3D: 0x10af9bb0
 */
extern s32 g_dwReplayCameraSwitchTimer;

/**
 * Set to 1 by the ray cast down from the active camera each frame. RenderPrecipitationEffect()
 * checks that many of g_awCameraRayHitInstances.
 *
 * @ghidraAddress USA: 0x8028ab00
 * @ghidraAddress EUR: 0x8028cd08
 * @ghidraAddress JPN: 0x8028c7d8
 * @ghidraAddress BRD3D: 0x106c1988
 */
extern s32 g_dwCameraRayHitCount;

/**
 * The placed models of the ground below the active camera, the nearest first.
 *
 * @ghidraAddress USA: 0x8031b248
 * @ghidraAddress EUR: 0x8031d748
 * @ghidraAddress JPN: 0x8031c378
 * @ghidraAddress BRD3D: 0x106c6498
 */
extern u16 g_awCameraRayHitInstances[];

#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
extern "C" {
#endif

/**
 * Keep a camera out of the track: if the line from the look-at point of a vehicle to the camera
 * passes through a triangle, and so does the line from a target point, put the camera just in
 * front of the triangle, no farther from the look-at point than it was.
 *
 * @param vehicle The vehicle the camera follows.
 * @param camera The camera to move.
 * @param target The point the second line starts from.
 * @ghidraAddress USA: 0x80220810
 * @ghidraAddress EUR: 0x80221b50
 * @ghidraAddress JPN: 0x80221340
 */
void ResolveVehicleTrackCollision(Vehicle *vehicle, CameraFrame *camera, Vec3f *target);

/**
 * Move the chase camera behind a vehicle, blending the direction it was in towards the one behind
 * the vehicle by a share.
 *
 * @param vehicle The vehicle the camera follows.
 * @param camera The chase camera.
 * @param share The share of the way to the direction behind, from 0 for none to 1 for all of it.
 * @ghidraAddress USA: 0x80220c58
 * @ghidraAddress EUR: 0x80221f98
 * @ghidraAddress JPN: 0x80221788
 */
void UpdateCameraChase(Vehicle *vehicle, CameraFrame *camera, f32 share);

/**
 * Put the look-at point above a vehicle, ahead of it by a lead that shrinks with its speed.
 *
 * @param vehicle The vehicle whose look-at point to move.
 * @ghidraAddress USA: 0x80220e84
 * @ghidraAddress EUR: 0x802221c4
 * @ghidraAddress JPN: 0x802219b4
 */
void UpdateCameraLookAtTarget(Vehicle *vehicle);

/**
 * Point a camera at the look-at point of a vehicle and build its axes about the world up.
 *
 * @param vehicle The vehicle whose look-at point to point at.
 * @param camera The camera whose axes to build.
 * @ghidraAddress USA: 0x80220ff4
 * @ghidraAddress EUR: 0x80222334
 * @ghidraAddress JPN: 0x80221b24
 */
void ComputeCameraBasis(Vehicle *vehicle, CameraFrame *camera);

/**
 * Make the trackside camera active, placed ahead of and beside a vehicle.
 *
 * @param vehicle The vehicle the camera watches.
 * @ghidraAddress USA: 0x80221108
 * @ghidraAddress EUR: 0x80222448
 * @ghidraAddress JPN: 0x80221c38
 */
void PlaceTrackCamera(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
}
#endif

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/**
 * Point a camera at a point one unit above a vehicle, build its axes about the world up, and zoom
 * it with its distance from that point, as the trackside camera does.
 *
 * @param vehicle The vehicle the camera watches.
 * @param camera The camera whose axes and field of view to set.
 * @ghidraAddress EUR: 0x802224b0
 * @ghidraAddress JPN: 0x80221ca0
 */
void UpdateCameraViewBasisAndFov(Vehicle *vehicle, CameraFrame *camera);
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
extern "C" {
#endif
/**
 * Update every camera of a vehicle for a frame.
 *
 * @param vehicle The vehicle whose cameras to update.
 * @ghidraAddress USA: 0x80221170
 * @ghidraAddress EUR: 0x80222600
 * @ghidraAddress JPN: 0x80221df0
 */
void UpdateCameraSystem(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
}
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
extern "C" {
#endif

/**
 * Set up the cameras of a vehicle at the start of a race.
 *
 * @param vehicle The vehicle whose cameras to set up.
 * @ghidraAddress USA: 0x80221864
 * @ghidraAddress EUR: 0x80222ba4
 * @ghidraAddress JPN: 0x80222394
 */
void InitCameraForVehicle(Vehicle *vehicle);
#if TGR_PLATFORM == TGR_PLATFORM_WIN && defined(__cplusplus)
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Build the view and the perspective of a camera for a rectangle of the screen, and set the far
 * distance.
 *
 * @param camera The camera.
 * @param fov The field of view, in radians, for a screen of 4 by 3.
 * @param farDistance The distance of the far plane.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @ghidraAddress USA: 0x8021b2f8
 * @ghidraAddress EUR: 0x8021c434
 * @ghidraAddress JPN: 0x8021bd04
 * @ghidraAddress BRD3D: 0x10033e83
 */
void SetupCameraMatrix(CameraFrame *camera, f32 fov, f32 farDistance, f32 width, f32 height);
#ifdef __cplusplus
}
#endif

/**
 * The size of the rear-view mirror over the bumper view: 0 for none, 1 small, 2 middling and 3
 * large.
 *
 * @ghidraAddress USA: 0x8028aa54
 * @ghidraAddress EUR: 0x8028cc5c
 * @ghidraAddress JPN: 0x8028c72c
 */
extern s32 g_dwRearViewMirrorMode;

/**
 * What CastRayAgainstTrack() found below the camera of the view being drawn.
 *
 * @ghidraAddress USA: 0x8028aaf8
 * @ghidraAddress EUR: 0x8028cd00
 * @ghidraAddress JPN: 0x8028c7d0
 */
extern f32 g_fCameraRayGroundDistance;

/**
 * @ghidraAddress USA: 0x8028aafc
 * @ghidraAddress EUR: 0x8028cd04
 * @ghidraAddress JPN: 0x8028c7d4
 */
extern s32 g_dwCameraRayGroundSurface;

/**
 * @ghidraAddress USA: 0x8028ab04
 * @ghidraAddress EUR: 0x8028cd0c
 * @ghidraAddress JPN: 0x8028c7dc
 */
extern s32 g_dwCameraRayNearbyCount;

/**
 * @ghidraAddress USA: 0x8031b288
 * @ghidraAddress EUR: 0x8031d788
 * @ghidraAddress JPN: 0x8031c3b8
 */
extern u16 g_awCameraRayNearbyInstances[];

/**
 * The middle of the far rectangle of the view.
 *
 * @ghidraAddress USA: 0x8031abf0
 * @ghidraAddress EUR: 0x8031d0f0
 * @ghidraAddress JPN: 0x8031bd20
 * @ghidraAddress BRD3D: 0x106c2990
 */
extern Vec3f g_afFrustumCenter;

/**
 * Half the far rectangle of the view, across.
 *
 * @ghidraAddress USA: 0x8031abd0
 * @ghidraAddress EUR: 0x8031d0d0
 * @ghidraAddress JPN: 0x8031bd00
 * @ghidraAddress BRD3D: 0x106c56f8
 */
extern Vec3f g_vFrustumHalfRight;

/**
 * Half the far rectangle of the view, up.
 *
 * @ghidraAddress USA: 0x8031abe0
 * @ghidraAddress EUR: 0x8031d0e0
 * @ghidraAddress JPN: 0x8031bd10
 * @ghidraAddress BRD3D: 0x106c0690
 */
extern Vec3f g_vFrustumHalfUp;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Set up the corners of the far rectangle of the view of a camera, which the shadows clip to.
 *
 * @param camera The camera.
 * @param fov The half angle of the view across, in radians.
 * @param farDistance The distance of the far plane.
 * @param width The width of the view.
 * @param height The height of the view.
 * @ghidraAddress USA: 0x8021b0c4
 * @ghidraAddress EUR: 0x8021c200
 * @ghidraAddress JPN: 0x8021bad0
 * @ghidraAddress BRD3D: 0x10033cb1
 */
void SetupFrustumCorners(CameraFrame *camera, f32 fov, f32 farDistance, f32 width, f32 height);
#ifdef __cplusplus
}
#endif

#endif

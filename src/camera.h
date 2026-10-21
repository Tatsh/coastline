#ifndef COASTLINE_CAMERA_H
#define COASTLINE_CAMERA_H

/**
 * @file
 * The cameras that follow each vehicle.
 */

#include <PR/ultratypes.h>

#include "render.h"
#include "vecmath.h"
#include "vehicle.h"

/**
 * Whether the last call of ResolveVehicleTrackCollision() moved the camera.
 *
 * @ghidraAddress USA: 0x8028b710
 */
extern s32 g_bVehicleTrackCollisionCorrected;

/** The vehicle the view being drawn follows. @ghidraAddress USA: 0x8028aaf0 */
extern Vehicle *g_pActiveCameraVehicle;

/** Whether an instant replay is playing. @ghidraAddress USA: 0x80270788 */
extern s32 gInstantReplayActive;

/**
 * Counts down the frames after the chase camera hits the track in a replay.
 *
 * @ghidraAddress USA: 0x8028b7fc
 */
extern s32 g_dwReplayCameraHoldTimer;

/** @ghidraAddress USA: 0x8028b7f8 */
extern s32 g_dwReplayCameraSwitchTimer;

/**
 * Set to 1 by the ray cast down from the active camera each frame. RenderPrecipitationEffect()
 * checks that many of g_awCameraRayHitInstances.
 *
 * @ghidraAddress USA: 0x8028ab00
 */
extern s32 g_dwCameraRayHitCount;

/**
 * The placed models of the ground below the active camera, the nearest first.
 *
 * @ghidraAddress USA: 0x8031b248
 */
extern u16 g_awCameraRayHitInstances[];

/**
 * Keep a camera out of the track: if the line from the look-at point of a vehicle to the camera
 * passes through a triangle, and so does the line from a target point, put the camera just in
 * front of the triangle, no farther from the look-at point than it was.
 *
 * @param vehicle The vehicle the camera follows.
 * @param camera The camera to move.
 * @param target The point the second line starts from.
 * @ghidraAddress USA: 0x80220810
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
 */
void UpdateCameraChase(Vehicle *vehicle, CameraFrame *camera, f32 share);

/**
 * Put the look-at point above a vehicle, ahead of it by a lead that shrinks with its speed.
 *
 * @param vehicle The vehicle whose look-at point to move.
 * @ghidraAddress USA: 0x80220e84
 */
void UpdateCameraLookAtTarget(Vehicle *vehicle);

/**
 * Point a camera at the look-at point of a vehicle and build its axes about the world up.
 *
 * @param vehicle The vehicle whose look-at point to point at.
 * @param camera The camera whose axes to build.
 * @ghidraAddress USA: 0x80220ff4
 */
void ComputeCameraBasis(Vehicle *vehicle, CameraFrame *camera);

/**
 * Make the trackside camera active, placed ahead of and beside a vehicle.
 *
 * @param vehicle The vehicle the camera watches.
 * @ghidraAddress USA: 0x80221108
 */
void PlaceTrackCamera(Vehicle *vehicle);

/**
 * Update every camera of a vehicle for a frame.
 *
 * @param vehicle The vehicle whose cameras to update.
 * @ghidraAddress USA: 0x80221170
 */
void UpdateCameraSystem(Vehicle *vehicle);

/**
 * Set up the cameras of a vehicle at the start of a race.
 *
 * @param vehicle The vehicle whose cameras to set up.
 * @ghidraAddress USA: 0x80221864
 */
void InitCameraForVehicle(Vehicle *vehicle);

#endif

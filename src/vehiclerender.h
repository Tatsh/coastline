#ifndef COASTLINE_VEHICLERENDER_H
#define COASTLINE_VEHICLERENDER_H

/**
 * @file
 * The drawing of the vehicles: their models, wheels, lights, reflections and dents.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "vecmath.h"
#include "vehicle.h"

/**
 * The model of the vehicle being drawn.
 *
 * @ghidraAddress USA: 0x8028ab08
 * @ghidraAddress EUR: 0x8028cd10
 * @ghidraAddress JPN: 0x8028c7e0
 */
extern VehicleModel *g_pCurrentVehicleModelData;

/**
 * The matrix of the vehicle being drawn, scaled to its model.
 *
 * @ghidraAddress USA: 0x80351cc0
 * @ghidraAddress EUR: 0x803541c0
 * @ghidraAddress JPN: 0x80352e30
 */
extern f32 g_mtxVehicleModelScaled[4][4];

/**
 * The blender part of the render mode of the vehicle being drawn, and its other flags.
 *
 * @ghidraAddress USA: 0x8028c300
 * @ghidraAddress EUR: 0x8028ea60
 * @ghidraAddress JPN: 0x8028e4b0
 */
extern u32 g_dwVehicleOtherModeLBlend;

/**
 * @ghidraAddress USA: 0x8028c304
 * @ghidraAddress EUR: 0x8028ea64
 * @ghidraAddress JPN: 0x8028e4b4
 */
extern u32 g_dwVehicleOtherModeLFlags;

/**
 * The alpha of the fog colour that fades a ghost vehicle.
 *
 * @ghidraAddress USA: 0x80351d00
 * @ghidraAddress EUR: 0x80354200
 * @ghidraAddress JPN: 0x80352e70
 */
extern s32 g_dwVehicleFogAlpha;

/**
 * Whether each vehicle is drawn in the view being drawn.
 *
 * @ghidraAddress USA: 0x80351c80
 * @ghidraAddress EUR: 0x80354180
 * @ghidraAddress JPN: 0x80352df0
 */
extern s32 g_adwVehicleInViewRange[MAX_VEHICLES];

/**
 * The level of detail each vehicle is drawn at.
 *
 * @ghidraAddress USA: 0x80351c90
 * @ghidraAddress EUR: 0x80354190
 * @ghidraAddress JPN: 0x80352e00
 */
extern s32 g_anVehicleLod[MAX_VEHICLES];

/**
 * The matrix of each vehicle, scaled to its model.
 *
 * @ghidraAddress USA: 0x80351ca0
 * @ghidraAddress EUR: 0x803541a0
 * @ghidraAddress JPN: 0x80352e10
 */
extern Mtx *g_apVehicleModelMatrix[MAX_VEHICLES];

/**
 * The matrix of each vehicle with the view and projection, forced for its lighting.
 *
 * @ghidraAddress USA: 0x80351cb0
 * @ghidraAddress EUR: 0x803541b0
 * @ghidraAddress JPN: 0x80352e20
 */
extern Mtx *g_apVehicleLightMatrix[MAX_VEHICLES];

/**
 * The lights of each vehicle at night or in the rain.
 *
 * @ghidraAddress USA: 0x80351c10
 * @ghidraAddress EUR: 0x80354110
 * @ghidraAddress JPN: 0x80352d80
 */
extern Lights1 g_aVehicleLights[MAX_VEHICLES];

/**
 * The direction of the light on the vehicle being drawn.
 *
 * @ghidraAddress USA: 0x80351d08
 * @ghidraAddress EUR: 0x80354208
 * @ghidraAddress JPN: 0x80352e78
 */
extern Vec3f g_vecVehicleLightDir;

/**
 * The direction of the specular highlight: halfway between the light and the camera.
 *
 * @ghidraAddress USA: 0x80351d18
 * @ghidraAddress EUR: 0x80354218
 * @ghidraAddress JPN: 0x80352e88
 */
extern Vec3f g_vecVehicleHiliteDir;

/**
 * The palette the textured parts of the cars share.
 *
 * @ghidraAddress USA: 0x8028c340
 * @ghidraAddress EUR: 0x8028eaa0
 * @ghidraAddress JPN: 0x8028e4f0
 */
extern u16 g_awVehicleShadePalette[16];

/**
 * Whether every vehicle is drawn in its other look, as Vehicle.otherLook draws one.
 *
 * @ghidraAddress USA: 0x8028ddc8
 * @ghidraAddress EUR: 0x80290568
 * @ghidraAddress JPN: 0x80290008
 */
extern s32 g_bForceVehicleAltVariant;

/**
 * Whether the reflection of the sky takes its coordinates linearly from the normals.
 *
 * @ghidraAddress USA: 0x8028ddcc
 * @ghidraAddress EUR: 0x8029056c
 * @ghidraAddress JPN: 0x8029000c
 */
extern s32 g_bVehicleEnvMapLinearTexGen;

/**
 * Whether the paint shop shows a car, which has no specular highlight.
 *
 * @ghidraAddress USA: 0x8028c334
 * @ghidraAddress EUR: 0x8028ea94
 * @ghidraAddress JPN: 0x8028e4e4
 */
extern s32 g_bPaintShopPreviewActive;

/**
 * The 64 by 64 texture of 4-bit intensities of the specular highlight, by day and by night.
 *
 * @ghidraAddress USA: 0x802a1968
 * @ghidraAddress EUR: 0x802a4128
 * @ghidraAddress JPN: 0x802a3bc8
 */
extern u8 g_aVehicleHighlightTexture[];

/**
 * @ghidraAddress USA: 0x802a2170
 * @ghidraAddress EUR: 0x802a4930
 * @ghidraAddress JPN: 0x802a43d0
 */
extern u8 g_aVehicleNightHighlightTexture[];

/**
 * The 32 by 128 textures of the reflection of the sky, of 4-bit colour indices, and their palettes.
 *
 * @ghidraAddress USA: 0x8028c308
 * @ghidraAddress EUR: 0x8028ea68
 * @ghidraAddress JPN: 0x8028e4b8
 */
extern u8 *g_apVehicleReflectionTextures[4];

/**
 * @ghidraAddress USA: 0x8028c318
 * @ghidraAddress EUR: 0x8028ea78
 * @ghidraAddress JPN: 0x8028e4c8
 */
extern u16 *g_apVehicleReflectionTluts[4];

/**
 * The reflection of each view mode, in the open and in a tunnel.
 *
 * @ghidraAddress USA: 0x8028c32c
 * @ghidraAddress EUR: 0x8028ea8c
 * @ghidraAddress JPN: 0x8028e4dc
 */
extern u8 g_abVehicleReflectionTexSelect[4][2];

/**
 * A count of frames; the fan of the speedometer needle is built while it is 0.
 *
 * @ghidraAddress USA: 0x8028c360
 * @ghidraAddress EUR: 0x8028eac0
 * @ghidraAddress JPN: 0x8028e510
 */
extern s32 g_dwGameFrameCount;

/**
 * The fan of the speedometer needle, around its centre.
 *
 * @ghidraAddress USA: 0x80351a20
 * @ghidraAddress EUR: 0x80353f20
 * @ghidraAddress JPN: 0x80352b90
 */
extern Vtx g_aSpeedometerNeedleVertices[32];

/**
 * @ghidraAddress USA: 0x803519e0
 * @ghidraAddress EUR: 0x80353ee0
 * @ghidraAddress JPN: 0x80352b50
 */
extern Mtx g_SpeedometerNeedleMtx;

/**
 * @ghidraAddress USA: 0x8028c368
 * @ghidraAddress EUR: 0x8028eac8
 * @ghidraAddress JPN: 0x8028e518
 */
extern Gfx g_aSpeedometerNeedleDisplayList[];

/**
 * @ghidraAddress USA: 0x8028c4f0
 * @ghidraAddress EUR: 0x8028ec50
 * @ghidraAddress JPN: 0x8028e6a0
 */
extern Gfx g_aTachometerDisplayList[];

/**
 * The shadow a car casts on the stage of a menu.
 *
 * @ghidraAddress USA: 0x8028c5d8
 * @ghidraAddress EUR: 0x8028ed38
 * @ghidraAddress JPN: 0x8028e788
 */
extern Gfx g_aTrackShadowDisplayList[];

/**
 * Place the four wheels of a vehicle: each takes the axes of the vehicle and the position of its
 * wheel in the model, carried by the vehicle.
 *
 * @param index The index of the vehicle in g_aVehicles.
 * @ghidraAddress USA: 0x8022fef0
 * @ghidraAddress EUR: 0x80231400
 * @ghidraAddress JPN: 0x80231200
 */
void TransformWheelPositions(s32 index);

/**
 * Draw the four wheels of a vehicle, faded like a ghost when it is one, and with the wheel for the
 * snow in the snow.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022ffb4
 * @ghidraAddress EUR: 0x802314c4
 * @ghidraAddress JPN: 0x802312c4
 */
void RenderWheelModels(Vehicle *vehicle);

/**
 * Decide whether a vehicle is drawn in the view being drawn, and set the fog at it.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802303d4
 * @ghidraAddress EUR: 0x802318e4
 * @ghidraAddress JPN: 0x802316e4
 */
void CheckVehicleVisibility(Vehicle *vehicle);

/**
 * Draw a vehicle: its level of detail, lights, lit parts, underside, near details, specular
 * highlight, the reflection of the sky on its paint, and its wheels.
 *
 * @param vehicle The vehicle.
 * @param lodOffset How many levels of detail coarser it is drawn, up to the coarsest.
 * @ghidraAddress USA: 0x80230554
 * @ghidraAddress EUR: 0x80231a64
 * @ghidraAddress JPN: 0x80231864
 */
void RenderVehicleModel(Vehicle *vehicle, s32 lodOffset);

/**
 * Draw the beams of the headlights of a vehicle at night or in the rain, and glare the view when
 * they shine at the camera.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x80232ed4
 * @ghidraAddress EUR: 0x802343e4
 * @ghidraAddress JPN: 0x802341e4
 */
void RenderVehicleHeadlightBeams(Vehicle *vehicle);

/**
 * Draw the needle of the speedometer, building its fan on the first frame.
 *
 * @param vehicle The vehicle, which is not used.
 * @ghidraAddress USA: 0x80233494
 * @ghidraAddress EUR: 0x802349a4
 * @ghidraAddress JPN: 0x802347a4
 */
void RenderSpeedometerNeedle(Vehicle *vehicle);

/**
 * Draw the tachometer.
 *
 * @param vehicle The vehicle, which is not used.
 * @ghidraAddress USA: 0x80233668
 * @ghidraAddress EUR: 0x80234b78
 * @ghidraAddress JPN: 0x80234978
 */
void EmitTachometerDisplayList(Vehicle *vehicle);

/**
 * Draw the shadow of a car on the stage of a menu, sized to its car.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x802336e0
 * @ghidraAddress EUR: 0x80234bf0
 * @ghidraAddress JPN: 0x802349f0
 */
void RenderTrackShadow(Vehicle *vehicle);

/**
 * Dent the model of a car on the side a blow comes from, unless that side is dented deeply enough.
 *
 * @param vehicle The vehicle.
 * @param direction The direction of the blow.
 * @param amount How hard the blow is.
 * @ghidraAddress USA: 0x80233880
 * @ghidraAddress EUR: 0x80234d90
 * @ghidraAddress JPN: 0x80234b90
 */
void DeformVehicleMeshByImpact(Vehicle *vehicle, Vec3f *direction, s16 amount);

#endif

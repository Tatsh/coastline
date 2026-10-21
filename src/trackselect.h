#ifndef COASTLINE_TRACKSELECT_H
#define COASTLINE_TRACKSELECT_H

/**
 * @file
 * The track selection screen.
 */

#include <PR/ultratypes.h>

#include "model.h"

/** The track chosen for the next race. @ghidraAddress USA: 0x8028b940 */
extern u32 g_dwSelectedTrackId;

/** The number of tracks. @ghidraAddress USA: 0x8028ae04 */
extern s32 g_dwNumTracks;

/**
 * Run one frame of the track selection screen.
 *
 * @ghidraAddress USA: 0x8020d004
 */
void ProcessTrackSelectScreen(void);

/**
 * Whether a track exists on the cartridge.
 *
 * @param track The index of the track.
 * @return Whether it is below g_dwNumTracks and has data.
 * @ghidraAddress USA: 0x8021e180
 */
s32 IsTrackIndexValid(s32 track);

/**
 * Write a time as minutes, seconds and hundredths.
 *
 * @param out The buffer to write to.
 * @param seconds The time, in seconds.
 * @ghidraAddress USA: 0x8020cf44
 */
void FormatLapTime(char *out, f32 seconds);

/**
 * Draw a vehicle from its trackside camera in a rectangle of the screen, and a second one beside
 * it, or two models in their place.
 *
 * @param model A model to draw in place of the first car, or NULL.
 * @param secondModel A model to draw in place of the second car, or NULL.
 * @param vehicleIndex The vehicle whose camera looks.
 * @param x The left edge of the rectangle, in pixels.
 * @param y The top edge of the rectangle, in pixels.
 * @param width The width of the rectangle, in pixels.
 * @param height The height of the rectangle, in pixels.
 * @param slide How far the cars slide apart, as a fraction that the square of moves them by.
 * @param hideTachometer Whether the tachometer is left out.
 * @param slot The slot of g_aVehicleModels of the first car.
 * @param secondSlot The slot of g_aVehicleModels of the second car.
 * @param spread The distance of the slide at its full extent.
 * @param angle The direction the camera faces, in radians.
 * @ghidraAddress USA: 0x8020c6d0
 */
void RenderCameraView(Model *model,
                      Model *secondModel,
                      s32 vehicleIndex,
                      s32 x,
                      s32 y,
                      s32 width,
                      s32 height,
                      f32 slide,
                      s32 hideTachometer,
                      s32 slot,
                      s32 secondSlot,
                      f32 spread,
                      f32 angle);

#endif

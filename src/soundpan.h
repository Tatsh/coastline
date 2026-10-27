#ifndef COASTLINE_SOUND_PAN_H
#define COASTLINE_SOUND_PAN_H

#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Pan a sound between the two halves of the volume of a channel by where it is across a camera, and
 * set its volume by its distance.
 *
 * @param source The source.
 * @param camera The camera that hears it.
 * @param upper The share of the upper half of the volume.
 * @param lower The share of the lower half.
 * @param volume The volume, 1024 over the distance, from at least 32.
 * @param narrow Whether to pan only a little.
 * @ghidraAddress USA: 0x8022b1d4
 * @ghidraAddress EUR: 0x8022c6e4
 * @ghidraAddress JPN: 0x8022be9c
 * @ghidraAddress BRD3D: 0x10067bc0
 */
void ComputeStereoPan(
    Vec3f *source, CameraFrame *camera, f32 *upper, f32 *lower, s32 *volume, s32 narrow);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SOUND_PAN_H */

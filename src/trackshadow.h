#ifndef COASTLINE_TRACK_SHADOW_H
#define COASTLINE_TRACK_SHADOW_H

#include "convertedvertex.h"
#include "track.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clip a shadow triangle to its texture and update the covered corners of its vehicle quad.
 *
 * @param instance The placed model.
 * @param quad The four output corners.
 * @param cornerValid Whether each corner has a projected vertex.
 * @param a The first vertex.
 * @param b The second vertex.
 * @param c The third vertex.
 * @param vehicleMatrix The vehicle frame.
 * @ghidraAddress BRD3D: 0x100106a0
 * @stub
 */
void ProjectClippedShadowTriangle(const TrackInstance *instance,
                                  ConvertedModelVertex quad[4],
                                  s32 cornerValid[4],
                                  const ConvertedModelVertex *a,
                                  const ConvertedModelVertex *b,
                                  const ConvertedModelVertex *c,
                                  const f32 vehicleMatrix[4][4]);

#ifdef __cplusplus
}
#endif

#endif

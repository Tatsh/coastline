#ifndef COASTLINE_TRACK_SHADOW_H
#define COASTLINE_TRACK_SHADOW_H

#include "common/convertedvertex.h"
#include "common/track.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The signed distance from the left boundary of the shadow texture.
 *
 * @param vertex The vertex measured.
 * @return The texture s coordinate.
 * @ghidraAddress BRD3D: 0x10010960
 * @ghidraAddress BRGlide: 0x1000dec0
 */
long double ShadowTextureDistanceLeft(const ConvertedModelVertex *vertex);

/**
 * The signed distance from the right boundary of the shadow texture.
 *
 * @param vertex The vertex measured.
 * @return 1024 minus the texture s coordinate.
 * @ghidraAddress BRD3D: 0x10010970
 * @ghidraAddress BRGlide: 0x1000ded0
 */
long double ShadowTextureDistanceRight(const ConvertedModelVertex *vertex);

/**
 * The signed distance from the top boundary of the shadow texture.
 *
 * @param vertex The vertex measured.
 * @return The texture t coordinate.
 * @ghidraAddress BRD3D: 0x10010980
 * @ghidraAddress BRGlide: 0x1000dee0
 */
long double ShadowTextureDistanceTop(const ConvertedModelVertex *vertex);

/**
 * The signed distance from the bottom boundary of the shadow texture.
 *
 * @param vertex The vertex measured.
 * @return 1024 minus the texture t coordinate.
 * @ghidraAddress BRD3D: 0x10010990
 * @ghidraAddress BRGlide: 0x1000def0
 */
long double ShadowTextureDistanceBottom(const ConvertedModelVertex *vertex);

/**
 * Set the texture coordinates of a shadow vertex with the current projection matrix.
 *
 * @param vertex The vertex to project.
 * @ghidraAddress BRD3D: 0x10010d10
 * @ghidraAddress BRGlide: 0x1000e270
 */
void ProjectShadowTextureCoordinates(ConvertedModelVertex *vertex);

/**
 * Update a shadow corner for a closer vertex at most one unit above the vehicle frame.
 *
 * @ghidraAddress BRD3D: 0x10010bf0
 * @ghidraAddress BRGlide: 0x1000e150
 */
void UpdateShadowCornerVertex(const TrackInstance *instance,
                              ConvertedModelVertex quad[4],
                              s32 cornerValid[4],
                              s32 corner,
                              const ConvertedModelVertex *vertex,
                              f32 s,
                              f32 t,
                              const f32 vehicleMatrix[4][4]);

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
 * @ghidraAddress BRGlide: 0x1000dc00
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

#endif /* COASTLINE_TRACK_SHADOW_H */

#pragma once

#include "common/vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Find the nearest track-triangle intersection within two units of @p position on the vertical
 * axis.
 *
 * @return Signed distance to the nearest hit, or 100 when no triangle qualifies.
 * @ghidraAddress BRD3D: 0x1006f310
 * @ghidraAddress BRGlide: 0x100682c0
 */
float RaycastTerrainHeight(Vec3f *position);
#ifdef __cplusplus
}
#endif

#pragma once

#include "vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x1006f310
 */
float RaycastTerrainHeight(Vec3f *position);
#ifdef __cplusplus
}
#endif

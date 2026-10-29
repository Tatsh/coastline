#pragma once

#include "common/vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x1006f310
 * @ghidraAddress BRGlide: 0x100682c0
 */
float RaycastTerrainHeight(Vec3f *position);
#ifdef __cplusplus
}
#endif

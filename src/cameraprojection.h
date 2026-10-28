#ifndef COASTLINE_CAMERAPROJECTION_H
#define COASTLINE_CAMERAPROJECTION_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set up a fixed projection, looking at (512, 384) from 1000 along z, and load it into the display
 * list.
 *
 * @param width The width of the screen; the fixed projection does not read it.
 * @param height The height of the screen; the fixed projection does not read it.
 * @ghidraAddress USA: 0x8021b458
 * @ghidraAddress EUR: 0x8021c594
 * @ghidraAddress BRD3D: 0x10033f7e
 * @ghidraAddress BRGlide: 0x1002d62f
 * @ghidraAddress JPN: 0x8021be64
 */
void SetupProjectionMatrix(f32 width, f32 height);

/**
 * Set up the menu projection and load it into the display list. N64 looks at the screen centre;
 * Windows maps screen coordinates directly.
 *
 * @param width The width of the screen; N64 does not read it.
 * @param height The height of the screen; N64 does not read it.
 * @ghidraAddress USA: 0x8021b5a4
 * @ghidraAddress EUR: 0x8021c6e0
 * @ghidraAddress BRD3D: 0x1003407d
 * @ghidraAddress BRGlide: 0x1002d72e
 * @ghidraAddress JPN: 0x8021bfb0
 */
void SetupMenuProjection(f32 width, f32 height);

/**
 * Whether SetupMenuProjection() keeps the near scale and far distance set before it once,
 * instead of its defaults. It clears this.
 *
 * @ghidraAddress USA: 0x8028a868
 * @ghidraAddress EUR: 0x8028ca68
 * @ghidraAddress JPN: 0x8028c538
 */
extern s32 g_dwKeepCustomClipPlanes;

/**
 * The near plane of perspective cameras.
 *
 * @ghidraAddress USA: 0x8028a86c
 * @ghidraAddress EUR: 0x8028ca6c
 * @ghidraAddress BRD3D: 0x106c3360
 * @ghidraAddress BRGlide: 0x106ea3f0
 * @ghidraAddress JPN: 0x8028c53c
 */
extern f32 g_fCameraNearScale;

/**
 * The far plane of perspective cameras.
 *
 * @ghidraAddress USA: 0x8028a870
 * @ghidraAddress EUR: 0x8028ca70
 * @ghidraAddress BRD3D: 0x106c5ab4
 * @ghidraAddress BRGlide: 0x106ecb44
 * @ghidraAddress JPN: 0x8028c540
 */
extern f32 g_fCameraFarDistance;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_CAMERAPROJECTION_H */

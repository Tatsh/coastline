#ifndef COASTLINE_PLATFORM_WIN_SNAPSHOT_OPS_H
#define COASTLINE_PLATFORM_WIN_SNAPSHOT_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress BRD3D: 0x10015c70 */
int InterpolateFrameSnapshotsAndRender(int forceRender);

/**
 * @ghidraAddress BRD3D: 0x100a66e8
 * @ghidraAddress BRGlide: 0x100a5ea8
 */
extern int g_bFrameTimerReset;

#ifdef __cplusplus
}
#endif

#endif

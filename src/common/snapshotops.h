#ifndef COASTLINE_PLATFORM_WIN_SNAPSHOT_OPS_H
#define COASTLINE_PLATFORM_WIN_SNAPSHOT_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Blend captured race frames and render the result, marking the referenced pool slots in use.
 *
 * @param forceRender Whether to render the current frame without interpolation or pacing skips.
 * @return 1 after rendering, or 0 before two captures are available or when pacing skips the frame.
 * @ghidraAddress BRD3D: 0x10015c70
 * @ghidraAddress BRGlide: 0x100131e0
 */
int InterpolateFrameSnapshotsAndRender(int forceRender);

/**
 * Whether race timing must restart when simulation resumes after a pause or initialisation.
 *
 * @ghidraAddress BRD3D: 0x100a66e8
 * @ghidraAddress BRGlide: 0x100a5ea8
 */
extern int g_bFrameTimerReset;

#ifdef __cplusplus
}
#endif

#endif

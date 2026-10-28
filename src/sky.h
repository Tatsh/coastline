#ifndef COASTLINE_SKY_H
#define COASTLINE_SKY_H

/**
 * @file
 * The sky behind a view.
 */

#include <PR/ultratypes.h>
#if TGR_PC
#include "render.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether the sky is left undrawn.
 *
 * @ghidraAddress USA: 0x8028aa5c
 * @ghidraAddress EUR: 0x8028cc64
 * @ghidraAddress JPN: 0x8028c734
 * @ghidraAddress BRD3D: 0x106c6608
 * @ghidraAddress BRGlide: 0x106ed698
 */
extern s32 g_dwSkyboxDisabled;

/**
 * Whether the sky is a fill of the fog colour: at night, in snow or rain, on a track without a
 * sky, or with two players.
 *
 * @return Non-zero when the sky is a fill of the fog colour.
 * @ghidraAddress USA: 0x8022f900
 * @ghidraAddress EUR: 0x80230e10
 * @ghidraAddress JPN: 0x80230c10
 * @ghidraAddress BRD3D: 0x10018070
 * @ghidraAddress BRGlide: 0x100155f0
 */
s32 IsSkyFogFilled(void);

#if TGR_PC
/**
 * Draw the current sky or fog fill and set the frame's clipping ratio.
 *
 * @param viewports The records of the views, indexed by the current split-screen view.
 * @ghidraAddress BRD3D: 0x100180b0
 * @ghidraAddress BRGlide: 0x10015630
 */
void RenderSkybox(const ViewportRect *viewports);
#else
/**
 * Draw the sky of the current view, and set the clipping ratio for the frame.
 *
 * @ghidraAddress USA: 0x8022f968
 * @ghidraAddress EUR: 0x80230e78
 * @ghidraAddress JPN: 0x80230c78
 */
void RenderSkybox(void);
#endif /* TGR_PC */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SKY_H */

#ifndef COASTLINE_SKY_H
#define COASTLINE_SKY_H

/**
 * @file
 * The sky behind a view.
 */

#include <PR/ultratypes.h>

/** Whether the sky is left undrawn. @ghidraAddress USA: 0x8028aa5c */
extern s32 g_dwSkyboxDisabled;

/**
 * Whether the sky is a fill of the fog colour: at night, in snow or rain, on a track without a
 * sky, or with two players.
 *
 * @ghidraAddress USA: 0x8022f900
 */
s32 IsSkyFogFilled(void);

/**
 * Draw the sky of the current view, and set the clipping ratio for the frame.
 *
 * @ghidraAddress USA: 0x8022f968
 */
void RenderSkybox(void);

#endif

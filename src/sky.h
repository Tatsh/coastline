#ifndef COASTLINE_SKY_H
#define COASTLINE_SKY_H

/**
 * @file
 * The sky behind a view.
 */

#include <PR/ultratypes.h>

/**
 * Whether the sky is left undrawn.
 *
 * @ghidraAddress USA: 0x8028aa5c
 * @ghidraAddress JPN: 0x8028c734
 * @ghidraAddress EUR: 0x8028cc64
 */
extern s32 g_dwSkyboxDisabled;

/**
 * Whether the sky is a fill of the fog colour: at night, in snow or rain, on a track without a
 * sky, or with two players.
 *
 * @return Non-zero when the sky is a fill of the fog colour.
 * @ghidraAddress USA: 0x8022f900
 * @ghidraAddress JPN: 0x80230c10
 * @ghidraAddress EUR: 0x80230e10
 */
s32 IsSkyFogFilled(void);

/**
 * Draw the sky of the current view, and set the clipping ratio for the frame.
 *
 * @ghidraAddress USA: 0x8022f968
 * @ghidraAddress JPN: 0x80230c78
 * @ghidraAddress EUR: 0x80230e78
 */
void RenderSkybox(void);

#endif

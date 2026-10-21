#ifndef COASTLINE_FADE_H
#define COASTLINE_FADE_H

/**
 * @file
 * The fades of the screen to and from black or white.
 */

#include <PR/ultratypes.h>

/**
 * Set the fade level and its target at once.
 *
 * @ghidraAddress USA: 0x8022389c
 */
void SetScreenFadeValues(float level);

/**
 * Draw the fade overlay over the scissor rectangle at a level from 0 to 1.
 *
 * @ghidraAddress USA: 0x80223de0
 */
void DrawScreenFadeOverlay(float level);

/**
 * Fade towards a level from 0 to 1 by a step each frame.
 *
 * @ghidraAddress USA: 0x80223688
 */
void SetFadeTarget(float target, float step);

/**
 * Whether the fade has reached level 0.
 *
 * @ghidraAddress USA: 0x80223988
 */
s32 IsFadedToBlack(void);

/**
 * Whether the fade has reached level 1.
 *
 * @ghidraAddress USA: 0x802239f4
 */
s32 IsFadedToWhite(void);

/**
 * Draw the fade of the frame.
 *
 * @ghidraAddress USA: 0x80223a70
 */
void ApplyScreenFade(void);

/**
 * Step the fade towards its target.
 *
 * @ghidraAddress USA: 0x80223f54
 */
void UpdateFadeTransitions(void);

#endif

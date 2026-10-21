#ifndef COASTLINE_FADE_H
#define COASTLINE_FADE_H

/**
 * @file
 * The fades of the screen to and from black or white.
 */

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

#endif

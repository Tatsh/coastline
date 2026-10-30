#ifndef COASTLINE_QUALITYOFLIFE_H
#define COASTLINE_QUALITYOFLIFE_H

/**
 * @file
 * The arithmetic and the parsing of the quality of life changes of the patched PC libraries.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Record a press and report whether it follows an earlier press within a window.
 *
 * A second press consumes both presses, and the next press starts a new pair.
 *
 * @param pLastPress The time of the unpaired earlier press, or 0 for none. Updated.
 * @param now The time of this press.
 * @param window The most time between the two presses, in the unit of the times.
 * @return Whether this press is the second of a pair.
 */
bool IsSecondPressWithin(unsigned *pLastPress, unsigned now, unsigned window);

/**
 * Return the vertical field of view of a camera for the shape of its viewport.
 *
 * At 4:3 and wider the vertical angle is preserved and the view widens. Narrower viewports retain
 * the horizontal angle of 4:3 and grow taller.
 *
 * @param fov The vertical field of view at 4:3, in radians.
 * @param width The width of the viewport.
 * @param height The height of the viewport.
 * @return The vertical field of view, in radians.
 */
float GetHorPlusFovY(float fov, float width, float height);

/**
 * Return the size of the canvas the HUD of a race is laid out on.
 *
 * The canvas has the shape of the frame buffer. At 4:3 and wider it is at most 480 units tall,
 * and narrower frame buffers get a canvas at most 640 units wide, so the 640 by 480 layout of the
 * HUD always fits. The routines that draw the HUD anchor it to the edges and the centre.
 *
 * @param width The width of the frame buffer, in pixels.
 * @param height The height of the frame buffer, in pixels.
 * @param pCanvasWidth Receives the width of the canvas.
 * @param pCanvasHeight Receives the height of the canvas.
 */
void GetHudCanvasSize(int width, int height, int *pCanvasWidth, int *pCanvasHeight);

/**
 * Return the mode of play of the value of `scene=` on the command line.
 *
 * The scenes are `arcade`, `demo`, `practice`, `seasonwinner`, and `timeattack`. The value ends
 * at the end of the string or at white space.
 *
 * @param pszValue The text after `scene=`.
 * @return The mode of play, or -1 when the value is not a scene.
 */
int ParseTestScene(const char *pszValue);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_QUALITYOFLIFE_H */

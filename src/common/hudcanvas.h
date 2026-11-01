#ifndef COASTLINE_HUDCANVAS_H
#define COASTLINE_HUDCANVAS_H

#include <common/render.h>

/** The display-list opcode that begins or ends a scaled HUD canvas. */
#define GBI_OP_HUD_CANVAS 0xd8
/** The display-list opcode for a scissor rectangle with full-width coordinates. */
#define GBI_OP_FULL_SCISSOR 0xd7
/** The display-list opcode for a texture rectangle with full-width coordinates. */
#define GBI_OP_FULL_TEXTURE_RECTANGLE 0xd6

#ifdef __cplusplus
extern "C" {
#endif

/** @return The active HUD width, or the framebuffer width outside an overlay, in pixels. */
int GetHudCanvasWidth(void);
/** @return The active HUD height, or the framebuffer height outside an overlay, in pixels. */
int GetHudCanvasHeight(void);
/**
 * @param coordinate A horizontal framebuffer coordinate, in pixels.
 * @return The coordinate scaled into the current HUD canvas, rounded towards zero.
 */
int GetHudCanvasX(int coordinate);
/**
 * @param coordinate A vertical framebuffer coordinate, in pixels.
 * @return The coordinate scaled into the current HUD canvas, rounded towards zero.
 */
int GetHudCanvasY(int coordinate);
/**
 * Record a scaled overlay without narrowing screen coordinates in legacy commands.
 *
 * @param viewports The framebuffer viewports, with g_dwNumViewports entries (at most four).
 * @return Shared canvas-space viewports, valid until the next call.
 */
ViewportRect *BeginRaceHudCanvas(const ViewportRect *viewports);
/** End the overlay and restore framebuffer coordinates for subsequent display-list commands. */
void EndRaceHudCanvas(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_HUDCANVAS_H */

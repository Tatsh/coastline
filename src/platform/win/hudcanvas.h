#ifndef COASTLINE_WIN_HUDCANVAS_H
#define COASTLINE_WIN_HUDCANVAS_H

#include <render.h>

#define GBI_OP_HUD_CANVAS 0xd8
#define GBI_OP_FULL_SCISSOR 0xd7
#define GBI_OP_FULL_TEXTURE_RECTANGLE 0xd6

#ifdef __cplusplus
extern "C" {
#endif

/** Return HUD dimensions while recording an overlay, or the framebuffer dimensions otherwise. */
int GetHudCanvasWidth(void);
int GetHudCanvasHeight(void);
/** Convert framebuffer coordinates to the current HUD canvas. */
int GetHudCanvasX(int coordinate);
int GetHudCanvasY(int coordinate);
/** Record a scaled overlay without narrowing screen coordinates in legacy commands. */
ViewportRect *BeginRaceHudCanvas(const ViewportRect *viewports);
void EndRaceHudCanvas(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_WIN_HUDCANVAS_H */

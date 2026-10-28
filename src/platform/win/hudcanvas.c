#include "platform/win/hudcanvas.h"

#include "displaylistptr.h"
#include "platform/win/gbicommand.h"
#include "platform/win/settings.h"

static int canvasWidth;
static int canvasHeight;
static ViewportRect canvasViews[4];

int GetHudCanvasWidth(void) {
    return canvasWidth ? canvasWidth : g_dwConfigFramebufferWidth;
}

int GetHudCanvasHeight(void) {
    return canvasHeight ? canvasHeight : g_dwConfigFramebufferHeight;
}

int GetHudCanvasX(int coordinate) {
    return (int)((double)coordinate * GetHudCanvasWidth() / g_dwConfigFramebufferWidth);
}

int GetHudCanvasY(int coordinate) {
    return (int)((double)coordinate * GetHudCanvasHeight() / g_dwConfigFramebufferHeight);
}

ViewportRect *BeginRaceHudCanvas(const ViewportRect *viewports) {
    canvasHeight = g_dwConfigFramebufferHeight < 480 ? g_dwConfigFramebufferHeight : 480;
    canvasWidth =
        (int)((double)g_dwConfigFramebufferWidth * canvasHeight / g_dwConfigFramebufferHeight +
              0.5);
    /* Integer rectangle commands have twelve bits per coordinate. */
    if (canvasWidth > 2000) {
        canvasHeight = (int)((double)canvasHeight * 2000 / canvasWidth);
        canvasWidth = 2000;
    }
    for (int index = 0; index < g_dwNumViewports; ++index) {
        canvasViews[index] = viewports[index];
        canvasViews[index].x = GetHudCanvasX(viewports[index].x);
        canvasViews[index].y = GetHudCanvasY(viewports[index].y);
        canvasViews[index].width =
            GetHudCanvasX(viewports[index].x + viewports[index].width) - canvasViews[index].x;
        canvasViews[index].height =
            GetHudCanvasY(viewports[index].y + viewports[index].height) - canvasViews[index].y;
    }
    gDisplayListPtr->w0 = GBI_OP_HUD_CANVAS << 24;
    gDisplayListPtr->w1 = ((unsigned)canvasWidth << 16) | (unsigned)canvasHeight;
    ++gDisplayListPtr;
    return canvasViews;
}

void EndRaceHudCanvas(void) {
    gDisplayListPtr->w0 = GBI_OP_HUD_CANVAS << 24;
    gDisplayListPtr->w1 = 0;
    ++gDisplayListPtr;
    canvasWidth = canvasHeight = 0;
}

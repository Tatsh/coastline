#ifndef COASTLINE_RENDER_H
#define COASTLINE_RENDER_H

/**
 * @file
 * The state the renderer shares between the parts of a frame.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

/** The camera a view is drawn from. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0x30];
    f32 x;
    f32 y;
    f32 z;
} Camera;

/** The rectangle of the screen a view fills. */
typedef struct {
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
    s32 unreconstructed10;
} ViewportRect;

/** The next command of the display list. @ghidraAddress USA: 0x8028a858 */
extern Gfx *gDisplayListPtr;

/** The projection of the current view. @ghidraAddress USA: 0x8028a878 */
extern Mtx *g_pProjectionMtx;

/** A matrix the renderer builds before converting it. @ghidraAddress USA: 0x8031ab10 */
extern f32 g_afMvpMatrix[4][4];

/** The texture filter of the other modes. @ghidraAddress USA: 0x8028a898 */
extern u32 g_dwTextureFilterMode;

/** Whether the track is drawn mirrored. @ghidraAddress USA: 0x8028a8ac */
extern s32 g_dwTrackMirrored;

/** Whether the current view is mirrored. @ghidraAddress USA: 0x8028a8a8 */
extern s32 g_dwViewportMirrored;

/** The camera of the view being drawn. @ghidraAddress USA: 0x8028aaf4 */
extern Camera *g_pActiveCamera;

/** The rectangles of the views. @ghidraAddress USA: 0x8031b2c8 */
extern ViewportRect g_aViewportRects[];

/** The view being drawn. @ghidraAddress USA: 0x8028aaec */
extern s32 g_dwCurrentSplitScreenIndex;

/**
 * Fill a rectangle of the screen with a colour.
 *
 * @ghidraAddress USA: 0x80218104
 */
void DrawFilledRect(s32 x0, s32 y0, s32 x1, s32 y1, s32 red, s32 green, s32 blue);

#endif

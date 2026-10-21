#ifndef COASTLINE_RENDER_H
#define COASTLINE_RENDER_H

/**
 * @file
 * The state the renderer shares between the parts of a frame.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "vecmath.h"

/**
 * A frame of reference: three axes and a position, each padded to four words, and a field of view.
 * The world is z up. A vehicle and each of its cameras has one.
 */
typedef struct {
    Vec3f forward;
    f32 unreconstructedC;
    Vec3f side;
    f32 unreconstructed1C;
    Vec3f up;
    f32 unreconstructed2C;
    Vec3f position;
    f32 unreconstructed3C;
    /** The field of view of a camera, in radians. */
    f32 fov;
} CameraFrame;

/** The rectangle of the screen a view fills. */
typedef struct {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    /** The vehicle of the player the view follows. */
    s32 vehicleIndex;
} ViewportRect;

/** The next command of the display list. @ghidraAddress USA: 0x8028a858 */
extern Gfx *gDisplayListPtr;

/** The projection of the current view. @ghidraAddress USA: 0x8028a878 */
extern Mtx *g_pProjectionMtx;

/** A matrix the renderer builds before converting it. @ghidraAddress USA: 0x8031ab10 */
extern f32 g_afMvpMatrix[4][4];

/** The perspective of the current view. @ghidraAddress USA: 0x8031aa90 */
extern f32 g_afPerspectiveMatrix[4][4];

/** A matrix the renderer fills as an operand. @ghidraAddress USA: 0x8031ab50 */
extern f32 g_afScratchMatrix[4][4];

/** The texture filter of the other modes. @ghidraAddress USA: 0x8028a898 */
extern u32 g_dwTextureFilterMode;

/** Whether the track is drawn mirrored. @ghidraAddress USA: 0x8028a8ac */
extern s32 g_dwTrackMirrored;

/** Whether the current view is mirrored. @ghidraAddress USA: 0x8028a8a8 */
extern s32 g_dwViewportMirrored;

/** The camera of the view being drawn. @ghidraAddress USA: 0x8028aaf4 */
extern CameraFrame *g_pActiveCamera;

/** The field of view of the cameras, in radians. @ghidraAddress USA: 0x8028aac0 */
extern f32 g_fCameraFovRadians;

/** The views on the screen, one for each player. @ghidraAddress USA: 0x8028ab0c */
extern s32 g_dwNumViewports;

/** The rectangles of the views. @ghidraAddress USA: 0x8031b2c8 */
extern ViewportRect g_aViewportRects[];

/** The view being drawn. @ghidraAddress USA: 0x8028aaec */
extern s32 g_dwCurrentSplitScreenIndex;

/**
 * Fill a rectangle of the screen with a colour.
 *
 * @ghidraAddress USA: 0x80218104
 */
void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue);

#endif

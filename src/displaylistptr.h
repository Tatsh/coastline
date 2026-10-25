#ifndef COASTLINE_DISPLAYLISTPTR_H
#define COASTLINE_DISPLAYLISTPTR_H

/**
 * @file
 * The next command of the display list the game builds.
 */

#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include <PR/gbi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The next command of the display list.
 *
 * @ghidraAddress USA: 0x8028a858
 * @ghidraAddress EUR: 0x8028ca58
 * @ghidraAddress JPN: 0x8028c528
 * @ghidraAddress BRD3D: 0x106c0680
 * @ghidraAddress BRGlide: 0x106e7710
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern struct GbiCommand *gDisplayListPtr;
#else
extern Gfx *gDisplayListPtr;
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifndef COASTLINE_DISPLAYLISTOPS_H
#define COASTLINE_DISPLAYLISTOPS_H

#include "types.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/gbicommand.h"
typedef GbiCommand DisplayListCommand;
#else
#include <PR/gbi.h>
typedef Gfx DisplayListCommand;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Patch the render modes and the colour combines of a display list in place for the weather. A
 * render mode found in the first column of a table becomes the one of the column of the weather.
 *
 * @param displayList The display list, or NULL.
 * @param table The render mode rows, with each mode first, then its replacements
 * for the clear weather, for a weather, and for a weather with the rain or the night. The last
 * three rows are translucent.
 * @return 1 if a translucent render mode was found, 0 otherwise.
 * @ghidraAddress USA: 0x8021b72c
 * @ghidraAddress EUR: 0x8021c868
 * @ghidraAddress JPN: 0x8021c138
 * @ghidraAddress BRD3D: 0x100341b3
 */
s32 ProcessDisplayList(DisplayListCommand *displayList, DisplayListCommand table[][4]);

/* The tables of render modes of ProcessDisplayList(). */
#define RENDER_MODE_ROWS 6            /*!< The rows of each table of render modes. */
#define RENDER_MODE_TRANSLUCENT_ROW 3 /*!< The first row of a translucent render mode. */

/**
 * The colour combine replaced in clear weather and its replacement.
 *
 * @ghidraAddress USA: 0x8028ab88
 * @ghidraAddress EUR: 0x8028ce38
 * @ghidraAddress JPN: 0x8028c868
 * @ghidraAddress BRD3D: 0x100aa8b8
 */
extern DisplayListCommand g_aDlClearCombinePatch[1][2];

/**
 * The colour combines that the snow tints.
 *
 * @ghidraAddress USA: 0x8028ab98
 * @ghidraAddress EUR: 0x8028ce48
 * @ghidraAddress JPN: 0x8028c878
 * @ghidraAddress BRD3D: 0x100aa8c8
 */
extern DisplayListCommand g_aDlSnowTintCombines[2];

/**
 * The render modes of the track and of the vehicles.
 *
 * @ghidraAddress USA: 0x8028aba8
 * @ghidraAddress EUR: 0x8028ce58
 * @ghidraAddress JPN: 0x8028c888
 * @ghidraAddress BRD3D: 0x100aa8d8
 */
extern DisplayListCommand g_aaDlTrackRenderModes[RENDER_MODE_ROWS][4];

/**
 * The render modes of the body of a ghost.
 *
 * @ghidraAddress USA: 0x8028ac68
 * @ghidraAddress EUR: 0x8028cf18
 * @ghidraAddress JPN: 0x8028c948
 * @ghidraAddress BRD3D: 0x100aa998
 */
extern DisplayListCommand g_aaDlGhostRenderModes[RENDER_MODE_ROWS][4];

/**
 * The render modes of the wheels of a ghost.
 *
 * @ghidraAddress USA: 0x8028ad28
 * @ghidraAddress EUR: 0x8028cfd8
 * @ghidraAddress JPN: 0x8028ca08
 * @ghidraAddress BRD3D: 0x100aaa58
 */
extern DisplayListCommand g_aaDlGhostWheelRenderModes[RENDER_MODE_ROWS][4];

/**
 * Whether ProcessDisplayList() tints the colour combines of the snow. It clears this.
 *
 * @ghidraAddress USA: 0x8028ade8
 * @ghidraAddress EUR: 0x8028d098
 * @ghidraAddress JPN: 0x8028cac8
 * @ghidraAddress BRD3D: 0x106c666c
 */
extern s32 g_bDisplayListSnowTint;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * @ghidraAddress BRD3D: 0x10b501d4
 */
extern void (*g_pfnRenderBackendUpdateFog)(void);
#endif

/**
 * Patch the render modes of vehicle display lists and, on N64, track display lists.
 *
 * @ghidraAddress USA: 0x8021b97c
 * @ghidraAddress EUR: 0x8021cab8
 * @ghidraAddress JPN: 0x8021c388
 * @ghidraAddress BRD3D: 0x100344d7
 */
void ProcessAllDisplayLists(void);

#ifdef __cplusplus
}
#endif

#endif

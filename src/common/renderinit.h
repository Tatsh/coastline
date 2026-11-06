#ifndef COASTLINE_RENDERINIT_H
#define COASTLINE_RENDERINIT_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mutex of the renderer during a race.
 * @ghidraAddress BRD3D: 0x106c6650
 * @ghidraAddress BRGlide: 0x106ed6e0
 */
extern HANDLE g_hRenderMutex;

#ifdef __cplusplus
}
#endif

#endif

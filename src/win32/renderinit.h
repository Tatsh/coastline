#pragma once

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/win32types.h"
#else
#include <windows.h>
#endif

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x106c6650
 * @ghidraAddress BRGlide: 0x106ed6e0
 */
extern HANDLE g_hRenderMutex;

/** Create the mutex used by the rendering session. */
void CreateWindowsRenderMutex(void);

/**
 * Do nothing.
 *
 * @return 0.
 * @ghidraAddress BRD3D: 0x1007a1c0
 * @ghidraAddress BRGlide: 0x10053e60
 */
s32 ReturnZero(void);

#ifdef __cplusplus
}
#endif

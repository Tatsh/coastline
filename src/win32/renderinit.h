#pragma once

#include <windows.h>

#include "common/screen.h"
#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x106c6650
 * @ghidraAddress BRGlide: 0x106ed6e0
 */
extern HANDLE g_hRenderMutex;

#ifdef __cplusplus
}
#endif

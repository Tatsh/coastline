#pragma once

#include <windows.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x106c6650
 */
extern HANDLE g_hRenderMutex;

/** Create the mutex used by the rendering session. */
void CreateWindowsRenderMutex(void);

/**
 * @ghidraAddress BRD3D: 0x1007a1c0
 * @ghidraAddress BRGlide: 0x10053e60
 */
s32 ControllerPakCompatibilityStub(void);

#ifdef __cplusplus
}
#endif

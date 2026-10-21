#pragma once

#include <windows.h>

/**
 * Load the resources of the front end menus.
 *
 * @return Whether every resource loaded.
 * @ghidraAddress BRD3D: 0x1005d440
 * @ghidraAddress BRGlide: 0x10056260
 */
BOOL InitializeFrontEndResources(void);

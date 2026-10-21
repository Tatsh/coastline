#pragma once

#include <windows.h>

/**
 * Load the string table of the language resource library.
 *
 * @ghidraAddress BRD3D: 0x10073f60
 * @ghidraAddress BRGlide: 0x1006d1a0
 */
void LoadStringTableFromDll(void);

/**
 * Look up a string of the loaded string table.
 *
 * @param nResourceId Identifier of the string resource.
 * @return The string, managed by the table.
 * @ghidraAddress BRD3D: 0x10074030
 * @ghidraAddress BRGlide: 0x1006d280
 */
LPCSTR GetStringResource(UINT nResourceId);

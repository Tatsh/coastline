#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Record process attachment, or shut EAR down when the process detaches.
 *
 * @param module The library module.
 * @param reason The reason for the call.
 * @param reserved Unused.
 * @return TRUE.
 * @ghidraAddress EAR: 0x100065aa
 */
BOOL WINAPI EarDllMain(HINSTANCE module, DWORD reason, LPVOID reserved);

#ifdef __cplusplus
}
#endif

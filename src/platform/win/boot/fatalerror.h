#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display an error message and wait for Escape to exit the application.
 *
 * @param pszMessage Error message.
 * @ghidraAddress BRD3D: 0x10008cf0
 * @ghidraAddress BRGlide: 0x10008ef0
 */
[[noreturn]] void ShowFatalErrorScreen(const char *pszMessage);

#ifdef __cplusplus
}
#endif

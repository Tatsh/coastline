/**
 * @file
 * Entry point the C runtime calls when the library is loaded and unloaded.
 */
#include <float.h>

#include <windows.h>

#if TGR_RENDERER == TGR_RENDERER_GLIDE
#ifdef _MSC_VER
extern BOOL(WINAPI *_pRawDllMain)(HINSTANCE, DWORD, LPVOID);
#else
/* The runtime of MSVC defines the raw entry point. mingw-w64 registers none. */
BOOL(WINAPI *_pRawDllMain)(HINSTANCE, DWORD, LPVOID) = NULL;
#endif
#endif

/**
 * Accept every notification. The Glide library also turns off the thread attach and detach
 * notifications on process attach, unless the runtime registered a raw entry point.
 *
 * @ghidraAddress BRD3D: 0x10086b90
 * @ghidraAddress BRGlide: 0x10074b00
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved) {
#ifndef _MSC_VER
    // The start-up of the MSVC runtime sets 53-bit x87 precision before this; mingw-w64 does not.
    if (dwReason == DLL_PROCESS_ATTACH) {
        _controlfp(_PC_53, _MCW_PC);
    }
#endif
#if TGR_RENDERER == TGR_RENDERER_GLIDE
    if (dwReason == DLL_PROCESS_ATTACH && _pRawDllMain == NULL) {
        DisableThreadLibraryCalls(hInstance);
    }
#endif
    return TRUE;
}

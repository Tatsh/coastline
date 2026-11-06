#include "win32/ear/dllmain.h"

#include <float.h>

#include "ear/runtime.h"

BOOL WINAPI EarDllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
#ifndef _MSC_VER
    // The start-up of the MSVC runtime sets 53-bit x87 precision before this; mingw-w64 does not.
    if (reason == DLL_PROCESS_ATTACH) {
        _controlfp(_PC_53, _MCW_PC);
    }
#endif
    if (reason == DLL_PROCESS_ATTACH) {
        g_bEarProcessAttached = 1;
        DisableThreadLibraryCalls(module);
    } else if (reason == DLL_PROCESS_DETACH) {
        if (g_bEarProcessAttached) {
            EarShutDownEarExportWrapper();
            ExitProcess(0);
        }
        g_bEarCodeValidated = 0;
    }
    return TRUE;
}

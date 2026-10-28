#include "win32/buildinfo.h"

#include <string.h>

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/dinputcompat.h"
#include "sdl/win32types.h"
#else
#include <windows.h>
#endif

#include "text.h"
#include "win32/diagnostics.h"
#include "win32/hudcanvas.h"
#include "win32/settings.h"

#ifdef ENABLE_PATCHES
static const char projectUri[] = "github.com/Tatsh/coastline";
static const char revision[] = COASTLINE_GIT_SHA;
static const char buildDate[] = __DATE__ " " __TIME__;
static const char banner[] =
    "github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__ " " __TIME__ "\n";

#if TGR_PLATFORM == TGR_PLATFORM_SDL
void PrintWineDiagnostic(const char *message) {
    WriteApplicationDiagnostic(message);
}
#else
void PrintWineDiagnostic(const char *message) {
    DWORD savedError = GetLastError();
    WriteApplicationDiagnostic(message);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll == nullptr || GetProcAddress(ntdll, "wine_get_version") == nullptr) {
        SetLastError(savedError);
        return;
    }
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != nullptr && output != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(output, message, (DWORD)strlen(message), &written, nullptr);
    }
    SetLastError(savedError);
}
#endif

void PrintWineBuildIdentification(void) {
    PrintWineDiagnostic(banner);
}

void DrawRaceBuildIdentification(void) {
    if (!(GetAsyncKeyState('B') & 0x8000)) {
        return;
    }
    const char *lines[] = {projectUri, revision, buildDate};
    int y = GetHudCanvasHeight() - 76;
    SetTextStyleNormal();
    SetTextAlignLeft();
    SetTextSize(15);
    for (unsigned index = 0; index < sizeof(lines) / sizeof(lines[0]); ++index) {
        DrawText(lines[index], 12, y);
        y += 20;
    }
}
#endif /* ENABLE_PATCHES */

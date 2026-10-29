#include "common/buildinfo.h"

#include <string.h>

#include <windows.h>

#include "common/hudcanvas.h"
#include "common/settings.h"
#include "common/text.h"
#include "win32/diagnostics.h"

#ifdef ENABLE_PATCHES
static const char projectUri[] = "github.com/Tatsh/coastline";
static const char revision[] = COASTLINE_GIT_SHA;
static const char buildDate[] = __DATE__ " " __TIME__;
static const char banner[] =
    "github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__ " " __TIME__ "\n";

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

#include "platform/win/buildinfo.h"

#include <string.h>

#include <windows.h>

#include "platform/win/hudcanvas.h"
#include "platform/win/settings.h"
#include "text.h"

#ifdef ENABLE_PATCHES
static const char projectUri[] = "github.com/Tatsh/coastline";
static const char revision[] = COASTLINE_GIT_SHA;
static const char buildDate[] = __DATE__ " " __TIME__;
static const char banner[] =
    "github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__ " " __TIME__ "\n";
#endif /* ENABLE_PATCHES */

void PrintWineDiagnostic(const char *message) {
#ifdef ENABLE_PATCHES
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll == nullptr || GetProcAddress(ntdll, "wine_get_version") == nullptr) {
        return;
    }
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != nullptr && output != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(output, message, (DWORD)strlen(message), &written, nullptr);
    }
    HANDLE log = CreateFileA("coastline.log",
                             FILE_APPEND_DATA,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                             nullptr);
    if (log != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(log, message, (DWORD)strlen(message), &written, nullptr);
        CloseHandle(log);
    }
#endif /* ENABLE_PATCHES */
}

void PrintWineBuildIdentification(void) {
#ifdef ENABLE_PATCHES
    PrintWineDiagnostic(banner);
#endif
}

void DrawRaceBuildIdentification(void) {
#ifdef ENABLE_PATCHES
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
#endif /* ENABLE_PATCHES */
}

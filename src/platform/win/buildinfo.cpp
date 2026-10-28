#include "platform/win/buildinfo.h"

#include <windows.h>

#include "platform/win/input.h"
#include "platform/win/menufont.h"
#include "platform/win/settings.h"

#ifdef ENABLE_PATCHES
namespace {
constexpr char projectUri[] = "github.com/Tatsh/coastline";
constexpr char revision[] = COASTLINE_GIT_SHA;
constexpr char buildDate[] = __DATE__ " " __TIME__;
constexpr char banner[] =
    "github.com/Tatsh/coastline " COASTLINE_GIT_SHA " " __DATE__ " " __TIME__ "\n";
bool showBuildIdentification;
} // namespace
#endif /* ENABLE_PATCHES */

void PrintWineBuildIdentification(void) {
#ifdef ENABLE_PATCHES
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (ntdll == nullptr || GetProcAddress(ntdll, "wine_get_version") == nullptr) {
        return;
    }
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != nullptr && output != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(output, banner, sizeof(banner) - 1, &written, nullptr);
    }
#endif /* ENABLE_PATCHES */
}

void DrawMenuBuildIdentification(bool atRootMenu) {
#ifdef ENABLE_PATCHES
    if (!atRootMenu) {
        return;
    }
    const bool control = (g_abKeyboardState[DIK_LCONTROL] | g_abKeyboardState[DIK_RCONTROL]) & 0x80;
    const bool shift = (g_abKeyboardState[DIK_LSHIFT] | g_abKeyboardState[DIK_RSHIFT]) & 0x80;
    if (control && shift && g_adwKeyPressedEdge[DIK_B]) {
        showBuildIdentification = !showBuildIdentification;
    }
    if (!showBuildIdentification) {
        return;
    }
    MenuFont font;
    const char *lines[] = {projectUri, revision, buildDate};
    float y = static_cast<float>(g_dwConfigFramebufferHeight - 72);
    for (const char *line : lines) {
        font.SetText(line);
        font.SetX(12.0f);
        font.SetY(y);
        font.DrawTextAltFont();
        y += 20.0f;
    }
#else
    (void)atRootMenu;
#endif /* ENABLE_PATCHES */
}

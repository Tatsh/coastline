/**
 * @file
 * Entry point of the Glide library. The launcher loads BRGlide.dll and calls its RallyMain export
 * with the arguments of WinMain().
 */
#include <cstring>

#include <objbase.h>
#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/frontend.h"

/**
 * Runs the game. It refuses to start below DirectX 6 and when another instance is running, and
 * the two early returns skip CoUninitialize() as the original does.
 *
 * @ghidraAddress BRGlide: 0x1001cc00
 */
extern "C" __declspec(dllexport) int
RallyMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nCmdShow) {
    g_dwAppExitCode = 0;
    if (SUCCEEDED(CoInitialize(nullptr))) {
        unsigned dwVersion;
        unsigned dwPlatform;

        g_hRallyMainInstance = hInstance;
        g_hRallyMainPrevInstance = hPrevInstance;
        g_pszRallyMainCmdLine = pszCmdLine;
        g_nRallyMainCmdShow = nCmdShow;
        GetDXVersion(&dwVersion, &dwPlatform);
        if (dwVersion < 0x600) {
            MessageBoxA(nullptr, GetStringResource(0x128), GetStringResource(0x126), MB_ICONERROR);
            return 0;
        }
        if (!FocusExistingBossRallyInstance()) {
            return 0;
        }
        RefreshTotalPhysicalMemory();
        ReadInstallDirectoryFromRegistry();
        LoadStringTableFromDll();
        LoadBossRallyIniSettings(pszCmdLine);
        strcpy(g_szConfigFilePath, g_szInstallDirectory);
        strcat(g_szConfigFilePath, "BossRally.cfg");
        g_GlobalRallyConfig.LoadRallyConfigFile(g_szConfigFilePath);
        if (CreateMainWindow()) {
            InitializeDirectPlayForDesktopWindow();
            if (InitializeFrontEndResources()) {
                RunMessageLoop();
            }
        }
    }
    CoUninitialize();
    return g_dwAppExitCode;
}

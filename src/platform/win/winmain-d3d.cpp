/**
 * @file
 * Entry point of the Direct3D library. The launcher loads BRD3D.dll and calls its RallyMain
 * export with the arguments of WinMain().
 */
#include <string.h>

#include <objbase.h>
#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/d3d/texture.h"
#include "platform/win/directplay.h"
#include "platform/win/frontend.h"

/**
 * Runs the game.
 *
 * @ghidraAddress BRD3D: 0x10079820
 */
extern "C" __declspec(dllexport) int
RallyMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nCmdShow) {
    g_dwAppExitCode = 0;
    if (SUCCEEDED(CoInitialize(NULL))) {
        RefreshTotalPhysicalMemory();
        ReadInstallDirectoryFromRegistry();
        LoadStringTableFromDll();
        LoadBossRallyIniSettings(pszCmdLine);
        strcpy(g_szConfigFilePath, g_szInstallDirectory);
        strcat(g_szConfigFilePath, "BossRally.cfg");
        g_GlobalRallyConfig.LoadRallyConfigFile(g_szConfigFilePath);
        if (InitializeApplication()) {
            InitializeDirectPlayForDesktopWindow();
            if (InitializeFrontEndResources()) {
                GetAvailableTextureMemory();
                RunMessageLoop();
                ShutdownMainWindow();
            }
        }
    }
    CoUninitialize();
    return g_dwAppExitCode;
}

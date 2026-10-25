/**
 * @file
 * Entry point of the Direct3D library. The launcher loads BRD3D.dll and calls its RallyMain
 * export with the arguments of WinMain().
 */
#include <cstring>

#include <objbase.h>
#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/d3d/texture.h"
#include "platform/win/directplay.h"
#include "platform/win/frontend.h"

// The accelerator table of the main window, a resource of the library.
#define IDR_MAIN_ACCELERATORS 0x71

bool InitializeApplication(void) {
    unsigned dwVersion;
    unsigned dwRevision;

    g_hAppInstance = GetModuleHandleA(nullptr);
    if (g_hAppInstance == nullptr) {
        return false;
    }
    GetDXVersion(&dwVersion, &dwRevision);
    if (dwVersion < 0x600) {
        MessageBoxA(nullptr, "This game requires DirectX 6.", "Boss Rally Error", MB_ICONERROR);
        return false;
    }
    if (!FocusExistingBossRallyInstance()) {
        return false;
    }
    return CreateMainWindow();
}

void ShutdownMainWindow(void) {
    DestroyMainWindow();
    UnregisterClassA(g_pszMainWindowClassName, g_hAppInstance);
}

void RunMessageLoop(void) {
    MSG msg;

    if (g_hAppInstance == nullptr || g_hMainWindow == nullptr) {
        return;
    }
    g_hAccelTable = LoadAcceleratorsA(g_hAppInstance, MAKEINTRESOURCEA(IDR_MAIN_ACCELERATORS));
    for (;;) {
        if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            if (g_hAccelTable == nullptr ||
                !TranslateAcceleratorA(g_hMainWindow, g_hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        } else if (g_dwAppActive && g_dwActivateState && !g_dwActivateMinimized) {
            DispatchGameStateFrame();
            NullStub();
        }
    }
    g_dwAppExitCode = msg.wParam;
}

/**
 * Runs the game.
 *
 * @ghidraAddress BRD3D: 0x10079820
 */
extern "C" __declspec(dllexport) int
RallyMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nCmdShow) {
    g_dwAppExitCode = 0;
    if (SUCCEEDED(CoInitialize(nullptr))) {
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

int DestroyMainWindow(void) {
    DirectDrawRenderer *pRenderer;

    if (g_hMainWindow != nullptr && IsWindow(g_hMainWindow)) {
        DestroyWindow(g_hMainWindow);
        g_hMainWindow = nullptr;
    }
    pRenderer = g_pMainRenderWindow;
    if (pRenderer != nullptr) {
        pRenderer->Cleanup();
        delete pRenderer;
        g_pMainRenderWindow = nullptr;
    }
    return 0;
}

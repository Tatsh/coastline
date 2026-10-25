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
#include "platform/win/d3d/renderer.h"
#include "platform/win/d3d/texture.h"
#include "platform/win/directplay.h"
#include "platform/win/frontend.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"

// The resources of the main window in the library.
#define IDI_MAIN_ICON 0x65
#define IDR_MAIN_MENU 0x66
#define IDR_MAIN_ACCELERATORS 0x71

// The size of the main window.
#define MAIN_WINDOW_WIDTH 640
#define MAIN_WINDOW_HEIGHT 480

// The constant the splash screen is shown with.
#define SPLASH_IMAGE_KEY 0x2ac7e58b

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

bool CreateMainWindow(void) {
    WNDCLASSA wc;
    HWND hWnd;

    g_hArrowCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(LONG);
    wc.hInstance = g_hAppInstance;
    wc.hIcon = LoadIconA(g_hAppInstance, MAKEINTRESOURCEA(IDI_MAIN_ICON));
    wc.hCursor = g_hArrowCursor;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    wc.lpszMenuName = MAKEINTRESOURCEA(IDR_MAIN_MENU);
    wc.lpszClassName = g_pszMainWindowClassName;
    if (!RegisterClassA(&wc)) {
        return false;
    }
    if (FAILED(CreateRenderWindow(WS_EX_APPWINDOW,
                                  g_pszMainWindowClassName,
                                  g_pszMainWindowTitle,
                                  WS_POPUP | WS_CAPTION | WS_MINIMIZEBOX,
                                  0,
                                  0,
                                  MAIN_WINDOW_WIDTH,
                                  MAIN_WINDOW_HEIGHT,
                                  nullptr,
                                  nullptr,
                                  g_hAppInstance,
                                  nullptr,
                                  &g_GlobalRallyConfig.DisplayDriverGuid,
                                  1,
                                  &hWnd,
                                  &g_pMainRenderWindow))) {
        return false;
    }
    g_hMainWindow = hWnd;
    g_pRenderHost = new RenderHost;
    // The new of the original returns NULL when memory runs out.
    if (g_pRenderHost == nullptr) {
        return false;
    }
    g_pRenderHost->RenderLoadingScreen(g_pMainRenderWindow, "splash.img", SPLASH_IMAGE_KEY);
    return SUCCEEDED(g_pMainRenderWindow->SetDeviceNotificationCallback(g_pRenderHost));
}

HRESULT CreateRenderWindow(DWORD dwExStyle,
                           LPCSTR pszClassName,
                           LPCSTR pszWindowName,
                           DWORD dwStyle,
                           int x,
                           int y,
                           int nWidth,
                           int nHeight,
                           HWND hParent,
                           HMENU hMenu,
                           HINSTANCE hInstance,
                           LPVOID pParam,
                           GUID *pDriverGuid,
                           int nUnused,
                           HWND *phWnd,
                           DirectDrawRenderer **ppRenderer) {
    DirectDrawRenderer *pRenderer;
    HWND hWnd;
    HRESULT hr;

    if (pszClassName == nullptr) {
        return E_INVALIDARG;
    }
    if (hInstance == nullptr) {
        hInstance = GetModuleHandleA(nullptr);
    }
    if (hParent == nullptr) {
        hParent = GetDesktopWindow();
    }
    pRenderer = new DirectDrawRenderer;
    if (pRenderer == nullptr) {
        return E_OUTOFMEMORY;
    }
    hWnd = CreateWindowExA(dwExStyle,
                           pszClassName,
                           pszWindowName,
                           dwStyle,
                           x,
                           y,
                           nWidth,
                           nHeight,
                           hParent,
                           hMenu,
                           hInstance,
                           pParam);
    if (hWnd == nullptr) {
        pRenderer->Cleanup();
        delete pRenderer;
        return E_FAIL;
    }
    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    if (g_dwPlayMusicMode == 2) {
        InitializeEarSound(hWnd);
    }
    hr = pRenderer->InitializeWindow(hWnd, pDriverGuid, nullptr, 0, 0, 0, 0, true);
    if (FAILED(hr)) {
        pRenderer->Cleanup();
        delete pRenderer;
        MessageBoxA(hWnd,
                    "Unable to initialize Direct3D accelerated video card.",
                    "Boss Rally Error",
                    MB_ICONERROR);
        DestroyWindow(hWnd);
        return hr;
    }
    if (phWnd != nullptr) {
        *phWnd = hWnd;
    }
    if (ppRenderer != nullptr) {
        *ppRenderer = pRenderer;
    }
    return S_OK;
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

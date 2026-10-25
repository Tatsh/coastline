/**
 * @file
 * Entry point of the Direct3D library. The launcher loads BRD3D.dll and calls its RallyMain
 * export with the arguments of WinMain().
 */
#include <cstdlib>
#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <dbt.h>
#include <objbase.h>

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

// Handle a message of music mode 2, the music library. Return whether the message was its own.
static inline bool
HandleEarSoundMessage(UINT nMessage, WPARAM wParam, LPARAM lParam, LRESULT *pResult) {
    if (nMessage == g_uEarSoundWindowMessage) {
        // A track that ends plays the next.
        if (lParam == 2 && wParam == GetEarMusicEventHandle()) {
            AdvanceMusicTrackWrapping();
        }
        *pResult = 0;
        return true;
    }
    if (nMessage == WM_DEVICECHANGE) {
        if (wParam == DBT_DEVICEARRIVAL) {
            InitializeMusicPlayback();
            SetCurrentMusicTrack(1);
        }
        if (wParam == DBT_DEVICEQUERYREMOVE || wParam == DBT_DEVICEREMOVEPENDING ||
            wParam == DBT_DEVICEREMOVECOMPLETE) {
            StopMusicTrack();
            ShutDownEarMusicChannel();
        }
        *pResult = 1;
        return true;
    }
    return false;
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer;
    LRESULT result;

    // The front end and the network play see each message first.
    if (g_pActiveStringTable != nullptr && g_pActiveStringTable->pUnknown68 != nullptr) {
        FrontEndWindowProc(g_pFrontEndScratchBuffer, hWnd, nMessage, wParam, lParam);
        if (g_dwDirectPlaySessionActive) {
            HandleNetworkWindowMessage(hWnd, nMessage, wParam, lParam);
        }
    }
    if (g_dwPlayMusicMode == 2 && HandleEarSoundMessage(nMessage, wParam, lParam, &result)) {
        return result;
    }
    switch (nMessage) {
    case WM_CREATE:
    case WM_RENDERER_CREATED + 2:
    case WM_RENDERER_CREATED + 3:
    case WM_RENDERER_CREATED + 4:
        // The handlers of the original return 0, and their code is shared with ReturnZero().
        return 0;
    case WM_DESTROY:
        return HandleWindowDestroy(hWnd);
    case WM_MOVE:
        return HandleWindowMove(hWnd, wParam, lParam);
    case WM_SIZE:
        return HandleWindowSize(hWnd, wParam, lParam);
    case WM_ACTIVATE:
        UpdateAppActiveStateFromActivate(wParam);
        return HandleWindowActivate(hWnd, wParam, lParam);
    case WM_CLOSE:
        return HandleWindowClose(hWnd);
    case WM_ERASEBKGND:
        return HandleWindowEraseBackground(hWnd, wParam, lParam);
    case WM_ACTIVATEAPP:
        return HandleWindowActivateApp(hWnd, wParam, lParam);
    case WM_SETCURSOR:
        return HandleWindowSetCursor(hWnd, wParam, lParam);
    case WM_GETMINMAXINFO:
        return HandleWindowGetMinMaxInfo(hWnd, wParam, lParam);
    case WM_WINDOWPOSCHANGING:
        return HandleWindowPosChanging(hWnd, wParam, lParam);
    case WM_DISPLAYCHANGE:
        return HandleDisplayChange(hWnd);
    case WM_NCPAINT:
        return HandleWindowNcPaint(hWnd, wParam, lParam);
    case WM_COMMAND:
        return HandleWindowCommand(hWnd, wParam, lParam);
    case WM_SYSCOMMAND:
        return HandleWindowSysCommand(hWnd, wParam, lParam);
    case WM_ENTERMENULOOP:
        return HandleWindowEnterMenuLoop(hWnd, wParam, lParam);
    case WM_EXITMENULOOP:
        return HandleWindowExitMenuLoop(hWnd, wParam, lParam);
    case MM_MCINOTIFY:
        if (g_dwPlayMusicMode != 1) {
            break;
        }
        // A track of the CD audio that ends plays again, unless the race is paused.
        if (static_cast<MCIDEVICEID>(lParam) == g_dwMciCdAudioDeviceId &&
            wParam == MCI_NOTIFY_SUCCESSFUL && !g_dwRacePauseState) {
            PlayCdAudioFromCurrentTrack();
        }
        return 0;
    case WM_RENDERER_IS_READY:
        pRenderer = reinterpret_cast<DirectDrawRenderer *>(GetWindowLongA(hWnd, GWL_USERDATA));
        return pRenderer != nullptr &&
               (pRenderer->dwRenderStateFlags & RENDERER_STATE_READY) == RENDERER_STATE_READY;
    case WM_RENDERER_GET:
        return GetWindowLongA(hWnd, GWL_USERDATA);
    case WM_RENDERER_GET_BACK_BUFFER:
        pRenderer = reinterpret_cast<DirectDrawRenderer *>(GetWindowLongA(hWnd, GWL_USERDATA));
        return pRenderer != nullptr ? reinterpret_cast<LRESULT>(pRenderer->pBackBuffer) : 0;
    case WM_RENDERER_CREATED:
        return AttachRenderDeviceToWindow(hWnd, lParam);
    case WM_RENDERER_DESTROYING:
        return DetachRenderDeviceFromWindow(hWnd, lParam);
    }
    return DefWindowProcA(hWnd, nMessage, wParam, lParam);
}

// The renderer the window stores, set by AttachRenderDeviceToWindow().
static inline DirectDrawRenderer *GetWindowRenderer(HWND hWnd) {
    return reinterpret_cast<DirectDrawRenderer *>(GetWindowLongA(hWnd, GWL_USERDATA));
}

static inline bool IsRendererReady(const DirectDrawRenderer *pRenderer) {
    return pRenderer != nullptr &&
           (pRenderer->dwRenderStateFlags & RENDERER_STATE_READY) == RENDERER_STATE_READY;
}

// Whether a renderer draws the whole screen: ready, in front, and with the screen not given to GDI.
static inline bool IsRendererFullScreen(const DirectDrawRenderer *pRenderer) {
    return IsRendererReady(pRenderer) && (pRenderer->dwFlags & 8) != 0 &&
           pRenderer->nGdiModeRefCount == 0;
}

LRESULT HandleWindowActivate(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hWnd, WM_ACTIVATE, wParam, lParam);
}

LRESULT HandleWindowActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer;

    g_dwAppActive = wParam;
    if (!wParam) {
        // The game pauses as it goes behind, unless it is paused or a replay plays.
        if (!g_dwRacePauseState && !g_dwReplayPlaybackMode) {
            SaveClockForPause();
            PauseMusicPlayback();
            MuteAllActiveSoundChannels();
            g_dwPauseMenuSelection = 0;
            // A network race that has started and not finished asks the others to pause instead.
            if (g_dwNetworkPlayMode && g_bNetRaceStartReached && !g_dwReplayPlaybackMode &&
                g_aVehicles_LapCount < g_dwTotalLaps) {
                RequestNetworkPause();
                TickNetworkPauseRequest();
            } else {
                g_dwRacePauseState = true;
            }
        }
        NullStub();
    }
    if (GetRenderBackendMode() == 1) {
        pRenderer = GetWindowRenderer(hWnd);
        if (g_dwAppActive) {
            if (pRenderer != nullptr) {
                pRenderer->dwFlags |= 8;
                pRenderer->RestoreLostSurfaces();
                pRenderer->SetPrimarySurfacePalette();
            }
        } else if (pRenderer != nullptr) {
            pRenderer->dwFlags &= ~8;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
    }
    if (g_dwAppActive) {
        NullStub();
        ResetFrameTimerState();
    }
    return DefWindowProcA(hWnd, WM_ACTIVATEAPP, wParam, lParam);
}

LRESULT HandleWindowClose(HWND hWnd) {
    DestroyWindow(hWnd);
    return 0;
}

LRESULT HandleWindowCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hWnd, WM_COMMAND, wParam, lParam);
}

int ReturnZero(void) {
    return 0;
}

LRESULT HandleWindowDestroy(HWND hWnd) {
    ShutdownApplication(0);
    PostQuitMessage(0);
    return 0;
}

LRESULT HandleDisplayChange(HWND hWnd) {
    // The original calls an empty method of the renderer, whose code is shared with ReturnZero().
    if (GetWindowRenderer(hWnd) != nullptr) {
        ReturnZero();
    }
    return 0;
}

LRESULT HandleWindowEraseBackground(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (IsRendererFullScreen(GetWindowRenderer(hWnd))) {
        return 1;
    }
    return DefWindowProcA(hWnd, WM_ERASEBKGND, wParam, lParam);
}

LRESULT HandleWindowEnterMenuLoop(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);

    if (IsRendererReady(pRenderer)) {
        pRenderer->AdjustGdiModeRefCount(true);
        SetCursor(g_hArrowCursor);
    }
    return DefWindowProcA(hWnd, WM_ENTERMENULOOP, wParam, lParam);
}

LRESULT HandleWindowExitMenuLoop(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);

    if (IsRendererReady(pRenderer)) {
        SetCursor(nullptr);
        pRenderer->AdjustGdiModeRefCount(false);
    }
    return DefWindowProcA(hWnd, WM_EXITMENULOOP, wParam, lParam);
}

LRESULT HandleWindowGetMinMaxInfo(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);
    MINMAXINFO *pInfo = reinterpret_cast<MINMAXINFO *>(lParam);
    RECT rc;

    // The window takes the size of the viewport alone while the display mode is set.
    if (pRenderer == nullptr || (pRenderer->dwRenderStateFlags & 2) == 0 || g_bWindowMinimized) {
        return DefWindowProcA(hWnd, WM_GETMINMAXINFO, wParam, lParam);
    }
    pRenderer->GetDeviceViewportRect(&rc);
    pInfo->ptMinTrackSize.x = pInfo->ptMaxTrackSize.x = abs(rc.right - rc.left);
    pInfo->ptMinTrackSize.y = pInfo->ptMaxTrackSize.y = abs(rc.bottom - rc.top);
    return 0;
}

LRESULT HandleWindowMove(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hWnd, WM_MOVE, wParam, lParam);
}

LRESULT HandleWindowNcPaint(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    if (IsRendererFullScreen(GetWindowRenderer(hWnd)) && !g_bWindowMinimized) {
        return 0;
    }
    return DefWindowProcA(hWnd, WM_NCPAINT, wParam, lParam);
}

LRESULT HandleWindowSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);

    if (pRenderer == nullptr || (pRenderer->dwRenderStateFlags & 2) == 0) {
        return DefWindowProcA(hWnd, WM_SETCURSOR, wParam, lParam);
    }
    // The cursor shows only while the screen is given to GDI or the renderer is behind.
    if (pRenderer->nGdiModeRefCount == 0 && (pRenderer->dwFlags & 8) != 0) {
        SetCursor(nullptr);
    } else {
        SetCursor(g_hArrowCursor);
    }
    return 1;
}

LRESULT HandleWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);

    switch (wParam) {
    case SIZE_RESTORED:
        // A window restored after it was minimised gets its lost surfaces back.
        if (g_bWindowMinimized) {
            if (pRenderer != nullptr) {
                pRenderer->RestoreLostSurfaces();
            }
            g_bWindowMaximized = false;
            g_bWindowMinimized = false;
        }
        return 0;
    case SIZE_MINIMIZED:
        g_bWindowMinimized = true;
        g_bWindowMaximized = false;
        break;
    case SIZE_MAXIMIZED:
        g_bWindowMaximized = true;
        g_bWindowMinimized = false;
        break;
    }
    return DefWindowProcA(hWnd, WM_SIZE, wParam, lParam);
}

LRESULT HandleWindowSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);

    switch (wParam) {
    case SC_SIZE:
    case SC_MOVE:
        // The window is not moved or resized.
        return 0;
    case SC_MINIMIZE:
        g_bWindowMinimized = true;
        g_bWindowMaximized = false;
        break;
    case SC_MAXIMIZE:
        g_bWindowMaximized = true;
        g_bWindowMinimized = false;
        return 0;
    case SC_RESTORE:
        if (g_bWindowMinimized && pRenderer != nullptr) {
            pRenderer->RestoreLostSurfaces();
        }
        g_bWindowMinimized = false;
        g_bWindowMaximized = false;
        break;
    }
    return DefWindowProcA(hWnd, WM_SYSCOMMAND, wParam, lParam);
}

// The smallest viewport the window is given.
#define MIN_VIEWPORT_WIDTH 320
#define MIN_VIEWPORT_HEIGHT 200

LRESULT HandleWindowPosChanging(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(hWnd);
    WINDOWPOS *pPos = reinterpret_cast<WINDOWPOS *>(lParam);
    RECT rcWindow;
    RECT rcView;
    unsigned nWidth;
    unsigned nHeight;
    unsigned nViewWidth;
    unsigned nViewHeight;

    if (pRenderer != nullptr && (pRenderer->dwRenderStateFlags & 2) != 0 && !g_bWindowMinimized &&
        pPos != nullptr) {
        if (IsIconic(hWnd)) {
            g_bWindowMinimized = true;
        } else {
            // The window takes the position and the size of the viewport.
            GetWindowRect(hWnd, &rcWindow);
            nWidth = abs(rcWindow.right - rcWindow.left);
            nHeight = abs(rcWindow.bottom - rcWindow.top);
            pRenderer->GetDeviceViewportRect(&rcView);
            nViewWidth = abs(rcView.right - rcView.left);
            nViewHeight = abs(rcView.bottom - rcView.top);
            if (nViewWidth < MIN_VIEWPORT_WIDTH) {
                nViewWidth = MIN_VIEWPORT_WIDTH;
            }
            if (nViewHeight < MIN_VIEWPORT_HEIGHT) {
                nViewHeight = MIN_VIEWPORT_HEIGHT;
            }
            if (nViewWidth == nWidth && nViewHeight == nHeight) {
                pPos->flags |= SWP_NOSIZE | SWP_NOMOVE;
            } else {
                pPos->x = rcView.left;
                pPos->y = rcView.top;
                pPos->cx = nViewWidth;
                pPos->cy = nViewHeight;
                pPos->flags &= ~(SWP_NOSIZE | SWP_NOMOVE);
            }
        }
    }
    return DefWindowProcA(hWnd, WM_WINDOWPOSCHANGING, wParam, lParam);
}

LRESULT AttachRenderDeviceToWindow(HWND hWnd, LPARAM lParam) {
    DirectDrawRenderer *pRenderer = reinterpret_cast<DirectDrawRenderer *>(lParam);

    if (!IsRendererReady(pRenderer)) {
        return 0;
    }
    SetWindowLongA(hWnd, GWL_USERDATA, lParam);
    // Bit 3 of the options marks the renderer of the active window.
    if (GetActiveWindow() == hWnd) {
        pRenderer->dwFlags |= 8;
    } else {
        pRenderer->dwFlags &= ~8;
    }
    return 0;
}

LRESULT DetachRenderDeviceFromWindow(HWND hWnd, LPARAM lParam) {
    SetWindowLongA(hWnd, GWL_USERDATA, 0);
    return 0;
}

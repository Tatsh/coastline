/**
 * @file
 * Entry point and main window of the Glide library. The launcher loads BRGlide.dll and calls its
 * RallyMain export with the arguments of WinMain().
 */
#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <dbt.h>
#include <glide.h>
#include <objbase.h>

#include "common/buildinfo.h"
#include "common/instantreplay.h"
#include "common/laps.h"
#include "common/pause.h"
#include "common/racesound.h"
#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/strings.h"
#include "common/timing.h"
#include "common/vehicle.h"
#include "win32/app.h"
#include "win32/directplay.h"
#include "win32/frametimer.h"
#include "win32/frontend.h"
#include "win32/glide/device.h"
#include "win32/glide/renderer.h"
#include "win32/render.h"
#include "win32/sound.h"

HINSTANCE g_hAppInstance;
HWND g_hMainWindow;
WPARAM g_dwAppActive;
HINSTANCE g_hRallyMainInstance;
HINSTANCE g_hRallyMainPrevInstance;
LPSTR g_pszRallyMainCmdLine;
int g_nRallyMainCmdShow;
int g_dwGlideSstWindowState = GLIDE_SST_WINDOW_OPEN;

namespace {
/**
 * Name of the window class of the main window, and of its menu.
 *
 * @ghidraAddress BRGlide: 0x1007b378
 */
const char g_szMainWindowClassName[] = "BossRally";
/**
 * Title of the main window.
 *
 * @ghidraAddress BRGlide: 0x1007b384
 */
const char g_szMainWindowTitle[] = "Boss Rally";
} // namespace

// The icon of the main window in the library.
#define IDI_MAIN_ICON 0x65

// The string of the message shown when the Glide window cannot be opened again.
#define IDS_GLIDE_REOPEN_FAILED 0x129

/**
 * Runs the game. It refuses to start below DirectX 6 and when another instance is running, and
 * the two early returns skip CoUninitialize() as the original does.
 *
 * @ghidraAddress BRGlide: 0x1001cc00
 */
extern "C" __declspec(dllexport) int
RallyMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nCmdShow) {
    PrintWineBuildIdentification();
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

bool CreateMainWindow(void) {
    WNDCLASSA wc;

    g_hAppInstance = g_hRallyMainInstance;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hRallyMainInstance;
    wc.hIcon = LoadIconA(g_hRallyMainInstance, MAKEINTRESOURCEA(IDI_MAIN_ICON));
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    // The library has no menu of this name, and the class takes none.
    wc.lpszMenuName = g_szMainWindowClassName;
    wc.lpszClassName = g_szMainWindowClassName;
    // The original creates the window without checking that the class was registered.
    RegisterClassA(&wc);
    return CreateWindowExA(WS_EX_APPWINDOW,
                           g_szMainWindowClassName,
                           g_szMainWindowTitle,
                           WS_POPUP | WS_CAPTION | WS_MINIMIZEBOX,
                           0,
                           0,
                           g_dwConfigFramebufferWidth,
                           g_dwConfigFramebufferHeight,
                           nullptr,
                           nullptr,
                           g_hRallyMainInstance,
                           nullptr) != nullptr;
}

void RunMessageLoop(void) {
    MSG msg;

    ShowWindow(g_hMainWindow, g_nRallyMainCmdShow);
    UpdateWindow(g_hMainWindow);
    SetFocus(g_hMainWindow);
    if (g_dwPlayMusicMode == 2) {
        InitializeEarSound(g_hMainWindow);
    }
    for (;;) {
        if (PeekMessageA(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
            if (!GetMessageA(&msg, nullptr, 0, 0)) {
                return;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else if (g_dwAppActive && g_dwActivateState && !g_dwActivateMinimized) {
            if (!DispatchGameStateFrame()) {
                return;
            }
        } else {
            WaitMessage();
        }
    }
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
    LRESULT result;

    // The front end and the network play see each message first.
    if (g_pActiveMenuScreen != nullptr && g_pActiveMenuScreen->IsRunning()) {
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
        g_hMainWindow = hWnd;
        return 0;
    case WM_DESTROY:
        ShutdownApplication(0);
        PostQuitMessage(0);
        break;
    case WM_ACTIVATE:
        UpdateAppActiveStateFromActivate(wParam);
        break;
    case WM_ACTIVATEAPP:
        return HandleWindowActivateApp(hWnd, wParam, lParam);
    case WM_SETCURSOR:
        SetCursor(nullptr);
        return 1;
    case WM_SYSCOMMAND:
        return HandleWindowSysCommand(hWnd, wParam, lParam);
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
    }
    return DefWindowProcA(hWnd, nMessage, wParam, lParam);
}

LRESULT HandleWindowActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    g_dwAppActive = wParam;
    if (!wParam) {
        // The game pauses as it goes behind, unless it is paused or a replay plays.
        if (!g_dwRacePauseState && !gInstantReplayActive) {
            SaveClockForPause();
            PauseMusicPlayback();
            MuteAllActiveSoundChannels();
            MuteVehicleSoundVoices();
            g_dwPauseMenuSelection = PAUSE_CONTINUE;
            // A network race that has started and not finished requests a pause from the others.
            if (g_dwNetworkPlayMode && g_bRaceStartReached && !gInstantReplayActive &&
                g_aVehicles[0].GetLap() < g_dwTotalLaps) {
                RequestNetworkPause();
                TickNetworkPauseRequest();
            } else {
                g_dwRacePauseState = TRUE;
            }
        }
        NullStub();
        grSstWinClose();
        g_dwGlideSstWindowState = GLIDE_SST_WINDOW_CLOSED;
        InvalidateRect(hWnd, nullptr, FALSE);
    }
    if (g_dwAppActive) {
        if (g_dwGlideSstWindowState == GLIDE_SST_WINDOW_CLOSED && !g_dwActivateMinimized) {
            if (!OpenGlideWindowForResolution(g_dwConfigFramebufferWidth,
                                              g_dwConfigFramebufferHeight)) {
                MessageBoxA(hWnd, GetStringResource(IDS_GLIDE_REOPEN_FAILED), nullptr, MB_OK);
                PostMessageA(hWnd, WM_CLOSE, 0, 0);
            }
            g_dwGlideSstWindowState = GLIDE_SST_WINDOW_OPEN;
        }
        NullStub();
        ResetFrameTimerState();
    }
    return DefWindowProcA(hWnd, WM_ACTIVATEAPP, wParam, lParam);
}

LRESULT HandleWindowSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    // The original reads the user data of the window and discards it.
    GetWindowLongA(hWnd, GWL_USERDATA);
    switch (wParam) {
    case SC_SIZE:
    case SC_MOVE:
    case SC_MAXIMIZE:
        // The window is not moved, resized, or maximised.
        return 0;
    }
    return DefWindowProcA(hWnd, WM_SYSCOMMAND, wParam, lParam);
}

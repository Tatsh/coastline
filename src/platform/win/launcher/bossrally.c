#define COBJMACROS
#include "bossrally.h"

#include <string.h>

#include <dshow.h>
#include <process.h>

typedef enum MovieState { MOVIE_CLOSED, MOVIE_STOPPED, MOVIE_PAUSED, MOVIE_PLAYING } MovieState;

// @ghidraAddress BossRally: 0x0040ac78
static MovieState g_dwMovieState;
// @ghidraAddress BossRally: 0x0040ac7c
static IGraphBuilder *g_pGraphBuilder;
// @ghidraAddress BossRally: 0x0040ac80
static OAEVENT g_hMovieEvent;
// @ghidraAddress BossRally: 0x0040afc0
static char g_szPlayerWindowClass[32];
// @ghidraAddress BossRally: 0x0040afe0
static HINSTANCE g_hInstance;
// @ghidraAddress BossRally: 0x0040afe4
static HWND g_hPlayerWnd;
// @ghidraAddress BossRally: 0x0040a970
static char g_szMovieFullPath[260];
// @ghidraAddress BossRally: 0x0040aa78
static char g_szMovieFileName[512];

static void StopMovie(void);
static void StopAndRewindMovie(void);
static void HandleMovieGraphEvent(void);

// @ghidraAddress BossRally: 0x00401000
static LRESULT HandlePlayerMenuCommand(HWND window, WPARAM command, LPARAM context) {
    if (command == 0xe141) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, WM_COMMAND, command, context);
}

// @ghidraAddress BossRally: 0x00401030
static LRESULT CALLBACK PlayerWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CHAR:
        if ((unsigned char)wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        return 0;
    case WM_SYSKEYDOWN:
        return 0;
    case WM_COMMAND:
        return HandlePlayerMenuCommand(window, wParam, lParam);
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
}

// @ghidraAddress BossRally: 0x004010b0
static int InitPlayerApplication(void) {
    strcpy(g_szPlayerWindowClass, "Player");
    return SUCCEEDED(CoInitialize(nullptr));
}

// @ghidraAddress BossRally: 0x00401100
static int RegisterPlayerWindowClass(HINSTANCE instance, HINSTANCE previousInstance) {
    g_hInstance = instance;
    if (!previousInstance) {
        WNDCLASSA windowClass;
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = PlayerWndProc;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = instance;
        windowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(128));
        windowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
        windowClass.hbrBackground = GetStockObject(LTGRAY_BRUSH);
        windowClass.lpszMenuName = MAKEINTRESOURCEA(128);
        windowClass.lpszClassName = g_szPlayerWindowClass;
        RegisterClassA(&windowClass);
    }
    return 1;
}

// @ghidraAddress BossRally: 0x00401190
static int CreatePlayerWindow(int showCommand) {
    char title[32];
    strcpy(title, "Player");
    strcat(title, " - Untitled");
    g_hPlayerWnd = CreateWindowExA(0,
                                   g_szPlayerWindowClass,
                                   title,
                                   WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   0,
                                   65,
                                   nullptr,
                                   nullptr,
                                   g_hInstance,
                                   nullptr);
    UpdateWindow(g_hPlayerWnd);
    // Window creation and class registration failures do not alter the returned status.
    return 1;
}

// @ghidraAddress BossRally: 0x00401410
static int CanPlayMovie(void) {
    return g_dwMovieState == MOVIE_STOPPED || g_dwMovieState == MOVIE_PAUSED;
}

// @ghidraAddress BossRally: 0x00401430
static int CanStopMovie(void) {
    return g_dwMovieState == MOVIE_PLAYING || g_dwMovieState == MOVIE_PAUSED;
}

// @ghidraAddress BossRally: 0x00401450
static int IsMovieStopped(void) {
    return g_dwMovieState == MOVIE_STOPPED;
}

// @ghidraAddress BossRally: 0x00401460
static int IsMovieOpen(void) {
    return g_dwMovieState != MOVIE_CLOSED;
}

// @ghidraAddress BossRally: 0x00401470
static void SetMovieState(MovieState state) {
    g_dwMovieState = state;
}

// @ghidraAddress BossRally: 0x00401480
static int InitMoviePlayer(void) {
    SetMovieState(MOVIE_CLOSED);
    g_hMovieEvent = 0;
    g_pGraphBuilder = nullptr;
    return 1;
}

// @ghidraAddress BossRally: 0x00401640
static void ReleaseFilterGraph(void) {
    if (g_pGraphBuilder) {
        IGraphBuilder_Release(g_pGraphBuilder);
        g_pGraphBuilder = nullptr;
    }
    g_hMovieEvent = 0;
    SetMovieState(MOVIE_CLOSED);
}

// @ghidraAddress BossRally: 0x004014b0
static int CreateFilterGraph(void) {
    HRESULT result = CoCreateInstance(&CLSID_FilterGraph,
                                      nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      &IID_IGraphBuilder,
                                      (void **)&g_pGraphBuilder);
    if (FAILED(result)) {
        g_pGraphBuilder = nullptr;
        return 0;
    }
    IMediaEvent *event;
    result = IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaEvent, (void **)&event);
    if (FAILED(result)) {
        ReleaseFilterGraph();
        return 0;
    }
    result = IMediaEvent_GetEventHandle(event, &g_hMovieEvent);
    IMediaEvent_Release(event);
    if (FAILED(result)) {
        ReleaseFilterGraph();
        return 0;
    }
    return 1;
}

// @ghidraAddress BossRally: 0x00401540
static int SetVideoWindowMessageDrain(HWND window) {
    IVideoWindow *video;
    HRESULT result =
        IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IVideoWindow, (void **)&video);
    if (FAILED(result)) {
        return 0;
    }
    result = IVideoWindow_put_MessageDrain(video, (OAHWND)window);
    IVideoWindow_Release(video);
    return SUCCEEDED(result);
}

// @ghidraAddress BossRally: 0x00401590
static int EnterVideoFullScreenMode(void) {
    IVideoWindow *video;
    HRESULT result =
        IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IVideoWindow, (void **)&video);
    if (FAILED(result)) {
        return 0;
    }
    LONG fullScreen;
    result = IVideoWindow_get_FullScreenMode(video, &fullScreen);
    if (FAILED(result)) {
        IVideoWindow_Release(video);
        return 0;
    }
    if (fullScreen != OATRUE) {
        // The SDK macro substitutes its argument into the member identifier.
        result = video->lpVtbl->HideCursor(video, OATRUE);
        if (FAILED(result)) {
            IVideoWindow_Release(video);
            return 0;
        }
    }
    result = IVideoWindow_put_FullScreenMode(video, OATRUE);
    IVideoWindow_Release(video);
    return SUCCEEDED(result);
}

// @ghidraAddress BossRally: 0x00401670
static int RenderMovieFile(const char *path) {
    WCHAR widePath[260];
    ReleaseFilterGraph();
    if (!CreateFilterGraph()) {
        return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, path, -1, widePath, 260);
    SetCursor(LoadCursorA(nullptr, IDC_WAIT));
    HRESULT result = IGraphBuilder_RenderFile(g_pGraphBuilder, widePath, nullptr);
    SetCursor(LoadCursorA(nullptr, IDC_ARROW));
    return SUCCEEDED(result);
}

// @ghidraAddress BossRally: 0x00401700
static BOOL SetPlayerWindowTitle(HWND window, const char *fileName) {
    char title[532];
    strcpy(title, "Player");
    strcat(title, " - ");
    strcat(title, fileName);
    return SetWindowTextA(window, title);
}

// @ghidraAddress BossRally: 0x004017b0
static void OpenMovie(HWND window, const char *path) {
    if (path && RenderMovieFile(path)) {
        char *filePart;
        GetFullPathNameA(path, 260, g_szMovieFullPath, &filePart);
        strncpy(g_szMovieFileName, filePart, 512);
        g_szMovieFileName[511] = '\0';
        SetPlayerWindowTitle(window, g_szMovieFileName);
        SetMovieState(MOVIE_STOPPED);
    }
}

// @ghidraAddress BossRally: 0x00401820
static void PlayMovie(void) {
    if (CanPlayMovie()) {
        IMediaControl *control;
        HRESULT result =
            IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaControl, (void **)&control);
        if (SUCCEEDED(result)) {
            result = IMediaControl_Run(control);
            IMediaControl_Release(control);
            if (SUCCEEDED(result)) {
                SetMovieState(MOVIE_PLAYING);
            }
        }
    }
}

// @ghidraAddress BossRally: 0x00401870
static void StopAndRewindMovie(void) {
    if (CanStopMovie()) {
        IMediaControl *control;
        HRESULT result =
            IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaControl, (void **)&control);
        if (FAILED(result)) {
            return;
        }
        IMediaControl_Stop(control);
        IMediaControl_Release(control);
        IMediaPosition *position;
        result =
            IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaPosition, (void **)&position);
        if (SUCCEEDED(result)) {
            IMediaPosition_put_CurrentPosition(position, 0.0);
            IMediaPosition_Release(position);
            SetMovieState(MOVIE_STOPPED);
        }
    }
}

// @ghidraAddress BossRally: 0x004018f0
static void StopMovie(void) {
    if (CanStopMovie()) {
        IMediaControl *control;
        HRESULT result =
            IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaControl, (void **)&control);
        if (FAILED(result)) {
            return;
        }
        IMediaControl_Pause(control);
        IMediaPosition *position;
        result =
            IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaPosition, (void **)&position);
        if (SUCCEEDED(result)) {
            IMediaPosition_put_CurrentPosition(position, 0.0);
            IMediaPosition_Release(position);
        }
        OAFilterState state;
        IMediaControl_GetState(control, -1, &state);
        IMediaControl_Stop(control);
        IMediaControl_Release(control);
        SetMovieState(MOVIE_STOPPED);
    }
}

// @ghidraAddress BossRally: 0x00401990
static HANDLE GetMovieEventHandle(void) {
    return (HANDLE)g_hMovieEvent;
}

// @ghidraAddress BossRally: 0x004019a0
static void HandleMovieGraphEvent(void) {
    IMediaEvent *event;
    HRESULT result =
        IGraphBuilder_QueryInterface(g_pGraphBuilder, &IID_IMediaEvent, (void **)&event);
    if (SUCCEEDED(result)) {
        LONG code;
        LONG_PTR first;
        LONG_PTR second;
        result = IMediaEvent_GetEvent(event, &code, &first, &second, 0);
        if (SUCCEEDED(result)) {
            switch (code) {
            case EC_COMPLETE:
                StopMovie();
                break;
            case EC_USERABORT:
            case EC_ERRORABORT:
                StopAndRewindMovie();
                break;
            case EC_FULLSCREEN_LOST:
                EnterVideoFullScreenMode();
                break;
            }
        }
        // Event parameters are not released by the original player.
        IMediaEvent_Release(event);
    }
}

// @ghidraAddress BossRally: 0x00401230
static int RunMoviePlayerMessageLoop(void) {
    MSG message;
    for (;;) {
        HANDLE event = GetMovieEventHandle();
        if (event) {
            DWORD result = MsgWaitForMultipleObjects(1, &event, FALSE, INFINITE, 0xff);
            if (result == WAIT_OBJECT_0) {
                HandleMovieGraphEvent();
                if (IsMovieStopped()) {
                    PostMessageA(g_hPlayerWnd, WM_QUIT, 0, 0);
                }
                continue;
            }
            if (result != WAIT_OBJECT_0 + 1) {
                continue;
            }
        } else {
            WaitMessage();
        }
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                return (int)message.wParam;
            }
            if (message.message == WM_CHAR && (unsigned char)message.wParam == VK_ESCAPE &&
                CanStopMovie()) {
                StopMovie();
                PostMessageA(g_hPlayerWnd, WM_QUIT, 0, 0);
            }
            if (message.message != WM_SYSKEYDOWN) {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE previousInstance,
                   LPSTR commandLine,
                   int showCommand) {
    if (!InitPlayerApplication()) {
        return 0;
    }
    int result = (int)(INT_PTR)previousInstance;
    if (RegisterPlayerWindowClass(instance, previousInstance) && CreatePlayerWindow(showCommand) &&
        InitMoviePlayer()) {
        OpenMovie(g_hPlayerWnd, "brally.avi");
        if (IsMovieOpen()) {
            if (CanPlayMovie()) {
                if (!SetVideoWindowMessageDrain(g_hPlayerWnd) || !EnterVideoFullScreenMode()) {
                    goto shutdown;
                }
                PlayMovie();
            }
            result = RunMoviePlayerMessageLoop();
            if (CanStopMovie()) {
                StopMovie();
            }
            ReleaseFilterGraph();
        }
    }
shutdown:
    ReleaseFilterGraph();
    CoUninitialize();
    _execl("brally.exe", "brally.exe", nullptr);
    return result;
}

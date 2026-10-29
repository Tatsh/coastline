#include <cstring>

#include <dshow.h>
#include <process.h>
#include <windows.h>

namespace {

enum MovieState { MOVIE_CLOSED, MOVIE_STOPPED, MOVIE_PAUSED, MOVIE_PLAYING };

// @ghidraAddress BossRally: 0x0040ac78
MovieState g_dwMovieState;
// @ghidraAddress BossRally: 0x0040ac7c
IGraphBuilder *g_pGraphBuilder;
// @ghidraAddress BossRally: 0x0040ac80
OAEVENT g_hMovieEvent;
// @ghidraAddress BossRally: 0x0040afc0
char g_szPlayerWindowClass[32];
// @ghidraAddress BossRally: 0x0040afe0
HINSTANCE g_hInstance;
// @ghidraAddress BossRally: 0x0040afe4
HWND g_hPlayerWnd;
// @ghidraAddress BossRally: 0x0040a970
char g_szMovieFullPath[260];
// @ghidraAddress BossRally: 0x0040aa78
char g_szMovieFileName[512];

void StopMovie(void);
void StopAndRewindMovie(void);
void HandleMovieGraphEvent(void);

// @ghidraAddress BossRally: 0x00401000
LRESULT HandlePlayerMenuCommand(HWND window, WPARAM command, LPARAM context) {
    if (command == 0xe141) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, WM_COMMAND, command, context);
}

// @ghidraAddress BossRally: 0x00401030
LRESULT CALLBACK PlayerWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CHAR:
        if (static_cast<unsigned char>(wParam) == VK_ESCAPE) {
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
int InitPlayerApplication(void) {
    std::strcpy(g_szPlayerWindowClass, "Player");
    return SUCCEEDED(CoInitialize(nullptr));
}

// @ghidraAddress BossRally: 0x00401100
int RegisterPlayerWindowClass(HINSTANCE instance, HINSTANCE previousInstance) {
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
        windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
        windowClass.lpszMenuName = MAKEINTRESOURCEA(128);
        windowClass.lpszClassName = g_szPlayerWindowClass;
        RegisterClassA(&windowClass);
    }
    return 1;
}

// @ghidraAddress BossRally: 0x00401190
int CreatePlayerWindow(int showCommand) {
    char title[32];
    std::strcpy(title, "Player");
    std::strcat(title, " - Untitled");
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
int CanPlayMovie(void) {
    return g_dwMovieState == MOVIE_STOPPED || g_dwMovieState == MOVIE_PAUSED;
}

// @ghidraAddress BossRally: 0x00401430
int CanStopMovie(void) {
    return g_dwMovieState == MOVIE_PLAYING || g_dwMovieState == MOVIE_PAUSED;
}

// @ghidraAddress BossRally: 0x00401450
int IsMovieStopped(void) {
    return g_dwMovieState == MOVIE_STOPPED;
}

// @ghidraAddress BossRally: 0x00401460
int IsMovieOpen(void) {
    return g_dwMovieState != MOVIE_CLOSED;
}

// @ghidraAddress BossRally: 0x00401470
void SetMovieState(MovieState state) {
    g_dwMovieState = state;
}

// @ghidraAddress BossRally: 0x00401480
int InitMoviePlayer(void) {
    SetMovieState(MOVIE_CLOSED);
    g_hMovieEvent = 0;
    g_pGraphBuilder = nullptr;
    return 1;
}

// @ghidraAddress BossRally: 0x00401640
void ReleaseFilterGraph(void) {
    if (g_pGraphBuilder) {
        g_pGraphBuilder->Release();
        g_pGraphBuilder = nullptr;
    }
    g_hMovieEvent = 0;
    SetMovieState(MOVIE_CLOSED);
}

// @ghidraAddress BossRally: 0x004014b0
int CreateFilterGraph(void) {
    HRESULT result = CoCreateInstance(CLSID_FilterGraph,
                                      nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IGraphBuilder,
                                      IID_PPV_ARGS_Helper(&g_pGraphBuilder));
    if (FAILED(result)) {
        g_pGraphBuilder = nullptr;
        return 0;
    }
    IMediaEvent *event;
    result = g_pGraphBuilder->QueryInterface(IID_IMediaEvent, IID_PPV_ARGS_Helper(&event));
    if (FAILED(result)) {
        ReleaseFilterGraph();
        return 0;
    }
    result = event->GetEventHandle(&g_hMovieEvent);
    event->Release();
    if (FAILED(result)) {
        ReleaseFilterGraph();
        return 0;
    }
    return 1;
}

// @ghidraAddress BossRally: 0x00401540
int SetVideoWindowMessageDrain(HWND window) {
    IVideoWindow *video;
    HRESULT result = g_pGraphBuilder->QueryInterface(IID_IVideoWindow, IID_PPV_ARGS_Helper(&video));
    if (FAILED(result)) {
        return 0;
    }
    result = video->put_MessageDrain(reinterpret_cast<OAHWND>(window));
    video->Release();
    return SUCCEEDED(result);
}

// @ghidraAddress BossRally: 0x00401590
int EnterVideoFullScreenMode(void) {
    IVideoWindow *video;
    HRESULT result = g_pGraphBuilder->QueryInterface(IID_IVideoWindow, IID_PPV_ARGS_Helper(&video));
    if (FAILED(result)) {
        return 0;
    }
    LONG fullScreen;
    result = video->get_FullScreenMode(&fullScreen);
    if (FAILED(result)) {
        video->Release();
        return 0;
    }
    if (fullScreen != OATRUE) {
        result = video->HideCursor(OATRUE);
        if (FAILED(result)) {
            video->Release();
            return 0;
        }
    }
    result = video->put_FullScreenMode(OATRUE);
    video->Release();
    return SUCCEEDED(result);
}

// @ghidraAddress BossRally: 0x00401670
int RenderMovieFile(const char *path) {
    WCHAR widePath[260];
    ReleaseFilterGraph();
    if (!CreateFilterGraph()) {
        return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, path, -1, widePath, 260);
    SetCursor(LoadCursorA(nullptr, IDC_WAIT));
    HRESULT result = g_pGraphBuilder->RenderFile(widePath, nullptr);
    SetCursor(LoadCursorA(nullptr, IDC_ARROW));
    return SUCCEEDED(result);
}

// @ghidraAddress BossRally: 0x00401700
BOOL SetPlayerWindowTitle(HWND window, const char *fileName) {
    char title[532];
    std::strcpy(title, "Player");
    std::strcat(title, " - ");
    std::strcat(title, fileName);
    return SetWindowTextA(window, title);
}

// @ghidraAddress BossRally: 0x004017b0
void OpenMovie(HWND window, const char *path) {
    if (path && RenderMovieFile(path)) {
        char *filePart;
        GetFullPathNameA(path, 260, g_szMovieFullPath, &filePart);
        std::strncpy(g_szMovieFileName, filePart, 512);
        g_szMovieFileName[511] = '\0';
        SetPlayerWindowTitle(window, g_szMovieFileName);
        SetMovieState(MOVIE_STOPPED);
    }
}

// @ghidraAddress BossRally: 0x00401820
void PlayMovie(void) {
    if (CanPlayMovie()) {
        IMediaControl *control;
        HRESULT result =
            g_pGraphBuilder->QueryInterface(IID_IMediaControl, IID_PPV_ARGS_Helper(&control));
        if (SUCCEEDED(result)) {
            result = control->Run();
            control->Release();
            if (SUCCEEDED(result)) {
                SetMovieState(MOVIE_PLAYING);
            }
        }
    }
}

// @ghidraAddress BossRally: 0x00401870
void StopAndRewindMovie(void) {
    if (CanStopMovie()) {
        IMediaControl *control;
        HRESULT result =
            g_pGraphBuilder->QueryInterface(IID_IMediaControl, IID_PPV_ARGS_Helper(&control));
        if (FAILED(result)) {
            return;
        }
        control->Stop();
        control->Release();
        IMediaPosition *position;
        result =
            g_pGraphBuilder->QueryInterface(IID_IMediaPosition, IID_PPV_ARGS_Helper(&position));
        if (SUCCEEDED(result)) {
            position->put_CurrentPosition(0.0);
            position->Release();
            SetMovieState(MOVIE_STOPPED);
        }
    }
}

// @ghidraAddress BossRally: 0x004018f0
void StopMovie(void) {
    if (CanStopMovie()) {
        IMediaControl *control;
        HRESULT result =
            g_pGraphBuilder->QueryInterface(IID_IMediaControl, IID_PPV_ARGS_Helper(&control));
        if (FAILED(result)) {
            return;
        }
        control->Pause();
        IMediaPosition *position;
        result =
            g_pGraphBuilder->QueryInterface(IID_IMediaPosition, IID_PPV_ARGS_Helper(&position));
        if (SUCCEEDED(result)) {
            position->put_CurrentPosition(0.0);
            position->Release();
        }
        OAFilterState state;
        control->GetState(-1, &state);
        control->Stop();
        control->Release();
        SetMovieState(MOVIE_STOPPED);
    }
}

// @ghidraAddress BossRally: 0x00401990
HANDLE GetMovieEventHandle(void) {
    return reinterpret_cast<HANDLE>(g_hMovieEvent);
}

// @ghidraAddress BossRally: 0x004019a0
void HandleMovieGraphEvent(void) {
    IMediaEvent *event;
    HRESULT result = g_pGraphBuilder->QueryInterface(IID_IMediaEvent, IID_PPV_ARGS_Helper(&event));
    if (SUCCEEDED(result)) {
        LONG code;
        LONG_PTR first;
        LONG_PTR second;
        result = event->GetEvent(&code, &first, &second, 0);
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
        event->Release();
    }
}

// @ghidraAddress BossRally: 0x00401230
int RunMoviePlayerMessageLoop(void) {
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
                return static_cast<int>(message.wParam);
            }
            if (message.message == WM_CHAR &&
                static_cast<unsigned char>(message.wParam) == VK_ESCAPE && CanStopMovie()) {
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

} // namespace

/** @ghidraAddress BossRally: 0x00401340 */
int WINAPI WinMain(HINSTANCE instance,
                   HINSTANCE previousInstance,
                   LPSTR commandLine,
                   int showCommand) {
    if (!InitPlayerApplication()) {
        return 0;
    }
    int result = static_cast<int>(reinterpret_cast<INT_PTR>(previousInstance));
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

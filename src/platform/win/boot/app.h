#pragma once

#include <cstddef>

#include <windows.h>

/**
 * Exit code RallyMain() returns to the launcher.
 *
 * @ghidraAddress BRD3D: 0x118abe2c
 * @ghidraAddress BRGlide: 0x105ccbc0
 */
extern int g_dwAppExitCode;

/**
 * Directory the game is installed in, read from the registry, with a trailing separator.
 *
 * @ghidraAddress BRD3D: 0x10b501e0
 * @ghidraAddress BRGlide: 0x10b73540
 */
extern char g_szInstallDirectory[];

/**
 * Path of BossRally.cfg, built from g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x10b4fbe8
 * @ghidraAddress BRGlide: 0x10b72f48
 */
extern char g_szConfigFilePath[];

/**
 * Arguments of WinMain() as the launcher passed them to RallyMain(). Only the Glide library
 * stores them.
 *
 * @ghidraAddress BRGlide: 0x105bc730
 */
extern HINSTANCE g_hRallyMainInstance;

/** @ghidraAddress BRGlide: 0x105bc734 */
extern HINSTANCE g_hRallyMainPrevInstance;

/** @ghidraAddress BRGlide: 0x105bc738 */
extern LPSTR g_pszRallyMainCmdLine;

/** @ghidraAddress BRGlide: 0x105bc73c */
extern int g_nRallyMainCmdShow;

/**
 * The amount of physical memory of the machine, in bytes.
 *
 * @ghidraAddress BRD3D: 0x1022b348
 * @ghidraAddress BRGlide: 0x10226e78
 */
extern size_t g_dwTotalPhysicalMemory;

/**
 * The module of the game, set by InitializeApplication().
 *
 * @ghidraAddress BRD3D: 0x118abe20
 */
extern HINSTANCE g_hAppInstance;

/**
 * The main window.
 *
 * @ghidraAddress BRD3D: 0x10680584
 */
extern HWND g_hMainWindow;

/**
 * The keyboard accelerators of the main window.
 *
 * @ghidraAddress BRD3D: 0x118abe24
 */
extern HACCEL g_hAccelTable;

/**
 * Name of the window class of the main window.
 *
 * @ghidraAddress BRD3D: 0x100bd594
 */
extern const char *g_pszMainWindowClassName;

/**
 * The first parameter of the last `WM_ACTIVATE`. A frame runs only while it and
 * g_dwActivateState are set and g_dwActivateMinimized is not.
 *
 * @ghidraAddress BRD3D: 0x10680598
 */
extern WPARAM g_dwAppActive;

/**
 * How the main window is activated, `WA_INACTIVE`, `WA_ACTIVE`, or `WA_CLICKACTIVE`.
 *
 * @ghidraAddress BRD3D: 0x1068059c
 */
extern int g_dwActivateState;

/**
 * Whether the main window is minimised.
 *
 * @ghidraAddress BRD3D: 0x106805a0
 */
extern int g_dwActivateMinimized;

/**
 * Whether the main window is minimised.
 *
 * @ghidraAddress BRD3D: 0x118ac230
 */
extern int g_bWindowMinimized;

/**
 * Whether the main window is maximised.
 *
 * @ghidraAddress BRD3D: 0x118ac234
 */
extern int g_bWindowMaximized;

/**
 * Release every subsystem of the game and end the process.
 *
 * @param nExitCode The exit code of the process.
 * @ghidraAddress BRD3D: 0x10038f30
 */
[[noreturn]] void ShutdownApplication(int nExitCode);

/** The states of the game, in the order of g_apfnGameStateHandlers. */
enum {
    GAME_STATE_INIT_RESOURCES,  /*!< Load the resources of the game. */
    GAME_STATE_ENTER_GAME_MODE, /*!< Start the mode of the game. */
    GAME_STATE_RUN_GAME_MODE,   /*!< Run a frame of the mode of the game. */
    GAME_STATE_LOAD_GAME,       /*!< Show the loading screen and set up the game. */
    GAME_STATE_RUN_FRONT_END,   /*!< Run a frame of the front end. */
};

/**
 * The state of the game, an index into g_apfnGameStateHandlers.
 *
 * @ghidraAddress BRD3D: 0x10690a14
 * @ghidraAddress BRGlide: 0x105ccbbc
 */
extern int g_dwGameState;

/**
 * The routine that runs a frame of each state of the game.
 *
 * @ghidraAddress BRD3D: 0x100aa5b0
 * @ghidraAddress BRGlide: 0x100a9900
 */
extern int (*g_apfnGameStateHandlers[])(void);

/**
 * The frames the mode of the game has run.
 *
 * @ghidraAddress BRD3D: 0x10690a10
 * @ghidraAddress BRGlide: 0x105ccbb8
 */
extern int g_dwGameModeFrameCounter;

/**
 * What GameState_RunGameMode() returns, 1 at the start.
 *
 * @ghidraAddress BRD3D: 0x100aa5a8
 * @ghidraAddress BRGlide: 0x100a98f8
 */
extern int g_dwGameModeContinue;

/**
 * Open the pod of the game files, load the first sound set and the names of the controls of the
 * game controller, and go on to the front end.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1002f6b0
 * @ghidraAddress BRGlide: 0x1001cd70
 */
int GameState_InitResources(void);

/**
 * Go on to run the mode of the game.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1002f6e0
 * @ghidraAddress BRGlide: 0x1001cda0
 */
int GameState_EnterGameMode(void);

/**
 * Run a frame of the mode of the game.
 *
 * @return g_dwGameModeContinue.
 * @ghidraAddress BRD3D: 0x1002f6f0
 * @ghidraAddress BRGlide: 0x1001cdb0
 */
int GameState_RunGameMode(void);

/**
 * Set the display mode of the configuration, show the loading screen, set up the game, and go on
 * to enter the mode of the game. Direct3D sets the display mode once more after the loading screen.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1002f710
 * @ghidraAddress BRGlide: 0x1001cdd0
 */
int GameState_ShowLoadingScreenAndInitGame(void);

/**
 * Run a frame of the front end.
 *
 * @return The result of the frame. Its meaning is not recovered yet.
 * @ghidraAddress BRD3D: 0x1002f790
 * @ghidraAddress BRGlide: 0x1001ce20
 */
int GameState_RunFrontEnd(void);

/**
 * Go back to the front end after a game, setting it up again.
 *
 * @ghidraAddress BRD3D: 0x1002f690
 * @ghidraAddress BRGlide: 0x1001cd50
 */
void SetGameStateFrontEnd(void);

/**
 * Open BossRally.pod, and set up the renderer, the damage bitmaps of the paint, DirectInput, the
 * keyboard, the music when it plays, and DirectSound when sound is on. The Glide library shows its
 * splash image once the renderer is set up.
 *
 * @return The result of InitializeDirectSound(), or false when sound is off.
 * @ghidraAddress BRD3D: 0x10038ec0
 * @ghidraAddress BRGlide: 0x10032530
 */
bool OpenBossRallyPodAndInit(void);

#if TGR_RENDERER == TGR_RENDERER_GLIDE
/**
 * Show an image of the Glide library over the whole screen.
 *
 * @param pszPath The path of the image.
 * @param nUnknown 0 for the loading image and 0x2ac7e58b for the splash image. Its meaning is not
 * recovered yet.
 * @ghidraAddress BRGlide: 0x1006c990
 */
void ShowGlideSplashImage(const char *pszPath, int nUnknown);
#endif

/**
 * Free the buffers of the sectors of the track.
 *
 * @ghidraAddress BRD3D: 0x1002c4a0
 */
void FreeAllTrackSectorBuffers(void);

class DirectDrawRenderer;

/**
 * Title of the main window.
 *
 * @ghidraAddress BRD3D: 0x100bd598
 */
extern const char *g_pszMainWindowTitle;

/**
 * The arrow cursor of the main window.
 *
 * @ghidraAddress BRD3D: 0x118abe28
 */
extern HCURSOR g_hArrowCursor;

/**
 * Handle the messages of the main window.
 *
 * @param hWnd The main window.
 * @param nMessage The message.
 * @param wParam The first parameter of the message.
 * @param lParam The second parameter of the message.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x10079ca0
 */
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT nMessage, WPARAM wParam, LPARAM lParam);

/**
 * The laps the first vehicle has driven.
 *
 * @ghidraAddress BRD3D: 0x10acee50
 */
extern int g_aVehicles_LapCount;

/**
 * Set whether the application is active from the state of `WM_ACTIVATE`.
 *
 * @param wParam The first parameter of `WM_ACTIVATE`.
 * @ghidraAddress BRD3D: 0x10077090
 */
void UpdateAppActiveStateFromActivate(WPARAM wParam);

/**
 * Handle `WM_ACTIVATE`.
 *
 * @param hWnd The main window.
 * @param wParam The state of the activation.
 * @param lParam The other window.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a070
 */
LRESULT HandleWindowActivate(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_ACTIVATEAPP`.
 *
 * @param hWnd The main window.
 * @param wParam Whether the application is activated.
 * @param lParam The thread of the other application.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a090
 */
LRESULT HandleWindowActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_CLOSE`.
 *
 * @param hWnd The main window.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a190
 */
LRESULT HandleWindowClose(HWND hWnd);

/**
 * Handle `WM_COMMAND`.
 *
 * @param hWnd The main window.
 * @param wParam The command.
 * @param lParam The control of the command.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a1a0
 */
LRESULT HandleWindowCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_DESTROY`.
 *
 * @param hWnd The main window.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a1d0
 */
LRESULT HandleWindowDestroy(HWND hWnd);

/**
 * Handle `WM_DISPLAYCHANGE`.
 *
 * @param hWnd The main window.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a1f0
 */
LRESULT HandleDisplayChange(HWND hWnd);

/**
 * Handle `WM_ERASEBKGND`.
 *
 * @param hWnd The main window.
 * @param wParam The device context.
 * @param lParam Not used by the message.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a210
 */
LRESULT HandleWindowEraseBackground(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_ENTERMENULOOP`.
 *
 * @param hWnd The main window.
 * @param wParam Whether the menu is a popup.
 * @param lParam Not used by the message.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a260
 */
LRESULT HandleWindowEnterMenuLoop(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_EXITMENULOOP`.
 *
 * @param hWnd The main window.
 * @param wParam Whether the menu is a popup.
 * @param lParam Not used by the message.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a2b0
 */
LRESULT HandleWindowExitMenuLoop(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_GETMINMAXINFO`.
 *
 * @param hWnd The main window.
 * @param wParam Not used by the message.
 * @param lParam The `MINMAXINFO` of the window.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a300
 */
LRESULT HandleWindowGetMinMaxInfo(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_MOVE`.
 *
 * @param hWnd The main window.
 * @param wParam Not used by the message.
 * @param lParam The position of the client area.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a380
 */
LRESULT HandleWindowMove(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_NCPAINT`.
 *
 * @param hWnd The main window.
 * @param wParam The region to paint.
 * @param lParam Not used by the message.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a3a0
 */
LRESULT HandleWindowNcPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_SETCURSOR`.
 *
 * @param hWnd The main window.
 * @param wParam The window under the cursor.
 * @param lParam The hit test and the message.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a3f0
 */
LRESULT HandleWindowSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_SIZE`.
 *
 * @param hWnd The main window.
 * @param wParam The kind of resize.
 * @param lParam The size of the client area.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a450
 */
LRESULT HandleWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_SYSCOMMAND`.
 *
 * @param hWnd The main window.
 * @param wParam The command.
 * @param lParam The position of the cursor.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a4e0
 */
LRESULT HandleWindowSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Handle `WM_WINDOWPOSCHANGING`.
 *
 * @param hWnd The main window.
 * @param wParam Not used by the message.
 * @param lParam The `WINDOWPOS` of the window.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a590
 */
LRESULT HandleWindowPosChanging(HWND hWnd, WPARAM wParam, LPARAM lParam);

/**
 * Store the renderer that has created its objects in the user data of its window.
 *
 * @param hWnd The main window.
 * @param lParam The renderer.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a6a0
 */
LRESULT AttachRenderDeviceToWindow(HWND hWnd, LPARAM lParam);

/**
 * Clear the renderer that is releasing its objects from the user data of its window.
 *
 * @param hWnd The main window.
 * @param lParam The renderer.
 * @return The result of the message.
 * @ghidraAddress BRD3D: 0x1007a6f0
 */
LRESULT DetachRenderDeviceFromWindow(HWND hWnd, LPARAM lParam);

/**
 * Create a window and the renderer that draws into it, and show the window. A renderer that
 * does not start is reported in a message box and destroys the window.
 *
 * @param dwExStyle The extended style of the window.
 * @param pszClassName The class of the window.
 * @param pszWindowName The title of the window.
 * @param dwStyle The style of the window.
 * @param x The left of the window.
 * @param y The top of the window.
 * @param nWidth The width of the window.
 * @param nHeight The height of the window.
 * @param hParent The parent, or `nullptr` for the desktop.
 * @param hMenu The menu of the window.
 * @param hInstance The module of the class, or `nullptr` for the game.
 * @param pParam The parameter of the creation of the window.
 * @param pDriverGuid The DirectDraw driver, or `nullptr` for the default.
 * @param nUnused Not read.
 * @param phWnd Receives the window, unless `nullptr`.
 * @param ppRenderer Receives the renderer, unless `nullptr`.
 * @return `S_OK`, `E_INVALIDARG` without a class, `E_OUTOFMEMORY`, `E_FAIL` for a window that
 * was not created, or the error of the renderer.
 * @ghidraAddress BRD3D: 0x10079a90
 */
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
                           DirectDrawRenderer **ppRenderer);

/**
 * The renderer of the main window.
 *
 * @ghidraAddress BRD3D: 0x1027736c
 */
extern DirectDrawRenderer *g_pMainRenderWindow;

/**
 * Destroy the main window and its renderer.
 *
 * @return 0.
 * @ghidraAddress BRD3D: 0x10079c40
 */
int DestroyMainWindow(void);

/**
 * Run a frame of the current state of the game.
 *
 * @return The result of the routine of the state.
 * @ghidraAddress BRD3D: 0x1002f8f0
 * @ghidraAddress BRGlide: 0x1001cf80
 */
int DispatchGameStateFrame(void);

/**
 * Do nothing.
 *
 * @ghidraAddress BRD3D: 0x10008b80
 */
void NullStub(void);

/**
 * Whether the front end is up. MenuScreen::ShutdownFrontEnd() clears it, notices of network play
 * are posted only while it is set, and NullStub() is called after the textures are loaded again.
 *
 * @ghidraAddress BRD3D: 0x100ac300
 * @ghidraAddress BRGlide: 0x100abaa0
 */
extern int g_dwNullStubEnable;

/**
 * Query the installed DirectX version by creating its objects in turn.
 *
 * @param pdwVersion Receives the version, 0x600 for DirectX 6, or 0 when DirectDraw is missing.
 * @param pdwPlatform Receives the platform of Windows (VER_PLATFORM_WIN32_WINDOWS or
 * VER_PLATFORM_WIN32_NT), or 0 when the version cannot be read.
 * @ghidraAddress BRD3D: 0x10030210
 * @ghidraAddress BRGlide: 0x1001d8a0
 */
void GetDXVersion(unsigned *pdwVersion, unsigned *pdwPlatform);

/**
 * Bring an instance of the game that is already running to the front.
 *
 * @return Whether this instance is the only one and may start.
 * @ghidraAddress BRD3D: 0x10007b10
 * @ghidraAddress BRGlide: 0x10007e80
 */
bool FocusExistingBossRallyInstance(void);

/**
 * Read the amount of physical memory the machine has.
 *
 * @ghidraAddress BRD3D: 0x10007ba0
 * @ghidraAddress BRGlide: 0x10007f10
 */
void RefreshTotalPhysicalMemory(void);

/**
 * Read the install directory from the registry into g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x1006a8a0
 * @ghidraAddress BRGlide: 0x10063860
 */
void ReadInstallDirectoryFromRegistry(void);

/**
 * Start the application: refuse DirectX before version 6 and a second instance, then create the
 * main window. Only the Direct3D library has it.
 *
 * @return Whether the application started.
 * @ghidraAddress BRD3D: 0x100796b0
 */
bool InitializeApplication(void);

/**
 * Create the main window.
 *
 * @return Whether the window was created.
 * @ghidraAddress BRD3D: 0x100798f0
 * @ghidraAddress BRGlide: 0x10019670
 * @stub{BRGlide}
 */
bool CreateMainWindow(void);

/**
 * Run the message loop until the game quits.
 *
 * @ghidraAddress BRD3D: 0x10079740
 * @ghidraAddress BRGlide: 0x10019730
 * @stub{BRGlide}
 */
void RunMessageLoop(void);

/**
 * Destroy the main window. Only the Direct3D library has it.
 *
 * @ghidraAddress BRD3D: 0x10079720
 */
void ShutdownMainWindow(void);
